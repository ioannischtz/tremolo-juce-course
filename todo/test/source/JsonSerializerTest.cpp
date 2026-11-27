#include <tremolo_plugin/tremolo_plugin.h>
#include <gtest/gtest.h>

namespace tremolo {
TEST(JsonSerializer, SerializeToString) {
    PluginProcessor processor;
    auto& parameters = processor.getParameterRefs();

    parameters.rateHz        = 10.f;
    parameters.inputGainDB   = 3.0f;
    parameters.outputGainDB  = 3.0f;
    parameters.isBypassed    = true;
    parameters.waveformShape = 1;

    const juce::String expectedOutput = u8R"({
  "__version__": 1,
  "pluginName": "TremoloTodo",
  "modulationRateHz": 10.0,
  "inputGainDB": 3.0,
  "outputGainDB": 3.0,
  "bypassed": true,
  "modulationWaveform": "Triangle"
})";
    juce::MemoryBlock block;
    juce::MemoryOutputStream outputStream {block, false};

    JsonSerializer::serialize(parameters, outputStream);
    outputStream.flush();

    const auto result = outputStream.toUTF8().removeCharacters("\r");

    EXPECT_EQ(expectedOutput, result);
}

TEST(JsonSerializer, DeserializeFromString) {
    const juce::String savedParameters = u8R"({
  "__version__": 1,
  "pluginName": "TremoloTodo",
  "modulationRateHz": 10.0,
  "inputGainDB": 3.0,
  "outputGainDB": 3.0,
  "bypassed": true,
  "modulationWaveform": "Triangle"
})";

    juce::MemoryInputStream inputStream {
        savedParameters.getCharPointer(), static_cast<size_t>(savedParameters.length()), false};

    PluginProcessor processor;
    auto& parameters = processor.getParameterRefs();

    const auto result = JsonSerializer::deserialize(inputStream, parameters);

    EXPECT_TRUE(result.wasOk());
    EXPECT_FLOAT_EQ(parameters.rateHz, 10.f);
    EXPECT_FLOAT_EQ(parameters.inputGainDB, 3.f);
    EXPECT_FLOAT_EQ(parameters.outputGainDB, 3.f);
    EXPECT_TRUE(parameters.isBypassed);
    EXPECT_EQ(juce::String {"Triangle"}, parameters.waveformShape.getCurrentChoiceName());
}

TEST(JsonSerializer, DontUpdateParametersWhenWaveformNameIsInvalid) {
    // given
    const juce::String savedParameters = u8R"({
  "__version__": 1,
  "pluginName": "TremoloTodo",
  "modulationRateHz": 10.0,
  "inputGainDB": 0.0,
  "outputGainDB": 0.0,
  "bypassed": true,
  "modulationWaveform": "Foo"
})";

    juce::MemoryInputStream inputStream {
        savedParameters.getCharPointer(), static_cast<size_t>(savedParameters.length()), false};

    PluginProcessor processor;
    auto& parameters = processor.getParameterRefs();

    parameters.waveformShape = 0;
    parameters.isBypassed    = false;
    parameters.rateHz        = 5.f;
    parameters.inputGainDB   = 0.0f;
    parameters.outputGainDB  = 0.0f;

    // when
    const auto result = JsonSerializer::deserialize(inputStream, parameters);

    // then
    EXPECT_TRUE(result.failed());
    EXPECT_FLOAT_EQ(parameters.rateHz.get(), 5.f);
    EXPECT_FLOAT_EQ(parameters.inputGainDB.get(), 0.f);
    EXPECT_FLOAT_EQ(parameters.outputGainDB.get(), 0.f);
    EXPECT_FLOAT_EQ(parameters.rateHz.get(), 5.f);
    EXPECT_FALSE(parameters.isBypassed.get());
    EXPECT_EQ(0, parameters.waveformShape.getIndex());
}
}  // namespace tremolo
