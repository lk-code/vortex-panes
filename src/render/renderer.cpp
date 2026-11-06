#include "render/renderer.h"

#include "core/flake_instances.h"
#include "core/hresult.h"
#include "render/shaders.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iterator>

using Microsoft::WRL::ComPtr;

namespace vp {

namespace {

// Matches cbuffer FrameConstants in shaders/flake_vs.hlsl (16-byte registers).
struct FrameConstants {
    float originPx[2];
    float invSizePx[2];
    float opacity;
    float padding[3];
};
static_assert(sizeof(FrameConstants) == 32);

} // namespace

Renderer::Renderer(HINSTANCE instance, std::span<const RectI> monitors, const GraphicsDeviceOptions& options,
                   uint32_t maxFlakes)
    : m_graphics(options), m_instanceCapacity(std::max<uint32_t>(1, maxFlakes)) {
    createPipeline();
    m_overlays.reserve(monitors.size());
    for (const RectI& bounds : monitors) {
        if (!bounds.empty()) {
            m_overlays.push_back(std::make_unique<MonitorOverlay>(instance, bounds, m_graphics));
        }
    }
}

Renderer::~Renderer() {
    m_graphics.context()->ClearState();
    m_overlays.clear();
}

void Renderer::createPipeline() {
    ID3D11Device* device = m_graphics.device();

    const auto vs = flakeVertexShaderBytecode();
    const auto ps = flakePixelShaderBytecode();
    THROW_IF_FAILED(device->CreateVertexShader(vs.data(), vs.size(), nullptr, &m_vertexShader));
    THROW_IF_FAILED(device->CreatePixelShader(ps.data(), ps.size(), nullptr, &m_pixelShader));

    // Per-instance data only; the quad corners come from SV_VertexID.
    const D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"FLAKE", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(FlakeInstance, x), D3D11_INPUT_PER_INSTANCE_DATA, 1},
    };
    THROW_IF_FAILED(
        device->CreateInputLayout(layout, static_cast<UINT>(std::size(layout)), vs.data(), vs.size(), &m_inputLayout));

    // Written with Map(WRITE_DISCARD) once per monitor and frame; never reallocated.
    D3D11_BUFFER_DESC instanceDesc{};
    instanceDesc.ByteWidth = static_cast<UINT>(sizeof(FlakeInstance)) * m_instanceCapacity;
    instanceDesc.Usage = D3D11_USAGE_DYNAMIC;
    instanceDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    instanceDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    THROW_IF_FAILED(device->CreateBuffer(&instanceDesc, nullptr, &m_instanceBuffer));

    D3D11_BUFFER_DESC constantDesc{};
    constantDesc.ByteWidth = sizeof(FrameConstants);
    constantDesc.Usage = D3D11_USAGE_DYNAMIC;
    constantDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constantDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    THROW_IF_FAILED(device->CreateBuffer(&constantDesc, nullptr, &m_constantBuffer));

    // Premultiplied alpha: out = src + dst * (1 - src.a), for color and alpha.
    D3D11_BLEND_DESC blend{};
    blend.RenderTarget[0].BlendEnable = TRUE;
    blend.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    blend.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blend.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    blend.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    THROW_IF_FAILED(device->CreateBlendState(&blend, &m_blendState));

    D3D11_RASTERIZER_DESC raster{};
    raster.FillMode = D3D11_FILL_SOLID;
    raster.CullMode = D3D11_CULL_NONE;
    raster.DepthClipEnable = TRUE;
    THROW_IF_FAILED(device->CreateRasterizerState(&raster, &m_rasterizerState));
}

FrameResult Renderer::renderFrame(std::span<const Flake> flakes, float opacity) {
    ID3D11DeviceContext* context = m_graphics.context();

    // Pipeline state shared by all overlays.
    constexpr UINT stride = sizeof(FlakeInstance);
    constexpr UINT offset = 0;
    ID3D11Buffer* const instanceBuffer = m_instanceBuffer.Get();
    ID3D11Buffer* const constantBuffer = m_constantBuffer.Get();
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    context->IASetInputLayout(m_inputLayout.Get());
    context->IASetVertexBuffers(0, 1, &instanceBuffer, &stride, &offset);
    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    context->VSSetConstantBuffers(0, 1, &constantBuffer);
    context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    context->RSSetState(m_rasterizerState.Get());
    context->OMSetBlendState(m_blendState.Get(), nullptr, 0xFFFFFFFF);

    constexpr float kTransparent[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    for (const auto& overlay : m_overlays) {
        const RectI& bounds = overlay->bounds();

        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (SUCCEEDED(context->Map(constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
            const FrameConstants constants{
                {static_cast<float>(bounds.left), static_cast<float>(bounds.top)},
                {1.0f / static_cast<float>(bounds.width()), 1.0f / static_cast<float>(bounds.height())},
                opacity,
                {},
            };
            std::memcpy(mapped.pData, &constants, sizeof(constants));
            context->Unmap(constantBuffer, 0);
        }

        // Only the flakes within this monitor, written straight into the mapped buffer.
        size_t instanceCount = 0;
        if (SUCCEEDED(context->Map(instanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
            const std::span<FlakeInstance> target(static_cast<FlakeInstance*>(mapped.pData), m_instanceCapacity);
            instanceCount = collectVisibleFlakes(flakes, bounds, target);
            context->Unmap(instanceBuffer, 0);
        }

        ID3D11RenderTargetView* const renderTarget = overlay->renderTarget();
        context->OMSetRenderTargets(1, &renderTarget, nullptr);
        const D3D11_VIEWPORT viewport{
            0.0f, 0.0f, static_cast<float>(bounds.width()), static_cast<float>(bounds.height()), 0.0f, 1.0f};
        context->RSSetViewports(1, &viewport);
        context->ClearRenderTargetView(renderTarget, kTransparent);
        if (instanceCount > 0) {
            context->DrawInstanced(4, static_cast<UINT>(instanceCount), 0, 0);
        }

        const HRESULT hr = overlay->present();
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
            return FrameResult::DeviceLost;
        }
    }
    return FrameResult::Ok;
}

void Renderer::showOverlays() noexcept {
    for (const auto& overlay : m_overlays) {
        overlay->show();
    }
}

void Renderer::refreshTopmost() noexcept {
    for (const auto& overlay : m_overlays) {
        overlay->refreshTopmost();
    }
}

} // namespace vp
