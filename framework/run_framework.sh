#!/usr/bin/env bash

JSON_PATH=$1

source /opt/serenity/mesos/etc/mesos-slave
export MESOS_MASTER=$MESOS_MASTER

./test-framework --role=serenity --logging_level=INFO --json_path="${JSON_PATH}"