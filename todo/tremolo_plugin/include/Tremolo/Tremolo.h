#pragma once

#include <array>
#include "juce_core/juce_core.h"
#include "juce_core/system/juce_PlatformDefs.h"
#include "juce_dsp/juce_dsp.h"
namespace tremolo {
class Tremolo {
public:
  enum class LfoWaveform : size_t { sine = 0u, triangle = 1u };
  Tremolo() {
    for (auto& lfo : lfos) {
      lfo.setFrequency(5.0f /* Hz */, true);
    }
  }

  void prepare(double sampleRate, int expectedMaxFramesPerBlock) {
    const juce::dsp::ProcessSpec processSpec{
        .sampleRate = sampleRate,
        .maximumBlockSize =
            static_cast<juce::uint32>(expectedMaxFramesPerBlock),
        .numChannels = 1u,
    };
    for (auto& lfo : lfos) {
      lfo.prepare(processSpec);
    }
  }

  void selectLfoWaveform(LfoWaveform waveform) {
    jassert(waveform == LfoWaveform::sine || LfoWaveform::triangle == waveform);
    lfoToSet = waveform;
  }

  void process(juce::AudioBuffer<float>& buffer) noexcept {
    // for each frame
    for (const auto frameIndex : std::views::iota(0, buffer.getNumSamples())) {
      // Deferred parameter update pattern
      updateLfoWaveform();
      /*
       * lfoSample ∈ [ -1, 1 ]
       * has a range of 2, we can constrain it to a positive range of 1:
       * (lfoSample + 1) / 2 ∈ [ 0, 1 ]
       * where we want the gain:
       * gain ∈ [ 1 - modulationDepth, 1 ]
       * therefore:
       * (lfoSample + 1) / 2 * modulationDepth + ( 1 - modulationDepth)
       */
      const auto lfoValue = getNextLfoValue();
      constexpr auto modulationDepth = 0.4f;
      const auto modulationValue =
          (1.0f - modulationDepth) + (((lfoValue + 1) / 2) * modulationDepth);

      // for each channel sample in the frame
      for (const auto channelIndex :
           std::views::iota(0, buffer.getNumChannels())) {
        // get the input sample
        const auto inputSample = buffer.getSample(channelIndex, frameIndex);

        const auto outputSample = inputSample * modulationValue;

        // set the output sample
        buffer.setSample(channelIndex, frameIndex, outputSample);
      }
    }
  }

  void reset() noexcept {
    for (auto& lfo : lfos) {
      lfo.reset();
    }
  }

private:
  // Methods
  static float triangle(float phase) {
    const auto ft = phase / juce::MathConstants<float>::twoPi;
    return (4.f * std::abs(ft - std::floor(ft * 0.5f))) - 1.f;
  }

  float getNextLfoValue() {
    const auto selectedLfoIndex = juce::toUnderlyingType(selectedLfo);
    return lfos.at(selectedLfoIndex).processSample(0.f);
  }

  void updateLfoWaveform() {
    // TODO: add smoothing
    if (selectedLfo != lfoToSet) {
      selectedLfo = lfoToSet;
    }
  }

  // You should put class members and private functions here
  std::array<juce::dsp::Oscillator<float>, 2u> lfos{
      juce::dsp::Oscillator<float>{[](auto phase) { return std::sin(phase); }},
      juce::dsp::Oscillator<float>{triangle},
  };
  LfoWaveform selectedLfo = LfoWaveform::sine;
  LfoWaveform lfoToSet = selectedLfo;
};
}  // namespace tremolo
