#include <stdlib.h>

#include <stout/try.hpp>

#include "filters/moving_average.hpp"
#include "filters/exponential_moving_average.hpp"
#include "qos_controllers/serenity_qos_controller.hpp"

#include "messages/serenity.hpp"


int main(int argc, char** argv)
{
  Try<int> foobar = 5;
  std::cout << "pipe test" << "\n";


  mesos::serenity::SerenityQoSController qos;

//  mesos::serenity::MovingAverageFilter defFilter;
//  mesos::serenity::ExponentialMovingAverageFilter expFilter(&defFilter);

//  mesos::serenity::MovingAverageFilter defFilter2(defFilter);
//  mesos::serenity::MovingAverageFilter mafilter

  mesos::serenity::MovingAverageFilter mafilter3(&qos);

  mesos::serenity::MovingAverageFilter mafilter2(&qos, &qos);

  mesos::serenity::MovingAverageFilter mafilter4(&qos, &mafilter3);

  return EXIT_SUCCESS;
}
