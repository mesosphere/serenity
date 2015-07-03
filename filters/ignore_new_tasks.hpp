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
 * threshold. It's purpose is to cut away tasks that are warming up.
 */
class IgnoreNewTasksFilter : public Consumer<ResourceUsage>,
                             public Producer<ResourceUsage> {
 public:
  explicit IgnoreNewTasksFilter(uint32_t _threshold = DEFAULT_THRESHOLD) :
      threshold(_threshold),
      executorMap(new ExecutorMap<time_t>) {}

  ~IgnoreNewTasksFilter() {}

  IgnoreNewTasksFilter(const IgnoreNewTasksFilter& other) :
       threshold(other.threshold) {}

  Try<Nothing> consume(const ResourceUsage& usage) override;

  void setThreshold(uint32_t _threshold) {
    this->threshold = _threshold;
  }

 protected:
  static constexpr uint32_t DEFAULT_THRESHOLD = 5 * 60;  //!< Five minutes
  uint32_t threshold;  //!< #seconds when executor is considered too fresh

  std::unique_ptr<ExecutorMap<time_t>> executorMap;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_IGNORE_NEW_TASKS_HPP
