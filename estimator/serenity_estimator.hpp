#ifndef ESTIMATOR_SERENITY_ESTIMATOR_HPP
#define ESTIMATOR_SERENITY_ESTIMATOR_HPP

#include <string>

#include "mesos/slave/resource_estimator.hpp"

#include "stout/lambda.hpp"
#include "stout/nothing.hpp"
#include "stout/try.hpp"

#include "pipeline/estimator_pipeline.hpp"

#include "process/future.hpp"
#include "process/owned.hpp"

namespace mesos {
namespace serenity {

// Forward declaration.
class SerenityEstimatorProcess;


class SerenityEstimator : public slave::ResourceEstimator {
 public:
  explicit SerenityEstimator(
      std::shared_ptr<ResourceEstimatorPipeline> _pipeline)
    : pipeline(_pipeline) {}

  static Try<slave::ResourceEstimator*> create(
      std::shared_ptr<ResourceEstimatorPipeline> _pipeline) {
    return new SerenityEstimator(_pipeline);
  }

  virtual ~SerenityEstimator();

  virtual Try<Nothing> initialize(
      const lambda::function<process::Future<ResourceUsage>()>& usage);

  virtual process::Future<Resources> oversubscribable();

 protected:
  process::Owned<SerenityEstimatorProcess> process;
  std::shared_ptr<ResourceEstimatorPipeline> pipeline;
};

}  // namespace serenity
}  // namespace mesos

#endif  // ESTIMATOR_SERENITY_ESTIMATOR_HPP
