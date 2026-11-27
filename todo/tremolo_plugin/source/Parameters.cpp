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

    juce::AudioParameterFloat& createModulationDepthParameter(juce::AudioProcessor& processor) {
        constexpr auto versionHint = 1;
        auto parameter             = std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID {
                "modulation.depth",
                versionHint,
            },
            "Modulation Depth",
            juce::NormalisableRange {0.0f, 1.0f, 0.0001f, 1.0f},
            0.4f,
            juce::AudioParameterFloatAttributes {});
        return addParameterToProcessor(processor, std::move(parameter));
    }

    juce::AudioParameterFloat&
    createGainParameter(juce::AudioProcessor& processor, const char* parameterIDstr, const char* parameterName) {
        constexpr auto versionHint = 1;
        auto parameter =
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID {parameterIDstr, versionHint},
                                                        parameterName,
                                                        juce::NormalisableRange {-12.f, 12.f, 0.1f, 1.f},
                                                        0.f,
                                                        juce::AudioParameterFloatAttributes {}.withLabel("dB"));
        return addParameterToProcessor(processor, std::move(parameter));
    }

    juce::AudioParameterFloat& createInputGainParameter(juce::AudioProcessor& processor) {
        return createGainParameter(processor, "gain.in", "Input Gain");
    }

    juce::AudioParameterFloat& createOutputGainParameter(juce::AudioProcessor& processor) {
        return createGainParameter(processor, "gain.out", "Output Gain");
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

    juce::AudioParameterChoice& createWaveformShapeParameter(juce::AudioProcessor& processor) {
        constexpr auto versionHint = 1;
        auto parameter =
            std::make_unique<juce::AudioParameterChoice>(juce::ParameterID {"modulation.waveform", versionHint},
                                                         "Waveform shape",
                                                         juce::StringArray {"Sine", "Triangle", "Soft Square"},
                                                         0);
        return addParameterToProcessor(processor, std::move(parameter));
    }

}  // namespace

Parameters::Parameters(juce::AudioProcessor& processor)
    : rateHz {createModulationRateParameter(processor)},
      modulationDepth {createModulationDepthParameter(processor)},
      inputGainDB {createInputGainParameter(processor)},
      outputGainDB {createOutputGainParameter(processor)},
      isBypassed(createIsBypassedParameter(processor)),
      waveformShape(createWaveformShapeParameter(processor)) {}
}  // namespace tremolo
