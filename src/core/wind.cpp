#include "core/wind.h"

#include "core/pcg32.h"

#include <cmath>

namespace vp {

namespace {

// Odd 64-bit constant (golden ratio) that spreads consecutive key indices over the seed space.
constexpr uint64_t kKeySpread = 0x9E3779B97F4A7C15ull;
constexpr uint64_t kFastOctaveSalt = 0xD1B54A32D192ED03ull;

// Random key in [-1, 1) for the integer position `index`.
float noiseKey(uint64_t seed, int64_t index) noexcept {
    Pcg32 rng(seed ^ (static_cast<uint64_t>(index) * kKeySpread));
    return rng.nextFloat() * 2.0f - 1.0f;
}

// Value noise in [-1, 1]: keys at integer positions, smoothstep in between (continuous slope).
float valueNoise(uint64_t seed, double position) noexcept {
    const double cell = std::floor(position);
    const auto index = static_cast<int64_t>(cell);
    const auto f = static_cast<float>(position - cell);
    const float s = f * f * (3.0f - 2.0f * f);
    const float a = noiseKey(seed, index);
    const float b = noiseKey(seed, index + 1);
    return a + (b - a) * s;
}

} // namespace

WindField::WindField(uint64_t seed, const WindParams& params) noexcept
    : m_params(params), m_slowSeed(seed), m_fastSeed(seed ^ kFastOctaveSalt) {}

float WindField::sample(double timeSeconds) const noexcept {
    const float slow = m_params.slowPeriod > 0.0f ? valueNoise(m_slowSeed, timeSeconds / m_params.slowPeriod) : 0.0f;
    const float fast = m_params.fastPeriod > 0.0f ? valueNoise(m_fastSeed, timeSeconds / m_params.fastPeriod) : 0.0f;
    const float noise = (slow + m_params.fastWeight * fast) / (1.0f + m_params.fastWeight);
    return m_params.baseSpeed + m_params.amplitude * noise;
}

} // namespace vp
