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
};


std::ostream& operator << (std::ostream& stream, const QueueBlock& job) {
  stream << "{ id: " << job.baseJob->id
  << ", baseProbability: " << job.baseProbability
  << ", aliasId: "
  << (job.aliasJob != nullptr ? std::to_string(job.aliasJob->id) : "<none>")
  << "}";
  return stream;
}


class SmokeAliasQueue {
 public:
  SmokeAliasQueue() : totalShares(0), initialized(false),
                 localIndex(0), finished(true) {}

  void add(std::shared_ptr<SmokeJob> job) {
    finished = false;
    this->jobs.push_back(QueueBlock(job, job->shares, localIndex));
    localIndex++;
  }

  // Seleting is O(1)
  std::shared_ptr<SmokeJob> selectJob() {
    if (jobs.empty()) {
      LOG(ERROR) << "Cannot select job when there is no job in queue";
      return nullptr;
    }
    if (!initialized) this->runAliasAlgorithm();
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
        if (this->jobs.empty()) {
          this->finished = true;
        } else {
          this->runAliasAlgorithm();
        }
        return;
      }
    LOG(ERROR) << "Requested job not found.";
  }

  size_t size() {
    return this->jobs.size();
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

  /**
  * Using Naive Alias Method from http://www.keithschwarz.com/darts-dice-coins/
  */
  void runAliasAlgorithm() {
    totalShares = 0;
    for (auto& job : jobs) {
      // Reset default values.
      job.baseProbability = job.originalProbability;
      totalShares += job.baseJob->shares;
      job.ready = false;
    }


    double_t blockShares = totalShares / (double_t) jobs.size();
    intRand = std::uniform_int_distribution<uint32_t>(0, jobs.size() - 1);
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

      if (overfullBlock == nullptr) {
        LOG(ERROR) << "Alias alghoritm failed. OverfullBlock == null";
        return;
      }


      for (auto& job : this->jobs) {
        if (overfullBlock->index == job.index || job.ready) continue;
        if (job.baseProbability <= blockShares) {
          underfullBlock = &job;
          break;
        }
      }

      if (underfullBlock == nullptr) {
        LOG(ERROR) << "Alias alghoritm failed. UnderfullBlock == null";
        return;
      }

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
};


#endif // SERENITY_SMOKE_QUEUE_HPP
