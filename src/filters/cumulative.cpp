#include <atomic>
#include <string>
#include <utility>

#include "glog/logging.h"

#include "mesos/mesos.hpp"

#include "cumulative.hpp"

namespace mesos {
namespace serenity {

using std::map;
using std::pair;
using std::string;

CumulativeFilter::CumulativeFilter() {}


CumulativeFilter::CumulativeFilter(Consumer<ResourceUsage>* _consumer)
  : Producer<ResourceUsage>(_consumer) {}


CumulativeFilter::~CumulativeFilter() {}


Try<Nothing> CumulativeFilter::consume(const ResourceUsage& in) {
  // TODO(bplotka)
}

}  // namespace serenity
}  // namespace mesos
