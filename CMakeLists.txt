cmake_minimum_required(VERSION 3.0.0)
project(serenity)
enable_testing()

#Command line options.
SET(WITH_MESOS "/usr" CACHE STRING "Mesos installation directory")
SET(MESOS_INSTALLATION_DIR ${WITH_MESOS})
MESSAGE("Mesos installation directory set to: " ${MESOS_INSTALLATION_DIR})

# Add gtest.
ADD_SUBDIRECTORY (lib/gmock-1.7.0)
include_directories( ${gmock_SOURCE_DIR} include)
include_directories( ${gtest_SOURCE_DIR} include)

# Add pbjson - it doesn't have CMakeList.txt so include it manually.
set(pbjson_SOURCE_DIR lib/pbjson)

# CXX Flags.
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

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
    filters/ema.cpp
    observers/ipc_drop.cpp
    qos_controller/serenity_controller.cpp
    sources/dummy_source.cpp
)

set(SERENITY_TEST_SOURCES
    tests/main.cpp
    tests/common/sources/json_source.cpp
    tests/controllers/qos_controller_test.cpp
    tests/estimators/estimator_test.cpp
    tests/filters/ema_test.cpp
    tests/sources/json_source_test.cpp
)

# Copy fixtures.
file(GLOB TestFixtures "tests/fixtures/*.json")
file(COPY ${TestFixtures} DESTINATION tests/fixtures)

# Serenity library.
add_library(serenity ${SERENITY_SOURCES})
target_link_libraries(serenity messages mesos glog)

# Tests.
add_executable(serenity-tests ${SERENITY_TEST_SOURCES})
target_link_libraries(serenity-tests
    gmock
    gmock_main
    gtest
    gtest_main
    mesos
    messages-test
    pbjson
    serenity
)
add_test(serenity-tests serenity-tests)
