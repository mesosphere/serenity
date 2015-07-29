#include <memory>
#include <string>

#include "estimator/serenity_estimator.hpp"

#include "mesos/mesos.hpp"
#include "mesos/module.hpp"

#include "mesos/module/resource_estimator.hpp"
#include "mesos/slave/resource_estimator.hpp"

#include "pipeline/estimator_pipeline.hpp"

#include "stout/try.hpp"

// TODO(nnielsen): Break up into explicit using-declarations instead.
using namespace mesos;  // NOLINT(build/namespaces).

using mesos::serenity::CpuEstimatorPipeline;
using mesos::serenity::ResourceEstimatorPipeline;
using mesos::serenity::SerenityEstimator;

using mesos::slave::ResourceEstimator;

static ResourceEstimator* createSerenityEstimator(
    const Parameters& parameters) {
  LOG(INFO) << "Loading Serenity Estimator module";
  // Obtain the type of pipeline from parameters. Default one is
  // CpuEstimatorPipeline.
  std::shared_ptr<ResourceEstimatorPipeline> pipeline(
      new CpuEstimatorPipeline());
  foreach(const Parameter& parameter, parameters.parameter())
    if (parameter.key() == "pipeline") {
      // TODO(bplotka) place different types here if needed.
    }
  Try<ResourceEstimator*> result = SerenityEstimator::create(pipeline);
  if (result.isError()) {
    return NULL;
  }
  return result.get();
}


mesos::modules::Module<ResourceEstimator>
  com_mesosphere_mesos_SerenityEstimator(
    MESOS_MODULE_API_VERSION,
    MESOS_VERSION,
    "Mesosphere & Intel",
    "support@mesosphere.com",
    "Serenity Estimator",
    NULL,
    createSerenityEstimator);
