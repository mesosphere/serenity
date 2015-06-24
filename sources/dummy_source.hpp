#ifndef SERENITY_DUMMY_SOURCE_H
#define SERENITY_DUMMY_SOURCE_H

#include <stout/none.hpp>
#include <stout/nothing.hpp>
#include <stout/try.hpp>

#include "serenity/serenity.hpp"

namespace mesos {
namespace serenity {

class DummySource : Producer<int> {
 public:
  template <typename ...Any>
  DummySource() {}
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_DUMMY_SOURCE_H
