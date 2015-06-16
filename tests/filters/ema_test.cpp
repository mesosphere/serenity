#include <gtest/gtest.h>

#include "filters/ema.hpp"

#include "tests/serenity.hpp"

namespace mesos {
namespace tests {

TEST(EMATest, Test) {
  serenity::IpcEMAFilter filter(nullptr, 0.3);
  //TODO(bplotka) test EMAFilter

}

} // tests {
} // mesos {

