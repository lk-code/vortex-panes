#pragma once

#include "core/flake.h"
#include "core/geometry.h"
#include "render/graphics_device.h"
#include "render/monitor_overlay.h"

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace vp {

enum class FrameResult {
    Ok,
    DeviceLost, // DXGI_ERROR_DEVICE_REMOVED / RESET; recovery follows in M9
};

// Owns the graphics device, one overlay per monitor and the flake pipeline.
class Renderer {
public:
    // Creates the device and an overlay for every monitor; throws on failure. `maxFlakes` sizes
    // the instance buffer (the capacity of the flake pool).
    Renderer(HINSTANCE instance, std::span<const RectI> monitors, const GraphicsDeviceOptions& options,
             uint32_t maxFlakes);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    // Draws the flakes on every overlay (one instanced draw call per monitor, only the flakes
    // within it) and presents. `opacity` scales everything drawn. No heap allocations.
    FrameResult renderFrame(std::span<const Flake> flakes, float opacity);

    // Shows all overlays (after the first frame has been presented).
    void showOverlays() noexcept;
    void refreshTopmost() noexcept;

    size_t overlayCount() const noexcept { return m_overlays.size(); }
    const GraphicsDevice& graphics() const noexcept { return m_graphics; }

private:
    void createPipeline();

    GraphicsDevice m_graphics;
    std::vector<std::unique_ptr<MonitorOverlay>> m_overlays;
    uint32_t m_instanceCapacity;
    Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_instanceBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_constantBuffer;
    Microsoft::WRL::ComPtr<ID3D11BlendState> m_blendState;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rasterizerState;
};

} // namespace vp
