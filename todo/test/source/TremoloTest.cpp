#include "TestUtils.h"
#include <gtest/gtest.h>
#include <tremolo_plugin/tremolo_plugin.h>

#include <wolfsound/file/wolfsound_WavFileWriter.hpp>
#include <wolfsound/dsp/wolfsound_testSignals.hpp>

#include <algorithm>
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

TEST(Tremolo, ZeroDepthLeavesInputUnchanged) {
    constexpr auto sampleRate = 48000.0;
    constexpr auto numSamples = 512;
    constexpr auto tolerance  = std::numeric_limits<float>::epsilon() * 32.0f;

    Tremolo testee;
    testee.selectLfoWaveformShape(Tremolo::LfoWaveformShape::sine);
    testee.setModulationRate(5.0f);
    testee.prepare(makeProcessSpec(sampleRate, numSamples));

    testee.setModulationDepth(0.0f);
    testee.snapToTargets();

    juce::AudioBuffer<float> buffer {1, numSamples};
    for (const auto sampleIndex : std::views::iota(0, numSamples)) {
        buffer.setSample(0, sampleIndex, std::sin(0.1f * static_cast<float>(sampleIndex)));
    }
    const auto input = buffer;

    juce::dsp::AudioBlock<float> const block {buffer};
    testee.process(block);

    for (const auto sampleIndex : std::views::iota(0, numSamples)) {
        EXPECT_NEAR(input.getSample(0, sampleIndex), buffer.getSample(0, sampleIndex), tolerance)
            << "sample " << sampleIndex;
    }
}

TEST(Tremolo, DepthSetBeforePrepareIsApplied) {
    constexpr auto sampleRate = 8.0;
    constexpr auto tolerance  = std::numeric_limits<float>::epsilon() * 32.0f;

    Tremolo testee;
    testee.selectLfoWaveformShape(Tremolo::LfoWaveformShape::sine);
    testee.setModulationRate(1.0f);
    testee.setModulationDepth(1.0f);
    testee.prepare(makeProcessSpec(sampleRate, 1));

    juce::AudioBuffer<float> buffer {1, 1};
    buffer.setSample(0, 0, 1.0f);

    juce::dsp::AudioBlock<float> const block {buffer};
    testee.process(block);

    // The first LFO sample is at phase 0, where the sine is 0.
    // Full depth therefore yields a gain of 0.5.
    EXPECT_NEAR(0.5f, buffer.getSample(0, 0), tolerance);
}

TEST(Tremolo, FullDepthUsesFullGainRange) {
    constexpr auto sampleRate = 8.0;
    constexpr auto rateHz     = 1.0f;
    constexpr auto numSamples = 8;
    constexpr auto tolerance  = std::numeric_limits<float>::epsilon() * 32.0f;

    Tremolo testee;
    testee.selectLfoWaveformShape(Tremolo::LfoWaveformShape::sine);
    testee.setModulationRate(rateHz);
    testee.prepare(makeProcessSpec(sampleRate, numSamples));

    testee.setModulationDepth(1.0f);
    testee.snapToTargets();

    juce::AudioBuffer<float> buffer {1, numSamples};
    for (const auto sampleIndex : std::views::iota(0, numSamples)) {
        buffer.setSample(0, sampleIndex, 1.0f);
    }

    juce::dsp::AudioBlock<float> const block {buffer};
    testee.process(block);

    for (const auto sampleIndex : std::views::iota(0, numSamples)) {
        // The LFO evaluates at the phase before advancing it.
        const auto phaseTurns   = static_cast<float>(sampleIndex) * rateHz / static_cast<float>(sampleRate);
        const auto lfoValue     = std::sin(juce::MathConstants<float>::twoPi * phaseTurns);
        const auto expectedGain = (lfoValue + 1.0f) * 0.5f;

        EXPECT_NEAR(expectedGain, buffer.getSample(0, sampleIndex), tolerance) << "sample " << sampleIndex;
    }

    EXPECT_NEAR(1.0f, buffer.getSample(0, 2), tolerance);
    EXPECT_NEAR(0.0f, buffer.getSample(0, 6), tolerance);
}

TEST(Tremolo, ModulationDepthIsSmoothed) {
    constexpr auto sampleRate  = 1024.0;
    constexpr auto rateHz      = 1.0f;
    constexpr auto rampSamples = 10;  // 0.01 s depth smoothing
    constexpr auto numSamples  = 20;
    constexpr auto tolerance   = std::numeric_limits<float>::epsilon() * 64.0f;

    Tremolo testee;
    testee.selectLfoWaveformShape(Tremolo::LfoWaveformShape::sine);
    testee.setModulationRate(rateHz);
    testee.prepare(makeProcessSpec(sampleRate, numSamples));

    testee.setModulationDepth(0.0f);
    testee.snapToTargets();

    juce::AudioBuffer<float> buffer {1, numSamples};
    for (const auto sampleIndex : std::views::iota(0, numSamples)) {
        buffer.setSample(0, sampleIndex, 1.0f);
    }

    // Do not snap: the depth should ramp to its new target over 0.01 s.
    testee.setModulationDepth(1.0f);

    juce::dsp::AudioBlock<float> const block {buffer};
    testee.process(block);

    for (const auto sampleIndex : std::views::iota(0, numSamples)) {
        const auto depth        = std::min(static_cast<float>(sampleIndex + 1) / static_cast<float>(rampSamples), 1.0f);
        const auto phaseTurns   = static_cast<float>(sampleIndex) * rateHz / static_cast<float>(sampleRate);
        const auto lfoValue     = std::sin(juce::MathConstants<float>::twoPi * phaseTurns);
        const auto expectedGain = (1.0f - depth) + (((lfoValue + 1.0f) * 0.5f) * depth);

        EXPECT_NEAR(expectedGain, buffer.getSample(0, sampleIndex), tolerance) << "sample " << sampleIndex;
    }
}
}  // namespace tremolo
