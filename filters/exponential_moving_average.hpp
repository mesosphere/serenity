#ifndef SERENITY_EXPONENTIAL_MOVING_AVERAGE_H
#define SERENITY_EXPONENTIAL_MOVING_AVERAGE_H

#include "serenity/serenity.hpp"

namespace mesos {
namespace serenity {


//TODO: Template it <IN, OUT> and add a strategy to constructor?
class ExponentialMovingAverageFilter : public Consumer<int>, public Producer<int>
{
public:

  ExponentialMovingAverageFilter(){}; //test constructor
  ~ExponentialMovingAverageFilter() noexcept;

  Try<Nothing> consume(int in);

protected:
  Try<int> handle(int in);

private:
  ExponentialMovingAverageFilter(ExponentialMovingAverageFilter& other) {};

};

} // namespace serenity
} // namespace mesos

#endif //SERENITY_EXPONENTIAL_MOVING_AVERAGE_H
