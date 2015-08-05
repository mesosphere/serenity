/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <list>
#include <string>
#include <vector>

#include <mesos/resources.hpp>
#include <mesos/scheduler.hpp>
#include <mesos/type_utils.hpp>

#include <stout/exit.hpp>
#include <stout/flags.hpp>
#include <stout/foreach.hpp>
#include <stout/hashset.hpp>
#include <stout/option.hpp>
#include <stout/os.hpp>
#include <stout/path.hpp>
#include <stout/stringify.hpp>
#include <stout/try.hpp>

#include "common/protobuf_utils.hpp"
#include "common/status_utils.hpp"

#include "logging/flags.hpp"
#include "logging/logging.hpp"

using namespace mesos;
using namespace mesos::internal;

using std::list;
using std::string;
using std::vector;

using mesos::Resources;

static Offer::Operation LAUNCH(const vector<TaskInfo>& tasks)
{
  Offer::Operation operation;
  operation.set_type(Offer::Operation::LAUNCH);

  foreach (const TaskInfo& task, tasks) {
    operation.mutable_launch()->add_task_infos()->CopyFrom(task);
  }

  return operation;
}


struct JobSpec {
  JobSpec(
      const string& _command,
      const Resources& _taskResources,
      const Option<size_t>& _totalTasks,
      const Option<string>& _targetHostname = None())
    : command(_command),
      taskResources(_taskResources),
      totalTasks((_totalTasks.isSome()?_totalTasks.get():1)),
      targetHostname(_targetHostname),
      tasksLaunched(0u),
      scheduled(false) {}

  const string command;
  const Resources taskResources;
  const size_t totalTasks;
  size_t tasksLaunched;
  bool scheduled;

  const Option<string> targetHostname;

  void print() {
    LOG(INFO) << "| Command: " << this->command
        << "; Resources: " <<  this->taskResources
        << "; Tasks to spawn: " << this->totalTasks
        << "; Target hostname: "
        << (this->targetHostname.isSome()?this->targetHostname.get():"<all>")
        << "|";

  }
};


class SerenityNoExecutorScheduler : public Scheduler
{
public:
  SerenityNoExecutorScheduler(
      const FrameworkInfo& _frameworkInfo,
      const list<JobSpec>& _jobs)
    : frameworkInfo(_frameworkInfo),
      jobs(_jobs),
      tasksFinished(0u),
      tasksTerminated(0u),
      tasksPending(0u),
      jobScheduled(0u) {
    foreach (const JobSpec& job, jobs) {
      tasksPending += job.totalTasks;
    }
  }

  virtual void registered(
      SchedulerDriver* driver,
      const FrameworkID& frameworkId,
      const MasterInfo& masterInfo)
  {
    LOG(INFO) << "Registered with master " << masterInfo
              << " and got framework ID " << frameworkId;

    frameworkInfo.mutable_id()->CopyFrom(frameworkId);
  }

  virtual void reregistered(
      SchedulerDriver* driver,
      const MasterInfo& masterInfo)
  {
    LOG(INFO) << "Reregistered with master " << masterInfo;
  }

  virtual void disconnected(
      SchedulerDriver* driver)
  {
    LOG(INFO) << "Disconnected!";
  }

