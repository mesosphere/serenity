#ifndef PWAVE_NOISE_HPP
#define PWAVE_NOISE_HPP

namespace pwave {

constexpr double_t DEFAULT_MAX_NOISE = 50;

/**
 * Base class for all Noise Generators.
 * It is needed for Load Generator to introduce noise in samples.
 */
class NoiseGenerator {
 public:
  virtual double_t generate(size_t iteration) = 0;
};


/**
 * Primary Noise Generator - generates no noise.
 */
class ZeroNoise : public NoiseGenerator {
 public:
  double_t generate(size_t iteration) {
    return 0;
  }
};


/**
 * Symetric Noise Generator produces deterministic noise.
 * It is similar to sine wave. It alternates: raises to maxNoise
 * and -maxNoise, then stays low.
 * Average of generated values equals 0.
 */
class SymetricNoiseGenerator : public NoiseGenerator {
 public:
  explicit SymetricNoiseGenerator(double_t _maxNoise) : maxNoise(_maxNoise) {}

  double_t generate(size_t iteration) {
    sign *= -1;
    if (iteration % 2 == 0) {
      noise += noiseModifier;
      if (std::abs(noise) >= maxNoise) noiseModifier *= -1;
    }
    return (noise * sign);
  }

  double_t noiseModifier = 2;
  double_t maxNoise = DEFAULT_MAX_NOISE;

 private:
  int16_t sign = -1;
  double_t noise = 0;
};

}  // namespace pwave

#endif  // PWAVE_NOISE_HPP
