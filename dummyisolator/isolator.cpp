#include <mesos/mesos.hpp>
#include <mesos/module.hpp>

#include <mesos/module/isolator.hpp>

#include <mesos/slave/isolator.hpp>

#include <process/future.hpp>
#include <process/owned.hpp>
#include <process/subprocess.hpp>

#include <stout/try.hpp>
#include <stout/stringify.hpp>
#include <stout/hashmap.hpp>
#include <stout/option.hpp>

using namespace mesos;

using mesos::slave::Isolator;

class DummyIsolatorProcess : public mesos::slave::IsolatorProcess
{
public:
  static Try<mesos::slave::Isolator*> create(const Parameters& parameters)
  {
    return new Isolator(process::Owned<IsolatorProcess>(
        new DummyIsolatorProcess(parameters)));
  }

  virtual ~DummyIsolatorProcess() {}

  virtual process::Future<Nothing> recover(
      const std::list<mesos::slave::ExecutorRunState>& states,
      const hashset<mesos::ContainerID>& containerId)
  {
    return Nothing();
  }

  virtual process::Future<Option<CommandInfo>> prepare(
      const ContainerID& containerId,
      const ExecutorInfo& executorInfo,
      const std::string& directory,
      const Option<std::string>& user)
  {
    return None();
  }

  virtual process::Future<Nothing> isolate(
      const ContainerID& containerId,
      pid_t pid)
  {
    return Nothing();
  }

  virtual process::Future<mesos::slave::Limitation> watch(
      const ContainerID& containerId)
  {
    return process::Future<mesos::slave::Limitation>();
  }

  virtual process::Future<Nothing> update(
      const ContainerID& containerId,
      const Resources& resources)
  {
    return Nothing();
  }

  virtual process::Future<ResourceStatistics> usage(
      const ContainerID& containerId)
  {
    return ResourceStatistics();
  }

  virtual process::Future<Nothing> cleanup(
      const ContainerID& containerId)
  {
    return Nothing();
  }

private:
  DummyIsolatorProcess(const Parameters& parameters_) {}
};


static Isolator* createDummyIsolator(const Parameters& parameters)
{
  LOG(INFO) << "Loading Dummy Isolator module";
  Try<Isolator*> result = DummyIsolatorProcess::create(parameters);
  if (result.isError()) {
    return NULL;
  }
  return result.get();
}


mesos::modules::Module<Isolator> com_mesosphere_mesos_DummyIsolator(
    MESOS_MODULE_API_VERSION,
    MESOS_VERSION,
    "Mesosphere",
    "support@mesosphere.com",
    "Dummy isolator for testing purposes",
    NULL,
    createDummyIsolator);
