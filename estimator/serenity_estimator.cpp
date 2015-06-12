/**
 * This file is © 2015 Mesosphere, Inc. ("Mesosphere"). Mesosphere
 * licenses this file to you solely pursuant to the agreement between
 * Mesosphere and you (if any).  If there is no such agreement between
 * Mesosphere, the following terms apply (and you may not use this
 * file except in compliance with such terms):
 *
 * 1) Subject to your compliance with the following terms, Mesosphere
 * hereby grants you a nonexclusive, limited, personal,
 * non-sublicensable, non-transferable, royalty-free license to use
 * this file solely for your internal business purposes.
 *
 * 2) You may not (and agree not to, and not to authorize or enable
 * others to), directly or indirectly:
 *   (a) copy, distribute, rent, lease, timeshare, operate a service
 *   bureau, or otherwise use for the benefit of a third party, this
 *   file; or
 *
 *   (b) remove any proprietary notices from this file.  Except as
 *   expressly set forth herein, as between you and Mesosphere,
 *   Mesosphere retains all right, title and interest in and to this
 *   file.
 *
 * 3) Unless required by applicable law or otherwise agreed to in
 * writing, Mesosphere provides this file on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied,
 * including, without limitation, any warranties or conditions of
 * TITLE, NON-INFRINGEMENT, MERCHANTABILITY, or FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * 4) In no event and under no legal theory, whether in tort
 * (including negligence), contract, or otherwise, unless required by
 * applicable law (such as deliberate and grossly negligent acts) or
 * agreed to in writing, shall Mesosphere be liable to you for
 * damages, including any direct, indirect, special, incidental, or
 * consequential damages of any character arising as a result of these
 * terms or out of the use or inability to use this file (including
 * but not limited to damages for loss of goodwill, work stoppage,
 * computer failure or malfunction, or any and all other commercial
 * damages or losses), even if Mesosphere has been advised of the
 * possibility of such damages.
 */

#include <list>
#include <stdlib.h>

#include <process/dispatch.hpp>
#include <process/process.hpp>

#include <stout/error.hpp>

#include "estimator/serenity_estimator.hpp"

using namespace process;

namespace mesos {
namespace serenity {

class SerenityEstimatorProcess :
    public Process<SerenityEstimatorProcess>
{
public:
  SerenityEstimatorProcess(
      const lambda::function<Future<ResourceUsage>()>& usages_)
  : usages(usages_) {}

  Future<Resources> oversubscribable()
  {
    // TODO(bplotka) Set up the main estimation pipeline here.
    std::cout << "pipe test" << "\n";

    // For now return empty resources.
    return Resources();
  }

private:
  const lambda::function<Future<ResourceUsage>()>& usages;
};


SerenityEstimator::~SerenityEstimator()
{
  if (process.get() != NULL) {
    terminate(process.get());
    wait(process.get());
  }
}


Try<Nothing> SerenityEstimator::initialize(
    const lambda::function<Future<ResourceUsage>()>& usages)
{
  if (process.get() != NULL) {
    return Error("Serenity estimator has already been initialized");
  }

  process.reset(new SerenityEstimatorProcess(usages));
  spawn(process.get());

  return Nothing();
}


Future<Resources> SerenityEstimator::oversubscribable()
{
  if (process.get() == NULL) {
    return Failure("Serenity estimator is not initialized");
  }

  return dispatch(
      process.get(),
      &SerenityEstimatorProcess::oversubscribable);
}

} // namespace serenity {
} // namespace mesos {
