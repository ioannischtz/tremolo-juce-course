#include <memory>
#include "juce_audio_processors_headless/juce_audio_processors_headless.h"
#include "juce_core/juce_core.h"
namespace tremolo {
namespace {
juce::AudioParameterFloat& createModulationRateParameter(
    juce::AudioProcessor& processor) {
  constexpr auto versionHint = 1;
  auto parameter = std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{
          "modulation.rate",
          versionHint,
      },
      "Modulation rate", juce::NormalisableRange{0.1f, 20.f, 0.001f, 0.4f}, 5.f,
      juce::AudioParameterFloatAttributes{}.withLabel("Hz"));
  auto& parameterReference = *parameter;
  processor.addParameter(parameter.release());
  return parameterReference;
}
}  // namespace
Parameters::Parameters(juce::AudioProcessor& processor)
    // TODO: create parameters
    // TODO: retrieve references to parameters
    // TODO: add parameters to the processor
    : rateHz{createModulationRateParameter(processor)} {}
}  // namespace tremolo
