#include "settings/xaml_runtime.h"

#include "settings/settings_log.h"

namespace mux = winrt::Microsoft::UI::Xaml;

namespace vp {

namespace {

// XAML application object. Islands need one for the metadata of the built-in controls (used by
// XamlReader) and for the Fluent theme resources (XamlControlsResources). Application::Start and
// the WinUI app model are not used.
struct SettingsApplication : mux::ApplicationT<SettingsApplication, mux::Markup::IXamlMetadataProvider> {
    SettingsApplication() {
        // Raises OnLaunched, which merges the theme resources.
        m_manager = mux::Hosting::WindowsXamlManager::InitializeForCurrentThread();
    }

    void OnLaunched(mux::LaunchActivatedEventArgs const&) { mergeThemeResources(); }

    void mergeThemeResources() {
        if (!m_themeResourcesMerged) {
            Resources().MergedDictionaries().Append(mux::Controls::XamlControlsResources());
            m_themeResourcesMerged = true;
        }
    }

    mux::Markup::IXamlType GetXamlType(winrt::Windows::UI::Xaml::Interop::TypeName const& type) {
        return m_provider.GetXamlType(type);
    }
    mux::Markup::IXamlType GetXamlType(winrt::hstring const& fullName) { return m_provider.GetXamlType(fullName); }
    winrt::com_array<mux::Markup::XmlnsDefinition> GetXmlnsDefinitions() { return m_provider.GetXmlnsDefinitions(); }

    void close() {
        if (m_manager) {
            m_manager.Close();
            m_manager = nullptr;
        }
    }

private:
    mux::XamlTypeInfo::XamlControlsXamlMetaDataProvider m_provider;
    mux::Hosting::WindowsXamlManager m_manager{nullptr};
    bool m_themeResourcesMerged = false;
};

} // namespace

struct XamlRuntime::State {
    winrt::Microsoft::UI::Dispatching::DispatcherQueueController queue{nullptr};
    winrt::com_ptr<SettingsApplication> application;
};

XamlRuntime::XamlRuntime() = default;

XamlRuntime::~XamlRuntime() {
    shutdown();
}

bool XamlRuntime::ensureInitialized() noexcept {
    if (m_state) {
        return true;
    }
    if (m_failed) {
        return false;
    }
    try {
        // Islands require a single-threaded apartment on the UI thread.
        const HRESULT hr = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        winrt::check_hresult(hr);
        m_comInitialized = true;

        auto state = std::make_unique<State>();
        state->queue = winrt::Microsoft::UI::Dispatching::DispatcherQueueController::CreateOnCurrentThread();
        state->application = winrt::make_self<SettingsApplication>();
        state->application->mergeThemeResources(); // in case OnLaunched was not raised
        m_state = std::move(state);

        // Loaded by now; resolved dynamically so that the executable does not import it.
        if (const HMODULE windowing = ::GetModuleHandleW(L"Microsoft.UI.Windowing.Core.dll")) {
            m_preTranslate =
                reinterpret_cast<PreTranslateFn>(::GetProcAddress(windowing, "ContentPreTranslateMessage"));
        }
        VP_SETTINGS_LOG(L"WinUI initialized (ContentPreTranslateMessage %s)", m_preTranslate ? L"found" : L"missing");
        return true;
    } catch (const winrt::hresult_error& e) {
        VP_SETTINGS_LOG(L"WinUI initialization failed: 0x%08X %s", static_cast<unsigned>(e.code().value),
                        e.message().c_str());
        (void)e;
    } catch (...) {
        VP_SETTINGS_LOG(L"WinUI initialization failed");
    }
    m_failed = true;
    shutdown();
    return false;
}

bool XamlRuntime::preTranslateMessage(const MSG& msg) const noexcept {
    return m_preTranslate && m_preTranslate(&msg);
}

void XamlRuntime::shutdown() noexcept {
    m_preTranslate = nullptr;
    if (m_state) {
        try {
            if (m_state->application) {
                m_state->application->close();
                m_state->application = nullptr;
            }
            if (m_state->queue) {
                m_state->queue.ShutdownQueue();
            }
        } catch (...) {
            VP_SETTINGS_LOG(L"WinUI shutdown failed");
        }
        m_state.reset();
        VP_SETTINGS_LOG(L"WinUI shut down");
    }
    if (m_comInitialized) {
        ::CoUninitialize();
        m_comInitialized = false;
    }
}

} // namespace vp
