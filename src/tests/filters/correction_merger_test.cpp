#include "gmock/gmock.h"

#include "filters/correction_merger.hpp"

#include "messages/serenity.hpp"

#include "tests/common/sinks/mock_sink.hpp"

namespace mesos {
namespace serenity {
namespace tests {

/**
 * Expect CorrectionMerger to filter out duplicates and pass merged corrections.
 */
TEST(CorrectonMergerTest, MergingCorrections) {
  MockSink<QoSCorrections> mockSink;
  CorrectionMergerFilter correctionMerger(&mockSink, 1);

  QoSCorrections corrections;

  // First correction.
  ExecutorInfo executorInfo;
  executorInfo.mutable_framework_id()->set_value("Framework1");
  executorInfo.mutable_executor_id()->set_value("Executor1");
  corrections.push_back(createKillQoSCorrection(createKill(executorInfo)));

  // Duplicated first correction.
  executorInfo.mutable_framework_id()->set_value("Framework1");
  executorInfo.mutable_executor_id()->set_value("Executor1");
  corrections.push_back(createKillQoSCorrection(createKill(executorInfo)));

  // Second correction (from the same framework).
  executorInfo.mutable_framework_id()->set_value("Framework1");
  executorInfo.mutable_executor_id()->set_value("Executor2");
  corrections.push_back(createKillQoSCorrection(createKill(executorInfo)));

  // Third correction.
  executorInfo.mutable_framework_id()->set_value("Framework2");
  executorInfo.mutable_executor_id()->set_value("Executor1");
  corrections.push_back(createKillQoSCorrection(createKill(executorInfo)));

  EXPECT_FALSE(correctionMerger.consume(corrections).isError());

  EXPECT_EQ(1, mockSink.numberOfMessagesConsumed);

  EXPECT_EQ(3, mockSink.currentConsumedT.size());
}

}  // namespace tests
}  // namespace serenity
}  // namespace mesos
