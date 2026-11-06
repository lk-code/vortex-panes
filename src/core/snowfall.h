#pragma once

#include "core/flake.h"
#include "core/geometry.h"
#include "core/pcg32.h"
#include "core/wind.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace vp {

// A property that scales linearly with the depth of a flake (0 = far back, 1 = front).
struct DepthRange {
    float back = 0.0f;
    float front = 0.0f;

    constexpr float at(float depth) const noexcept { return back + (front - back) * depth; }
};

// Depth layers: back, middle, front. A flake's depth lies in the band [layer/3, (layer+1)/3).
inline constexpr size_t kDepthLayerCount = 3;

// Parameters of the falling snow; the defaults are the "Light Snow" mode.
struct SnowfallParams {
    uint32_t capacity = 4000;        // hard upper limit of flakes; nothing spawns while full
    float spawnRatePer1000Px = 8.0f; // flakes per second per 1000 px of covered screen width
    std::array<float, kDepthLayerCount> layerWeights{0.5f, 0.3f, 0.2f}; // back, middle, front
    DepthRange radius{1.0f, 4.0f};                                      // px
    float radiusJitter = 0.2f;                                          // +- share of the radius
    DepthRange fallSpeed{40.0f, 120.0f};                                // px/s, terminal velocity
    DepthRange alpha{0.4f, 1.0f};
    float alphaJitter = 0.15f;              // up to this share more transparent
    DepthRange windFactor{0.4f, 1.0f};      // share of the wind speed (parallax)
    DepthRange wobbleSpeed{4.0f, 14.0f};    // px/s, amplitude of the sideways sway
    DepthRange wobbleFrequency{0.7f, 0.4f}; // Hz
    float responseTime = 0.5f;              // s, time constant towards fall speed and wind drift
    WindParams wind;
};

// A vertical strip of the virtual screen covered by at least one monitor. Flakes spawn just
// above `top` (the highest monitor edge in the strip) and leave the screen below `bottom`
// (the lowest monitor edge in the strip).
struct SnowColumn {
    float left = 0.0f;
    float right = 0.0f; // exclusive
    float top = 0.0f;
    float bottom = 0.0f;

    friend constexpr bool operator==(const SnowColumn&, const SnowColumn&) noexcept = default;
};

// Splits the monitors into columns sorted from left to right. Neighboring strips with equal
// edges are merged; strips covered by no monitor (gaps) are left out.
std::vector<SnowColumn> buildSnowColumns(std::span<const RectI> monitors);

// Advances the flakes by `dt` seconds: fall and wind velocities approach their targets for the
// flake's depth, the wobble phase advances and the position moves. Pure arithmetic, no
// spawning or removal.
void integrateFlakes(std::span<Flake> flakes, float dt, float wind, const SnowfallParams& params) noexcept;

// Falling snow across all monitors at the fixed simulation rate. Deterministic: equal
// parameters, seed and monitors give bit-identical flakes. Allocates only in the constructor
// and in setMonitors().
class Snowfall {
public:
    Snowfall(const SnowfallParams& params, uint64_t seed);

    // Sets the screen area (monitor rectangles in virtual-screen pixels) and removes all flakes.
    void setMonitors(std::span<const RectI> monitors);

    // One simulation step of kTickSeconds: spawn, integrate, wrap and remove.
    void tick();

    // Simulates until the slowest flakes have crossed the highest column once, so that the
    // flakes are spread evenly over the screen. Returns the number of ticks (at most 60 s).
    uint32_t prewarm();

    std::span<const Flake> flakes() const noexcept { return m_pool.flakes(); }
    std::span<const SnowColumn> columns() const noexcept { return m_columns; }
    const SnowfallParams& params() const noexcept { return m_params; }

    uint64_t tickCount() const noexcept { return m_ticks; }
    uint64_t spawnedCount() const noexcept { return m_spawned; }
    double timeSeconds() const noexcept;

private:
    void spawn(float wind);
    void spawnFlake(float wind);
    void wrapAndRemove() noexcept;
    float bottomAt(float x) const noexcept;

    SnowfallParams m_params;
    Pcg32 m_rng;
    WindField m_wind;
    FlakePool m_pool;
    std::vector<SnowColumn> m_columns;
    float m_coveredWidth = 0.0f; // sum of the column widths
    float m_left = 0.0f;         // left edge of the leftmost column
    float m_right = 0.0f;        // right edge of the rightmost column
    float m_minBottom = 0.0f;    // lowest `bottom` of all columns, for an early out
    float m_maxHeight = 0.0f;    // largest column height
    double m_spawnCarry = 0.0;   // fractional flakes carried over to the next tick
    uint64_t m_ticks = 0;
    uint64_t m_spawned = 0;
};

} // namespace vp
