#ifndef SERENITY_SMOKE_QUEUE_HPP
#define SERENITY_SMOKE_QUEUE_HPP

#include <memory>
#include <string>
#include <random>
#include <vector>

#include <mesos/resources.hpp>

#include <stout/json.hpp>
#include <stout/option.hpp>

#include "logging/logging.hpp"

#include "smoke_job.hpp"

struct QueueBlock {
  QueueBlock(std::shared_ptr<SmokeJob> _job,
              double_t _probability,
              size_t _index)
    : baseJob(_job),
      originalProbability(_probability),
      baseProbability(_probability),
      index(_index),
      aliasJob(nullptr),
      ready(false) {}

  double_t originalProbability;
  std::shared_ptr<SmokeJob> baseJob;
  double_t baseProbability;

  std::shared_ptr<SmokeJob> aliasJob;

  size_t index;
  bool ready;

  void print() {

  }

  std::string toString() const {
    std::stringstream result;
    result << "{ id: " << baseJob->id
    << ", baseProbability: " << baseProbability
    << ", aliasId: "
    << (aliasJob != nullptr ? std::to_string(aliasJob->id) : "<none>")
    << "}";
    return result.str();
  }

};


class SmokeAliasQueue {
 public:
  SmokeAliasQueue() : totalShares(0), initialized(false),
                 localIndex(0), finished(false) {}

  /**
   * Using Naive Alias Method from http://www.keithschwarz.com/darts-dice-coins/
   */
  void init() {
    totalShares = 0;
    for (auto& job : this->jobs) {
      // Reset default values.
      job.baseProbability = job.originalProbability;
      totalShares += job.baseJob->shares;
      job.ready = false;
    }


    double_t blockShares = totalShares / (double_t) this->jobs.size();
    intRand = std::uniform_int_distribution<uint32_t>(0, this->jobs.size() - 1);
    doubleRand = std::uniform_real_distribution<double_t>(0, blockShares);

    for (int i =1; i < this->jobs.size(); i++) {
      QueueBlock* underfullBlock = nullptr;
      QueueBlock* overfullBlock = nullptr;

      for (auto& job : this->jobs) {
        if (job.ready) continue;
        if (job.baseProbability >= blockShares) {
          overfullBlock = &job;
          break;
        }
      }

      BOOST_ASSERT(overfullBlock != nullptr);

      for (auto& job : this->jobs) {
        if (overfullBlock->index == job.index || job.ready) continue;
        if (job.baseProbability <= blockShares) {
          underfullBlock = &job;
          break;
        }
      }

      BOOST_ASSERT(underfullBlock != nullptr);

      underfullBlock->aliasJob = overfullBlock->baseJob;
      overfullBlock->baseProbability -=
        (overfullBlock->baseProbability - blockShares);

      underfullBlock->ready = true;
    }

    // Start debug log section.
    LOG(INFO) << "Alias alghoritm results: [ blockShares = "
      << blockShares << "]";
    for (auto& job : this->jobs)
      job.baseJob->probability = 0;
    for (auto& job : this->jobs) {
      job.baseJob->probability += job.baseProbability;
      if (job.aliasJob != nullptr)
        job.aliasJob->probability += blockShares - job.baseProbability;
    }
    for (auto& job : this->jobs)
      LOG(INFO) <<"Job(" <<  job.baseJob->id << ") current host probabilty: "
                << (job.baseJob->probability / totalShares)*100 << " %.";
    // End debug log section.

    initialized = true;
  }

  void add(std::shared_ptr<SmokeJob> job) {
    this->jobs.push_back(QueueBlock(job, job->shares, localIndex));
    localIndex++;
  }

  // Seleting is O(1)
  std::shared_ptr<SmokeJob> selectJob() {
    if (!initialized) this->init();
    uint32_t chosenBlock = intRand(generator);
    double_t chosenProbability = doubleRand(generator);

    if (chosenProbability > jobs[chosenBlock].baseProbability) {
      return jobs[chosenBlock].aliasJob;
    } else {
      return jobs[chosenBlock].baseJob;
    }
  }

  // Resetting is non-optimal: O(n^2)
  void removeAndReset(std::shared_ptr<SmokeJob> job) {

    for (auto jobIter = this->jobs.begin(); jobIter != this->jobs.end(); jobIter++)
      if (jobIter->baseJob->id == job->id) {
        this->jobs.erase(jobIter);
        if (this->jobs.size() == 0) this->finished = true;
        else this->init();
        return;
      }
  }

  bool finished;
 protected:
  bool initialized;
  size_t localIndex;
  double_t totalShares;
  std::vector<QueueBlock> jobs;
  std::default_random_engine generator;
  std::uniform_int_distribution<uint32_t> intRand;
  std::uniform_real_distribution<double_t> doubleRand;
};


#endif // SERENITY_SMOKE_QUEUE_HPP
