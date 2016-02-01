#include <memory>
#include <string>

#include "mesos/mesos.hpp"
#include "mesos/module.hpp"

#include "mesos/module/resource_estimator.hpp"
#include "mesos/slave/resource_estimator.hpp"

#include "mesos_modules/resource_estimator/serenity_estimator.hpp"

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
  // TODO(bplotka) Obtain the type of pipeline from parameters.

  Try<ResourceEstimator*> result = SerenityEstimator::create(
    std::shared_ptr<ResourceEstimatorPipeline>(
        new CpuEstimatorPipeline(false, true)));
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
