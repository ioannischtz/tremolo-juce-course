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

    void extractLfo(Tremolo& tremolo, juce::AudioBuffer<float>& bufferToUse) {
        juce::dsp::AudioBlock<float> block {bufferToUse};
        block.fill(1.f);
        tremolo.process(block);
        block.subtract(1.f);
    }
}  // namespace

/** This test extracts the LFO used by the Tremolo effect and saves it to a WAV
 * file "lfo.wav".
 *
 * You can find the file in the same folder where the test executable resides,
 * most probably [CMake binary dir]/test/.
 *
 * The LFO rate is not changed; thus, the one set in Tremolo's constructor will
 * be used.
 */
TEST(Tremolo, ExtractLfo) {
    for (const auto lfoWaveform : {Tremolo::LfoWaveformShape::sine, Tremolo::LfoWaveformShape::triangle}) {
        Tremolo testee;
        constexpr auto sampleRate = 48000.0;
        testee.selectLfoWaveformShape(lfoWaveform);
        testee.prepare(makeProcessSpec(sampleRate, static_cast<int>(sampleRate)));

        juce::AudioBuffer<float> buffer;
        buffer.setSize(1, static_cast<int>(sampleRate));

        extractLfo(testee, buffer);

        const auto* const fileName = lfoWaveform == Tremolo::LfoWaveformShape::sine ? "sineLfo.wav" : "triangleLfo.wav";

        wolfsound::WavFileWriter::writeToFile(
            getFileOutputPath(fileName),
            juce::Span {buffer.getReadPointer(0), static_cast<size_t>(buffer.getNumSamples())},
            wolfsound::Frequency {sampleRate});
    }
}

/** This test extracts the LFO used by the Tremolo effect switching the LFO
 * shape mid-processing, and saves it to a WAV file "smoothedLfo.wav".
 *
 * You can find the file in the same folder where the test executable resides,
 * most probably [CMake binary dir]/test/.
 *
 * The LFO rate is not changed; thus, the one set in Tremolo's constructor will
 * be used.
 */
TEST(Tremolo, LfoWaveformTransitionIsSmooth) {
    Tremolo testee;
    constexpr auto sampleRate       = 48000.0;
    constexpr auto channelCount     = 1;
    constexpr auto blockSizeSeconds = 1;
    constexpr auto blockSizeSamples = static_cast<int>(blockSizeSeconds * sampleRate);
    testee.prepare(makeProcessSpec(sampleRate, blockSizeSamples));

    juce::AudioBuffer<float> outputBuffer;
    constexpr auto recordingLengthSamples = 2 * blockSizeSamples;
    outputBuffer.setSize(channelCount, recordingLengthSamples);

    juce::AudioBuffer<float> processBuffer;
    processBuffer.setSize(channelCount, blockSizeSamples);

    testee.selectLfoWaveformShape(Tremolo::LfoWaveformShape::sine);
    extractLfo(testee, processBuffer);
    outputBuffer.copyFrom(0, 0, processBuffer, 0, 0, blockSizeSamples);
    testee.selectLfoWaveformShape(Tremolo::LfoWaveformShape::triangle);
    extractLfo(testee, processBuffer);
    outputBuffer.copyFrom(0, blockSizeSamples, processBuffer, 0, 0, blockSizeSamples);

    wolfsound::WavFileWriter::writeToFile(
        getFileOutputPath("smoothedLfo.wav"),
        juce::Span {outputBuffer.getReadPointer(0), static_cast<size_t>(outputBuffer.getNumSamples())},
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
    juce::dsp::AudioBlock<float> firstBlock {firstBuffer};
    testee.process(firstBlock);

    testee.selectLfoWaveformShape(Tremolo::LfoWaveformShape::softSquare);
    testee.snapToTargets();

    juce::AudioBuffer<float> secondBuffer {1, 1};
    secondBuffer.clear();
    secondBuffer.setSample(0, 0, 1.0f);
    juce::dsp::AudioBlock<float> secondBlock {secondBuffer};
    testee.process(secondBlock);

    const auto expectedGain = [](float lfoValue) {
        constexpr auto modulationDepth = 0.4f;
        return (1.0f - modulationDepth) + (((lfoValue + 1.0f) * 0.5f) * modulationDepth);
    };

    EXPECT_NEAR(expectedGain(std::tanh(3.0f * std::sqrt(0.5f))), secondBuffer.getSample(0, 0), tolerance);
}
}  // namespace tremolo
