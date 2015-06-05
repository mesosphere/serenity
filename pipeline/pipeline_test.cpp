#include <stdlib.h>

#include <stout/try.hpp>

#include "filters/moving_average.cpp"
#include "filters/exponential_moving_average.cpp"
#include "qos_controllers/serenity_qos_controller.cpp"

#include "messages/serenity.hpp"


int main(int argc, char** argv)
{
  Try<int> foobar = 5;
  std::cout << "pipe test" << "\n";

  mesos::serenity::MovingAverageFilter defFilter;
  mesos::serenity::ExponentialMovingAverageFilter expFilter;
  expFilter.addConsumer(&defFilter);
  defFilter.consume(0);

  mesos::serenity::SerenityQoSController qos;
  defFilter.addConsumer(&qos);

  // Copy contructor?
  //mesos::serenity::MovingAverageFilter defFilter2(defFilter);

  return EXIT_SUCCESS;
}
