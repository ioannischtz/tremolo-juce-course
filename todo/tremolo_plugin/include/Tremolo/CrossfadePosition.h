#pragma once

#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace tremolo {
/**
 * Class representing a normalized crossfade position in  [0,1]
 *
 * 0: only source A
 * 1: only source B
 *
 * It moves toward a target at a **constant rate**
 * It is source agnostic
 *
 * The constructor's lengthSeconds is the full-travel time 
 * for the transition of 0->1
 *
 * Constant rate: 
 * Shorter distances take proportionally less time.
 * Re-targeting mid-transition starts from **current position**
 *
 * Invariants:
 *
 * - Position always within [0,1]
 * - No step should exceed 1 / (lengthSeconds * sampleRateHz)
 * - Reaches the target at exact floating-point value
 * - After reaching the target, it holds, without further stepping
 *  
 */
class CrossfadePosition {
  public:
    explicit CrossfadePosition(double lengthSecondsArg)
        : lengthSeconds {lengthSecondsArg} {
        jassert(lengthSecondsArg > 0.0);
    }

    // =========
    // Public Methods
    // =========
    void prepare(double sampleRateHzArg) noexcept {
        jassert(sampleRateHzArg > 0.0);
        if (juce::approximatelyEqual(sampleRateHzArg, sampleRateHz)) {
            return;
        }

        sampleRateHz = sampleRateHzArg;

        if (position.isSmoothing()) {
            retargetFromCurrent(position.getTargetValue());
        }
    }

    /**
     * Start or redirect the ramp
     *
     * newTarget: clamped to [0.0f, 1.0f]
     */
    void setTarget(float newTarget) noexcept {
        jassert(newTarget >= 0.0f);

        const auto clampedTarget = std::clamp(newTarget, 0.0f, 1.0f);
        if (juce::exactlyEqual(clampedTarget, position.getTargetValue())) {
            return;
        }
        retargetFromCurrent(clampedTarget);
    }

    /**
     * Cancel the ongoing ramp and snap the (current and target) 
     * position to the requested value
     */
    void snapTo(float newPosition) noexcept {
        jassert(newPosition >= 0.0f);
        const auto clampedPosition = std::clamp(newPosition, 0.0f, 1.0f);
        position.setCurrentAndTargetValue(clampedPosition);
    }

    [[nodiscard]] float getNextPosition() noexcept {
        return position.getNextValue();
    }

    [[nodiscard]] float getCurrentPosition() const noexcept {
        return position.getCurrentValue();
    }

    [[nodiscard]] float getTargetPosition() const noexcept {
        return position.getTargetValue();
    }

    [[nodiscard]] float skip(int numSamples) noexcept {
        jassert(numSamples >= 0);
        return position.skip(numSamples);
    }

    [[nodiscard]] bool isTransitioning() const noexcept {
        return position.isSmoothing();
    }

    [[nodiscard]] bool isAtStart() const noexcept {
        return juce::exactlyEqual(position.getCurrentValue(), 0.0f);
    }

    [[nodiscard]] bool isAtEnd() const noexcept {
        return juce::exactlyEqual(position.getCurrentValue(), 1.0f);
    }

  private:
    // =========
    // Private Methods
    // =========

    void retargetFromCurrent(float newTarget) noexcept {
        const auto lengthSamples   = fullLengthSamples();
        const auto currentPosition = position.getCurrentValue();
        const auto distance        = static_cast<double>(std::abs(newTarget - currentPosition));

        if (distance < 1.0f / lengthSamples) {
            // Sub-step -> snap to target
            position.setCurrentAndTargetValue(newTarget);
        } else {
            // Steps needed to reach distance
            const auto numSteps = static_cast<int>(std::ceil(distance * lengthSamples));
            // Reset the ramp length for the next target
            position.reset(numSteps);
            // Restore current position, because reset(numSteps) also does setCurrentAndTargetValue(oldTarget)
            position.setCurrentAndTargetValue(currentPosition);
            position.setTargetValue(newTarget);
        }
    }

    [[nodiscard]] int fullLengthSamples() const noexcept {
        const auto roundedSampleCount = static_cast<int>(std::lround(lengthSeconds * sampleRateHz));
        return std::max(1, roundedSampleCount);  // protect against returning 0, e.g. in unprepared state
    }

    // =========
    // State
    // =========

    double lengthSeconds = 0.0;
    double sampleRateHz  = 0.0;
    juce::LinearSmoothedValue<float> position {0.0f};
};
}  // namespace tremolo
