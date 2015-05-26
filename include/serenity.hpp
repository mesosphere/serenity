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


#ifndef SERENITY_SERENITY_HPP
#define SERENITY_SERENITY_HPP

#include <string>
#include <tuple>
#include <functional>

#include <mesos/resources.hpp>

#include <process/future.hpp>

#include <stout/lambda.hpp>
#include <stout/nothing.hpp>
#include <stout/option.hpp>
#include <stout/try.hpp>

//#include <boost/any.hpp>

namespace mesos {
namespace serenity {

// The bus socket allows peers to communicate (subscribe and publish)
// asynchronously.
class BusSocket {
public:
  // Filters need to claim a topic before being able to
  // publish to it.
  Try<Nothing> registration(std::string topic);

  // NOTE: Subscribe can return a future instead of relying
  // on a provided callback.
  Try<Nothing> subscribe(
      std::string topic,
      std::function<void(mesos::scheduler::Event)> callback); //skonefal TODO: Waiting for serenity.proto generic serenity event

  Try<Nothing> publish(std::string topic, mesos::scheduler::Event event); //skonefal TODO: Waiting for serenity.proto generic serenity event
};


template<typename T, typename S>
class Filter : public BusSocket
{
public:

  // Variadic template to allow fanout.
  template<typename ...Any>
  Filter(Filter<S, Any> *... out) {
    recursiveUnboxing(out...);
  };

  virtual Try<Nothing> input(T in) = 0;


//  typedef Try<Nothing> (*InputMPtr)(T in);
  std::vector<std::function<Try<Nothing>(T)>> outputVector;

private:
  template<typename Some, typename ...Any>
  void recursiveUnboxing(Filter<S, Some>* head, Filter<S, Any> *...  tail) {

    //typedef Try<Nothing> (mesos::serenity::Filter<S, Some>::*Input)(T in);
    //Input lol = &head->input<S>;
    // InputMPtr imptr = &(head->input);

    //std::function<Try<Nothing>(S)> pFunc = &head->input<S>;
    //outputVector.push_back(&head->input(T));


    std::function<Try<Nothing>(S)> funt2 = std::bind(&Filter<S, Some>::input, head);
//    auto fff = std::bind(&Filter::input, head);

    recursiveUnboxing(tail...);
  }

  // end condition
  void recursiveUnboxing() {}


};


template<typename T>
class Source : public Filter<None, T>
{
protected:
  Source(Filter<None, T>* out...) {}
};


template<typename T>
class Sink : public Filter<T, None>
{
protected:
  virtual Try<Nothing> input(T in) = 0;
};

} // namespace serenity
} // namespace mesos


#endif //SERENITY_SERENITY_HPP
