#ifndef SERENITY_IGNORE_NEW_TASKS_FILTER_HPP
#define SERENITY_IGNORE_NEW_TASKS_FILTER_HPP

#include <ctime>
#include <memory>
#include <string>

#include "mesos/mesos.hpp"

#include "messages/serenity.hpp"

#include "serenity/default_vars.hpp"
#include "serenity/executor_map.hpp"
#include "serenity/serenity.hpp"

#include "stout/nothing.hpp"
#include "stout/option.hpp"
#include "stout/try.hpp"

namespace mesos {
namespace serenity {


/**
 * IgnoreNewExecutorsFilter removes executors from ResourceUsage collection
 * that run for less time than threshold (expressed in seconds).
 *
 * It's purpose is to cut away tasks that are warming up.
 */
class IgnoreNewExecutorsFilter : public Consumer<ResourceUsage>,
                                 public Producer<ResourceUsage> {
 public:
  explicit IgnoreNewExecutorsFilter(
    Consumer<ResourceUsage>* _consumer = nullptr,
    uint32_t _thresholdSeconds = new_executor::DEFAULT_THRESHOLD_SEC) :
      Producer<ResourceUsage>(_consumer),
      threshold(_thresholdSeconds),
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

  uint32_t threshold;  //!< #seconds when executor is considered too fresh.

  std::unique_ptr<ExecutorMap<time_t>> executorTimestamps;

  static constexpr const char* name =
    "[SerenityEstimator] IgnoreNewExecutorsFilter: ";
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_IGNORE_NEW_TASKS_FILTER_HPP
