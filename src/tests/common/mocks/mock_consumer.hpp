#ifndef SERENITY_MOCK_CONSUMER_HPP
#define SERENITY_MOCK_CONSUMER_HPP

#include <vector>

#include "gmock/gmock.h"

#include "serenity/serenity.hpp"

namespace mesos {
namespace serenity {
namespace tests {

template <typename C1>
class MockConsumer : public Consumer<C1> {
 public:
  MOCK_METHOD0(allProductsReady, void());
  MOCK_METHOD1_T(consume, Try<Nothing>(const C1&));

  const std::vector<C1>& getBaseConsumables() const {
    return Consumer<C1>::getConsumables();
  }

  Option<C1> getBaseConsumable() const {
    return Consumer<C1>::getConsumable();
  }
};

}  // namespace tests
}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_MOCK_CONSUMER_HPP
