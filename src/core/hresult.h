#pragma once

#include <cstdint>
#include <stdexcept>

namespace vp {

// HRESULT is a 32-bit signed value on every Windows architecture.
constexpr bool hresultFailed(int32_t hr) noexcept {
    return hr < 0;
}

// Error raised by THROW_IF_FAILED. Only used on initialization paths, never per frame.
class HResultError : public std::runtime_error {
public:
    HResultError(int32_t hr, const char* expression);

    int32_t code() const noexcept { return m_code; }

private:
    int32_t m_code;
};

// Throws HResultError if hr indicates failure.
void throwIfFailed(int32_t hr, const char* expression);

} // namespace vp

#define THROW_IF_FAILED(expr) ::vp::throwIfFailed(static_cast<int32_t>(expr), #expr)
