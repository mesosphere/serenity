#include <stdlib.h>

#include <stout/try.hpp>

#include "filters/moving_average.cpp"
#include "filters/exponential_moving_average.cpp"
#include "qos_controller/serenity_controller.cpp"

#include "tests/helpers/sources/json_source.hpp"

#include "messages/serenity.hpp"


using namespace mesos::serenity;

int main(int argc, char** argv)
{
  Try<int> foobar = 5;
  std::cout << "pipe test" << "\n";

  MovingAverageFilter defFilter;
  ExponentialMovingAverageFilter expFilter;
  expFilter.addConsumer(&defFilter);

  SerenityController qos;
  //defFilter.addConsumer(&qos);


  return EXIT_SUCCESS;
}
