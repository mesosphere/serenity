#include <stout/gtest.hpp>

#include <gtest/gtest.h>

#include "pwave/scenario.hpp"

namespace pwave {

TEST(PwaveTest, ScenarioExample) {
  const int32_t ITERATIONS = 100;

  SignalScenario signalGen =
    SignalScenario(ITERATIONS)
      .use(math::linearFunction)
      .after(12).add(-24.05)
      .after(2).use(new SymetricNoiseGenerator(3))
      .after(23).use(math::const10Function)
      .after(4).constantAdd(-3.2, 10);

  ITERATE_SIGNAL(signalGen) {
    // Use generated result in your code.
    double_t result = (*signalGen)();
    // See result as CSV:
    (*signalGen).printCSVLine(signalGen->cumulative());
  }
}

}  // namespace pwave
