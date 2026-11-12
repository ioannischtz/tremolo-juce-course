#pragma once

#include "juce_audio_basics/juce_audio_basics.h"

namespace tremolo {
class Gain {
  public:
    Gain() = default;

    void prepare(double sampleRate) {
        targetGainDB.reset(sampleRate, 0.020f);
        targetGainDB.setCurrentAndTargetValue(0.f);
    }

    void setTargetGainDB(float gainDB) {
        targetGainDB.setTargetValue(gainDB);
    }

    void process(juce::AudioBuffer<float>& buffer) noexcept {
        // for each frame
        for (const auto frameIndex :
             std::views::iota(0, buffer.getNumSamples())) {
            auto gain =
                juce::Decibels::decibelsToGain(targetGainDB.getNextValue());
            for (const auto channelIndex :
                 std::views::iota(0, buffer.getNumChannels())) {
                const auto inputSample =
                    buffer.getSample(channelIndex, frameIndex);
                const auto outputSample = inputSample * gain;
                buffer.setSample(channelIndex, frameIndex, outputSample);
            }
        }
    }

    void reset() noexcept {}

  private:
    juce::SmoothedValue<float> targetGainDB;
};
}  // namespace tremolo
