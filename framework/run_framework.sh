#!/usr/bin/env bash

JSON_PATH=$1

source /opt/serenity/mesos/etc/mesos-slave
export MESOS_MASTER=$MESOS_MASTER

./test-framework --logging_level=INFO --json_path="${JSON_PATH}"