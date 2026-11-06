#include "core/snowfall.h"

#include "core/fixed_timestep.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace vp {

namespace {

constexpr float kTwoPi = 2.0f * std::numbers::pi_v<float>;

// Separate seeds for the flake generator and the wind, so that changing one does not shift
// the other.
constexpr uint64_t kFlakeStream = 1;
constexpr uint64_t kWindSalt = 0x2545F4914F6CDD1Dull;

constexpr double kMaxPrewarmSeconds = 60.0;

} // namespace

std::vector<SnowColumn> buildSnowColumns(std::span<const RectI> monitors) {
    std::vector<int32_t> edges;
    edges.reserve(monitors.size() * 2);
    for (const RectI& monitor : monitors) {
        if (!monitor.empty()) {
            edges.push_back(monitor.left);
            edges.push_back(monitor.right);
        }
    }
    std::sort(edges.begin(), edges.end());
    edges.erase(std::unique(edges.begin(), edges.end()), edges.end());

    std::vector<SnowColumn> columns;
    for (size_t i = 0; i + 1 < edges.size(); ++i) {
        const int32_t left = edges[i];
        const int32_t right = edges[i + 1];
        // Every monitor either covers the strip [left, right) completely or not at all,
        // because all monitor edges are strip boundaries.
        bool covered = false;
        int32_t top = 0;
        int32_t bottom = 0;
        for (const RectI& monitor : monitors) {
            if (monitor.empty() || monitor.left > left || monitor.right < right) {
                continue;
            }
            top = covered ? std::min(top, monitor.top) : monitor.top;
            bottom = covered ? std::max(bottom, monitor.bottom) : monitor.bottom;
            covered = true;
        }
        if (!covered) {
            continue;
        }
        const SnowColumn column{static_cast<float>(left), static_cast<float>(right), static_cast<float>(top),
                                static_cast<float>(bottom)};
        if (!columns.empty() && columns.back().right == column.left && columns.back().top == column.top &&
            columns.back().bottom == column.bottom) {
            columns.back().right = column.right;
        } else {
            columns.push_back(column);
        }
    }
    return columns;
}

void integrateFlakes(std::span<Flake> flakes, float dt, float wind, const SnowfallParams& params) noexcept {
    // Exact exponential approach for a fixed step: no overshoot for any dt.
    const float response = 1.0f - std::exp(-dt / std::max(params.responseTime, 1e-3f));
    const float phaseStep = dt * kTwoPi;
    for (Flake& flake : flakes) {
        const float depth = flake.depth;
        flake.vy += (params.fallSpeed.at(depth) - flake.vy) * response;
        flake.vx += (wind * params.windFactor.at(depth) - flake.vx) * response;
        flake.phase += phaseStep * params.wobbleFrequency.at(depth);
        flake.phase = flake.phase >= kTwoPi ? flake.phase - kTwoPi : flake.phase;
        flake.x += (flake.vx + std::sin(flake.phase) * params.wobbleSpeed.at(depth)) * dt;
        flake.y += flake.vy * dt;
    }
}

Snowfall::Snowfall(const SnowfallParams& params, uint64_t seed)
    : m_params(params), m_rng(seed, kFlakeStream), m_wind(seed ^ kWindSalt, params.wind), m_pool(params.capacity) {}

void Snowfall::setMonitors(std::span<const RectI> monitors) {
    m_columns = buildSnowColumns(monitors);
    m_pool.clear();
    m_spawnCarry = 0.0;
    m_coveredWidth = 0.0f;
    m_maxHeight = 0.0f;
    if (m_columns.empty()) {
        m_left = m_right = m_minBottom = 0.0f;
        return;
    }
    m_left = m_columns.front().left;
    m_right = m_columns.back().right;
    m_minBottom = m_columns.front().bottom;
    for (const SnowColumn& column : m_columns) {
        m_coveredWidth += column.right - column.left;
        m_minBottom = std::min(m_minBottom, column.bottom);
        m_maxHeight = std::max(m_maxHeight, column.bottom - column.top);
    }
}

double Snowfall::timeSeconds() const noexcept {
    return static_cast<double>(m_ticks) / kSimulationRate;
}

