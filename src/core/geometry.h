#pragma once

#include <cstdint>

namespace vp {

// Axis-aligned rectangle in physical pixels. right and bottom are exclusive.
struct RectI {
    int32_t left = 0;
    int32_t top = 0;
    int32_t right = 0;
    int32_t bottom = 0;

    constexpr int32_t width() const noexcept { return right - left; }
    constexpr int32_t height() const noexcept { return bottom - top; }
    constexpr bool empty() const noexcept { return right <= left || bottom <= top; }

    friend constexpr bool operator==(const RectI&, const RectI&) noexcept = default;
};

} // namespace vp
