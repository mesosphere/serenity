#include <glog/logging.h>
#include <gtest/gtest.h>

#include "messages/serenity.hpp"


int main(int argc, char** argv) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;
  // Initialize Google's logging library.
  google::InitGoogleLogging(argv[0]);

  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
