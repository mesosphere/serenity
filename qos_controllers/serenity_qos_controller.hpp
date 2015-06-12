#ifndef SERENITY_SERENITY_QOS_CONTROLLER_HPP
#define SERENITY_SERENITY_QOS_CONTROLLER_HPP

#include <stout/nothing.hpp>
#include <stout/none.hpp>
#include <stout/try.hpp>

#include "serenity/serenity.hpp"

namespace mesos {
namespace serenity {

class SerenityQoSController : public Consumer<int>
{
public:
  SerenityQoSController() {};
  ~SerenityQoSController() noexcept;

  Try<Nothing> consume(int in);

protected:
  Try<None> handle(int in);

};

} // namespace serenity
} // namespace mesos

#endif //SERENITY_SERENITY_QOS_CONTROLLER_HPP
