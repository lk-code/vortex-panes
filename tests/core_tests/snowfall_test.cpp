#include "core/snowfall.h"

#include "core/fixed_timestep.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <vector>

namespace {

// The local setup: two 1440p monitors, the secondary one left of the primary.
constexpr std::array<vp::RectI, 2> kSideBySide = {vp::RectI{0, 0, 2560, 1440}, vp::RectI{-2560, 0, 0, 1440}};

vp::Flake flakeWithDepth(float depth) {
    vp::Flake flake;
    flake.depth = depth;
    return flake;
}

bool sameFlakes(std::span<const vp::Flake> a, std::span<const vp::Flake> b) {
    return a.size() == b.size() && std::memcmp(a.data(), b.data(), a.size_bytes()) == 0;
}

} // namespace

// --- Columns --------------------------------------------------------------------------------

TEST(SnowColumns, SideBySideMonitorsFormOneColumn) {
    const auto columns = vp::buildSnowColumns(kSideBySide);
    ASSERT_EQ(columns.size(), 1u);
    EXPECT_EQ(columns[0], (vp::SnowColumn{-2560.0f, 2560.0f, 0.0f, 1440.0f}));
}

TEST(SnowColumns, StackedMonitorsSpanBothHeights) {
    const std::array monitors = {vp::RectI{0, 0, 1920, 1080}, vp::RectI{0, 1080, 1920, 2160}};
    const auto columns = vp::buildSnowColumns(monitors);
    ASSERT_EQ(columns.size(), 1u);
    EXPECT_EQ(columns[0], (vp::SnowColumn{0.0f, 1920.0f, 0.0f, 2160.0f}));
}

TEST(SnowColumns, OffsetMonitorsKeepTheirOwnEdges) {
    const std::array monitors = {vp::RectI{0, 0, 2560, 1440}, vp::RectI{2560, 300, 4480, 1380}};
    const auto columns = vp::buildSnowColumns(monitors);
    ASSERT_EQ(columns.size(), 2u);
    EXPECT_EQ(columns[0], (vp::SnowColumn{0.0f, 2560.0f, 0.0f, 1440.0f}));
    EXPECT_EQ(columns[1], (vp::SnowColumn{2560.0f, 4480.0f, 300.0f, 1380.0f}));
}

TEST(SnowColumns, WiderMonitorBelowSplitsIntoStrips) {
    const std::array monitors = {vp::RectI{0, 0, 1920, 1080}, vp::RectI{-320, 1080, 2240, 2520}};
    const auto columns = vp::buildSnowColumns(monitors);
    ASSERT_EQ(columns.size(), 3u);
    EXPECT_EQ(columns[0], (vp::SnowColumn{-320.0f, 0.0f, 1080.0f, 2520.0f}));
    EXPECT_EQ(columns[1], (vp::SnowColumn{0.0f, 1920.0f, 0.0f, 2520.0f}));
    EXPECT_EQ(columns[2], (vp::SnowColumn{1920.0f, 2240.0f, 1080.0f, 2520.0f}));
}

TEST(SnowColumns, GapsAreLeftOut) {
    const std::array monitors = {vp::RectI{1200, 0, 2200, 1000}, vp::RectI{0, 0, 1000, 1000}};
    const auto columns = vp::buildSnowColumns(monitors);
    ASSERT_EQ(columns.size(), 2u);
    EXPECT_EQ(columns[0], (vp::SnowColumn{0.0f, 1000.0f, 0.0f, 1000.0f}));
    EXPECT_EQ(columns[1], (vp::SnowColumn{1200.0f, 2200.0f, 0.0f, 1000.0f}));
}

TEST(SnowColumns, NoMonitorsGiveNoColumns) {
    EXPECT_TRUE(vp::buildSnowColumns({}).empty());
    const std::array monitors = {vp::RectI{10, 10, 10, 500}};
    EXPECT_TRUE(vp::buildSnowColumns(monitors).empty());
}

// --- Integrator -----------------------------------------------------------------------------

TEST(IntegrateFlakes, ReachesFallSpeedWithoutOvershoot) {
    const vp::SnowfallParams params;
    std::array flakes = {flakeWithDepth(0.5f)};
    const float terminal = params.fallSpeed.at(0.5f);
    float previous = 0.0f;
    for (int tick = 0; tick < 300; ++tick) {
        vp::integrateFlakes(flakes, vp::kTickSeconds, 0.0f, params);
        ASSERT_GT(flakes[0].vy, previous);
        ASSERT_LE(flakes[0].vy, terminal);
        previous = flakes[0].vy;
    }
    EXPECT_NEAR(flakes[0].vy, terminal, terminal * 1e-3f);

    // From then on the flake falls linearly.
    const float y = flakes[0].y;
    for (int tick = 0; tick < 60; ++tick) {
        vp::integrateFlakes(flakes, vp::kTickSeconds, 0.0f, params);
    }
    EXPECT_NEAR(flakes[0].y - y, terminal, 0.5f);
}

