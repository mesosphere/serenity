#!/usr/bin/env bash

DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
SERENITY_ROOT=$( dirname $(readlink -e "${DIR}/"))

# Run lint on all src/* cpp/hpp files except smoke_test_framework source yet.
python ${DIR}/cpplint.py \
  --extensions="hpp,cpp" \
  --filter="-legal/copyright" \
$( find "${SERENITY_ROOT}/" -name "*.cpp" -or -name "*.hpp" | \
	grep -e "${SERENITY_ROOT}/src/" | \
	grep -v -e "${SERENITY_ROOT}/src/mesos_frameworks/smoke_test/")
