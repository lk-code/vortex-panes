#include "render/monitor_overlay.h"

#include "core/hresult.h"

#include <stdexcept>

using Microsoft::WRL::ComPtr;

namespace vp {

void MonitorOverlay::registerWindowClass(HINSTANCE instance) {
    WNDCLASSEXW existing{};
    existing.cbSize = sizeof(existing);
    if (::GetClassInfoExW(instance, kWindowClass, &existing)) {
        return;
    }
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = &MonitorOverlay::windowProc;
    wc.hInstance = instance;
    wc.lpszClassName = kWindowClass;
    if (!::RegisterClassExW(&wc)) {
        throw std::runtime_error("RegisterClassExW failed for the overlay window");
    }
}

LRESULT CALLBACK MonitorOverlay::windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_NCHITTEST:
        // Defensive only: WS_EX_LAYERED | WS_EX_TRANSPARENT already pass input to the windows below.
        return HTTRANSPARENT;
    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;
    default:
        return ::DefWindowProcW(window, message, wParam, lParam);
    }
}

MonitorOverlay::MonitorOverlay(HINSTANCE instance, const RectI& bounds, GraphicsDevice& graphics) : m_bounds(bounds) {
    registerWindowClass(instance);

    // NOREDIRECTIONBITMAP: no GDI surface, content only via DirectComposition.
    // LAYERED | TRANSPARENT: mouse input passes through to other processes' windows.
    // TOOLWINDOW: no taskbar button and no Alt+Tab entry. NOACTIVATE: never takes focus.
    constexpr DWORD kExStyle = WS_EX_NOREDIRECTIONBITMAP | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST |
                               WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
    m_window = ::CreateWindowExW(kExStyle, kWindowClass, L"", WS_POPUP, bounds.left, bounds.top, bounds.width(),
                                 bounds.height(), nullptr, nullptr, instance, nullptr);
    if (!m_window) {
        throw std::runtime_error("CreateWindowExW failed for the overlay window");
    }
    // A layered window is only displayed once its attributes have been set.
    ::SetLayeredWindowAttributes(m_window, 0, 255, LWA_ALPHA);

    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.Width = static_cast<UINT>(bounds.width());
    desc.Height = static_cast<UINT>(bounds.height());
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    desc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
    THROW_IF_FAILED(
        graphics.dxgiFactory()->CreateSwapChainForComposition(graphics.device(), &desc, nullptr, &m_swapChain));

    IDCompositionDevice* composition = graphics.composition();
    THROW_IF_FAILED(composition->CreateTargetForHwnd(m_window, TRUE, &m_target));
    THROW_IF_FAILED(composition->CreateVisual(&m_visual));
    THROW_IF_FAILED(m_visual->SetContent(m_swapChain.Get()));
    THROW_IF_FAILED(m_target->SetRoot(m_visual.Get()));
    THROW_IF_FAILED(composition->Commit());

    // With the flip model, buffer 0 is always the current back buffer in D3D11.
    ComPtr<ID3D11Texture2D> backBuffer;
    THROW_IF_FAILED(m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)));
    THROW_IF_FAILED(graphics.device()->CreateRenderTargetView(backBuffer.Get(), nullptr, &m_renderTarget));
}

MonitorOverlay::~MonitorOverlay() {
    m_renderTarget.Reset();
    m_visual.Reset();
    m_target.Reset();
    m_swapChain.Reset();
    if (m_window) {
        ::DestroyWindow(m_window);
    }
}

void MonitorOverlay::show() noexcept {
    ::ShowWindow(m_window, SW_SHOWNOACTIVATE);
}

HRESULT MonitorOverlay::present() noexcept {
    return m_swapChain->Present(0, 0);
}

void MonitorOverlay::refreshTopmost() noexcept {
    ::SetWindowPos(m_window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
}

} // namespace vp
