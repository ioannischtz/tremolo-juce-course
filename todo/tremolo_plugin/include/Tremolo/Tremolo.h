#pragma once

#include <array>
#include "juce_core/juce_core.h"
#include "juce_dsp/juce_dsp.h"
namespace tremolo {
class Tremolo {
public:
  enum class LfoWAveform : size_t { sine = 0u, triangle = 1u };
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

  void process(juce::AudioBuffer<float>& buffer) noexcept {
    // for each frame
    for (const auto frameIndex : std::views::iota(0, buffer.getNumSamples())) {
      /*
       * lfoSample ∈ [ -1, 1 ]
       * has a range of 2, we can constrain it to a positive range of 1:
       * (lfoSample + 1) / 2 ∈ [ 0, 1 ]
       * where we want the gain:
       * gain ∈ [ 1 - modulationDepth, 1 ]
       * therefore:
       * (lfoSample + 1) / 2 * modulationDepth + ( 1 - modulationDepth)
       */
      const auto selectedLfoIndex = juce::toUnderlyingType(selectedLfo);
      const auto lfoValue = lfos.at(selectedLfoIndex).processSample(0.f);
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
  // You should put class members and private functions here
  std::array<juce::dsp::Oscillator<float>, 1u> lfos{
      juce::dsp::Oscillator<float>{[](auto phase) { return std::sin(phase); }}};
  LfoWAveform selectedLfo = LfoWAveform::sine;
};
}  // namespace tremolo
