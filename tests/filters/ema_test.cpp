#include <gtest/gtest.h>

// TODO(bplotka): Not able to include hpp file here! ):
#include "filters/ema.cpp"

#include "tests/serenity.hpp"

namespace mesos {
namespace tests {

TEST(EMATest, Test) {
  serenity::EMAFilter<ResourceStatistics, double> filter;
  //TODO(bplotka) test EMAFilter

}

} // tests {
} // mesos {

