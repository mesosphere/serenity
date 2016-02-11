#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "stout/gtest.hpp"
#include "stout/nothing.hpp"
#include "stout/option.hpp"
#include "stout/try.hpp"

#include "serenity/serenity.hpp"

#include "tests/common/mocks/mock_consumer.hpp"
#include "tests/common/mocks/mock_filter.hpp"
#include "tests/common/mocks/mock_multiple_consumer.hpp"

namespace mesos {
namespace serenity {
namespace tests {

using ::testing::Exactly;
using ::testing::AtLeast;
using ::testing::Return;
using ::testing::InSequence;


TEST(SerenityFrameworkTests, SingleConsumerAllMethodsRun) {
  MockFilter<int, int> producer;
  MockConsumer<int> consumer;

  producer.addConsumer(&consumer);

  // Consumer is empty in the begining.
  ASSERT_TRUE(consumer.getBaseConsumable().isNone());

  int product_42 = 42;
  int product_84 = 84;

  // Consume and allProductsReady will be invoked.
  {
    InSequence seq;
    EXPECT_CALL(consumer, consume(42)).Times(Exactly(1)).
      WillRepeatedly(Return(Nothing()));
    EXPECT_CALL(consumer, allProductsReady()).Times(Exactly(1));
  }
  producer.produce(product_42);

  // Consumer is has the product in memory
  ASSERT_TRUE(consumer.getBaseConsumable().isSome());

  ASSERT_EQ(product_42, consumer.getBaseConsumable().get());

  // After second consumption, client will have new value in memory
  {
    InSequence seq;
    EXPECT_CALL(consumer, consume(84)).Times(Exactly(1)).
      WillRepeatedly(Return(Nothing()));
    EXPECT_CALL(consumer, allProductsReady()).Times(Exactly(1));
  }
  producer.produce(product_84);

  ASSERT_TRUE(consumer.getBaseConsumable().isSome());
  ASSERT_EQ(product_84, consumer.getBaseConsumable().get());
}


TEST(SerenityFrameworkTests, MulitpleProducersSingleConsumer) {
  MockFilter<int, int> firstProducer;
  MockFilter<int, int> secondProducer;
  MockConsumer<int> consumer;

  firstProducer.addConsumer(&consumer);
  secondProducer.addConsumer(&consumer);

  // Consumer is empty in the begining.
  ASSERT_TRUE(consumer.getBaseConsumable().isNone());

  int firstProduct = 42;
  int secondProduct = 84;
  constexpr size_t PRODUCERS_COUNT = 2;
  {
    InSequence seq;

    // Consume and allProductsReady will be invoked.
    EXPECT_CALL(consumer, consume(firstProduct)).Times(Exactly(1))
      .WillOnce(Return(Nothing()));
    EXPECT_CALL(consumer, consume(secondProduct)).Times(Exactly(1))
      .WillOnce(Return(Nothing()));
    EXPECT_CALL(consumer, allProductsReady()).Times(Exactly(1));
  }
  firstProducer.produce(firstProduct);
  ASSERT_TRUE(consumer.getBaseConsumable().isSome());
  ASSERT_EQ(firstProduct, consumer.getBaseConsumable().get());
  ASSERT_EQ(1, consumer.getBaseConsumables().size());
  ASSERT_EQ(firstProduct, consumer.getBaseConsumables()[0]);

  // After second produce, Consumer is has two values in memory.
  secondProducer.produce(secondProduct);

  ASSERT_TRUE(consumer.getBaseConsumable().isSome());
  ASSERT_EQ(firstProduct, consumer.getBaseConsumable().get());

  ASSERT_EQ(PRODUCERS_COUNT, consumer.getBaseConsumables().size());
  ASSERT_EQ(firstProduct, consumer.getBaseConsumables()[0]);
  ASSERT_EQ(secondProduct, consumer.getBaseConsumables()[1]);

  // New iteration - consumer should have new value in memory.
  int thirdProduct = 144;
  {
    InSequence seq;

    // Consume and allProductsReady will be invoked.
    EXPECT_CALL(consumer, consume(thirdProduct)).Times(Exactly(1))
      .WillOnce(Return(Nothing()));
  }
  firstProducer.produce(thirdProduct);
  ASSERT_TRUE(consumer.getBaseConsumable().isSome());
  ASSERT_EQ(thirdProduct, consumer.getBaseConsumable().get());
  ASSERT_EQ(1, consumer.getBaseConsumables().size());
  ASSERT_EQ(thirdProduct, consumer.getBaseConsumables()[0]);
}


TEST(SerenityFrameworkTests, MultiProductTypesConsumer) {
  MockFilter<std::string, std::string> stringProducer;
  MockFilter<int, int> intProducer;
  const size_t PRODUCT_PER_TYPE = 1;
  const size_t PRODUCERS_COUNT = 2;

  MockMulitpleConsumer<int, std::string> consumer;

  stringProducer.addConsumer(&consumer);
  intProducer.addConsumer(&consumer);

  // Consumer is empty in the beginning.
  ASSERT_TRUE(consumer.getBaseConsumable<int>().isNone());
  ASSERT_TRUE(consumer.getBaseConsumable<std::string>().isNone());

  const int FIRST_INT_PRODUCT = 42;
  const std::string FIRST_STRING_PRODUCT = "first";
  const int SECOND_INT_PRODUCT = 88;
  const std::string SECOND_STRING_PRODUCT = "second";

  {
    InSequence seq;

    // Consume and allProductsReady will be invoked.
    EXPECT_CALL(consumer, consume(FIRST_INT_PRODUCT)).Times(Exactly(1))
      .WillOnce(Return(Nothing()));
    EXPECT_CALL(consumer, consume(FIRST_STRING_PRODUCT)).Times(Exactly(1))
      .WillOnce(Return(Nothing()));
    EXPECT_CALL(consumer, allProductsReady()).Times(Exactly(1));
  }

  // First product is in memory.
  intProducer.produce(FIRST_INT_PRODUCT);
  ASSERT_TRUE(consumer.getBaseConsumable<int>().isSome());
  ASSERT_TRUE(consumer.getBaseConsumable<std::string>().isNone());
  ASSERT_EQ(PRODUCT_PER_TYPE, consumer.getBaseConsumables<int>().size());
  ASSERT_EQ(FIRST_INT_PRODUCT, consumer.getBaseConsumables<int>()[0]);
  ASSERT_EQ(FIRST_INT_PRODUCT, consumer.getBaseConsumable<int>().get());

  stringProducer.produce(FIRST_STRING_PRODUCT);
  ASSERT_TRUE(consumer.getBaseConsumable<int>().isSome());
  ASSERT_EQ(PRODUCT_PER_TYPE, consumer.getBaseConsumables<int>().size());
  ASSERT_EQ(FIRST_INT_PRODUCT, consumer.getBaseConsumables<int>()[0]);
  ASSERT_EQ(FIRST_INT_PRODUCT, consumer.getBaseConsumable<int>().get());

  ASSERT_TRUE(consumer.getBaseConsumable<std::string>().isSome());
  ASSERT_EQ(1, consumer.getBaseConsumables<std::string>().size());
  ASSERT_EQ(FIRST_STRING_PRODUCT, consumer.getBaseConsumable<std::string>().get());
  ASSERT_EQ(FIRST_STRING_PRODUCT, consumer.getBaseConsumables<std::string>()[0]);
  ASSERT_EQ(FIRST_STRING_PRODUCT, consumer.getBaseConsumable<std::string>().get());

  // Second interation
  {
    InSequence seq;

    // Consume and allProductsReady will be invoked.
    EXPECT_CALL(consumer, consume(SECOND_INT_PRODUCT)).Times(Exactly(1))
    .WillOnce(Return(Nothing()));
    EXPECT_CALL(consumer, consume(SECOND_STRING_PRODUCT)).Times(Exactly(1))
    .WillOnce(Return(Nothing()));
    EXPECT_CALL(consumer, allProductsReady()).Times(Exactly(1));
  }

  intProducer.produce(SECOND_INT_PRODUCT);
  ASSERT_TRUE(consumer.getBaseConsumable<int>().isSome());
  ASSERT_EQ(PRODUCT_PER_TYPE, consumer.getBaseConsumables<int>().size());
  ASSERT_EQ(SECOND_INT_PRODUCT, consumer.getBaseConsumables<int>()[0]);
  ASSERT_EQ(SECOND_INT_PRODUCT, consumer.getBaseConsumable<int>().get());

  // String consumer should still have previous value in memory
  ASSERT_TRUE(consumer.getBaseConsumable<std::string>().isSome());
  ASSERT_EQ(PRODUCT_PER_TYPE, consumer.getBaseConsumables<std::string>().size());
  ASSERT_EQ(FIRST_STRING_PRODUCT, consumer.getBaseConsumable<std::string>().get());
  ASSERT_EQ(FIRST_STRING_PRODUCT, consumer.getBaseConsumables<std::string>()[0]);
  ASSERT_EQ(FIRST_STRING_PRODUCT, consumer.getBaseConsumable<std::string>().get());

  // Production of second product
  stringProducer.produce(SECOND_STRING_PRODUCT);
  ASSERT_TRUE(consumer.getBaseConsumable<int>().isSome());
  ASSERT_TRUE(consumer.getBaseConsumable<std::string>().isSome());
  ASSERT_EQ(1, consumer.getBaseConsumables<int>().size());
  ASSERT_EQ(SECOND_INT_PRODUCT, consumer.getBaseConsumables<int>()[0]);
  ASSERT_EQ(SECOND_INT_PRODUCT, consumer.getBaseConsumable<int>().get());

  ASSERT_EQ(1, consumer.getBaseConsumables<std::string>().size());
  ASSERT_EQ(SECOND_STRING_PRODUCT, consumer.getBaseConsumable<std::string>().get());
  ASSERT_EQ(SECOND_STRING_PRODUCT, consumer.getBaseConsumables<std::string>()[0]);
  ASSERT_EQ(SECOND_STRING_PRODUCT, consumer.getBaseConsumable<std::string>().get());
}

}  // namespace tests
}  // namespace serenity
}  // namespace mesos
