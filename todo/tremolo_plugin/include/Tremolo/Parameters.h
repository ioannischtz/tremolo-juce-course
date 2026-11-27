#pragma once

#include "juce_audio_processors_headless/juce_audio_processors_headless.h"
#include "juce_core/system/juce_PlatformDefs.h"

namespace tremolo {
struct Parameters {
    explicit Parameters(juce::AudioProcessor&);

    juce::AudioParameterFloat& rateHz;
    juce::AudioParameterFloat& modulationDepth;
    juce::AudioParameterFloat& inputGainDB;
    juce::AudioParameterFloat& outputGainDB;
    juce::AudioParameterBool& isBypassed;
    juce::AudioParameterChoice& waveformShape;

    JUCE_DECLARE_NON_COPYABLE(Parameters)
    JUCE_DECLARE_NON_MOVEABLE(Parameters)
};
}  // namespace tremolo