  virtual void resourceOffers(
      SchedulerDriver* driver,
      const vector<Offer>& offers)
  {
    Filters filters;
    filters.set_refuse_seconds(Duration::max().secs());
    foreach (const Offer& offer, offers) {
      if (jobScheduled >= jobs.size()) {
        driver->declineOffer(offer.id(), filters);
        continue;
      }
      LOG(INFO) << "Received offer " << offer.id() << " from slave "
                << offer.slave_id() << " (" << offer.hostname() << ") "
                << "with " << offer.resources();

      Resources remaining = offer.resources();
      vector<TaskInfo> tasks;
      for(auto job=jobs.begin(); job != jobs.end(); ++job) {
        if (job->scheduled) continue;
        if (job->targetHostname.isSome() &&
            job->targetHostname.get().compare(offer.hostname()) != 0){
          LOG(INFO) << "Offered host " << offer.hostname()
                    << " not matched with target " << job->targetHostname.get()
                    << ". Omitting.";
          continue;
        }

        while (true) {
          if (!remaining.contains(job->taskResources)) {
            LOG(INFO) << "Not enough resources for "
                      << stringify(jobScheduled) + "_"
                         + stringify(job->tasksLaunched)
                      << " job. Needed: " << job->taskResources
                      << " Offered: " << remaining;
            break;
          }
          TaskInfo task;
          task.mutable_task_id()->set_value(
              stringify(jobScheduled) + "_" + stringify(job->tasksLaunched));
          task.set_name(stringify(jobScheduled) + "_" + job->command);
          task.mutable_slave_id()->CopyFrom(offer.slave_id());
          task.mutable_resources()->CopyFrom(job->taskResources);
          task.mutable_command()->set_shell(true);
          task.mutable_command()->set_value(job->command);

          remaining -= job->taskResources;

          tasks.push_back(task);
          this->activeTasks.insert(task.task_id());
          job->tasksLaunched++;
          LOG(INFO) << "Launching " << task.task_id();
          if (job->tasksLaunched  >= job->totalTasks) {
            job->scheduled = true;

            this->jobScheduled++;
            break;
          }
        }
      }

      driver->acceptOffers({offer.id()}, {LAUNCH(tasks)});
    }
  }

  virtual void offerRescinded(
      SchedulerDriver* driver,
      const OfferID& offerId)
  {
    LOG(INFO) << "Offer " << offerId << " has been rescinded";
  }

  virtual void statusUpdate(
      SchedulerDriver* driver,
      const TaskStatus& status)
  {
    if (!activeTasks.contains(status.task_id())) {
      LOG(WARNING) << "Unknown task '" << status.task_id() << "'"
                   << " is in state " << status.state();
      return;
    }

    if (status.state() == TASK_LOST ||
        status.state() == TASK_KILLED ||
        status.state() == TASK_FAILED) {
      LOG(ERROR) << "Task '" << status.task_id() << "'"
                 << " is in unexpected state " << status.state()
                 << (status.has_reason()
                     ? " with reason " + stringify(status.reason()) : "")
                 << " from source " << status.source()
                 << " with message '" << status.message() << "'";
    } else {
      LOG(INFO) << "Task '" << status.task_id() << "'"
                << " is in state " << status.state();
    }
    if (internal::protobuf::isTerminalState(status.state())) {
      if (status.state() == TASK_FINISHED) {
        tasksFinished++;
      }

      tasksTerminated++;
      activeTasks.erase(status.task_id());
    }

    if ( tasksTerminated == tasksPending) {
      if (tasksTerminated - tasksFinished > 0) {
        EXIT(EXIT_FAILURE)
          << "Failed to complete successfully: "
          << stringify(tasksTerminated - tasksFinished)
          << " of " << stringify(tasksPending) << " terminated abnormally";
      } else {
        driver->stop();
      }
    }
  }

  virtual void frameworkMessage(
      SchedulerDriver* driver,
      const ExecutorID& executorId,
      const SlaveID& slaveId,
      const string& data)
  {
    LOG(FATAL) << "Received framework message from executor '" << executorId
               << "' on slave " << slaveId << ": '" << data << "'";
  }

  virtual void slaveLost(
      SchedulerDriver* driver,
      const SlaveID& slaveId)
  {
    LOG(INFO) << "Lost slave " << slaveId;
  }

  virtual void executorLost(
      SchedulerDriver* driver,
      const ExecutorID& executorId,
      const SlaveID& slaveId,
      int status)
  {
    LOG(INFO) << "Lost executor '" << executorId << "' on slave "
              << slaveId << ", " << WSTRINGIFY(status);
  }

