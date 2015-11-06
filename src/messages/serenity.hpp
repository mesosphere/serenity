#ifndef SERENITY_MESSAGES_SERENITY_HPP
#define SERENITY_MESSAGES_SERENITY_HPP

#include <list>

#include "mesos/slave/oversubscription.hpp"
#include "mesos/mesos.hpp"

// ONLY USEFUL AFTER RUNNING PROTOC.
#include "serenity.pb.h"  // NOLINT(build/include)

#include "stout/option.hpp"
#include "stout/none.hpp"

namespace mesos {
namespace serenity {

using Contentions = std::list<mesos::Contention>;
using QoSCorrections = std::list<slave::QoSCorrection>;

inline Contention createCpuContention(
    Option<double_t> severity,
    WorkID victim,
    double_t timestamp,
    Option<WorkID> aggressor = None()) {
  Contention contention;
  contention.set_type(Contention_Type_CPU);
  if (severity.isSome()) {
    contention.set_severity(severity.get());
  }
  contention.set_timestamp(timestamp);
  contention.mutable_victim()->CopyFrom(victim);
  if (aggressor.isSome())
    contention.mutable_aggressor()->CopyFrom(aggressor.get());

  return contention;
}


inline slave::QoSCorrection createKillQoSCorrection(
    slave::QoSCorrection_Kill kill_msg,
    slave::QoSCorrection_Type actionType = slave::QoSCorrection_Type_KILL) {
  slave::QoSCorrection correction;
  correction.set_type(actionType);
  correction.mutable_kill()->CopyFrom(kill_msg);

  return correction;
}


inline slave::QoSCorrection_Kill createKill(const ExecutorInfo info) {
  slave::QoSCorrection_Kill kill;
  kill.mutable_framework_id()->CopyFrom(info.framework_id());
  kill.mutable_executor_id()->CopyFrom(info.executor_id());

  return kill;
}


inline WorkID createExecutorWorkID(const ExecutorInfo info) {
  WorkID workID;
  workID.mutable_framework_id()->CopyFrom(info.framework_id());
  workID.mutable_executor_id()->CopyFrom(info.executor_id());

  return workID;
}

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_MESSAGES_SERENITY_HPP
