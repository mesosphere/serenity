#ifndef SERENITY_PYPELINE_FILTER_HPP
#define SERENITY_PYPELINE_FILTER_HPP

#include <ctime>
#include <list>
#include <memory>
#include <string>

#include <boost/python.hpp>

#include "mesos/mesos.hpp"

#include "messages/serenity.hpp"

#include "serenity/config.hpp"
#include "serenity/default_vars.hpp"
#include "serenity/serenity.hpp"

namespace mesos {
namespace serenity {

class PypelineFilterConfig : public SerenityConfig {
 public:
  PypelineFilterConfig() { }

  explicit PypelineFilterConfig(const SerenityConfig& customCfg) {
    this->initDefaults();
    this->applyConfig(customCfg);
  }

  void initDefaults() {
    this->fields[python_pipeline::SERENITY_PYPELINE_PATH] =
      (std::string) python_pipeline::DEFAULT_SERENITY_PYPELINE_PATH;
  }
};


/**
 * Initializing python interpreter and runnnig `serenity-pypeline`.
 * https://github.com/Bplotka/serenity-pypeline
 */
class PypelineFilter :
  public Consumer<ResourceUsage>, public Producer<Contentions> {
 public:
  explicit PypelineFilter(const Tag& _tag = Tag(QOS_CONTROLLER, NAME))
    : tag(_tag) {}

  explicit PypelineFilter(
      Consumer<Contentions>* _consumer,
      SerenityConfig _conf,
      const Tag& _tag = Tag(QOS_CONTROLLER, NAME))
      : Producer<Contentions>(_consumer), tag(_tag) {
    SerenityConfig config = PypelineFilterConfig(_conf);
    cfgSerenityPypelinePath =
      config.getS(python_pipeline::SERENITY_PYPELINE_PATH);

    initializePythonInterpreter();
  }

  ~PypelineFilter() {
    Py_Finalize();
  }

  static const constexpr char* NAME = "PypelineFilter";

  Try<Nothing> consume(const ResourceUsage& in) {
    if (pySerenityPypeline == NULL) {
      return Error("Module from path " + cfgSerenityPypelinePath
                   + " not imported.");
    }

    // Continue pipeline. Currently, we won't receive any automatic
    // scheduling actions.
    produce(Contentions());

    return Nothing();
  }

 private:
  const Tag tag;

  PyObject* pySerenityPypeline, *pDict;

  std::string cfgSerenityPypelinePath;

  void initializePythonInterpreter() {
    // Initialize Python interpreter.
    Py_Initialize();

    // Configure sys path.
    PyRun_SimpleString("import sys\n");
    PyRun_SimpleString(std::string("sys.path.append('" +
                         cfgSerenityPypelinePath + "')\n").c_str());

    SERENITY_LOG(INFO) << "Importing...";
    pySerenityPypeline =
      PyImport_ImportModule("pypeline_test");
    SERENITY_LOG(INFO) << "Debug...";
    if (pySerenityPypeline == NULL) {
      PyErr_Print();
      SERENITY_LOG(ERROR) << "Cannot import Python Module from path: "
      << cfgSerenityPypelinePath;
    } else {
      SERENITY_LOG(INFO) << "Imported Python Module from path: "
      << cfgSerenityPypelinePath;
    }
  }
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_PYPELINE_FILTER_HPP
