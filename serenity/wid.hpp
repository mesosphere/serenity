#ifndef SERENITY_WID_HPP
#define SERENITY_WID_HPP

#include <iostream>
#include <list>
#include <string>

#include "mesos/slave/oversubscription.hpp"
#include "mesos/mesos.hpp"

#include "messages/serenity.hpp"

namespace mesos {
namespace serenity {

class WID;

bool operator == (const WID& lhs, const WID& rhs);


bool operator != (const WID& lhs, const WID& rhs);

/**
 * Serenity Work ID wrapper.
 * It defined universal Executor Id.
 */
class WID {
 public:
  //! ExecutorInfo.
  explicit WID(const ExecutorInfo& that) {
    this->executor_id.CopyFrom(that.executor_id());
    this->framework_id.CopyFrom(that.framework_id());
  }

  //! WorkID.
  explicit WID(const WorkID& that) {
    this->executor_id.CopyFrom(that.executor_id());
    this->framework_id.CopyFrom(that.framework_id());
  }

  //! slave::QoSCorrection_Kill.
  explicit WID(const slave::QoSCorrection_Kill& that) {
    this->executor_id.CopyFrom(that.executor_id());
    this->framework_id.CopyFrom(that.framework_id());
  }

  //! WorkID getter.
  inline mesos::WorkID getWorkID() const {
    WorkID exported;
    exported.mutable_executor_id()->CopyFrom(this->executor_id);
    exported.mutable_framework_id()->CopyFrom(this->framework_id);
    return exported;
  }

  //! slave::QoSCorrection_Kill getter.
  inline slave::QoSCorrection_Kill getKill() {
    slave::QoSCorrection_Kill exported;
    exported.mutable_executor_id()->CopyFrom(this->executor_id);
    exported.mutable_framework_id()->CopyFrom(this->framework_id);
    return exported;
  }

  //! ExecutorInfo getter.
  inline ExecutorInfo getExecutorInfo() {
    ExecutorInfo exported;
    exported.mutable_executor_id()->CopyFrom(this->executor_id);
    exported.mutable_framework_id()->CopyFrom(this->framework_id);
    return exported;
  }

  inline std::string toString() {
    return executor_id.value() + "FrameworkID(" + framework_id.value() + ")";
  }

  ExecutorID executor_id;
  FrameworkID framework_id;
};


struct WIDHasher {
  //! Hasher for WorkID.
  size_t operator()(const WorkID& that) const {
    std::string hashKey = that.executor_id().value() +
                          that.framework_id().value();
    std::hash<std::string> hashFunc;
    return hashFunc(hashKey);
  }
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_WID_HPP
