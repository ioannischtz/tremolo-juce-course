#include "TestUtils.h"
#include <gtest/gtest.h>
#include <tremolo_plugin/tremolo_plugin.h>

#include <wolfsound/file/wolfsound_WavFileWriter.hpp>
#include <wolfsound/dsp/wolfsound_testSignals.hpp>

#include <cmath>
#include <cstdint>
#include <limits>

namespace tremolo {
namespace {
    juce::dsp::ProcessSpec makeProcessSpec(double sampleRate, int maximumBlockSize, int numChannels = 1) {
        return {
            .sampleRate       = sampleRate,
            .maximumBlockSize = static_cast<uint32_t>(maximumBlockSize),
            .numChannels      = static_cast<uint32_t>(numChannels),
        };
    }
}  // namespace

/** Processes a sine wave through the Tremolo effect and writes the result to
 * "tremoloSine.wav" for inspection.
 *
 * You can find the file in the same folder where the test executable resides,
 * most probably [CMake binary dir]/test/.
 */
TEST(Tremolo, ProcessesSineInput) {
    constexpr auto sampleRate       = 48000.0;
    constexpr auto inputFrequencyHz = 440.0f;
    constexpr auto duration         = wolfsound::Seconds {1.0f};

    const auto input = wolfsound::generateSine(
        wolfsound::Frequency {inputFrequencyHz}, wolfsound::Frequency {sampleRate}, duration);

    Tremolo testee;
    testee.selectLfoWaveformShape(Tremolo::LfoWaveformShape::sine);
    testee.setModulationRate(5.0f);
    testee.prepare(makeProcessSpec(sampleRate, static_cast<int>(input.size())));

    juce::AudioBuffer<float> buffer {1, static_cast<int>(input.size())};
    buffer.copyFrom(0, 0, input.data(), static_cast<int>(input.size()));

    juce::dsp::AudioBlock<float> const block {buffer};
    testee.process(block);

    wolfsound::WavFileWriter::writeToFile(
        getFileOutputPath("tremoloSine.wav"),
        juce::Span {buffer.getReadPointer(0), static_cast<size_t>(buffer.getNumSamples())},
        wolfsound::Frequency {sampleRate});
}

TEST(Tremolo, SnapToTargetsAppliesLatestShape) {
    Tremolo testee;
    constexpr auto sampleRate = 8.0;
    constexpr auto tolerance  = std::numeric_limits<float>::epsilon() * 32.0f;

    testee.setModulationRate(1.0f);
    testee.prepare(makeProcessSpec(sampleRate, 2));

    juce::AudioBuffer<float> firstBuffer {1, 1};
    firstBuffer.clear();
    firstBuffer.setSample(0, 0, 1.0f);
    juce::dsp::AudioBlock<float> const firstBlock {firstBuffer};
    testee.process(firstBlock);

    testee.selectLfoWaveformShape(Tremolo::LfoWaveformShape::softSquare);
    testee.snapToTargets();

    juce::AudioBuffer<float> secondBuffer {1, 1};
    secondBuffer.clear();
    secondBuffer.setSample(0, 0, 1.0f);
    juce::dsp::AudioBlock<float> const secondBlock {secondBuffer};
    testee.process(secondBlock);

    const auto expectedGain = [](float lfoValue) {
        constexpr auto modulationDepth = 0.4f;
        return (1.0f - modulationDepth) + (((lfoValue + 1.0f) * 0.5f) * modulationDepth);
    };

    EXPECT_NEAR(expectedGain(std::tanh(3.0f * std::sqrt(0.5f))), secondBuffer.getSample(0, 0), tolerance);
}
}  // namespace tremolo
