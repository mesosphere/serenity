#ifndef SERENITY_MOCK_MULTIPLE_CONSUMER_HPP
#define SERENITY_MOCK_MULTIPLE_CONSUMER_HPP

#include <vector>

#include "gmock/gmock.h"

#include "serenity/serenity.hpp"

namespace mesos {
namespace serenity {
namespace tests {

template <typename C1, typename C2>
class MockMulitpleConsumer : public Consumer<C1>,
                             public Consumer<C2> {
 public:
  MOCK_METHOD0(allProductsReady, void());
  MOCK_METHOD0(cleanup, void());
  MOCK_METHOD1_T(consume, Try<Nothing>(const C1&));
  MOCK_METHOD1_T(consume, Try<Nothing>(const C2&));

  template <typename T>
  const std::vector<T>&getConsumables() const {
    return Consumer<T>::getConsumables();
  }

  template <typename T>
  const Option<T> getConsumable() const {
    return Consumer<T>::getConsumable();
  }
};

}  // namespace tests
}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_MOCK_MULTIPLE_CONSUMER_HPP
