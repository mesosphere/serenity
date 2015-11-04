#include <list>
#include <vector>
#include <utility>

#include "observers/qos_correction.hpp"

#include "mesos/slave/oversubscription.hpp"

#include "messages/serenity.hpp"

#include "serenity/wid.hpp"

namespace mesos {
namespace serenity {

QoSCorrectionObserver::~QoSCorrectionObserver() {}


Try<Nothing> QoSCorrectionObserver::_syncConsume(
  const std::vector<Contentions> products) {
  Contentions newContentions;

  for (Contentions contentions : products)
    for (Contention contention : contentions)
      newContentions.push_back(contention);

  this->currentContentions = newContentions;
  this->currentContentions.get().sort(compareSeverity);

  if (this->currentUsage.isSome()) {
    this->__correctSlave();
  }

  return Nothing();
}


Try<Nothing> QoSCorrectionObserver::consume(const ResourceUsage& usage) {
  this->currentUsage = usage;

  if (this->currentContentions.isSome()) {
    this->__correctSlave();
  }

  return Nothing();
}


Try<Nothing> QoSCorrectionObserver::__correctSlave() {
  // Consumer base code ensures we have needed information here.
  if (!this->currentContentions.isSome() || !this->currentUsage.isSome())
    return Nothing();

  if (this->currentContentions.get().empty()) {
    produce(QoSCorrections());
  } else {
    // Allowed to interpret contention using different algorithms.
    Try<QoSCorrections> corrections =
      this->revStrategy->decide(ageFilter,
                                      this->currentContentions.get(),
                                      this->currentUsage.get());
    if (corrections.isError()) {
      // In case of Error produce empty corrections.
      produce(QoSCorrections());
    } else {
      produce(corrections.get());
    }
  }

  this->currentContentions = None();
  this->currentUsage = None();

  return Nothing();
}

}  // namespace serenity
}  // namespace mesos
