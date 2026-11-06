#pragma once

#include "core/geometry.h"
#include "render/graphics_device.h"

#include <windows.h>

namespace vp {

// Click-through, topmost, layered tool window covering one monitor. Its content comes
// exclusively from a composition swap chain attached through DirectComposition.
class MonitorOverlay {
public:
    static constexpr wchar_t kWindowClass[] = L"VortexPanes.Overlay";

    // Throws HResultError or std::runtime_error on failure. The window starts hidden.
    MonitorOverlay(HINSTANCE instance, const RectI& bounds, GraphicsDevice& graphics);
    ~MonitorOverlay();

    MonitorOverlay(const MonitorOverlay&) = delete;
    MonitorOverlay& operator=(const MonitorOverlay&) = delete;

    const RectI& bounds() const noexcept { return m_bounds; }
    HWND window() const noexcept { return m_window; }
    ID3D11RenderTargetView* renderTarget() const noexcept { return m_renderTarget.Get(); }

    void show() noexcept;
    HRESULT present() noexcept;

    // Topmost can be lost to the taskbar or Start menu animations; re-assert it periodically.
    void refreshTopmost() noexcept;

private:
    static void registerWindowClass(HINSTANCE instance);
    static LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

    RectI m_bounds;
    HWND m_window = nullptr;
    Microsoft::WRL::ComPtr<IDXGISwapChain1> m_swapChain;
    Microsoft::WRL::ComPtr<IDCompositionTarget> m_target;
    Microsoft::WRL::ComPtr<IDCompositionVisual> m_visual;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTarget;
};

} // namespace vp