TEST(IntegrateFlakes, NoWindAndNoWobbleKeepXConstant) {
    vp::SnowfallParams params;
    params.wobbleSpeed = {0.0f, 0.0f};
    std::array flakes = {flakeWithDepth(0.2f), flakeWithDepth(0.9f)};
    flakes[0].x = 100.0f;
    flakes[1].x = -350.5f;
    for (int tick = 0; tick < 600; ++tick) {
        vp::integrateFlakes(flakes, vp::kTickSeconds, 0.0f, params);
    }
    EXPECT_EQ(flakes[0].x, 100.0f);
    EXPECT_EQ(flakes[1].x, -350.5f);
    EXPECT_GT(flakes[0].y, 0.0f);
}

TEST(IntegrateFlakes, WobbleSwaysWithoutNetDrift) {
    vp::SnowfallParams params;
    params.wobbleSpeed = {10.0f, 10.0f};
    params.wobbleFrequency = {0.5f, 0.5f}; // one period = 120 ticks
    std::array flakes = {flakeWithDepth(0.5f)};
    float largestSway = 0.0f;
    for (int tick = 0; tick < 120; ++tick) {
        vp::integrateFlakes(flakes, vp::kTickSeconds, 0.0f, params);
        largestSway = std::max(largestSway, std::abs(flakes[0].x));
    }
    EXPECT_GT(largestSway, 3.0f); // 2 * 10 / (2 * pi * 0.5) = 6.4 px peak to peak
    EXPECT_NEAR(flakes[0].x, 0.0f, 0.01f);
    EXPECT_GE(flakes[0].phase, 0.0f);
    EXPECT_LT(flakes[0].phase, 6.2832f);
}

TEST(IntegrateFlakes, WindDriftScalesWithDepth) {
    vp::SnowfallParams params;
    params.wobbleSpeed = {0.0f, 0.0f};
    std::array flakes = {flakeWithDepth(0.0f), flakeWithDepth(1.0f)};
    for (int tick = 0; tick < 600; ++tick) {
        vp::integrateFlakes(flakes, vp::kTickSeconds, 30.0f, params);
    }
    EXPECT_NEAR(flakes[0].vx, 30.0f * params.windFactor.back, 0.01f);
    EXPECT_NEAR(flakes[1].vx, 30.0f * params.windFactor.front, 0.01f);
    EXPECT_GT(flakes[1].x, flakes[0].x);
}

// --- Snowfall: spawning and removal ---------------------------------------------------------

TEST(Snowfall, SpawnsOnlyJustAboveTheTopEdges) {
    vp::SnowfallParams params;
    params.spawnRatePer1000Px = 6000.0f; // about 450 flakes in one tick
    const std::array monitors = {vp::RectI{0, 0, 2560, 1440}, vp::RectI{2560, 300, 4480, 1380}};
    vp::Snowfall snowfall(params, 1u);
    snowfall.setMonitors(monitors);
    snowfall.tick();
    ASSERT_GT(snowfall.flakes().size(), 400u);

    for (const vp::Flake& flake : snowfall.flakes()) {
        const float extent = vp::flakeExtent(flake.radius);
        // At most one tick of fall into the screen, and within a column (wind and wobble may
        // have moved the flake by less than a pixel).
        bool insideAColumn = false;
        for (const vp::SnowColumn& column : snowfall.columns()) {
            const bool horizontally = flake.x >= column.left - 1.0f && flake.x <= column.right + 1.0f;
            const float entered = flake.y + extent - column.top;
            if (horizontally && entered > -0.01f && entered <= flake.vy * vp::kTickSeconds + 0.01f) {
                insideAColumn = true;
            }
        }
        ASSERT_TRUE(insideAColumn) << "x=" << flake.x << " y=" << flake.y;
    }
}

TEST(Snowfall, RemovesFlakesBelowTheBottomEdge) {
    const vp::SnowfallParams params;
    const std::array monitors = {vp::RectI{0, 0, 1000, 500}};
    vp::Snowfall snowfall(params, 2u);
    snowfall.setMonitors(monitors);
    for (int tick = 0; tick < 1800; ++tick) {
        snowfall.tick();
        for (const vp::Flake& flake : snowfall.flakes()) {
            ASSERT_LE(flake.y - vp::flakeExtent(flake.radius), 500.0f);
        }
    }
    EXPECT_GT(snowfall.spawnedCount(), snowfall.flakes().size() + 100);
}

