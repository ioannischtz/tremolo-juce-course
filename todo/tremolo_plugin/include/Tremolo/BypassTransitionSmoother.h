#pragma once

#include <ranges>
#include "juce_dsp/juce_dsp.h"
#include "tremolo_plugin/include/Tremolo/Crossfade.h"

namespace tremolo {
/**
 * Class facilitating transition to and from bypassed state over a single block.
 *
 * It allows 2 things:
 *  - detecting that a bypass state transition takes place
 *  - fade-in and fade-out of dry (unprocessed) and wet (processed) buffers
 *    according to the transition type
 *
 * Use the following code in the processBlock() method of your plugin processor:
 *
 * @code
 * //...
 *
 * bypassTransitionSmoother.setBypass(parameters.bypassed);
 *
 * if (bypassTransitionSmoother.isTransitioning()) {
 *   bypassTransitionSmoother.setDryBuffer(buffer);
 *
 *   yourAudioEffectClassInstance.process(buffer);
 *
 *   bypassTransitionSmoother.mixToWetBuffer(buffer);
 *
 *   return;
 * }
 *
 * // avoid processing if bypassed, process otherwise...
 * @endcode
 *
 * Alternatively, you can unconditionally call setDryBuffer() and
 * mixToWetBuffer(), like this:
 *
 * @code
 * //...
 *
 * bypassTransitionSmoother.setBypass(parameters.bypassed);
 *
 * if (parameters.bypassed && !bypassTransitionSmoother.isTransitioning()) {
 *   // avoid processing if the plugin is bypassed
 *   return;
 * }
 *
 * bypassTransitionSmoother.setDryBuffer(buffer);
 * yourAudioEffectClassInstance.process(buffer);
 * bypassTransitionSmoother.mixToWetBuffer(buffer);
 *
 * // no more processing necessary
 * @endcode
 *
 * Remember to call prepare() in prepareToPlay(),
 * setBypassForced() in setStateInformation(), and reset() in
 * releaseResources().
 */
class BypassTransitionSmoother {
  static constexpr Crossfade::Side wet = Crossfade::Side::A;
  static constexpr Crossfade::Side dry = Crossfade::Side::B;

  public:
    explicit BypassTransitionSmoother(double crossfadeLengthSecondsValue = 0.01)
        : crossfade {crossfadeLengthSecondsValue} {
        jassert(0.0 < crossfadeLengthSecondsValue);
    }

    void prepare(const juce::dsp::ProcessSpec& spec) {
        crossfade.prepare(spec.sampleRate);
        dryBuffer.setSize(static_cast<int>(spec.numChannels), static_cast<int>(spec.maximumBlockSize));
    }

    void setBypass(bool bypass) noexcept {
        crossfade.fadeTo(bypass ? dry : wet);
    }

    void setBypassForced(bool bypass) noexcept {
        crossfade.snapTo(bypass ? dry : wet);
    }

    [[nodiscard]] bool isTransitioning() const noexcept {
        return crossfade.isFading();
    }

    void setDryBuffer(const juce::AudioBuffer<float>& buffer) noexcept {
        jassert(buffer.getNumSamples() <= dryBuffer.getNumSamples());
        jassert(buffer.getNumChannels() <= dryBuffer.getNumChannels());

        for (const auto channel : std::views::iota(0, buffer.getNumChannels())) {
            dryBuffer.copyFrom(channel, 0, buffer, channel, 0, buffer.getNumSamples());
        }
    }

    void mixToWetBuffer(juce::AudioBuffer<float>& wetBuffer) noexcept {
        jassert(wetBuffer.getNumSamples() <= dryBuffer.getNumSamples());
        jassert(wetBuffer.getNumChannels() <= dryBuffer.getNumChannels());

        for (const auto sampleIndex: std::views::iota(0, wetBuffer.getNumSamples())) {
            const auto crossFadeMix= crossfade.getNextMix();
            const auto wetGain           = crossFadeMix.get(wet);
            const auto dryGain           = crossFadeMix.get(dry);

            for (const auto channel : std::views::iota(0, wetBuffer.getNumChannels())) {
                const auto wetSample   = wetBuffer.getSample(channel, sampleIndex);
                const auto drySample   = dryBuffer.getSample(channel, sampleIndex);
                const auto mixedSample = (wetGain * wetSample) + (dryGain * drySample);
                wetBuffer.setSample(channel, sampleIndex, mixedSample);
            }
        }
    }

    void reset() noexcept {
        setBypassForced(false);
        dryBuffer.clear();
    }

  private:
    Crossfade crossfade {0.01, wet};
    juce::AudioBuffer<float> dryBuffer;
};
}  // namespace tremolo
