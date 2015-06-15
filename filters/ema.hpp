#ifndef SERENITY_EXPONENTIAL_MOVING_AVERAGE_H
#define SERENITY_EXPONENTIAL_MOVING_AVERAGE_H

#include "serenity/serenity.hpp"

namespace mesos {
namespace serenity {


template <typename In, typename Out>
class EMAFilter : public Consumer<In>, public Producer<Out>
{
public:
  // TODO(bplotka) how to pass potential templates to other entities in
  // pipeline?
  EMAFilter() {}; //For testing purposes
  EMAFilter(Consumer<Out>& other) {};

  ~EMAFilter();

  Try<Nothing> consume(In in);

protected:
  Try<Out> handle(In in);

private:
  double prevSample;
  double prevSampleTimestamp; //Use for counting deltaTime
  double prevEma;
  /*
   * Inspired by: oroboro.com/irregular-ema/
   */
  double exponentialMovingAverage(
      double alpha,
      double sample,
      double deltaTime)
  {
    //TODO(bplotka): rename ambigous var names.
    double a = deltaTime / alpha;
    double u = exp(a * -1);
    double v = (1 - u) / a;

    return (u * prevEma) + (( v - u ) * prevSample) + ((1.0 - v ) * sample);
  }
};

} // namespace serenity
} // namespace mesos

#endif //SERENITY_EXPONENTIAL_MOVING_AVERAGE_H
