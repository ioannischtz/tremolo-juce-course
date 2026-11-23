#pragma once

#include "Lfo.h"

#include <array>
#include "juce_audio_basics/juce_audio_basics.h"
#include "juce_core/juce_core.h"
#include "juce_dsp/juce_dsp.h"

namespace tremolo {
class Tremolo {
  public:
    using LfoWaveformShape = tremolo::LfoWaveformShape;

    // TODO Magic numbers
    Tremolo()
        : lfo {waveformFadeSeconds, defaultWaveformShape, defaultRateHz} {}

    // ========
    // Public Methods
    // ========

    void prepare(const double sampleRateHz, int expectedMaxFramesPerBlock) {
        juce::ignoreUnused(expectedMaxFramesPerBlock);
        lfo.prepare(sampleRateHz);
        snapToTargets();
    }

    void reset() noexcept {
        lfo.reset();
    }

    void selectLfoWaveformShape(LfoWaveformShape shape) noexcept {
        jassert(shape != LfoWaveformShape::count);
        requestedWaveformShape.store(shape, std::memory_order_relaxed);
    }

    void setModulationRate(float rateHz) noexcept {
        jassert(rateHz > 0.0f);
        requestedRateHz.store(rateHz, std::memory_order_relaxed);
    }

    void snapToTargets() noexcept {
        const auto requestedShape = requestedWaveformShape.load(std::memory_order_relaxed);
        lfo.switchWaveformShapeImmediately(requestedShape);
        appliedWaveformShape = requestedShape;

        const auto requestedRate = requestedRateHz.load(std::memory_order_relaxed);
        lfo.setModulationRateImmediately(requestedRate);
        appliedRateHz = requestedRate;
    }

    void process(juce::AudioBuffer<float>& buffer) noexcept {
        applyRequests();

        for (const auto sampleIndex : std::views::iota(0, buffer.getNumSamples())) {
            const auto lfoValue        = lfo.getNextValue();
            const auto modulationValue = (1.0f - modulationDepth) + (((lfoValue + 1.0f) * 0.5f) * modulationDepth);

            for (const auto channelIndex : std::views::iota(0, buffer.getNumChannels())) {
                const auto inputSample = buffer.getSample(channelIndex, sampleIndex);
                buffer.setSample(channelIndex, sampleIndex, inputSample * modulationValue);
            }
        }
    }

  private:
    // ========
    // Private Methods
    // ========

    void applyRequests() noexcept {
        // TODO: comment rationale for `memory_order_relaxed`
        if (const auto requestedShape = requestedWaveformShape.load(std::memory_order_relaxed);
            requestedShape != appliedWaveformShape) {
            lfo.switchWaveformShape(requestedShape);
            appliedWaveformShape = requestedShape;
        }

        if (const auto requestedRate = requestedRateHz.load(std::memory_order_relaxed);
            !juce::approximatelyEqual(requestedRate, appliedRateHz)) {
            lfo.setModulationRate(requestedRate);
            appliedRateHz = requestedRate;
        }
    }

    // constants
    static constexpr auto waveformFadeSeconds  = 0.02f;
    static constexpr auto defaultRateHz        = 5.0f;
    static constexpr auto defaultWaveformShape = LfoWaveformShape::sine;
    static constexpr auto modulationDepth      = 0.4f;

    // ========
    // State
    // ========
    Lfo lfo;
    std::atomic<float> requestedRateHz {defaultRateHz};
    float appliedRateHz = defaultRateHz;
    std::atomic<LfoWaveformShape> requestedWaveformShape {defaultWaveformShape};
    LfoWaveformShape appliedWaveformShape = defaultWaveformShape;
};
}  // namespace tremolo
