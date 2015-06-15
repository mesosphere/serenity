#include "exponential_moving_average.hpp"


namespace mesos {
namespace serenity {

ExponentialMovingAverageFilter::~ExponentialMovingAverageFilter() {}

Try<int> ExponentialMovingAverageFilter::handle(int in)
{
  return in;
}

Try<Nothing> ExponentialMovingAverageFilter::consume(int& in)
{
  return Nothing();
}

} // namespace serenity
} // namespace mesos
