#include "moving_average.hpp"

namespace mesos {
namespace serenity {

MovingAverageFilter::~MovingAverageFilter() {}

Try<Nothing> MovingAverageFilter::consume(int in)
{
  return Nothing();
}

Try<int> MovingAverageFilter::handle(int in)
{
  return in;
}

} // namespace serenity
} // namespace mesos
