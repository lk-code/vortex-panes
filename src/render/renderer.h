#pragma once

#include "core/geometry.h"
#include "render/graphics_device.h"
#include "render/monitor_overlay.h"

#include <memory>
#include <span>
#include <vector>

namespace vp {

enum class FrameResult {
    Ok,
    DeviceLost, // DXGI_ERROR_DEVICE_REMOVED / RESET; recovery follows in M9
};

// Owns the graphics device, one overlay per monitor and the shape pipeline.
class Renderer {
public:
    // Creates the device and an overlay for every monitor; throws on failure.
    Renderer(HINSTANCE instance, std::span<const RectI> monitors, const GraphicsDeviceOptions& options);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    // Draws the M1 test scene on every overlay and presents. No heap allocations.
    FrameResult renderFrame(double timeSeconds);

    // Shows all overlays (after the first frame has been presented).
    void showOverlays() noexcept;
    void refreshTopmost() noexcept;

    size_t overlayCount() const noexcept { return m_overlays.size(); }
    const GraphicsDevice& graphics() const noexcept { return m_graphics; }

private:
    void createPipeline();

    GraphicsDevice m_graphics;
    std::vector<std::unique_ptr<MonitorOverlay>> m_overlays;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_instanceBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_constantBuffer;
    Microsoft::WRL::ComPtr<ID3D11BlendState> m_blendState;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rasterizerState;
};

} // namespace vp
