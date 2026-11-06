#pragma once

#include <cstdint>

namespace vp {

// Deterministic 32-bit random number generator: PCG-XSH-RR with 64-bit state, as described by
// M. E. O'Neill, "PCG: A Family of Simple Fast Space-Efficient Statistically Good Algorithms for
// Random Number Generation" (2014). Equal seed and stream give an equal sequence on every
// architecture. Not suitable for cryptography.
class Pcg32 {
public:
    // `stream` selects one of 2^63 independent sequences for the same seed.
    constexpr explicit Pcg32(uint64_t seed, uint64_t stream = 0) noexcept : m_increment((stream << 1u) | 1u) {
        nextU32();
        m_state += seed;
        nextU32();
    }

    constexpr uint32_t nextU32() noexcept {
        const uint64_t old = m_state;
        m_state = old * kMultiplier + m_increment;
        const auto xorShifted = static_cast<uint32_t>(((old >> 18u) ^ old) >> 27u);
        const auto rotation = static_cast<uint32_t>(old >> 59u);
        return (xorShifted >> rotation) | (xorShifted << ((32u - rotation) & 31u));
    }

    // Uniform in [0, 1) with 24 bits of resolution.
    constexpr float nextFloat() noexcept { return static_cast<float>(nextU32() >> 8u) * (1.0f / 16777216.0f); }

    // Uniform in [low, high]; the upper bound may be reached through rounding.
    constexpr float range(float low, float high) noexcept { return low + (high - low) * nextFloat(); }

private:
    static constexpr uint64_t kMultiplier = 6364136223846793005ull;

    uint64_t m_state = 0;
    uint64_t m_increment;
};

} // namespace vp
