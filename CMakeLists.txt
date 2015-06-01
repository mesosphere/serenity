cmake_minimum_required(VERSION 2.6)
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



include_directories(
    ./
    ../
    serenity/
    include/
    ${MESOS_INSTALLATION_DIR}/include
    ${gmock_SOURCE_DIR}/include/
    ${gmock_SOURCE_DIR}/gtest/include/
)


set(SERENITY_SOURCES
    bus/bus_socket.cpp
    filters/moving_average.cpp
    filters/moving_average.hpp
    observers/ipc_drop.cpp
    qos_controllers/serenity_qos_controller.cpp
    sources/dummy_source.cpp
)

set(PIPELINE_TEST_SOURCES
    pipeline/pipeline_test.cpp
)

set(SERENITY_TEST_SOURCES
    tests/main.cpp
    tests/filters/moving_average_test.cpp
)

add_library(serenity ${SERENITY_SOURCES})

add_executable(pipeline_test ${PIPELINE_TEST_SOURCES})
target_link_libraries(pipeline_test serenity)

add_executable(serenity-tests ${SERENITY_TEST_SOURCES})
target_link_libraries(serenity-tests serenity gtest gtest_main gmock gmock_main)
add_test(serenity-tests serenity-tests)
