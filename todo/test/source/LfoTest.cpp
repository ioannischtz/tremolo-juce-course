#include "TestUtils.h"
#include <gtest/gtest.h>
#include <tremolo_plugin/tremolo_plugin.h>

#include <wolfsound/file/wolfsound_WavFileWriter.hpp>

#include <cmath>
#include <limits>
#include <ranges>
#include <vector>

namespace tremolo {
namespace {
    constexpr auto tolerance       = std::numeric_limits<float>::epsilon() * 32.0f;
    constexpr auto lfoSampleRate   = 48000.0;
    constexpr auto lfoRateHz       = 5.0f;
    constexpr auto lfoFadeSeconds  = 0.020f;
    constexpr auto lfoSampleCount  = static_cast<int>(lfoSampleRate);

    std::vector<float> renderLfo(Lfo& lfo, int sampleCount) {
        std::vector<float> samples(static_cast<size_t>(sampleCount));

        for (const auto sampleIndex : std::views::iota(0, sampleCount)) {
            samples[static_cast<size_t>(sampleIndex)] = lfo.getNextValue();
        }

        return samples;
    }

    void writeLfoFile(const std::vector<float>& samples, const char* fileName) {
        wolfsound::WavFileWriter::writeToFile(
            getFileOutputPath(fileName), samples, wolfsound::Frequency {lfoSampleRate});
    }

    void writeLfoWaveform(LfoWaveformShape shape, const char* fileName) {
        Lfo lfo {lfoFadeSeconds, shape, lfoRateHz};
        lfo.prepare(lfoSampleRate);
        lfo.setModulationRateImmediately(lfoRateHz);

        writeLfoFile(renderLfo(lfo, lfoSampleCount), fileName);
    }

    void writeSmoothedLfoTransition() {
        Lfo lfo {lfoFadeSeconds, LfoWaveformShape::sine, lfoRateHz};
        lfo.prepare(lfoSampleRate);
        lfo.setModulationRateImmediately(lfoRateHz);

        std::vector<float> samples(static_cast<size_t>(lfoSampleCount) * 2);

        for (const auto sampleIndex : std::views::iota(0, lfoSampleCount)) {
            samples[static_cast<size_t>(sampleIndex)] = lfo.getNextValue();
        }

        lfo.switchWaveformShape(LfoWaveformShape::triangle);

        for (const auto sampleIndex : std::views::iota(lfoSampleCount, 2 * lfoSampleCount)) {
            samples[static_cast<size_t>(sampleIndex)] = lfo.getNextValue();
        }

        writeLfoFile(samples, "smoothedSineToTriangleLfo.wav");
    }
}  // namespace

TEST(WaveformShape, GoldenValuesAt45Degrees) {
    constexpr auto phase = 0.125f;

    EXPECT_NEAR(std::sqrt(0.5f), evaluateWaveformShape(LfoWaveformShape::sine, phase), tolerance);
    EXPECT_NEAR(0.5f, evaluateWaveformShape(LfoWaveformShape::triangle, phase), tolerance);
    EXPECT_NEAR(
        std::tanh(3.0f * std::sqrt(0.5f)), evaluateWaveformShape(LfoWaveformShape::softSquare, phase), tolerance);
}

TEST(WaveformShape, AllShapesStayInUnitRange) {
    constexpr auto step = 1.0f / 4096.0f;

    for (const auto shape : enumRange<LfoWaveformShape>()) {
        for (const auto sampleIndex : std::views::iota(0, 4097)) {
            const auto value = evaluateWaveformShape(shape, static_cast<float>(sampleIndex) * step);

            EXPECT_GE(value, -1.0f);
            EXPECT_LE(value, 1.0f);
        }
    }
}

TEST(Lfo, PhaseAdvancesOncePerSample) {
    Lfo lfo {0.020f, LfoWaveformShape::sine, 1.0f};
    lfo.prepare(4.0);
    lfo.setModulationRateImmediately(1.0f);

    EXPECT_NEAR(0.0f, lfo.getNextValue(), tolerance);
    EXPECT_NEAR(1.0f, lfo.getNextValue(), tolerance);
    EXPECT_NEAR(0.0f, lfo.getNextValue(), tolerance);
    EXPECT_NEAR(-1.0f, lfo.getNextValue(), tolerance);
    EXPECT_NEAR(0.0f, lfo.getNextValue(), tolerance);
}

TEST(Lfo, ImmediateWaveformSwitchTakesEffectOnNextSample) {
    Lfo lfo {0.020f, LfoWaveformShape::sine, 1.0f};
    lfo.prepare(8.0);
    lfo.setModulationRateImmediately(1.0f);
    lfo.switchWaveformShapeImmediately(LfoWaveformShape::triangle);

    EXPECT_NEAR(0.0f, lfo.getNextValue(), tolerance);
    EXPECT_NEAR(0.5f, lfo.getNextValue(), tolerance);
}

TEST(Lfo, ResetAndPrepareRestartPhase) {
    Lfo lfo {0.020f, LfoWaveformShape::triangle, 1.0f};
    lfo.prepare(8.0);
    lfo.setModulationRateImmediately(1.0f);

    EXPECT_NEAR(0.0f, lfo.getNextValue(), tolerance);
    EXPECT_NEAR(0.5f, lfo.getNextValue(), tolerance);

    lfo.reset();
    EXPECT_NEAR(0.0f, lfo.getNextValue(), tolerance);
    EXPECT_NEAR(0.5f, lfo.getNextValue(), tolerance);

    lfo.prepare(8.0);
    EXPECT_NEAR(0.0f, lfo.getNextValue(), tolerance);
}

TEST(Lfo, WritesSineWaveform) {
    writeLfoWaveform(LfoWaveformShape::sine, "sineLfo.wav");
}

TEST(Lfo, WritesTriangleWaveform) {
    writeLfoWaveform(LfoWaveformShape::triangle, "triangleLfo.wav");
}

TEST(Lfo, WritesSoftSquareWaveform) {
    writeLfoWaveform(LfoWaveformShape::softSquare, "softSquareLfo.wav");
}

TEST(Lfo, WritesSmoothedSineToTriangleWaveform) {
    writeSmoothedLfoTransition();
}
}  // namespace tremolo
