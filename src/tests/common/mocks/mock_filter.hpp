#ifndef SERENITY_MOCKFILTER_HPP
#define SERENITY_MOCKFILTER_HPP

#include <vector>

#include "gmock/gmock.h"

#include "serenity/serenity.hpp"

namespace mesos {
namespace serenity {
namespace tests {

template <typename P, typename C>
class MockFilter : public Producer<P>,
                   public Consumer<C> {
 public:
  MOCK_METHOD0(allProductsReady, void());
  MOCK_METHOD0(cleanup, void());
  MOCK_METHOD1_T(consume, Try<Nothing>(const C&));

  const std::vector<C>&getConsumables() const {
    return Consumer<C>::getConsumables();
  }

  const Option<C> getConsumable() const {
    return Consumer<C>::getConsumable();
  }

  void produce(P product) {
    Producer<P>::produce(product);
  }
};

}  // namespace tests
}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_MOCKFILTER_HPP
