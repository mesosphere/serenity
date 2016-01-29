#ifndef SERENITY_SMOKE_FLAGS_HPP
#define SERENITY_SMOKE_FLAGS_HPP

#include <string>

#include <stout/option.hpp>

#include "logging/flags.hpp"


class SmokeFlags : public mesos::internal::logging::Flags
{
public:
  SmokeFlags()
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

    add(&name,
        "name",
        "Framework name.",
        "Serenity Smoke Test Framework");

    add(&principal,
        "principal",
        "To enable authentication, both --principal and --secret\n"
          "must be supplied.");

    add(&secret,
        "secret",
        "To enable authentication, both --principal and --secret\n"
          "must be supplied.");

    // Task specification via args.
    add(&command,
        "command",
        "The command to run for each task.",
        "echo hello");

    add(&uri_value,
        "URI value",
        "URI for Mesos fetcher.");

    add(&task_resources,
        "task_resources",
        "The resources that the task uses.",
        "cpus:0.1;mem:32;disk:32");

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

    // Task specification via JSON.
    add(&tasks_json_path,
        "json_path",
        "File path for JSON file which specifies task to be run.\n"
          "Scheme:\n"
          "{\n"
          "  \"tasks\": [\n"
          "    {\n"
          "      \"command\": <command>,\n"
          "      \"uri\": [Optional] {\n"
          "                \"value\": <custom uri for fetcher>,,\n"
          "                \"executable\": <if exec (bool)>,\n"
          "                \"extract\": \"<if extract after fetching (bool)>\",\n"
          "                \"cache\": \"<if using cache fetcher (bool)>\"\n"
          "      },\n"
          "      \"taskResources\": <Needed resources>,\n"
          "      \"revocableResources\": <[Optional] needed revocable resources>,\n"
          "      \"targetHostname\": <[Optional] target host>,\n"
          "      \"totalTasks\": <[Optional] number of tasks or if not specified - unlimited>,\n"
          "      \"priority\" : <[Optional] priority of the task>\n"
          "      \"name\" : <[Optional] name of task in InfluxDb>\n"
          "    }\n"
          "  ]\n"
          "}");
  }

  Option<std::string> master;
  bool checkpoint;
  Option<std::string> principal;
  Option<std::string> secret;
  std::string command;
  std::string task_resources;
  Option<std::string> task_revocable_resources;
  Option<size_t> num_tasks;
  Option<std::string> tasks_json_path;
  Option<std::string> target_hostname;
  Option<std::string> uri_value;
  std::string role;
  std::string name;
};


#endif //SERENITY_SMOKE_FLAGS_HPP