  virtual void error(
      SchedulerDriver* driver,
      const string& message)
  {
    LOG(ERROR) << message;
  }

private:
  FrameworkInfo frameworkInfo;
  list<JobSpec> jobs;
  size_t tasksFinished;
  size_t tasksTerminated;
  hashset<TaskID> activeTasks;
  size_t tasksPending;
  size_t jobScheduled;
};


class Flags : public logging::Flags
{
public:
  Flags()
  {
    add(&master,
        "master",
        "The master to connect to. May be one of:\n"
        "  master@addr:port (The PID of the master)\n"
        "  zk://host1:port1,host2:port2,.../path\n"
        "  zk://username:password@host1:port1,host2:port2,.../path\n"
        "  file://path/to/file (where file contains one of the above)");

    add(&checkpoint,
        "checkpoint",
        "Whether to enable checkpointing (true by default).",
        true);

    add(&role,
        "role",
        "Framework role.",
        "*");

    add(&principal,
        "principal",
        "To enable authentication, both --principal and --secret\n"
        "must be supplied.");

    add(&secret,
        "secret",
        "To enable authentication, both --principal and --secret\n"
        "must be supplied.");

    add(&tasks_json_path,
        "json_path",
        "File path for JSON file which specifies task to be run.");

    // If JSON is not specified.
    add(&command,
        "command",
        "The command to run for each task.",
        "echo hello");

    add(&task_resources,
        "task_resources",
        "The resources that the task uses.",
        "cpus:0.1;mem:32;disk:32");

    // TODO(bmahler): We need to take a separate flag for
    // revocable resources because there is no support yet
    // for specifying revocable resources in a resource string.
    add(&task_revocable_resources,
        "task_revocable_resources",
        "The revocable resources that the task uses.");

    add(&num_tasks,
        "num_tasks",
        "Optionally, the number of tasks to run to completion before exiting.\n"
        "If unset, as many tasks as possible will be launched.");

    add(&target_hostname,
        "target_hostname",
        "Target Slave. (Waiting for offer from specified slave.");

  }

  Option<string> master;
  bool checkpoint;
  Option<string> principal;
  Option<string> secret;
  string command;
  string task_resources;
  Option<string> task_revocable_resources;
  Option<size_t> num_tasks;
  Option<string> tasks_json_path;
  Option<string> target_hostname;
  string role;
};


list<JobSpec> parseTaskJson(const Flags flags, bool& revocable) {

  list<JobSpec> jobs;
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

  int numJobs = tasks.get().values.size();

  for(int i=0; i<numJobs;i++) {
    Result<JSON::String> _command =
        json.get().find<JSON::String>("tasks[" + stringify(i) +"].command");

    if (!_command.isSome()) {
      EXIT(EXIT_FAILURE)
      << flags.usage(
          "JSON task " + stringify(i) + "does not contain command");
    }

    Result<JSON::String> _taskResources =
        json.get().find<JSON::String>(
            "tasks[" + stringify(i) +"].taskResources");

    if (!_taskResources.isSome()) {
      EXIT(EXIT_FAILURE)
      << flags.usage(
          "JSON task " + stringify(i) + "does not contain taskResources");
    }

    Try<Resources> _resources =
        Resources::parse(_taskResources.get().value);

    if (_resources.isError()) {
      EXIT(EXIT_FAILURE)
      << flags.usage("Invalid taskResources in JSON:" +
                     _resources.error());
    }

    Result<JSON::String> _revocableTaskResources =
        json.get().find<JSON::String>(
            "tasks[" + stringify(i) +"].revocableResources");

    if (_revocableTaskResources.isSome()) {
      revocable = true;
      Try<Resources> _revocableResources =
          Resources::parse(_revocableTaskResources.get().value);

      if (_revocableResources.isError()) {
        EXIT(EXIT_FAILURE)
        << flags.usage("Invalid revocableResources in JSON: " +
                       _revocableResources.error());
      }

      foreach (Resource _revocable, _revocableResources.get()) {
        _revocable.mutable_revocable();
        _resources.get() += _revocable;
      }
    }

    Option<string> _targetHostname = None();

    Result<JSON::String> _target =
        json.get().find<JSON::String>(
            "tasks[" + stringify(i) +"].targetHostname");

    if (_target.isSome()) {
      _targetHostname = _target.get().value;
    }

    Option<size_t> _taskNum = None();

    Result<JSON::Number> _totalTasks =
        json.get().find<JSON::Number>(
            "tasks[" + stringify(i) +"].totalTasks");

    if (_totalTasks.isSome()) {
      _taskNum = _totalTasks.get().value;
    }

    jobs.push_back(JobSpec(_command.get().value,
                           _resources.get(),
                           _taskNum,
                           _targetHostname));
    jobs.back().print();
  }

  return jobs;
}

