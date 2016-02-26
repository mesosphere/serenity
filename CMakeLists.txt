cmake_minimum_required(VERSION 3.0.0)

option(USE_CLANG "Build using clang 3.5 compiler" "")
if (USE_CLANG)
    message("Using Clang 3.5 as CXX compiler")
    set(CMAKE_C_COMPILER "/usr/bin/clang-3.5")
    set(CMAKE_C_FLAGS  "-Wall -std=c99")
    set(CMAKE_C_FLAGS_DEBUG   "-g")
    set(CMAKE_C_FLAGS_MINSIZEREL "-Os -DNDEBUG")
    set(CMAKE_C_FLAGS_RELEASE   "-O4 -DNDEBUG")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g")
    set(CMAKE_CXX_COMPILER "/usr/bin/clang-3.5")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(EXTRA_LIBRARIES "stdc++ m")
endif(USE_CLANG)

IF(CMAKE_COMPILER_IS_GNUCXX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

project(serenity)
enable_testing()

# Command line options.
set(WITH_MESOS "/usr" CACHE STRING "Mesos installation directory")
set(MESOS_INSTALLATION_DIR ${WITH_MESOS})
message("Mesos installation directory set to: " ${MESOS_INSTALLATION_DIR})

option(INTEGRATION_TESTS "Enable compilation of integration tests." OFF)
option(CMT_ENABLED "Enable revocation based on LLC_OCCUPANCY metric." OFF)
if(CMT_ENABLED)
    message(WARNING "CMT SUPPORT IS ENABLED.")
    message("It needs to have llc_occupacy metric in ResourceUsage proto.")
    message("If Serenity does not compile, try to compile it against")
    message("https://github.com/Bplotka/mesos/tree/perf-fix-0.24.1")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCMT_ENABLED")
endif()

# Compiler things.
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-std=c++11" COMPILER_SUPPORTS_CXX11)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
    message(STATUS "Error: ${CMAKE_CXX_COMPILER} has no C++11 support")
endif()

# Some libs needs to be compiled position indepenedent.
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")

# Untar libs.
execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar xzf curlcpp.tar.gz
    COMMAND ${CMAKE_COMMAND} -E tar xzf rapidjson-v1.0.2.tar.gz
    COMMAND ${CMAKE_COMMAND} -E tar xzf gmock-1.7.0.tar.gz
    COMMAND ${CMAKE_COMMAND} -E tar xzf pbjson.tar.gz
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/lib
)

# Curlcpp.
add_subdirectory(3rdparty/lib/curlcpp)
include_directories(${CURLCPP_SOURCE_DIR}/include)

# Add gtest.
add_subdirectory (3rdparty/lib/gmock-1.7.0)
include_directories(${gmock_SOURCE_DIR} include)
include_directories(${gtest_SOURCE_DIR} include)

# Add pbjson - it doesn't have CMakeList.txt so include it manually.
set(pbjson_SOURCE_DIR 3rdparty/lib/pbjson)
include_directories(src)

# Protobuf stuff.
find_package(Protobuf REQUIRED)
set(PROTOBUF_IMPORT_DIRS ${MESOS_INSTALLATION_DIR}/include/)
include_directories(${PROTOBUF_INCLUDE_DIRS})

# Add message proto files.
file(GLOB ProtoFiles "src/messages/serenity.proto")
PROTOBUF_GENERATE_CPP(ProtoSources ProtoHeaders ${ProtoFiles})
add_library(messages STATIC ${ProtoSources} ${ProtoHeaders})
target_link_libraries(messages)

# Add message-test proto files.
file(GLOB TestProtoFiles "src/tests/common/sources/json_source.proto")
PROTOBUF_GENERATE_CPP(TestProtoSources TestProtoHeaders ${TestProtoFiles})
add_library(messages-test STATIC ${TestProtoSources} ${TestProtoHeaders})

add_library(pbjson ${pbjson_SOURCE_DIR}/src/pbjson.cpp)
target_link_libraries(pbjson protobuf)

link_directories(${MESOS_INSTALLATION_DIR}/lib/)

include_directories(
    ./
    ../
    include/
    3rdparty/lib/
    ${gmock_SOURCE_DIR}/include/
    ${gmock_SOURCE_DIR}/gtest/include/
    ${pbjson_SOURCE_DIR}/src/
    ${MESOS_INSTALLATION_DIR}/include/
    ${CMAKE_BINARY_DIR}/
    $(CMAKE_INCLUDE_PATH)
)