TEST(Snowfall, SpawnRateFollowsTheCoveredWidth) {
    const vp::SnowfallParams params;
    vp::Snowfall snowfall(params, 3u);
    snowfall.setMonitors(kSideBySide);
    for (int tick = 0; tick < 60 * vp::kSimulationRate; ++tick) {
        snowfall.tick();
    }
    const double expected = params.spawnRatePer1000Px * 5120.0 / 1000.0 * 60.0; // 2457.6
    EXPECT_NEAR(static_cast<double>(snowfall.spawnedCount()), expected, 2.0);
}

TEST(Snowfall, FullPoolSpawnsNothingMore) {
    vp::SnowfallParams params;
    params.capacity = 50;
    params.spawnRatePer1000Px = 100'000.0f;
    vp::Snowfall snowfall(params, 4u);
    snowfall.setMonitors(kSideBySide);
    for (int tick = 0; tick < 10; ++tick) {
        snowfall.tick();
        ASSERT_LE(snowfall.flakes().size(), 50u);
    }
    EXPECT_EQ(snowfall.flakes().size(), 50u);
    EXPECT_EQ(snowfall.spawnedCount(), 50u);
}

TEST(Snowfall, DepthLayersFollowTheirWeightsAndRanges) {
    vp::SnowfallParams params;
    params.capacity = 30'000;
    params.spawnRatePer1000Px = 20'000.0f * vp::kSimulationRate; // 20000 flakes in one tick
    const std::array monitors = {vp::RectI{0, 0, 1000, 1000}};
    vp::Snowfall snowfall(params, 5u);
    snowfall.setMonitors(monitors);
    snowfall.tick();
    const auto flakes = snowfall.flakes();
    ASSERT_NEAR(static_cast<double>(flakes.size()), 20'000.0, 1.0);

    std::array<int, vp::kDepthLayerCount> counts{};
    std::array<double, vp::kDepthLayerCount> radius{};
    std::array<double, vp::kDepthLayerCount> alpha{};
    std::array<double, vp::kDepthLayerCount> speed{};
    for (const vp::Flake& flake : flakes) {
        ASSERT_GE(flake.depth, 0.0f);
        ASSERT_LT(flake.depth, 1.0f);
        ASSERT_GE(flake.radius, params.radius.back * (1.0f - params.radiusJitter) - 1e-4f);
        ASSERT_LE(flake.radius, params.radius.front * (1.0f + params.radiusJitter) + 1e-4f);
        ASSERT_GE(flake.alpha, params.alpha.back * (1.0f - params.alphaJitter) - 1e-4f);
        ASSERT_LE(flake.alpha, params.alpha.front + 1e-4f);
        ASSERT_GE(flake.vy, params.fallSpeed.back - 1e-3f);
        ASSERT_LE(flake.vy, params.fallSpeed.front + 1e-3f);
        const auto layer = static_cast<size_t>(flake.depth * vp::kDepthLayerCount);
        ++counts[layer];
        radius[layer] += flake.radius;
        alpha[layer] += flake.alpha;
        speed[layer] += flake.vy;
    }
    for (size_t layer = 0; layer < vp::kDepthLayerCount; ++layer) {
        EXPECT_NEAR(static_cast<double>(counts[layer]) / static_cast<double>(flakes.size()), params.layerWeights[layer],
                    0.02)
            << "layer " << layer;
        radius[layer] /= counts[layer];
        alpha[layer] /= counts[layer];
        speed[layer] /= counts[layer];
    }
    // Front flakes are larger, more opaque and faster than the ones behind them.
    EXPECT_LT(radius[0], radius[1]);
    EXPECT_LT(radius[1], radius[2]);
    EXPECT_LT(alpha[0], alpha[1]);
    EXPECT_LT(alpha[1], alpha[2]);
    EXPECT_LT(speed[0], speed[1]);
    EXPECT_LT(speed[1], speed[2]);
}

TEST(Snowfall, PrewarmSpreadsFlakesOverTheWholeHeight) {
    const vp::SnowfallParams params;
    vp::Snowfall snowfall(params, 6u);
    snowfall.setMonitors(kSideBySide);
    const uint32_t ticks = snowfall.prewarm();
    EXPECT_EQ(ticks, (36u + 1u) * vp::kSimulationRate); // 1440 px at 40 px/s, plus one second
    EXPECT_EQ(snowfall.tickCount(), ticks);

    const size_t population = snowfall.flakes().size();
    ASSERT_GT(population, 500u);
    std::array<int, 4> quarters{};
    for (const vp::Flake& flake : snowfall.flakes()) {
        const int quarter = std::clamp(static_cast<int>(flake.y / 360.0f), 0, 3);
        ++quarters[quarter];
    }
    for (const int count : quarters) {
        EXPECT_GT(static_cast<double>(count) / static_cast<double>(population), 0.18);
    }

    // Steady state: the population hardly changes afterwards.
    for (int tick = 0; tick < 10 * vp::kSimulationRate; ++tick) {
        snowfall.tick();
    }
    EXPECT_NEAR(static_cast<double>(snowfall.flakes().size()), static_cast<double>(population), population * 0.1);
}

TEST(Snowfall, WrapsSidewaysSoThatSteadyWindKeepsTheDensityEven) {
    vp::SnowfallParams params;
    params.spawnRatePer1000Px = 40.0f; // enough flakes per quarter for a stable comparison
    params.wind.baseSpeed = 200.0f;
    params.wind.amplitude = 0.0f;
    const std::array monitors = {vp::RectI{0, 0, 2000, 1000}};
    vp::Snowfall snowfall(params, 7u);
    snowfall.setMonitors(monitors);
    snowfall.prewarm();
    for (int tick = 0; tick < 600; ++tick) {
        snowfall.tick();
    }

    int leftQuarter = 0;
    int rightQuarter = 0;
    for (const vp::Flake& flake : snowfall.flakes()) {
        const float extent = vp::flakeExtent(flake.radius);
        ASSERT_GE(flake.x, -extent);
        ASSERT_LE(flake.x, 2000.0f + extent);
        leftQuarter += flake.x < 500.0f ? 1 : 0;
        rightQuarter += flake.x >= 1500.0f ? 1 : 0;
    }
    ASSERT_GT(rightQuarter, 50);
    EXPECT_GT(static_cast<double>(leftQuarter) / rightQuarter, 0.75);
}

TEST(Snowfall, SetMonitorsRemovesAllFlakes) {
    const vp::SnowfallParams params;
    vp::Snowfall snowfall(params, 8u);
    snowfall.setMonitors(kSideBySide);
    snowfall.prewarm();
    ASSERT_FALSE(snowfall.flakes().empty());
    const std::array monitors = {vp::RectI{0, 0, 1920, 1080}};
    snowfall.setMonitors(monitors);
    EXPECT_TRUE(snowfall.flakes().empty());
    EXPECT_EQ(snowfall.columns().size(), 1u);
}

TEST(Snowfall, WithoutMonitorsNothingHappens) {
    const vp::SnowfallParams params;
    vp::Snowfall snowfall(params, 9u);
    snowfall.setMonitors({});
    EXPECT_EQ(snowfall.prewarm(), 0u);
    snowfall.tick();
    EXPECT_TRUE(snowfall.flakes().empty());
    EXPECT_EQ(snowfall.tickCount(), 1u);
}

// --- Determinism ----------------------------------------------------------------------------

TEST(Snowfall, SameSeedGivesBitIdenticalFlakes) {
    const vp::SnowfallParams params;
    vp::Snowfall a(params, 12345u);
    vp::Snowfall b(params, 12345u);
    a.setMonitors(kSideBySide);
    b.setMonitors(kSideBySide);
    for (int tick = 0; tick < 600; ++tick) {
        a.tick();
        b.tick();
    }
    ASSERT_FALSE(a.flakes().empty());
    EXPECT_TRUE(sameFlakes(a.flakes(), b.flakes()));
    EXPECT_EQ(a.spawnedCount(), b.spawnedCount());
}

TEST(Snowfall, PrewarmIsDeterministic) {
    const vp::SnowfallParams params;
    vp::Snowfall a(params, 777u);
    vp::Snowfall b(params, 777u);
    a.setMonitors(kSideBySide);
    b.setMonitors(kSideBySide);
    EXPECT_EQ(a.prewarm(), b.prewarm());
    EXPECT_TRUE(sameFlakes(a.flakes(), b.flakes()));
}

TEST(Snowfall, DifferentSeedsGiveDifferentFlakes) {
    const vp::SnowfallParams params;
    vp::Snowfall a(params, 1u);
    vp::Snowfall b(params, 2u);
    a.setMonitors(kSideBySide);
    b.setMonitors(kSideBySide);
    for (int tick = 0; tick < 600; ++tick) {
        a.tick();
        b.tick();
    }
    EXPECT_FALSE(sameFlakes(a.flakes(), b.flakes()));
}
