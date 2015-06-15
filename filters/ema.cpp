#include "filters/ema.hpp"

namespace mesos {
namespace serenity {






template <typename In, typename Out>
EMAFilter<In,Out>::~EMAFilter() {}

template <typename In, typename Out>
Try<Out> EMAFilter<In,Out>::handle(In in)
{
  Out result;
  return result;
}

template <typename In, typename Out>
Try<Nothing> EMAFilter<In,Out>::consume(In in)
{
  return Nothing();
}

} // namespace serenity
} // namespace mesos
