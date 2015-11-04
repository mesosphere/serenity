# P-Wave (pwave)

![P-Wave](https://github.com/Bplotka/pwave/blob/master/doc/pwave.png)

Simple & smart header-only library for sequential signal generation.

Useful for testing complicated algorithms like Moving Average calculations, Change Point Detections.

Used in https://github.com/mesosphere/serenity

## Features

* Generated samples based on custom math model (any f(x) like linear, sinus etc)
* Optional noise modifiers (deterministic or random)
* Custom modifiers (spikes, drops etc)
* Print as CSV


## Usage

1. Include `pwave` in your `c++11` code.
2. Use robust `SignalScenario` class for creating custom, complex scenarios:

```cpp
SignalScenario signalGen =
  SignalScenario(NUMBER_OF_ITERATIONS)
    .use(math::linearFunction)
    .after(12).add(-24.05)
    .after(2).use(new SymetricNoiseGenerator(MAX_NOISE))
    .after(23).use(math::sinFunction);
```

3. Iterate over generated values:

```cpp
ITERATE_SIGNAL(signalGen) {
  // Use generated result in your code.
  double_t result = (*signalGen)();
  // See result as CSV:
  (*signalGen).printCSVLine(result);
}
```
