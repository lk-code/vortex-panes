#include "core/hresult.h"

#include <cstdio>
#include <string>

namespace vp {

namespace {

std::string formatMessage(int32_t hr, const char* expression) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "0x%08X", static_cast<unsigned int>(hr));
    std::string message = expression ? expression : "<unknown>";
    message += " failed with HRESULT ";
    message += buffer;
    return message;
}

} // namespace

HResultError::HResultError(int32_t hr, const char* expression)
    : std::runtime_error(formatMessage(hr, expression)), m_code(hr) {}

void throwIfFailed(int32_t hr, const char* expression) {
    if (hresultFailed(hr)) {
        throw HResultError(hr, expression);
    }
}

} // namespace vp
