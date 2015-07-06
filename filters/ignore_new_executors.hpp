#include <ctime>
#include <memory>

#include "messages/serenity.hpp"

#include "serenity/executor_map.hpp"
#include "serenity/serenity.hpp"

#include "stout/option.hpp"

#ifndef SERENITY_IGNORE_NEW_TASKS_HPP
#define SERENITY_IGNORE_NEW_TASKS_HPP

namespace mesos {
namespace serenity {

/**
 * IgnoreNewTasksFilter ignores removes executors that run for less time than
 * threshold.
 * It's purpose is to cut away tasks that are warming up.
 */
class IgnoreNewExecutorsFilter : public Consumer<ResourceUsage>,
                             public Producer<ResourceUsage> {
 public:
  explicit IgnoreNewExecutorsFilter(uint32_t _threshold = DEFAULT_THRESHOLD) :
      threshold(_threshold),
      executorTimestamps(new ExecutorMap<time_t>) {}

  ~IgnoreNewExecutorsFilter() {}

  IgnoreNewExecutorsFilter(const IgnoreNewExecutorsFilter& other) :
       threshold(other.threshold) {}

  Try<Nothing> consume(const ResourceUsage& usage) override;

  /// Set #seconds when executor is considered too fresh.
  void setThreshold(uint32_t _threshold) {
    this->threshold = _threshold;
  }

 protected:
  /// ctime function wrapped for mocking purposes.
  inline virtual time_t GetTime(time_t* arg) {
    return time(arg);
  }

  static constexpr uint32_t DEFAULT_THRESHOLD = 5 * 60;  //!< Five minutes.
  uint32_t threshold;  //!< #seconds when executor is considered too fresh.

  std::unique_ptr<ExecutorMap<time_t>> executorTimestamps;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_IGNORE_NEW_TASKS_HPP
