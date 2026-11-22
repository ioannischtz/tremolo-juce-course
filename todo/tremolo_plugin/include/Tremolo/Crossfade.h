#pragma once

#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>

namespace tremolo {

class Crossfade {
  public:
    // ========
    // Nested Types
    // ========
    enum class Side {
        A,
        B
    };

    class Mix {
      public:
        constexpr Mix(float weightA, float weightB) noexcept
            : weights {weightA, weightB} {}

        [[nodiscard]] float get(const Side side) const noexcept {
            return side == Side::A ? weights[0] : weights[1];
        }

      private:
        std::array<float, 2> weights;
    };

    // ========
    // Constructor
    // ========
    explicit Crossfade(const double fadeSecondsArg, const Side initialSide = Side::A) noexcept
        : fadeSeconds {fadeSecondsArg},
          position {positionFor(initialSide)} {
        jassert(fadeSecondsArg > 0.0f);
    }

    // ========
    // Public Methods
    // ========
    void prepare(double sampleRateHzArg) noexcept {
        jassert(sampleRateHzArg > 0.0);
        if (juce::approximatelyEqual(sampleRateHzArg, sampleRateHz)) {
            return;
        }

        sampleRateHz = sampleRateHzArg;

        if (position.isSmoothing()) {
            retarget(getTargetSide());
        }
    }

    void fadeTo(Side side) noexcept {
        if (getTargetSide() == side) {
            return;
        }

        retarget(side);
    }

    void snapTo(Side side) noexcept {
        if (getTargetSide() == side) {
            return;
        }
        position.setCurrentAndTargetValue(positionFor(side));
    }

    [[nodiscard]] Mix getNextMix() noexcept {
        const auto mixPosition = position.getNextValue();
        return Mix {1.0f - mixPosition, mixPosition};
    }

    [[nodiscard]] bool isFading() const noexcept {
        return position.isSmoothing();
    }
    [[nodiscard]] bool isAt(Side side) const noexcept {
        return juce::exactlyEqual(position.getCurrentValue(), positionFor(side));
    }

    [[nodiscard]] Side getTargetSide() const noexcept {
        return juce::exactlyEqual(position.getTargetValue(), positionFor(Side::B)) ? Side::B : Side::A;
    }

  private:
    // ========
    // Private Methods
    // ========
    void retarget(Side side) noexcept {
        const auto targetPosition        = positionFor(side);
        const auto currentPosition       = position.getCurrentValue();
        const auto distance              = static_cast<double>(std::abs(targetPosition - currentPosition));
        const auto fullFadeLengthSamples = fullLengthSamples();

        if (distance < 1.0 / fullFadeLengthSamples) {
            // Sub-step -> snap to target
            position.setCurrentAndTargetValue(targetPosition);
            return;
        }

        // Samples needed to reach distance
        const auto exactDistanceSamples = distance * fullFadeLengthSamples;
        const auto countTolerance = static_cast<double>(std::numeric_limits<float>::epsilon()) * fullFadeLengthSamples;
        const auto remainingFadeSamples = static_cast<int>(std::ceil(exactDistanceSamples - countTolerance));

        // Reset the ramp length for the remaining number of samples
        position.reset(remainingFadeSamples);
        // Restore current position, because reset(numSteps) also does setCurrentAndTargetValue(oldTarget)
        position.setCurrentAndTargetValue(currentPosition);
        position.setTargetValue(targetPosition);
    }

    static constexpr float positionFor(Side side) noexcept {
        return side == Side::A ? 0.0f : 1.0f;
    }

    [[nodiscard]] double fullLengthSamples() const noexcept {
        const auto roundedSampleCount = static_cast<double>(std::lround(fadeSeconds * sampleRateHz));
        return std::max(1.0, roundedSampleCount);  // protect against returning 0, e.g. in unprepared state
    }

    // ========
    // State
    // ========
    double fadeSeconds  = 0.0;
    double sampleRateHz = 0.0;
    juce::LinearSmoothedValue<float> position {0.0f};
};

}  // namespace tremolo