#pragma once

#include <algorithm>

namespace vp {

// Opacity while fading in: rises from 0 at `elapsedSeconds` = 0 to 1 at `durationSeconds`
// along a smoothstep curve (no sudden start or stop). A duration of zero or less means no fade.
constexpr float fadeInOpacity(float elapsedSeconds, float durationSeconds) noexcept {
    if (durationSeconds <= 0.0f) {
        return 1.0f;
    }
    const float t = std::clamp(elapsedSeconds / durationSeconds, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

} // namespace vp
