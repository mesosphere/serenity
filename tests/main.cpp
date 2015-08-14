#include <glog/logging.h>
#include <gtest/gtest.h>

#include "messages/serenity.hpp"


int main(int argc, char** argv) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;
  FLAGS_logtostderr = true;
  FLAGS_minloglevel = google::WARNING;

  ::testing::InitGoogleTest(&argc, argv);
  // Initialize Google's logging library.
  // Comment that for debug log (INFO lvl)
  google::InitGoogleLogging(argv[0]);

  return RUN_ALL_TESTS();
}
