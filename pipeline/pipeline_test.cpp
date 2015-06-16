#include <stdlib.h>

#include <stout/try.hpp>
#include <filters/ema.hpp>

#include "filters/ema.cpp"
#include "qos_controller/serenity_controller.cpp"

#include "messages/serenity.hpp"


int main(int argc, char** argv)
{
  std::cout << "pipe test" << "\n";

  // TODO(bplotka) do we need that?
  // Everything we can do in gtest...

  return EXIT_SUCCESS;
}
