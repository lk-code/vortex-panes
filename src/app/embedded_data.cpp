#include "embedded_data.h"

namespace vp {

std::string_view embeddedData(UINT id) noexcept {
    const HMODULE module = ::GetModuleHandleW(nullptr);
    const HRSRC resource = ::FindResourceW(module, MAKEINTRESOURCEW(id), RT_RCDATA);
    if (!resource) {
        return {};
    }
    const HGLOBAL loaded = ::LoadResource(module, resource);
    const void* data = loaded ? ::LockResource(loaded) : nullptr;
    if (!data) {
        return {};
    }
    return {static_cast<const char*>(data), ::SizeofResource(module, resource)};
}

} // namespace vp
