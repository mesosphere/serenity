#!/usr/bin/env bash

DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
SERENITY_ROOT=$( dirname $(readlink -e "${DIR}/"))

python ${DIR}/cpplint.py \
  --extensions="hpp,cpp" \
  --filter="-legal/copyright" \
$( find "${SERENITY_ROOT}/" -name "*.cpp" -or -name "*.hpp" | grep -v -e "${SERENITY_ROOT}/build/" -e "${SERENITY_ROOT}/src/mesos_framework/smoke_test/" -e "${SERENITY_ROOT}/3rdparty/" )
