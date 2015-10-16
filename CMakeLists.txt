cmake_minimum_required(VERSION 3.0.0)
project(serenity)
enable_testing()

# Command line options.
set(WITH_MESOS "/usr" CACHE STRING "Mesos installation directory")
set(MESOS_INSTALLATION_DIR ${WITH_MESOS})
message("Mesos installation directory set to: " ${MESOS_INSTALLATION_DIR})

option(INTEGRATION_TESTS "Enable compilation of integration tests." OFF)

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
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

# Curlcpp.
add_subdirectory(lib/curlcpp)
include_directories(${CURLCPP_SOURCE_DIR}/include)

# Rapidjson.
add_subdirectory(lib/rapidjson-1.0.2)
include_directories(lib/rapidjson-1.0.2/include)

# Add gtest.
add_subdirectory (lib/gmock-1.7.0)
include_directories(${gmock_SOURCE_DIR} include)
include_directories(${gtest_SOURCE_DIR} include)

# Add pbjson - it doesn't have CMakeList.txt so include it manually.
set(pbjson_SOURCE_DIR lib/pbjson)
include_directories(src)

# Protobuf stuff.
find_package(Protobuf REQUIRED)
set(PROTOBUF_IMPORT_DIRS ${MESOS_INSTALLATION_DIR}/include/)

# Add message proto files.
file(GLOB ProtoFiles "src/messages/serenity.proto")
PROTOBUF_GENERATE_CPP(ProtoSources ProtoHeaders ${ProtoFiles})
add_library(messages STATIC ${ProtoSources} ${ProtoHeaders})

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
    ${gmock_SOURCE_DIR}/include/
    ${gmock_SOURCE_DIR}/gtest/include/
    ${pbjson_SOURCE_DIR}/src/
    ${MESOS_INSTALLATION_DIR}/include
    ${CMAKE_BINARY_DIR}/
)

set(SERENITY_SOURCES
    src/bus/event_bus.cpp
    src/detectors/assurance.cpp
    src/estimator/serenity_estimator.cpp
    src/estimator/serenity_estimator_module.cpp
    src/filters/drop.cpp
    src/filters/ema.cpp
    src/filters/ignore_new_executors.cpp
    src/filters/pr_executor_pass.cpp
    src/filters/utilization_threshold.cpp
    src/filters/valve.cpp
    src/filters/executor_age.cpp
    src/observers/qos_correction.cpp
    src/observers/slack_resource.cpp
    src/qos_controller/serenity_controller.cpp
    src/qos_controller/serenity_controller_module.cpp
    src/serenity/agent_utils.cpp
    src/serenity/wid.cpp
    src/time_series_export/resource_usage_ts_export.cpp
    src/time_series_export/slack_ts_export.cpp
    src/time_series_export/backend/influx_db8.cpp)

set(SERENITY_TEST_SOURCES
    src/tests/main.cpp
    src/tests/bus/event_bus_tests.cpp
    src/tests/common/sources/json_source.cpp
    src/tests/controllers/qos_controller_test.cpp
    src/tests/estimators/estimator_test.cpp
    src/tests/filters/drop_test.cpp
    src/tests/filters/detectors/naive_test.cpp
    src/tests/filters/detectors/rolling_test.cpp
    src/tests/filters/ema_test.cpp
    src/tests/filters/ignore_new_executors_test.cpp
    src/tests/filters/pr_executor_pass_test.cpp
    src/tests/filters/utilization_threshold_test.cpp
    src/tests/filters/valve_test.cpp
    src/tests/pipeline/estimator_pipeline_test.cpp
    src/tests/pipeline/qos_pipeline_test.cpp
    src/tests/observers/slack_resource_test.cpp
    src/tests/observers/qos_correction_test.cpp
    src/tests/serenity/os_utils_tests.cpp
    src/tests/sources/json_source_test.cpp
)

if (INTEGRATION_TESTS)
    SET(INTEGRATION_TEST_SOURCES
        # Needs Mesos Agent working on http://localhost:5051.
        src/tests/serenity/agent_utils_tests.cpp

        # Needs InfluxDB 0.8.8
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
    gmock
    gmock_main
    gtest
    gtest_main
    mesos
    glog # Need to be specified after libmesos
    messages-test
    pbjson
    serenity
)
add_test(serenity-tests serenity-tests)


# Smoke-test-framework requires mesos source directory.
# If WITH_SOURCE_MESOS is not specified, test-framework is omitted.
set(WITH_SOURCE_MESOS "" CACHE STRING "Mesos source directory")
set(MESOS_SOURCE_DIR ${WITH_SOURCE_MESOS})

if (MESOS_SOURCE_DIR)
    message("Mesos source directory set to: " ${MESOS_SOURCE_DIR})

    include_directories(
        ${MESOS_SOURCE_DIR}/build/src/
        ${MESOS_SOURCE_DIR}/src/
    )
    # Serenity test framework exe.
    add_executable(test-framework
        src/framework/smoke_test_framework.cpp)
    target_link_libraries(test-framework mesos protobuf glog)

else(MESOS_SOURCE_DIR)
    message(
        "Mesos source directory was not set. "
        "Test-framework compilation will be omitted.")
endif(MESOS_SOURCE_DIR)