int main(int argc, char** argv)
{
  bool enableRevocable = false;
  Flags flags;

  Try<Nothing> load = flags.load("MESOS_", argc, argv);

  if (load.isError()) {
    EXIT(EXIT_FAILURE)
      << flags.usage(load.error());
  }

  if (flags.help) {
    EXIT(EXIT_SUCCESS)
      << flags.usage();
  }

  if (flags.master.isNone()) {
    EXIT(EXIT_FAILURE)
      << flags.usage("Missing required option --master");
  }

  if (flags.principal.isSome() != flags.secret.isSome()) {
    EXIT(EXIT_FAILURE)
      << flags.usage("Both --principal and --secret are required"
                     " to enable authentication");
  }

  logging::initialize(argv[0], flags, true); // Catch signals.

  FrameworkInfo framework;
  framework.set_user(""); // Have Mesos fill in the current user.
  framework.set_name("Serenity Smoke Test Framework");
  framework.set_checkpoint(flags.checkpoint);
  framework.set_role(flags.role);

  list<JobSpec> jobs;
  if (flags.tasks_json_path.isSome())
  {
    jobs = parseTaskJson(flags, enableRevocable);

  } else {
    // Task specification.
    Try<Resources> resources =
        Resources::parse(flags.task_resources);

    if (resources.isError()) {
      EXIT(EXIT_FAILURE)
      << flags.usage("Invalid --task_resources: " +
                     resources.error());
    }

    Resources taskResources = resources.get();

    if (flags.task_revocable_resources.isSome()) {
      enableRevocable = true;
      Try<Resources> revocableResources =
          Resources::parse(flags.task_revocable_resources.get());

      if (revocableResources.isError()) {
        EXIT(EXIT_FAILURE)
        << flags.usage("Invalid --task_revocable_resources: " +
                       revocableResources.error());
      }

      foreach (Resource revocable, revocableResources.get()) {
          revocable.mutable_revocable();
          taskResources += revocable;
      }
    }

    jobs.push_back(JobSpec(flags.command,
                           taskResources,
                           flags.num_tasks,
                           flags.target_hostname));

  }

  if (enableRevocable) {
    LOG(INFO) << "Enabled getting revocable resources.";
    framework.add_capabilities()->set_type(
        FrameworkInfo::Capability::REVOCABLE_RESOURCES);
  }

  if (flags.principal.isSome()) {
    framework.set_principal(flags.principal.get());
  }

  SerenityNoExecutorScheduler scheduler(framework, jobs);

  MesosSchedulerDriver* driver;

  if (flags.principal.isSome() && flags.secret.isSome()) {
    Credential credential;
    credential.set_principal(flags.principal.get());
    credential.set_secret(flags.secret.get());

    driver = new MesosSchedulerDriver(
        &scheduler, framework, flags.master.get(), credential);
  } else {
    driver = new MesosSchedulerDriver(
        &scheduler, framework, flags.master.get());
  }

  int status = driver->run() == DRIVER_STOPPED ? 0 : 1;

  // Ensure that the driver process terminates.
  driver->stop();

  delete driver;
  return status;
}