set(SERENITY_SOURCES
    src/bus/event_bus.cpp
    src/contention_detectors/overload.cpp
    src/contention_detectors/signal_based.cpp
    src/contention_detectors/signal_analyzers/drop.cpp
    src/filters/cumulative.cpp
    src/filters/ema.cpp
    src/filters/executor_age.cpp
    src/filters/ignore_new_executors.cpp
    src/filters/pr_executor_pass.cpp
    src/filters/too_low_usage.cpp
    src/filters/utilization_threshold.cpp
    src/filters/valve.cpp
    src/mesos_modules/qos_controller/serenity_controller.cpp
    src/mesos_modules/qos_controller/serenity_controller_module.cpp
    src/mesos_modules/resource_estimator/serenity_estimator.cpp
    src/mesos_modules/resource_estimator/serenity_estimator_module.cpp
    src/observers/qos_correction.cpp
    src/observers/slack_resource.cpp
    src/observers/strategies/cache_occupancy.cpp
    src/observers/strategies/cpu_contention.cpp
    src/observers/strategies/seniority.cpp
    src/serenity/agent_utils.cpp
    src/serenity/resource_helper.cpp
    src/serenity/wid.cpp
    src/time_series_export/resource_usage_ts_export.cpp
    src/time_series_export/slack_ts_export.cpp
    src/time_series_export/backend/influx_db9.cpp
)

set(SERENITY_TEST_SOURCES
    src/tests/main.cpp
    src/tests/bus/event_bus_tests.cpp
    src/tests/common/sources/json_source.cpp
    src/tests/contention_detectors/signal_analyzers/drop_test.cpp
    src/tests/contention_detectors/overload_test.cpp
    src/tests/filters/correction_merger_test.cpp
    src/tests/filters/ema_test.cpp
    src/tests/filters/ignore_new_executors_test.cpp
    src/tests/filters/pr_executor_pass_test.cpp
    src/tests/filters/utilization_threshold_test.cpp
    src/tests/filters/valve_test.cpp
    src/tests/mesos_modules/qos_controller/qos_controller_test.cpp
    src/tests/mesos_modules/resource_estimator/estimator_test.cpp
    src/tests/pipeline/estimator_pipeline_test.cpp
    src/tests/observers/slack_resource_test.cpp
    src/tests/observers/qos_correction_test.cpp
    src/tests/observers/strategies/cache_occupancy_strategy_test.cpp
    src/tests/observers/strategies/seniority_strategy_test
    src/tests/serenity/config_test.cpp
    src/tests/serenity/os_utils_tests.cpp
    src/tests/serenity/resource_helper_test.cpp
    src/tests/serenity/serenity_tests.cpp
    src/tests/sources/json_source_test.cpp
)

if (INTEGRATION_TESTS)
    SET(INTEGRATION_TEST_SOURCES
        # Needs Mesos Agent working on http://localhost:5051.
        src/tests/serenity/agent_utils_tests.cpp

        # Needs InfluxDB 0.9
        src/tests/time_series_export/backend/influx_db9_test.cpp

        # Needs InfluxDB 0.9 and Mesos Agent working on http://localhost:5051.
        src/tests/time_series_export/resource_usage_ts_export_test.cpp
        src/tests/time_series_export/slack_ts_export_test.cpp
    )
endif(INTEGRATION_TESTS)

# Copy fixtures.
file(GLOB TestFixtures "src/tests/fixtures")
file(COPY ${TestFixtures} DESTINATION tests/)

# Serenity library.
add_library(serenity SHARED ${SERENITY_SOURCES})
target_link_libraries(serenity messages curlcpp)

# Tests.
add_executable(serenity-tests
    ${SERENITY_TEST_SOURCES}
    ${INTEGRATION_TEST_SOURCES}
)

target_link_libraries(serenity-tests
    stdc++
    m
    gmock
    gmock_main
    gtest
    gtest_main
    mesos
    messages-test
    pbjson
    serenity
    # Clang cannot compile it because of the error.
    # Without glog ./serenity-tests ends with segmentation fault.
    glog
)
add_test(serenity-tests serenity-tests)


# Smoke Test Framework requires mesos source directory.
# If WITH_SOURCE_MESOS is not specified, STF is omitted.
set(WITH_SOURCE_MESOS "" CACHE STRING "Mesos source directory")
set(MESOS_SOURCE_DIR ${WITH_SOURCE_MESOS})

if (MESOS_SOURCE_DIR)
    message("Mesos source directory set to: " ${MESOS_SOURCE_DIR})

    include_directories(
        ${MESOS_SOURCE_DIR}/build/src/
        ${MESOS_SOURCE_DIR}/src/
        src/mesos_frameworks/smoke_test/
        src/time_series_export/
    )
    # Serenity test framework exe.
    add_executable(smoke_test_framework
        src/mesos_frameworks/smoke_test/smoke_test_framework.cpp
        src/time_series_export/backend/influx_db9.cpp)
    target_link_libraries(smoke_test_framework mesos protobuf glog stdc++ m curlcpp)

else(MESOS_SOURCE_DIR)
    message(
        "Mesos source directory was not set. "
        "Smoke Test Framework compilation will be omitted.")
endif(MESOS_SOURCE_DIR)
