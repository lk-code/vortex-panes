#pragma once

#include <cstddef>
#include <span>
#include <vector>

namespace vp {

// A falling snowflake in virtual-screen physical pixels.
struct Flake {
    float x = 0.0f;      // center
    float y = 0.0f;      // center
    float vx = 0.0f;     // px/s, without the wobble
    float vy = 0.0f;     // px/s
    float radius = 0.0f; // px
    float phase = 0.0f;  // wobble phase in radians, [0, 2*pi)
    float depth = 0.0f;  // 0 = far back, 1 = front
    float alpha = 0.0f;  // opacity
};
static_assert(sizeof(Flake) == 8 * sizeof(float));

// Flakes in one contiguous array with a capacity fixed at construction. Adding never
// reallocates; removal swaps the last flake into the freed slot (order is not preserved).
class FlakePool {
public:
    explicit FlakePool(size_t capacity);

    // Returns false (and adds nothing) when the pool is full.
    bool tryAdd(const Flake& flake) noexcept;

    // Removes the flake at `index` by moving the last flake into its place.
    void removeAt(size_t index) noexcept;

    void clear() noexcept { m_flakes.clear(); }

    size_t size() const noexcept { return m_flakes.size(); }
    size_t capacity() const noexcept { return m_capacity; }
    bool empty() const noexcept { return m_flakes.empty(); }
    bool full() const noexcept { return m_flakes.size() >= m_capacity; }

    Flake& operator[](size_t index) noexcept { return m_flakes[index]; }
    const Flake& operator[](size_t index) const noexcept { return m_flakes[index]; }

    std::span<Flake> flakes() noexcept { return m_flakes; }
    std::span<const Flake> flakes() const noexcept { return m_flakes; }

private:
    std::vector<Flake> m_flakes;
    size_t m_capacity;
};

} // namespace vp
