#ifndef SERENITY_SMOKE_JOB_HPP
#define SERENITY_SMOKE_JOB_HPP

#include <list>
#include <string>

#include <mesos/resources.hpp>

#include <stout/json.hpp>
#include <stout/option.hpp>

#include "logging/logging.hpp"

#include "smoke_flags.hpp"

template<typename T, typename JSONType>
inline Option<T> getOption(Result<JSONType> result) {
  return (result.isSome() ? Option<T>(result.get().value) : None());
};


/**
 * Smoke URI.
 */
class SmokeURI {
 public:
  SmokeURI(
    const std::string& _value,
    const Option<bool>& _executable = None(),
    const Option<bool>& _extract = None(),
    const Option<bool>& _cache = None())
    : value(_value),
      executable(_executable),
      extract(_extract),
      cache(_cache) {}

  const std::string value;
  const Option<bool> executable;
  const Option<bool> extract;
  const Option<bool> cache;

  void fillURI(mesos::CommandInfo_URI* uri) const {
    uri->set_value(this->value);
    if (this->executable.isSome()) {
      uri->set_executable(this->executable.get());
    }
    if (this->extract.isSome()) {
      uri->set_extract(this->extract.get());
    }
    if (this->cache.isSome()) {
      uri->set_cache(this->cache.get());
    }
  }

  std::string toString() const {
    std::stringstream result;
    result  << "{ "
      << " Value: " << this->value
      << "; Executable: "
      <<  (this->executable.isSome() ?
           std::to_string(this->executable.get()) : "0")
      << "; Extract: "
      <<  (this->extract.isSome() ? std::to_string(this->extract.get()): "1")
      << "; Cache: "
      <<  (this->cache.isSome() ? std::to_string(this->cache.get()): "0")
      << "}";
    return result.str();
  }
};


/**
 * Smoke Job specification.
 */
class SmokeJob {
 public:
  SmokeJob(
    const std::string& _command,
    const mesos::Resources& _taskResources,
    const Option<size_t>& _totalTasks,
    const Option<std::string>& _targetHostname = None(),
    const Option<SmokeURI>& _uri = None(),
    const size_t& _priority = 1)
    : command(_command),
      taskResources(_taskResources),
      totalTasks(_totalTasks),
      targetHostname(_targetHostname),
      uri(_uri),
      priority(_priority),
      tasksLaunched(0u),
      scheduled(false) {


  }

  const std::string command;
  const mesos::Resources taskResources;
  const Option<size_t> totalTasks;
  const Option<std::string> targetHostname;
  const Option<SmokeURI>& uri;
  const size_t priority;

  // Stats
  size_t tasksLaunched;
  bool scheduled;

  bool isUnlimited() const {
    return this->totalTasks.isNone();
  }

  void print() {
    LOG(INFO) << "| Command: " << this->command
    << "; URI: "
    << (this->uri.isSome() ? this->uri.get().toString() : "<None>")
    << "; Resources: " << this->taskResources
    << "; Tasks to spawn: "
    << (this->totalTasks.isSome() ?
        std::to_string(this->totalTasks.get()) : "<none>")
    << "; Target hostname: "
    << (this->targetHostname.isSome() ? this->targetHostname.get() : "<all>")
    << "; Priority: " << this->priority
    << "|";
  }

  // Create new task.
  mesos::TaskInfo createTask(const size_t jid, mesos::SlaveID slave_id) {
    // Create new task.
    mesos::TaskInfo task;
    // Generate Task ID.
    task.mutable_task_id()->set_value(
      stringify(jid) + "_" + stringify(this->tasksLaunched));
    // Add Name.
    task.set_name(stringify(jid) + "_" + this->command);
    // Add Slave id.
    task.mutable_slave_id()->CopyFrom(slave_id);
    // Add Resources.
    task.mutable_resources()->CopyFrom(this->taskResources);
    // Add command.
    task.mutable_command()->set_shell(true);
    task.mutable_command()->set_value(this->command);
    // Add command if exists.
    if (this->uri.isSome()) {
      this->uri.get().fillURI(task.mutable_command()->add_uris());
    }

    return task;
  }

