cmake_minimum_required(VERSION 3.0.0)
project(serenity)
enable_testing()

#Command line options.
SET(WITH_MESOS "/usr" CACHE STRING "Mesos installation directory")
SET(MESOS_INSTALLATION_DIR ${WITH_MESOS})
MESSAGE("Mesos installation directory set to: " ${MESOS_INSTALLATION_DIR})

OPTION(INTEGRATION_TESTS "Enable compliation of integration tests." OFF)

# Compiler things.
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
    message(STATUS "Error: ${CMAKE_CXX_COMPILER} has no C++11 support")
endif()

# some libs needs to be compiled position indepenedent.
IF( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
ENDIF( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )


# untar libs.
execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar xzf curlcpp.tar.gz
    COMMAND ${CMAKE_COMMAND} -E tar xzf rapidjson-v1.0.2.tar.gz
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

# Add gtest.
ADD_SUBDIRECTORY (lib/gmock-1.7.0)
include_directories( ${gmock_SOURCE_DIR} include)
include_directories( ${gtest_SOURCE_DIR} include)

# Add pbjson - it doesn't have CMakeList.txt so include it manually.
set(pbjson_SOURCE_DIR lib/pbjson)

# curlcpp
ADD_SUBDIRECTORY(lib/curlcpp)
INCLUDE_DIRECTORIES(${CURLCPP_SOURCE_DIR}/include)

# rapidjson
ADD_SUBDIRECTORY(lib/rapidjson-1.0.2)
INCLUDE_DIRECTORIES(lib/rapidjson-1.0.2/include)

# Protobuf stuff.
find_package(Protobuf REQUIRED)
set(PROTOBUF_IMPORT_DIRS ${MESOS_INSTALLATION_DIR}/include/)

# Add message proto files.
file(GLOB ProtoFiles "messages/serenity.proto")
PROTOBUF_GENERATE_CPP(ProtoSources ProtoHeaders ${ProtoFiles})
add_library(messages STATIC ${ProtoSources} ${ProtoHeaders})

# Add message-test proto files.
file(GLOB TestProtoFiles "tests/common/sources/json_source.proto")
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
    bus/bus_socket.cpp
    estimator/serenity_estimator.cpp
    estimator/serenity_estimator_module.cpp
    filters/drop.cpp
    filters/ema.cpp
    filters/ignore_new_executors.cpp
    filters/pr_executor_pass.cpp
    filters/utilization_threshold.cpp
    filters/valve.cpp
    observers/qos_correction.cpp
    observers/slack_resource.cpp
    qos_controller/serenity_controller.cpp
    qos_controller/serenity_controller_module.cpp
    serenity/wid.cpp
    time_series_export/resource_usage_ts_export.cpp
    time_series_export/slack_ts_export.cpp
    time_series_export/backend/influx_db8.cpp
    serenity/agent_utils.cpp
)

set(SERENITY_TEST_SOURCES
    tests/main.cpp
    tests/common/sources/json_source.cpp
    tests/controllers/qos_controller_test.cpp
    tests/estimators/estimator_test.cpp
    tests/filters/drop/test.cpp
    tests/filters/drop/naive_changepoint_test.cpp
    tests/filters/drop/rolling_changepoint_test.cpp
    tests/filters/ema_test.cpp
    tests/filters/ignore_new_executors_test.cpp
    tests/filters/pr_executor_pass_test.cpp
    tests/filters/utilization_threshold_test.cpp
    tests/filters/valve_test.cpp
    tests/pipeline/estimator_pipeline_test.cpp
    tests/pipeline/qos_pipeline_test.cpp
    tests/observers/slack_resource_test.cpp
    tests/observers/qos_correction_test.cpp
    tests/serenity/os_utils_tests.cpp
    tests/sources/json_source_test.cpp
)

if (INTEGRATION_TESTS)
    SET(INTEGRATION_TEST_SOURCES
        # needs InfluxDB 8.
        tests/time_series_export/resource_usage_ts_export_test.cpp
        tests/time_series_export/slack_ts_export_test.cpp

        # needs Mesos Agent working on http://localhost:5051.
        tests/serenity/agent_utils_tests.cpp
    )
endif(INTEGRATION_TESTS)

# Copy fixtures.
file(GLOB TestFixtures "tests/fixtures")
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


# For our smoke-test-framework we use NoExecutorFramework
# which is sufficient for us. It requires mesos source directory.
# If WITH_SOURCE_MESOS is not specified, test-framework is omitted.
SET(WITH_SOURCE_MESOS "" CACHE STRING "Mesos source directory")
SET(MESOS_SOURCE_DIR ${WITH_SOURCE_MESOS})

if (MESOS_SOURCE_DIR)
    MESSAGE("Mesos source directory set to: " ${MESOS_SOURCE_DIR})

    include_directories(
        ${MESOS_SOURCE_DIR}/build/src/
        ${MESOS_SOURCE_DIR}/src/
    )
    # Serenity test framework exe.
    add_executable(test-framework
        ${MESOS_SOURCE_DIR}/src/examples/no_executor_framework.cpp)
    target_link_libraries(test-framework mesos protobuf glog)

else(MESOS_SOURCE_DIR)
    MESSAGE(
        "Mesos source directory was not set. "
        "Test-framework compilation will be omitted.")
endif(MESOS_SOURCE_DIR)