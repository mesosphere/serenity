#ifndef SERENITY_MOVINGAVERAGE_H
#define SERENITY_MOVINGAVERAGE_H

#include "serenity/serenity.hpp"

namespace mesos {
namespace serenity {

class MovingAverageFilter : public Consumer<int>, public Producer<int>
{
public:
  MovingAverageFilter() {}
  ~MovingAverageFilter() noexcept;

  Try<Nothing> consume(int& in);

protected:
  Try<int> handle(int in);

private:
  MovingAverageFilter(MovingAverageFilter& other) {};

};

} // namespace serenity
} // namespace mesos

#endif //SERENITY_MOVINGAVERAGE_H