void Snowfall::tick() {
    const float wind = m_wind.sample(timeSeconds());
    spawn(wind);
    integrateFlakes(m_pool.flakes(), kTickSeconds, wind, m_params);
    wrapAndRemove();
    ++m_ticks;
}

uint32_t Snowfall::prewarm() {
    if (m_columns.empty()) {
        return 0;
    }
    const float slowest = std::max(1.0f, std::min(m_params.fallSpeed.back, m_params.fallSpeed.front));
    const double seconds = std::min(kMaxPrewarmSeconds, std::ceil(m_maxHeight / slowest) + 1.0);
    const auto ticks = static_cast<uint32_t>(seconds * kSimulationRate);
    for (uint32_t i = 0; i < ticks; ++i) {
        tick();
    }
    return ticks;
}

void Snowfall::spawn(float wind) {
    if (m_columns.empty()) {
        return;
    }
    m_spawnCarry += static_cast<double>(m_params.spawnRatePer1000Px) * m_coveredWidth / 1000.0 * kTickSeconds;
    const double whole = std::floor(m_spawnCarry);
    m_spawnCarry -= whole;
    const auto count = static_cast<uint32_t>(whole);
    for (uint32_t i = 0; i < count && !m_pool.full(); ++i) {
        spawnFlake(wind);
    }
}

void Snowfall::spawnFlake(float wind) {
    // Depth layer by weight, then a depth within the layer's band.
    float totalWeight = 0.0f;
    for (const float weight : m_params.layerWeights) {
        totalWeight += weight;
    }
    float pick = m_rng.nextFloat() * totalWeight;
    size_t layer = 0;
    while (layer + 1 < kDepthLayerCount && pick >= m_params.layerWeights[layer]) {
        pick -= m_params.layerWeights[layer];
        ++layer;
    }
    const float depth = (static_cast<float>(layer) + m_rng.nextFloat()) / static_cast<float>(kDepthLayerCount);

    Flake flake;
    flake.depth = depth;
    const float radiusVariation = m_rng.range(-1.0f, 1.0f) * m_params.radiusJitter;
    flake.radius = m_params.radius.at(depth) * (1.0f + radiusVariation);
    const float alphaVariation = m_rng.nextFloat() * m_params.alphaJitter;
    flake.alpha = m_params.alpha.at(depth) * (1.0f - alphaVariation);
    flake.vy = m_params.fallSpeed.at(depth);
    flake.vx = wind * m_params.windFactor.at(depth);
    flake.phase = m_rng.nextFloat() * kTwoPi;

    // Uniform over the covered width.
    float offset = m_rng.nextFloat() * m_coveredWidth;
    size_t index = 0;
    while (index + 1 < m_columns.size() && offset >= m_columns[index].right - m_columns[index].left) {
        offset -= m_columns[index].right - m_columns[index].left;
        ++index;
    }
    const SnowColumn& column = m_columns[index];
    flake.x = std::min(column.left + offset, column.right);

    // Just above the top edge, with a random part of one tick's fall already done, so that
    // flakes spawned in the same tick do not enter the screen as a row.
    const float advance = m_rng.nextFloat() * flake.vy * kTickSeconds;
    flake.y = column.top - flakeExtent(flake.radius) - advance;

    if (m_pool.tryAdd(flake)) {
        ++m_spawned;
    }
}

void Snowfall::wrapAndRemove() noexcept {
    const float width = m_right - m_left;
    size_t i = 0;
    while (i < m_pool.size()) {
        Flake& flake = m_pool[i];
        const float extent = flakeExtent(flake.radius);
        // Leaving the virtual screen sideways: re-enter at the opposite edge, just out of sight.
        if (flake.x < m_left - extent) {
            flake.x += width + 2.0f * extent;
        } else if (flake.x > m_right + extent) {
            flake.x -= width + 2.0f * extent;
        }
        const float topEdge = flake.y - extent;
        if (topEdge > m_minBottom && topEdge > bottomAt(flake.x)) {
            m_pool.removeAt(i);
            continue;
        }
        ++i;
    }
}

float Snowfall::bottomAt(float x) const noexcept {
    const float clamped = std::clamp(x, m_left, m_right);
    for (const SnowColumn& column : m_columns) {
        if (clamped >= column.left && clamped <= column.right) {
            return column.bottom;
        }
    }
    return m_minBottom; // in a gap between monitors: nothing is visible there
}

} // namespace vp
