#include "core/flake.h"

namespace vp {

FlakePool::FlakePool(size_t capacity) : m_capacity(capacity) {
    m_flakes.reserve(capacity);
}

bool FlakePool::tryAdd(const Flake& flake) noexcept {
    if (full()) {
        return false;
    }
    m_flakes.push_back(flake); // within the reserved capacity: no allocation
    return true;
}

void FlakePool::removeAt(size_t index) noexcept {
    if (index >= m_flakes.size()) {
        return;
    }
    if (index + 1 != m_flakes.size()) {
        m_flakes[index] = m_flakes.back();
    }
    m_flakes.pop_back();
}

} // namespace vp
