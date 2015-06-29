#
# Usage generator for Serenity tests.
#
# Produces artificial resource usage scenarios to exercise filters.
#

import time
import uuid
import random
from jinja2 import Template
from math import sin

USAGE_JSON = """
{
  "resource_usage": [{% for sample in samples %}
    {
      "executors": [{% for executor in sample.executor_snapshots %}
        {
          "allocated": [
            {
              "name": "cpus",
              "role": "*",
              "scalar": {
                "value": {{executor.allocation.cpus}}
              }
            },
            {
              "name": "mem",
              "role": "*",
              "scalar": {
                "value": {{executor.allocation.mem}}
              }
            }
          ],
          "executor_info": {
            "executor_id": {
              "value": "{{executor.id}}"
            },
            "framework_id": {
              "value": "{{executor.framework_id}}"
            }
          },
          "statistics": {
            "cpus_limit": {{executor.allocation.cpus}},
            "cpus_system_time_secs": {{executor.used.system_time}},
            "cpus_user_time_secs": {{executor.used.user_time}},
            "mem_limit_bytes": {{executor.allocation.mem * 1024}},
            "mem_rss_bytes": {{executor.used.mem_rss_bytes}},
            "timestamp": {{sample.timestamp}}
          }
        }{% if (sample.executor_snapshots[-1] != executor) %},{% endif %}{% endfor %}
      ]
    }{% if (samples[-1] != sample) %},{% endif %}{% endfor %}
  ]
}
"""

#
# Framework
#
class Framework:
    def __init__(self, executors):
        self.id = uuid.uuid4()
        self.executors = []

        for i in range(0, executors):
            self.executors.append(Executor(self.id))

#
# Executor
#
class Executor:
    def __init__(self, framework_id):
        self.id = uuid.uuid4()
        self.framework_id = framework_id
        self.last_timestamp = 0

        # TODO(nnielsen): Make allocation configurable and tied to a changing
        # number of tasks.
        self.cpus = random.randint(1, 8)
        self.mem = random.randint(32, 4096)

        # TODO(nnielsen): Put together random function parameters here for load
        # generator.
        y = random.random()
        self.load = lambda x: ((y*1+sin(x))/2 + (y*1+sin(x/2))/2)/2

    def cpus_allocated(self, timestamp):
        return self.cpus

    def mem_allocated(self, timestamp):
        return self.mem

    def system_time(self, timestamp):
        # TODO(nnielsen): Distribute between system and user time.
        return 0.0

    def user_time(self, timestamp):
        if (self.last_timestamp == 0):
            self.last_timestamp = timestamp
            return 0

        delta = timestamp - self.last_timestamp

        load = abs(self.load(timestamp))

        time = (self.cpus_allocated(timestamp) * load) / delta

        self.last_timestamp = timestamp

        return time

    def mem_rss_bytes(self, timestamp):
        # TODO(nnielsen): Make memory usage follow a trend as well.
        return (self.mem * 1024) / random.randint(1, 8)

#
# Classes for template modeling.
#
class ExecutorSnapshot:
    def __init__(self, executor, timestamp):
        self.id = executor.id
        self.framework_id = executor.framework_id
        self.allocation = ExecutorAllocation(executor, timestamp)
        self.used = ExecutorUsage(executor, timestamp)

class ExecutorAllocation:
    def __init__(self, executor, timestamp):
        self.cpus = executor.cpus_allocated(timestamp)
        self.mem = executor.mem_allocated(timestamp)

class ExecutorUsage:
    def __init__(self, executor, timestamp):
        self.system_time = executor.system_time(timestamp)
        self.user_time = executor.user_time(timestamp)
        self.mem_rss_bytes = executor.mem_rss_bytes(timestamp)

class Sample:
    def __init__(self, timestamp, frameworks):
        self.executor_snapshots = []

        self.timestamp = timestamp

        for framework in frameworks:
            for executor in framework.executors: 
                self.executor_snapshots.append(ExecutorSnapshot(executor, timestamp))

#
# Clock
#
# Control sample rate and total session time.
#
class Clock:
    def __init__(self, frameworks):
        self.cur_time = int(time.time())
        self.frameworks = []

        for i in range(0, frameworks):
            # TODO(nnielsen): Make executor count a random value.
            self.frameworks.append(Framework(2))

    def sample(self):
        # TODO(nnielsen): Add random sample interval time.
        self.cur_time += 1
        return Sample(self.cur_time, self.frameworks)

if __name__ == '__main__':
    # TODO(nnielsen): Read number of frameworks from argument list.
    # TODO(nnielsen): Read slave capacity from argument list.

    clock = Clock(4)

    samples = []

    # TODO(nnielsen): Read session length from argument list.
    for i in range(0, 1000):
        samples.append(clock.sample())

    t = Template(USAGE_JSON)
    print t.render(samples=samples)
