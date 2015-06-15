#include <stdlib.h>

#include <stout/try.hpp>

#include "filters/moving_average.cpp"
#include "filters/exponential_moving_average.cpp"
#include "qos_controller/serenity_controller.cpp"

#include "tests/sources/json_source.hpp"

#include "messages/serenity.hpp"


using namespace mesos::serenity;

int main(int argc, char** argv)
{
  Try<int> foobar = 5;
  std::cout << "pipe test" << "\n";

  MovingAverageFilter defFilter;
  ExponentialMovingAverageFilter expFilter;
  expFilter.addConsumer(&defFilter);
  defFilter.consume(0);

  SerenityController qos;
  //defFilter.addConsumer(&qos);

  //TODO: add target in cmake to copy fixtures to build folder
  JsonSource jsonSource;
  jsonSource.RunTests("../tests/sources/fixtures/ut_fixture.json");


  return EXIT_SUCCESS;
}
