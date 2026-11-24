#pragma once

#include "juce_audio_basics/juce_audio_basics.h"

namespace tremolo {
class Gain {
  public:
    Gain() = default;

    void prepare(const juce::dsp::ProcessSpec& spec) {
        targetGainDB.reset(spec.sampleRate, 0.020f);
        targetGainDB.setCurrentAndTargetValue(0.f);
    }

    void setTargetGainDB(float gainDB) {
        targetGainDB.setTargetValue(gainDB);
    }

    // Overload for ProcessorChain.process
    void process(const juce::dsp::ProcessContextReplacing<float>& context) noexcept {
        process(context.getOutputBlock());
    }

    void process(const juce::dsp::AudioBlock<float>& block) noexcept {
        // for each frame
        for (const auto sampleIndex : std::views::iota(0, static_cast<int>(block.getNumSamples()))) {
            const auto gain = juce::Decibels::decibelsToGain(targetGainDB.getNextValue());
            for (const auto channelIndex : std::views::iota(0, static_cast<int>(block.getNumChannels()))) {
                const auto inputSample  = block.getSample(channelIndex, sampleIndex);
                const auto outputSample = inputSample * gain;
                block.setSample(channelIndex, sampleIndex, outputSample);
            }
        }
    }

    void reset() noexcept {}

  private:
    juce::SmoothedValue<float> targetGainDB;
};
}  // namespace tremolo
