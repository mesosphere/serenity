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

#define PICOJSON_USE_INT64

#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <mesos/resources.hpp>
#include <mesos/scheduler.hpp>
#include <mesos/type_utils.hpp>

#include <stout/exit.hpp>
#include <stout/flags.hpp>
#include <stout/foreach.hpp>
#include <stout/hashset.hpp>
#include <stout/json.hpp>
#include <stout/option.hpp>
#include <stout/os.hpp>
#include <stout/path.hpp>
#include <stout/stringify.hpp>
#include <stout/try.hpp>
#include <stout/duration.hpp>

#include "common/protobuf_utils.hpp"
#include "common/status_utils.hpp"

#include "logging/logging.hpp"

#include "time_series_export/backend/influx_db8.hpp"
#include "time_series_export/backend/time_series_backend.hpp"

#include "smoke_flags.hpp"
#include "smoke_job.hpp"
#include "smoke_queue.hpp"

#include "process/future.hpp"
#include "process/owned.hpp"
#include "process/defer.hpp"
#include "process/delay.hpp"
#include "process/dispatch.hpp"
#include "process/process.hpp"


using namespace mesos;
using namespace mesos::internal;
using namespace mesos::serenity;

using std::list;
using std::map;
using std::pair;
using std::string;
using std::vector;


using mesos::Resources;

const constexpr char* const ANY_HOSTNAME = "ANY_HOSTNAME";

static Offer::Operation LAUNCH(const vector<TaskInfo>& tasks)
{
  Offer::Operation operation;
  operation.set_type(Offer::Operation::LAUNCH);

  foreach (const TaskInfo& task, tasks) {
    operation.mutable_launch()->add_task_infos()->CopyFrom(task);
  }

  return operation;
}

// Needed for Serenity demo.
class SerenityNoExecutorSchedulerProcess
  : public process::Process<SerenityNoExecutorSchedulerProcess> {
public:
  explicit SerenityNoExecutorSchedulerProcess(
    const list<std::shared_ptr<SmokeJob>> _jobs,
    const Duration _reportInterval = Duration::create(0.5).get())
    : jobs(_jobs),
      reportInterval(reportInterval),
      dbBackend(new InfluxDb8Backend()),
      ProcessBase("serenity_scheduler_stats") {

    process::delay(reportInterval,
                   self(),
                   &SerenityNoExecutorSchedulerProcess::reportToInfluxDb);
  }

  void reportToInfluxDb() {
    LOG(INFO) << "Reporting to influx";
    for (std::shared_ptr<SmokeJob> job : jobs) {
      sendToInflux(Series::RUNNING_TASKS,
                   job->name,
                   job->runningTasks);

      sendToInflux(Series::REVOKED_TASKS,
                   job->name,
                   job->revokedTasks);

      sendToInflux(Series::FINISHED_TASKS,
                   job->name,
                   job->finishedTasks);

      sendToInflux(Series::FAILED_TASKS,
                   job->name,
                   job->failedTasks);
    }

    process::delay(reportInterval,
                   self(),
                   &SerenityNoExecutorSchedulerProcess::reportToInfluxDb);
  }

  // TODO(bplotka): Add hostname.
  void sendToInflux(const Series series,
                    const std::string taskName,
                    const int64_t value = 1) {
    TimeSeriesRecord record(series, value);
    record.setTag(TsTag::TASK_NAME, taskName);
    dbBackend->PutMetric(record);
  }

  virtual ~SerenityNoExecutorSchedulerProcess() {}

private:
  const list<std::shared_ptr<SmokeJob>> jobs;
  std::unique_ptr<TimeSeriesBackend> dbBackend;
  const Duration reportInterval;
};


/**
 * Serenity No Executor Scheduler based on Mesos No Executor Scheduler.
 */
