#include <stdio.h>
#include <stdlib.h>

#include <stout/try.hpp>

int main(int argc, char** argv)
{
  Try<int> foobar = 5;

  return EXIT_SUCCESS;
}
