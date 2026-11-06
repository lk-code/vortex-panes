#include "core/flake_instances.h"

namespace vp {

size_t collectVisibleFlakes(std::span<const Flake> flakes, const RectI& bounds, std::span<FlakeInstance> out) noexcept {
    const auto left = static_cast<float>(bounds.left);
    const auto top = static_cast<float>(bounds.top);
    const auto right = static_cast<float>(bounds.right);
    const auto bottom = static_cast<float>(bounds.bottom);
    const size_t capacity = out.size();
    FlakeInstance* const target = out.data();

    size_t count = 0;
    for (const Flake& flake : flakes) {
        if (count == capacity) {
            break;
        }
        const float extent = flakeExtent(flake.radius);
        if (flake.x + extent <= left || flake.x - extent >= right || flake.y + extent <= top ||
            flake.y - extent >= bottom) {
            continue;
        }
        target[count++] = FlakeInstance{flake.x, flake.y, flake.radius, flake.alpha};
    }
    return count;
}

} // namespace vp
