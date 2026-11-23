#include "TestUtils.h"
#include <gtest/gtest.h>
#include <tremolo_plugin/tremolo_plugin.h>

#include <cmath>
#include <limits>
#include <ranges>

namespace tremolo {
namespace {
    constexpr auto tolerance = std::numeric_limits<float>::epsilon() * 32.0f;
}

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
}  // namespace tremolo