  static std::list<SmokeJob> createJobsFromJson(const SmokeFlags flags, bool&
  revocable) {
    std::list<SmokeJob> jobs;
    LOG(INFO) << "Loading JSON with tasks from: " << flags.tasks_json_path.get();

    Try<std::string> read = os::read(flags.tasks_json_path.get());
    if (read.isError()) {
      EXIT(EXIT_FAILURE) << read.error() << " "
      << flags.usage("Bad path for JSON tasks");
    } else if (read.get().empty()) {
      EXIT(EXIT_FAILURE) << "File is empty.  "
      << flags.usage("Bad path for JSON tasks");
    }

    Try<JSON::Object> json = JSON::parse<JSON::Object>(read.get());
    if (json.isError()) {
      EXIT(EXIT_FAILURE) << json.error() << " "
      << flags.usage("Bad JSON file.");
    }

    Result<JSON::Array> tasks = json.get().find<JSON::Array>("tasks");
    if (!tasks.isSome()) {
      EXIT(EXIT_FAILURE)
      << flags.usage("JSON file does not contain array 'tasks' in root.");
    }

    size_t numJobs = tasks.get().values.size();

    for(size_t i=0; i < numJobs; i++) {
      // Get command.

      Result<JSON::String> optionCommand =
        json.get().find<JSON::String>("tasks[" + stringify(i) +"].command");

      if (!optionCommand.isSome()) {
        EXIT(EXIT_FAILURE)
        << flags.usage(
          "JSON task " + stringify(i) + "does not contain command");
      }

      // Get URI.
      Option<SmokeURI> optionUri = None();

      Result<JSON::String> _uri_value =
        json.get().find<JSON::String>(
          "tasks[" + stringify(i) +"].uri.value");

      if (_uri_value.isSome()) {

        Result<JSON::Boolean> _uri_executable =
          json.get().find<JSON::Boolean>(
            "tasks[" + stringify(i) +"].uri.executable");

        Result<JSON::Boolean> _uri_extract =
          json.get().find<JSON::Boolean>(
            "tasks[" + stringify(i) +"].uri.extract");

        Result<JSON::Boolean> _uri_cache =
          json.get().find<JSON::Boolean>(
            "tasks[" + stringify(i) +"].uri.cache");

        optionUri = SmokeURI(
          _uri_value.get().value,
          getOption<bool, JSON::Boolean>(_uri_executable),
          getOption<bool, JSON::Boolean>(_uri_extract),
          getOption<bool, JSON::Boolean>(_uri_cache));
      }

      // Get TaskResources.
      Result<JSON::String> _taskResources =
        json.get().find<JSON::String>(
          "tasks[" + stringify(i) +"].taskResources");

      if (!_taskResources.isSome()) {
        EXIT(EXIT_FAILURE)
        << flags.usage(
          "JSON task " + stringify(i) + "does not contain taskResources");
      }

      Try<mesos::Resources> optionResources =
        mesos::Resources::parse(_taskResources.get().value);

      if (optionResources.isError()) {
        EXIT(EXIT_FAILURE)
        << flags.usage("Invalid taskResources in JSON:" +
                                 optionResources.error());
      }

      // Get RevocableTaskResources.
      Result<JSON::String> _revocableTaskResources =
        json.get().find<JSON::String>(
          "tasks[" + stringify(i) +"].revocableResources");

      if (_revocableTaskResources.isSome()) {
        revocable = true;
        Try<mesos::Resources> _revocableResources =
          mesos::Resources::parse(_revocableTaskResources.get().value);

        if (_revocableResources.isError()) {
          EXIT(EXIT_FAILURE)
          << flags.usage("Invalid revocableResources in JSON: " +
                         _revocableResources.error());
        }

        foreach (mesos::Resource _revocable, _revocableResources.get()) {
          _revocable.mutable_revocable();
          optionResources.get() += _revocable;
        }
      }

      // Get TargetHostname.
      Option<std::string> optionTargetHostname = None();

      Result<JSON::String> _targetHostname =
        json.get().find<JSON::String>(
          "tasks[" + stringify(i) +"].targetHostname");

      if (_targetHostname.isSome()) {
        optionTargetHostname = _targetHostname.get().value;
      }

      // Get TaskNum.
      Option<size_t> optionTotalTasks = None();

      Result<JSON::Number> _totalTasks =
        json.get().find<JSON::Number>(
          "tasks[" + stringify(i) +"].totalTasks");

      if (_totalTasks.isSome()) {
        optionTotalTasks = _totalTasks.get().as<size_t>();
      }

      // Get priority.
      size_t optionPriority = 1;

      Result<JSON::Number> _priority =
        json.get().find<JSON::Number>(
          "tasks[" + stringify(i) +"].totalTasks");

      if (_priority.isSome()) {
        optionPriority = _priority.get().as<size_t>();
      }

      jobs.push_back(
        SmokeJob(optionCommand.get().value,
                 optionResources.get(),
                 optionTotalTasks,
                 optionTargetHostname,
                 optionUri,
                 optionPriority));
      jobs.back().print();
    }

    return jobs;
  }
};

#endif //SERENITY_SMOKE_JOB_HPP
