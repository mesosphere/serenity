#include <stdio.h>
#include <stdlib.h>

#include <stout/try.hpp>
//#include "serenity.hpp"

#include <boost/any.hpp>
#include <mesos/resources.hpp>

#include "filters/moving_average.hpp"
#include "qoscontroller/serenity_qos_controller.hpp"


int main(int argc, char** argv)
{
  Try<int> foobar = 5;
  std::cout << "pipe test" << "\n";


  mesos::serenity::SerenityQoSController qos;

  mesos::serenity::MovingAverageFilter mafilter3(&qos);

//  mesos::serenity::MovingAverageFilter mafilter2(&qos, &qos);

//  mesos::serenity::MovingAverageFilter mafilter4(&qos, &mafilter3);

  return EXIT_SUCCESS;
}
