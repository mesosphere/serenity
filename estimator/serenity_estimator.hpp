#ifndef ESTIMATOR_SERENITY_ESTIMATOR_HPP
#define ESTIMATOR_SERENITY_ESTIMATOR_HPP

#include <mesos/slave/resource_estimator.hpp>

#include <stout/lambda.hpp>
#include <stout/nothing.hpp>
#include <stout/try.hpp>

#include <process/future.hpp>
#include <process/owned.hpp>

#include <string>

namespace mesos {
namespace serenity {

// Forward declaration.
class SerenityEstimatorProcess;


class SerenityEstimator : public slave::ResourceEstimator {
 public:
  SerenityEstimator() {}

  static Try<slave::ResourceEstimator*>create(const Option<std::string>& type) {
    return new SerenityEstimator();
  }

  virtual ~SerenityEstimator();

  virtual Try<Nothing> initialize(
      const lambda::function<process::Future<ResourceUsage>()>& usage);

  virtual process::Future<Resources> oversubscribable();

 protected:
  process::Owned<SerenityEstimatorProcess> process;
};

}  // namespace serenity
}  // namespace mesos

#endif  // ESTIMATOR_SERENITY_ESTIMATOR_HPP
