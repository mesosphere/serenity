#ifndef SERENITY_WID_HPP
#define SERENITY_WID_HPP

#include <list>
#include <string>
#include <iostream>

#include "mesos/mesos.hpp"

#include "mesos/slave/oversubscription.hpp"

#include "messages/serenity.hpp"

namespace mesos {
namespace serenity {

struct WID;

bool operator == (const WID& lhs, const WID& rhs);


bool operator != (const WID& lhs, const WID& rhs);

/**
 * Serenity Work ID wrapper.
 * It defined universal Executor Id.
 */
struct WID {
  //! ExecutorInfo
  explicit WID(const ExecutorInfo& that) {
    this->executor_id.CopyFrom(that.executor_id());
    this->framework_id.CopyFrom(that.framework_id());
  }

  //! WorkID
  explicit WID(const WorkID& that) {
    this->executor_id.CopyFrom(that.executor_id());
    this->framework_id.CopyFrom(that.framework_id());
  }

  //! slave::QoSCorrection_Kill
  explicit WID(const slave::QoSCorrection_Kill& that) {
    this->executor_id.CopyFrom(that.executor_id());
    this->framework_id.CopyFrom(that.framework_id());
  }

  //! Hasher for ExecutorInfo.
  size_t operator()(const ExecutorInfo& that) const {
    std::string hashKey = that.executor_id().value() +
                          that.framework_id().value();
    std::hash<std::string> hashFunc;
    return hashFunc(hashKey);
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
