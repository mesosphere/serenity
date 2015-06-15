#include <gtest/gtest.h>

#include "messages/serenity.hpp"



int main(int argc, char** argv) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
