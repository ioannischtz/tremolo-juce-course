#pragma once

#include "CrossfadeSwitcher.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <utility>
#include "juce_dsp/juce_dsp.h"

namespace tremolo {

inline constexpr auto twoPi = juce::MathConstants<float>::twoPi;

enum class LfoWaveformShape : std::size_t {
    sine,
    triangle,
    softSquare,
    count
};

using WaveformShapeFunction = float (*)(float phaseTurns) noexcept;

inline float sineShape(float p) noexcept {
    return std::sin(twoPi * p);
}

// https://en.wikipedia.org/wiki/Triangle_wave general definition:
// x(t) = 4 |t - floor(t + 3/4) + 1/4| - 1
inline float triangleShape(float p) noexcept {
    return 4.0f * std::abs(p - std::floor(p + 0.75f) + 0.25f) - 1.0f;
}

inline float softSquareShape(float p) noexcept {
    return std::tanh(3.0f * std::sin(twoPi * p));
}

inline constexpr auto waveformTable = std::to_array<WaveformShapeFunction>({
    &sineShape,
    &triangleShape,
    &softSquareShape,
});

static_assert(waveformTable.size() == std::to_underlying(LfoWaveformShape::count));

inline float evaluateWaveformShape(LfoWaveformShape shape, float p) noexcept {
    const auto index = std::to_underlying(shape);
    jassert(index < waveformTable.size());
    return waveformTable[index](p);
}

class Lfo {
  public:
    Lfo(float waveformFadeSeconds, LfoWaveformShape initialWaveformShape, float initialRateHz)
        : rateHz {initialRateHz},
          waveformSwitcher {waveformFadeSeconds, initialWaveformShape} {}

    // ========
    // Public Methods
    // ========

    void prepare(double sampleRateHzArg) noexcept {
        jassert(sampleRateHzArg > 0.0);
        sampleRateHz = static_cast<float>(sampleRateHzArg);
        phase.reset();
        rateHz.reset(sampleRateHz, rateSmoothingSeconds);
        waveformSwitcher.prepare(sampleRateHz);
    }

    void setModulationRate(const float newRateHz) noexcept {
        jassert(newRateHz > 0.0f);
        rateHz.setTargetValue(newRateHz);
    }

    void setModulationRateImmediately(float newRateHz) noexcept {
        jassert(newRateHz > 0.0f);
        rateHz.setCurrentAndTargetValue(newRateHz);
    }

    void switchWaveformShape(const LfoWaveformShape shape) noexcept {
        waveformSwitcher.switchTo(shape);
    }

    void switchWaveformShapeImmediately(LfoWaveformShape shape) noexcept {
        waveformSwitcher.switchImmediatelyTo(shape);
    }

    void reset() noexcept {
        phase.reset();
        rateHz.setCurrentAndTargetValue(rateHz.getTargetValue());
    }

    [[nodiscard]] float getNextValue() noexcept {
        jassert(sampleRateHz > 0.0f);
        const auto increment  = twoPi * rateHz.getNextValue() / sampleRateHz;
        const auto phaseTurns = phase.advance(increment) / twoPi;

        float value = 0.0f;
        for (const auto& [shape, weight] : waveformSwitcher.nextActive()) {
            if (weight > 0.0f) {
                value += weight * evaluateWaveformShape(shape, phaseTurns);
            }
        }
        return value;
    }

  private:
    static constexpr double rateSmoothingSeconds = 0.05;

    // ========
    // State
    // ========
    juce::dsp::Phase<float> phase;
    juce::SmoothedValue<float> rateHz;
    float sampleRateHz = 0.0f;
    CrossfadeSwitcher<LfoWaveformShape> waveformSwitcher;
};

}  // namespace tremolo