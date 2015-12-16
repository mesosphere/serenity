#include <string>
#include <memory>

#include "mesos/mesos.hpp"
#include "mesos/module.hpp"

#include "mesos/module/qos_controller.hpp"
#include "mesos/slave/qos_controller.hpp"

#include "pipeline/qos_pipeline.hpp"

#include "serenity/config.hpp"

#include "stout/try.hpp"

#include "qos_controller/serenity_controller.hpp"

// TODO(nnielsen): Should be explicit using-directives.
using namespace mesos;  // NOLINT(build/namespaces)
using namespace mesos::serenity::ema;  // NOLINT(build/namespaces)
using namespace mesos::serenity::decider;  // NOLINT(build/namespaces)
using namespace mesos::serenity::detector;  // NOLINT(build/namespaces)
using namespace mesos::serenity::too_low_usage;  // NOLINT(build/namespaces)
using namespace mesos::serenity::qos_pipeline;  // NOLINT(build/namespaces)

using mesos::serenity::SignalBasedDetector;
using mesos::serenity::TooHighCpuUsageDetector;
using mesos::serenity::TooLowUsageFilter;
using mesos::serenity::CpuQoSPipeline;
using mesos::serenity::SerenityConfig;
using mesos::serenity::SerenityController;
using mesos::serenity::QoSControllerPipeline;

using mesos::slave::QoSController;


// IPC QoS pipeline.
static QoSController* createSerenityController(
    const Parameters& parameters) {
  LOG(INFO) << "Loading Serenity QoS Controller module";
  // TODO(bplotka): Fetch configuration from parameters or conf file.
  //
  // --Hardcoded configuration for Serenity QoS Controller---

  SerenityConfig conf;
  // AssuranceDropAnalyzer configuration:
  // How far we look back in samples.
  conf[SIGNAL_DROP_ANALYZER_NAME].set(WINDOW_SIZE, (uint64_t) 10);
  // Defines how much (relatively to base point) value must drop to trigger
  // contention.
  // Most signal_analyzer will use that.
  conf[SIGNAL_DROP_ANALYZER_NAME].set(FRACTIONAL_THRESHOLD, (double_t) 0.3);
  conf[SIGNAL_DROP_ANALYZER_NAME].set(SEVERITY_FRACTION, (double_t) 2.1);

  // How many iterations detector will wait with creating another
  // contention.
  conf[SIGNAL_DROP_ANALYZER_NAME].set(CONTENTION_COOLDOWN, (uint64_t) 10);

  // UtilizationDetector configuration:
  // CPU utilization threshold.
  conf[THRESHOLD].set(THRESHOLD, (double_t) 0.72);

  conf[TooLowUsageFilter::NAME].set(MINIMAL_CPU_USAGE, (double_t) 0.25);

  conf.set(ALPHA, (double_t) 0.9);
  conf.set(ENABLED_VISUALISATION, false);
  conf.set(VALVE_OPENED, true);

  // Since slave is configured for 5 second perf interval, it is useless to
  // check correction more often then 5 sec.
  double onEmptyCorrectionInterval = 2;

  // --End of hardcoded configuration for Serenity QoS Controller---

  // Use static constructor of QoSController.
  Try<QoSController*> result =
    SerenityController::create(
      std::shared_ptr<QoSControllerPipeline>(
          new CpuQoSPipeline(conf)),
          onEmptyCorrectionInterval);

  if (result.isError()) {
    return NULL;
  }
  return result.get();
}

mesos::modules::Module<QoSController> com_mesosphere_mesos_SerenityController(
    MESOS_MODULE_API_VERSION,
    MESOS_VERSION,
    "Mesosphere & Intel",
    "support@mesosphere.com",
    "Serenity QoS Controller",
    NULL,
    createSerenityController);
