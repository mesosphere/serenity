cmake_minimum_required(VERSION 3.0.0)
project(serenity)
enable_testing()

#Command line options
SET(WITH_MESOS "/usr" CACHE STRING "Mesos installation directory")
SET(MESOS_INSTALLATION_DIR ${WITH_MESOS})
MESSAGE("Mesos installation directory set to: " ${MESOS_INSTALLATION_DIR})

# Add gtest
ADD_SUBDIRECTORY (lib/gmock-1.7.0)
include_directories( ${gmock_SOURCE_DIR} include)
include_directories( ${gtest_SOURCE_DIR} include)

# CXX Flags
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# Protobuf stuff
find_package(Protobuf REQUIRED)
set(PROTOBUF_IMPORT_DIRS ${MESOS_INSTALLATION_DIR}/include/)
file(GLOB ProtoFiles "${CMAKE_CURRENT_SOURCE_DIR}/messages/serenity.proto")
file(GLOB TestProtoFiles "tests/helpers/sources/json_source.proto")
PROTOBUF_GENERATE_CPP(ProtoSources ProtoHeaders ${ProtoFiles})
add_library(messages STATIC ${ProtoSources} ${ProtoHeaders})

PROTOBUF_GENERATE_CPP(TestProtoSources TestProtoHeaders ${TestProtoFiles})
add_library(messages-test STATIC ${TestProtoSources} ${TestProtoHeaders})


# pbjson + rapidjson
add_library(pbjson lib/pbjson/src/pbjson.cpp)
target_link_libraries(pbjson protobuf)
include_directories(

    ./
    ../
    include/
    ${MESOS_INSTALLATION_DIR}/include

    ${gmock_SOURCE_DIR}/include/
    ${gmock_SOURCE_DIR}/gtest/include/

    lib/pbjson/src/

    ${CMAKE_BINARY_DIR}/
)

set(SERENITY_SOURCES
    bus/bus_socket.cpp
    estimator/serenity_estimator.cpp
    filters/moving_average.cpp
    filters/moving_average.hpp
    observers/ipc_drop.cpp
    qos_controller/serenity_controller.cpp
    sources/dummy_source.cpp
)

set(PIPELINE_TEST_SOURCES
    tests/integration/pipeline_test.cpp
)

set(SERENITY_UT_TESTS_SOURCES
    tests/ut/estimators/estimator_test.cpp
    tests/ut/controllers/qos_controller_test.cpp
    tests/ut/filters/moving_average_test.cpp
    tests/main.cpp
    tests/integration/sources/json_source_test.cpp
)

set(SERENITY_INTEGRATION_TESTS_SOURCES
    tests/main.cpp
    tests/integration/sources/json_source_test.cpp
)

set(SERENITY_TESTS_HELPERS_SOURCES
    tests/helpers/sources/json_source.cpp
)

link_directories(${MESOS_INSTALLATION_DIR}/lib/)

#Serenity library
add_library(serenity ${SERENITY_SOURCES})
target_link_libraries(serenity messages mesos glog)

#Integration tests library
add_library(serenity-tests-helpers ${SERENITY_TESTS_HELPERS_SOURCES})
target_link_libraries(serenity-tests-helpers serenity messages-test pbjson)

#Unit tests
add_executable(serenity-tests ${SERENITY_UT_TESTS_SOURCES})
target_link_libraries(serenity-tests
    serenity
    serenity-tests-helpers
    gtest
    gtest_main
    gmock
    gmock_main
)
add_test(serenity-tests serenity-tests)


#Integration tests
add_executable(serenity-integration-tests ${SERENITY_INTEGRATION_TESTS_SOURCES})
target_link_libraries(serenity-integration-tests
    serenity
    serenity-tests-helpers
    gtest
    gtest_main
    gmock
    gmock_main
)
add_test(serenity-integration-tests serenity-integration-tests)


# Integration tests
add_executable(pipeline_test ${PIPELINE_TEST_SOURCES})
target_link_libraries(pipeline_test serenity-tests-helpers)