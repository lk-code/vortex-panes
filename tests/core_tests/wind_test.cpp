#include "core/wind.h"

#include "core/fixed_timestep.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>

namespace {

constexpr int kTicksPerHour = 3600 * vp::kSimulationRate;

double tickTime(int tick) {
    return static_cast<double>(tick) / vp::kSimulationRate;
}

} // namespace

TEST(WindField, StaysWithinBaseAndAmplitude) {
    const vp::WindParams params;
    const vp::WindField wind(7u, params);
    for (int tick = 0; tick < kTicksPerHour; tick += 7) {
        const float value = wind.sample(tickTime(tick));
        ASSERT_GE(value, params.baseSpeed - params.amplitude - 1e-3f);
        ASSERT_LE(value, params.baseSpeed + params.amplitude + 1e-3f);
    }
}

TEST(WindField, ChangesSmoothlyWithoutJumps) {
    const vp::WindParams params;
    const vp::WindField wind(7u, params);
    float previous = wind.sample(0.0);
    float largestStep = 0.0f;
    for (int tick = 1; tick < kTicksPerHour / 4; ++tick) {
        const float value = wind.sample(tickTime(tick));
        largestStep = std::max(largestStep, std::abs(value - previous));
        previous = value;
    }
    EXPECT_LT(largestStep, params.amplitude * 0.02f);
}

TEST(WindField, VariesOverTime) {
    const vp::WindParams params;
    const vp::WindField wind(11u, params);
    float minimum = wind.sample(0.0);
    float maximum = minimum;
    for (int tick = 0; tick < 120 * vp::kSimulationRate; ++tick) {
        const float value = wind.sample(tickTime(tick));
        minimum = std::min(minimum, value);
        maximum = std::max(maximum, value);
    }
    EXPECT_GT(maximum - minimum, params.amplitude * 0.5f);
}

TEST(WindField, AveragesToTheBaseSpeed) {
    const vp::WindParams params;
    const vp::WindField wind(3u, params);
    double sum = 0.0;
    int samples = 0;
    for (int tick = 0; tick < kTicksPerHour; tick += 30) {
        sum += wind.sample(tickTime(tick));
        ++samples;
    }
    EXPECT_NEAR(sum / samples, params.baseSpeed, params.amplitude * 0.15);
}

TEST(WindField, IsDeterministicPerSeed) {
    const vp::WindParams params;
    const vp::WindField a(42u, params);
    const vp::WindField b(42u, params);
    const vp::WindField other(43u, params);
    int differences = 0;
    for (int tick = 0; tick < 600; ++tick) {
        ASSERT_EQ(a.sample(tickTime(tick)), b.sample(tickTime(tick)));
        differences += a.sample(tickTime(tick)) != other.sample(tickTime(tick)) ? 1 : 0;
    }
    EXPECT_GT(differences, 500);
}

TEST(WindField, NoAmplitudeGivesConstantWind) {
    vp::WindParams params;
    params.baseSpeed = -12.0f;
    params.amplitude = 0.0f;
    const vp::WindField wind(1u, params);
    EXPECT_FLOAT_EQ(wind.sample(0.0), -12.0f);
    EXPECT_FLOAT_EQ(wind.sample(1234.5), -12.0f);
}
