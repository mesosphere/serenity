#include <stdlib.h>

#include <stout/try.hpp>
#include <filters/ema.hpp>

#include "filters/moving_average.cpp"
#include "filters/ema.cpp"
#include "qos_controller/serenity_controller.cpp"

#include "messages/serenity.hpp"


int main(int argc, char** argv)
{
  Try<int> foobar = 5;
  std::cout << "pipe test" << "\n";

  // OBSOLETE - TODO(bplotka)needs update!
  //  mesos::serenity::MovingAverageFilter defFilter;
  //  mesos::serenity::EMAFilter<ResourceStatistics> expFilter;
  //  expFilter.addConsumer(&defFilter);
  //  defFilter.consume(0);
  //
  //  mesos::serenity::SerenityController qos;
  //  defFilter.addConsumer(&qos);

  // Copy contructor?
  //mesos::serenity::MovingAverageFilter defFilter2(defFilter);

  return EXIT_SUCCESS;
}