class SerenityNoExecutorScheduler : public Scheduler
{
 public:
  SerenityNoExecutorScheduler(
      const FrameworkInfo& _frameworkInfo,
      const list<std::shared_ptr<SmokeJob>>& _jobs)
    : frameworkInfo(_frameworkInfo),
      tasksLaunched(0u),
      tasksFinished(0u),
      tasksTerminated(0u),
      jobsScheduled(0u),
      jobs(_jobs) {
    anyHostnameQueue = SmokeAliasQueue();

    // For all jobs with specified target hostname.
    for (auto& job : jobs) {
      if (job->targetHostname.isNone()) continue;

      // Add job for targeted queue.
      auto jobQueue = queue.find(job->targetHostname.get());
      if (jobQueue == queue.end()) {
        queue[job->targetHostname.get()] = SmokeAliasQueue();
      }

      queue[job->targetHostname.get()]
        .add(job);
    }

    // For all jobs with not specified target hostname.
    for (auto& job : jobs) {
      if (job->targetHostname.isSome()) continue;

      // Add job to all targeted queues.
      for (std::pair<string, SmokeAliasQueue> jobQueue : queue) {
        queue[jobQueue.first].add(job);
      }

      // Additionaly add job to anyHostname queue.
      anyHostnameQueue.add(job);
    }

    process = process::Owned<SerenityNoExecutorSchedulerProcess>(
      new SerenityNoExecutorSchedulerProcess(jobs));

    spawn(process.get());

    LOG(INFO) << "SerenityNoExecutorScheduler initialized. Jobs: "
              << jobs.size();
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
    for (const Offer& offer : offers) {
      // Check each offer.
      if (allJobsScheduled()) {
        // In case of end of our scheduling - fully resign from any offer.
        LOG(INFO) << "End of scheduling. Decling offers";
        Filters filters;
        filters.set_refuse_seconds(Duration::max().secs());
        driver->declineOffer(offer.id(), filters);
        continue;
      }
      LOG(INFO) << " ---- Received offer " << offer.id() << " from slave "
                << offer.slave_id() << " (" << offer.hostname() << ") "
                << "with " << offer.resources();

      Resources remaining = offer.resources();
      vector<TaskInfo> tasks;
      while(!allJobsScheduled()) {

        auto jobQueuePair = queue.find(offer.hostname());
        SmokeAliasQueue* jobQueue = &anyHostnameQueue;
        if (jobQueuePair != queue.end()) {
          jobQueue = &jobQueuePair->second;
          LOG(INFO) << "Found jobQueue of size: " << jobQueue->size();
        }

        if (jobQueue->finished) break;
        std::shared_ptr<SmokeJob> job = jobQueue->selectJob();
        if (job == nullptr) break;
        if (job->finished()) {
          jobQueue->removeAndReset(job);
          continue;
        }

        // Check if there are still resources for next task.
        if (!remaining.contains(job->taskResources)) {
          LOG(INFO) << "Not enough resources for "
          << stringify(job->id) + "_"
             + stringify(job->tasksLaunched)
          << "( " << stringify(job->command) << ") "
          << "job. Needed: " << job->taskResources
          << " Offered: " << remaining;
          break;
        }

        remaining -= job->taskResources;

        tasks.push_back(job->createTask(offer.slave_id()));

        this->activeTasks.insert(std::pair<TaskID, SmokeTask>(
            tasks.back().task_id(), SmokeTask(job, offer.hostname())));

        job->tasksLaunched++;
        tasksLaunched++;
        LOG(INFO) << "Prepared " << tasks.back().task_id()
                  << " ( " << stringify(job->command) << ")";

        if (job->finished()) {
          // In case of limited jobs stop when scheduled totalTasks.
          this->jobsScheduled++;
          // Recalculate Alias alghoritm.
          jobQueue->removeAndReset(job);
        }
      }

      if (tasks.size() > 0)
        LOG(INFO) << " ---- Launching these " << tasks.size() << " tasks.";
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
    auto task = activeTasks.find(status.task_id());
    if (task == activeTasks.end()) {
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


      if (status.state() == TASK_LOST &&
          status.reason() ==  TaskStatus::REASON_EXECUTOR_PREEMPTED) {
        // Executor was preempted.
//        LOG(INFO) << "Sending data about revoked task to InfluxDB";
//        statTaskRevoked(task->second, status);
      } else {
//        LOG(INFO) << "Sending data about failed task to InfluxDB";
//        statTaskFailed(task->second, status);
      }
    } else {
//      LOG(INFO) << "Task '" << status.task_id() << "'"
//                << " is in state " << status.state();
//      LOG(INFO) << "Sending data about started task to InfluxDB";
//      statTaskRunning(task->second, status);
    }

    if (internal::protobuf::isTerminalState(status.state())) {
      if (status.state() == TASK_FINISHED) {
//        LOG(INFO) << "Sending data about finished task to InfluxDB";
//        statTaskFinished(task->second, status);
        tasksFinished++;
      }

      tasksTerminated++;
      activeTasks.erase(status.task_id());
    }

    if (this->allJobsScheduled() &&
        tasksTerminated >= tasksLaunched) {
      if (tasksTerminated - tasksFinished > 0) {
        EXIT(EXIT_FAILURE)
          << "Failed to complete successfully: "
          << stringify(tasksTerminated - tasksFinished)
          << " of " << stringify(tasksLaunched) << " terminated abnormally";
      } else {
        LOG(INFO) << "Stopping framework.";
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

  // Needed for Serenity demo.
  void statTaskRunning(const SmokeTask& task,
                       const TaskStatus& status) {
    sendToInflux(Series::RUNNING_TASKS, task,
                 status, ++task.jobPtr->runningTasks);
  }

  void statTaskRevoked(const SmokeTask& task,
                       const TaskStatus& status) {
    sendToInflux(Series::RUNNING_TASKS, task,
                 status, --task.jobPtr->runningTasks);
    sendToInflux(Series::REVOKED_TASKS, task,
                 status, ++task.jobPtr->revokedTasks);
  }

  void statTaskFailed(const SmokeTask& task,
                       const TaskStatus& status) {
    sendToInflux(Series::RUNNING_TASKS, task,
                 status, --task.jobPtr->runningTasks);
    sendToInflux(Series::FAILED_TASKS, task,
                 status, ++task.jobPtr->failedTasks);
  }

  void statTaskFinished(const SmokeTask& task,
                      const TaskStatus& status) {
    sendToInflux(Series::RUNNING_TASKS, task,
                 status, --task.jobPtr->runningTasks);
    sendToInflux(Series::FINISHED_TASKS, task,
                 status, ++task.jobPtr->finishedTasks);
  }


  void sendToInflux(const Series series,
                    const SmokeTask& task,
                    const TaskStatus& status,
                    const int64_t value = 1) {
    TimeSeriesRecord record(series, value);
    record.setTag(TsTag::TASK_ID, status.task_id().value());
    record.setTag(TsTag::EXECUTOR_ID, status.executor_id().value());
    record.setTag(TsTag::HOSTNAME, task.hostname); //get hostname
    record.setTag(TsTag::TASK_NAME, task.jobPtr->name);
    //dbBackend->PutMetric(record);
  }

  ~SerenityNoExecutorScheduler() {
    process::terminate(process.get());
    process::wait(process.get());
  }

private:
  FrameworkInfo frameworkInfo;
  list<std::shared_ptr<SmokeJob>> jobs;
  map<string, SmokeAliasQueue> queue;
  SmokeAliasQueue anyHostnameQueue;
  size_t tasksLaunched;
  size_t tasksFinished;
  size_t tasksTerminated;
  hashmap<TaskID, SmokeTask> activeTasks;
  size_t jobsScheduled;

  process::Owned<SerenityNoExecutorSchedulerProcess> process;

  bool allJobsScheduled() {
    return jobsScheduled >= jobs.size();
  }
};


int main(int argc, char** argv)
{
  bool enableRevocable = false;
  SmokeFlags flags;

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
  framework.set_name(flags.name);
  framework.set_checkpoint(flags.checkpoint);
  framework.set_role(flags.role);

  list<std::shared_ptr<SmokeJob>> jobs;
  if (flags.tasks_json_path.isSome())
  {
    jobs = SmokeJob::createJobsFromJson(flags, enableRevocable);

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

    Option<SmokeURI> uri = None();
    if (flags.uri_value.isSome()) {
      uri = SmokeURI(flags.uri_value.get());
    }
    jobs.push_back(std::shared_ptr<SmokeJob>(
      new SmokeJob(0, flags.command,
               taskResources,
               flags.num_tasks,
               flags.command,
               flags.target_hostname,
               uri)));
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
