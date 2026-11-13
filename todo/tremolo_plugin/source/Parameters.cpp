#include <memory>

#include "juce_audio_processors_headless/juce_audio_processors_headless.h"
#include "juce_core/juce_core.h"

namespace tremolo {
namespace {
    auto& addParameterToProcessor(juce::AudioProcessor& processor, auto parameter) {
        auto& parameterReference = *parameter;
        processor.addParameter(parameter.release());
        return parameterReference;
    }

    juce::AudioParameterFloat& createModulationRateParameter(juce::AudioProcessor& processor) {
        constexpr auto versionHint = 1;
        auto parameter             = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID {
                "modulation.rate",
                versionHint,
            },
            "Modulation rate",
            juce::NormalisableRange {0.1f, 20.f, 0.001f, 0.4f},
            5.f,
            juce::AudioParameterFloatAttributes {}.withLabel("Hz"));
        return addParameterToProcessor(processor, std::move(parameter));
    }

    juce::AudioParameterFloat& createGainParameter(juce::AudioProcessor& processor, bool isIn) {
        constexpr auto versionHint = 1;
        const auto* parameterID    = isIn ? "gain.in" : "gain.out";
        const auto* parameterName  = isIn ? "Input Gain" : "Output Gain";
        auto parameter =
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {parameterID, versionHint},
                                                        parameterName,
                                                        juce::NormalisableRange {-12.f, 12.f, 0.1f, 1.f},
                                                        0.f,
                                                        juce::AudioParameterFloatAttributes {}.withLabel("dB"));
        return addParameterToProcessor(processor, std::move(parameter));
    }

    juce::AudioParameterBool& createIsBypassedParameter(juce::AudioProcessor& processor) {
        constexpr auto versionHint = 1;
        auto parameter             = std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID {
                "isBypassed",
                versionHint,
            },
            "Bypass",
            false);
        return addParameterToProcessor(processor, std::move(parameter));
    }

}  // namespace

Parameters::Parameters(juce::AudioProcessor& processor)
    // TODO: create parameters
    // TODO: retrieve references to parameters
    // TODO: add parameters to the processor
    : rateHz {createModulationRateParameter(processor)},
      inputGainDB {createGainParameter(processor, true)},
      outputGainDB {createGainParameter(processor, false)},
      isBypassed(createIsBypassedParameter(processor)) {}
}  // namespace tremolo
