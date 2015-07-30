#include "serenity/wid.hpp"

namespace mesos {
namespace serenity {

bool operator == (const WID& lhs, const WID& rhs) {
  if (lhs.framework_id.value() != rhs.framework_id.value())
    return false;
  if (lhs.executor_id.value() != rhs.executor_id.value())
    return false;

  return true;
}

bool operator != (const WID& lhs, const WID& rhs) {
  return !(lhs == rhs);
}

}  // namespace serenity
}  // namespace mesos
