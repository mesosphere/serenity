#include <gtest/gtest.h>

#include "filters/ema.hpp"

namespace mesos {
namespace serenity {
namespace tests {

TEST(EMATest, Test) {
  serenity::IpcEMAFilter filter(nullptr, 0.3);
  //TODO(bplotka) test EMAFilter

}

} // namespace tests {
} // namespace serenity {
} // namespace mesos {

