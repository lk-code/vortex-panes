#include "localization.h"

#include "core/string_block.h"
#include "core/ui_language.h"
#include "debug_log.h"

#include <vector>

namespace vp {

namespace {

constexpr LANGID kFallbackLangId = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);

LANGID g_uiLangId = kFallbackLangId;

std::vector<std::wstring> userPreferredUiLanguages() {
    std::vector<std::wstring> result;
    ULONG count = 0;
    ULONG size = 0;
    if (!::GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &count, nullptr, &size) || size == 0) {
        return result;
    }
    std::wstring buffer(size, L'\0');
    if (!::GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &count, buffer.data(), &size)) {
        return result;
    }
    // Double-null-terminated list of names.
    for (const wchar_t* p = buffer.c_str(); *p != L'\0'; p += wcslen(p) + 1) {
        result.emplace_back(p);
    }
    return result;
}

LANGID langIdFromTag(std::wstring_view tag) {
    const LCID lcid = ::LocaleNameToLCID(std::wstring(tag).c_str(), 0);
    return lcid != 0 ? LANGIDFROMLCID(lcid) : kFallbackLangId;
}

std::wstring_view findString(UINT id, LANGID langId) {
    const HMODULE module = ::GetModuleHandleW(nullptr);
    const HRSRC resource =
        ::FindResourceExW(module, RT_STRING, MAKEINTRESOURCEW(stringBlockId(id)), static_cast<WORD>(langId));
    if (!resource) {
        return {};
    }
    const HGLOBAL loaded = ::LoadResource(module, resource);
    const auto* data = loaded ? static_cast<const wchar_t*>(::LockResource(loaded)) : nullptr;
    if (!data) {
        return {};
    }
    const std::wstring_view block(data, ::SizeofResource(module, resource) / sizeof(wchar_t));
    return stringFromBlock(block, stringBlockIndex(id)).value_or(std::wstring_view{});
}

} // namespace

std::wstring_view initializeUiLanguage(std::wstring_view overrideTag) {
    std::wstring_view tag;
    if (!overrideTag.empty()) {
        tag = matchUiLanguage(overrideTag).value_or(kFallbackUiLanguage);
    } else {
        const std::vector<std::wstring> preferred = userPreferredUiLanguages();
        std::vector<std::wstring_view> views(preferred.begin(), preferred.end());
        tag = resolveUiLanguage(views);
    }
    g_uiLangId = langIdFromTag(tag);

    // Thread UI languages: the chosen language, then English. Affects system-provided UI such as
    // message box buttons; our own strings are loaded explicitly by language in loadString().
    std::wstring list(tag);
    list.push_back(L'\0');
    if (tag != kFallbackUiLanguage) {
        list.append(kFallbackUiLanguage);
        list.push_back(L'\0');
    }
    ULONG count = 0;
    ::SetThreadPreferredUILanguages(MUI_LANGUAGE_NAME, list.c_str(), &count);
    if (!overrideTag.empty()) {
        ::SetThreadUILanguage(g_uiLangId);
    }

    VP_DEBUG_LOG(L"UI language %.*s (LANGID 0x%04X)", static_cast<int>(tag.size()), tag.data(),
                 static_cast<unsigned>(g_uiLangId));
    return tag;
}

std::wstring loadString(UINT id) {
    std::wstring_view text = findString(id, g_uiLangId);
    if (text.empty() && g_uiLangId != kFallbackLangId) {
        text = findString(id, kFallbackLangId);
    }
    return std::wstring(text);
}

} // namespace vp
