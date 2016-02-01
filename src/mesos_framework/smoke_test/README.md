### Smoke Test Framework Guide

Serenity introduces improved smoke_test_framework. 
It adds several features and is based on example Mesos NoExecutor framework:

1. Defining tasks scenarios using JSON files.
2. Adding URI to task.
3. Adding ability to fill all nodes resources with custom tasks.
4. Targeting task to particular host.
5. Specifying role of framework.
6. Specifying shares for each task to customize how often they should be chosen. (shares)

## Building

Smoke Test Framework builds with Serenity when `-DWITH_SOURCE_MESOS=` option 
is specified.

## Task Specification via JSON

You can run Smoke Test Framework and as the input give JSON file. 
Scheme: 
 
```javascript
{
  "tasks": [
    {
      "command": <command>,
      "uri": [Optional] {
                "value": <custom uri for fetcher>,,
                "executable": <if exec (bool)[default=false]>,
                "extract": <extract after fetching [default=true]>,
                "cache": <if using cache fetcher (bool)[default=false]>
      },
      "taskResources": <Needed resources>,
      "revocableResources": <[Optional] needed revocable resources>,
      "targetHostname": <[Optional] target host>,
      "totalTasks": <[Optional] number of tasks or if not specified - unlimited>,
      "shares" : <[Optional] share (priority) of the task> 
    }
  ]
}
```

Example (using rkt container):

```javascript
{
  "tasks": [
    {
      "command": "/usr/local/bin/rkt run  --insecure-skip-verify --mds-register=false docker://jess/stress --exec /usr/bin/stress -- -c 1",
      "uri": {
          "value": "custom_uri",
          "executable": "false",
          "extract": "true",
          "cache": "false"
      },
      "taskResources": "mem(serenity):64",
      "revocableResources": "cpus(serenity):1",
      "targetHostname": "my_super_agent@serenity.com",
      "totalTasks": 5,
      "shares" : 99
    }
  ]
}
```

## Usage

You can easily run framework using CLI:

Using JSON file (recommended):

`./test-framework --role=<role> --logging_level=INFO --json_path="${JSON_PATH}"`

Using Args:

`./test-framework --role=<role> --logging_level=INFO --task_resources=<> 
--task_revocable_resources=<> --num_tasks=<> --target_hostname=<> --uri_value --command=<>`
