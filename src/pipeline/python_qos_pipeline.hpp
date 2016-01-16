#ifndef SERENITY_PYTHON_QOS_PIPELINE_HPP
#define SERENITY_PYTHON_QOS_PIPELINE_HPP

#include "filters/pypeline.hpp"

#include "messages/serenity.hpp"

#include "pipeline/pipeline.hpp"

#include "serenity/config.hpp"
#include "serenity/data_utils.hpp"
#include "serenity/serenity.hpp"

#include "observers/qos_correction.hpp"
#include "observers/strategies/simple.hpp"

namespace mesos {
namespace serenity {

using namespace python_pipeline;  // NOLINT(build/namespaces)

class PythonQoSPipelineConfig : public SerenityConfig {
 public:
  PythonQoSPipelineConfig() {}

  explicit PythonQoSPipelineConfig(const SerenityConfig& customCfg) {
    this->initDefaults();
    this->applyConfig(customCfg);
  }

  void initDefaults() {}
};


/**
 * Pipeline which run python pipeline using external `serenity-pypeline`
 * project. Path of project can be defined via SERENITY_PYPELINE_PATH
 */
class PythonQoSPipeline: public QoSControllerPipeline {
 public:
  explicit PythonQoSPipeline(const SerenityConfig& _conf)
    : conf(PythonQoSPipelineConfig(_conf)),
      simpleCorrectionObserver(this, 1,
                               nullptr, new SimpleStrategy()),
      pypeline(&simpleCorrectionObserver,
               conf[PypelineFilter::NAME]) {
    this->addConsumer(&pypeline);
  }

 private:
  SerenityConfig conf;

  QoSCorrectionObserver simpleCorrectionObserver;
  PypelineFilter pypeline;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_PYTHON_QOS_PIPELINE_HPP
