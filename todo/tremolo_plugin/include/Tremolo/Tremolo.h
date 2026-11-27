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

    Tremolo()
        : lfo {waveformFadeSeconds, defaultWaveformShape, defaultRateHz} {}

    // ========
    // Public Methods
    // ========

    void prepare(const juce::dsp::ProcessSpec& spec) {
        juce::ignoreUnused(spec.maximumBlockSize);
        modulationDepth.reset(spec.sampleRate, depthSmoothingSeconds);
        lfo.prepare(spec.sampleRate);
        snapToTargets();
    }

    void reset() noexcept {
        lfo.reset();
    }

    void selectLfoWaveformShape(LfoWaveformShape shape) noexcept {
        jassert(shape != LfoWaveformShape::count);
        requestedWaveformShape.store(shape, std::memory_order_relaxed);
    }

    void setModulationRate(const float rateHz) noexcept {
        jassert(rateHz > 0.0f);
        requestedRateHz.store(rateHz, std::memory_order_relaxed);
    }

    void setModulationDepth(const float depth) noexcept {
        jassert(depth >= 0.0f && depth <= 1.0f);
        requestedModulationDepth.store(depth, std::memory_order_relaxed);
    }

    void snapToTargets() noexcept {
        const auto requestedShape = requestedWaveformShape.load(std::memory_order_relaxed);
        lfo.switchWaveformShapeImmediately(requestedShape);
        appliedWaveformShape = requestedShape;

        const auto requestedRate = requestedRateHz.load(std::memory_order_relaxed);
        lfo.setModulationRateImmediately(requestedRate);
        appliedRateHz = requestedRate;

        const auto requestedDepth = requestedModulationDepth.load(std::memory_order_relaxed);
        modulationDepth.setCurrentAndTargetValue(requestedDepth);
        appliedModulationDepth = requestedDepth;
    }

    // Overload for ProcessorChain.process
    void process(const juce::dsp::ProcessContextReplacing<float>& context) noexcept {
        process(context.getOutputBlock());
    }

    void process(const juce::dsp::AudioBlock<float>& block) noexcept {
        applyRequests();

        for (const auto sampleIndex : std::views::iota(0, static_cast<int>(block.getNumSamples()))) {
            const auto lfoValue             = lfo.getNextValue();
            const auto modulationDepthValue = modulationDepth.getNextValue();
            const auto modulationValue =
                (1.0f - modulationDepthValue) + (((lfoValue + 1.0f) * 0.5f) * modulationDepthValue);

            for (const auto channelIndex : std::views::iota(0, static_cast<int>(block.getNumChannels()))) {
                const auto inputSample = block.getSample(channelIndex, sampleIndex);
                block.setSample(channelIndex, sampleIndex, inputSample * modulationValue);
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

        if (const auto requestedDepth = requestedModulationDepth.load(std::memory_order_relaxed);
            !juce::approximatelyEqual(requestedDepth, appliedModulationDepth)) {
            modulationDepth.setTargetValue(requestedDepth);
            appliedModulationDepth = requestedDepth;
        }
    }

    // constants
    static constexpr auto waveformFadeSeconds    = 0.02f;
    static constexpr auto depthSmoothingSeconds  = 0.01;
    static constexpr auto defaultRateHz          = 5.0f;
    static constexpr auto defaultWaveformShape   = LfoWaveformShape::sine;
    static constexpr auto defaultModulationDepth = 0.4f;

    // ========
    // State
    // ========
    Lfo lfo;
    juce::SmoothedValue<float> modulationDepth {defaultModulationDepth};
    std::atomic<float> requestedModulationDepth {defaultModulationDepth};
    float appliedModulationDepth = defaultModulationDepth;
    std::atomic<float> requestedRateHz {defaultRateHz};
    float appliedRateHz = defaultRateHz;
    std::atomic<LfoWaveformShape> requestedWaveformShape {defaultWaveformShape};
    LfoWaveformShape appliedWaveformShape = defaultWaveformShape;
};
}  // namespace tremolo
