#include "render/graphics_device.h"

#include "core/hresult.h"

#include <iterator>

using Microsoft::WRL::ComPtr;

namespace vp {

namespace {

constexpr D3D_FEATURE_LEVEL kFeatureLevels[] = {
    D3D_FEATURE_LEVEL_11_1,
    D3D_FEATURE_LEVEL_11_0,
    D3D_FEATURE_LEVEL_10_1,
    D3D_FEATURE_LEVEL_10_0,
};

HRESULT createDevice(D3D_DRIVER_TYPE type, UINT flags, ComPtr<ID3D11Device>& device,
                     ComPtr<ID3D11DeviceContext>& context, D3D_FEATURE_LEVEL& level) {
    HRESULT hr = ::D3D11CreateDevice(nullptr, type, nullptr, flags, kFeatureLevels,
                                     static_cast<UINT>(std::size(kFeatureLevels)), D3D11_SDK_VERSION,
                                     device.ReleaseAndGetAddressOf(), &level, context.ReleaseAndGetAddressOf());
    if (hr == DXGI_ERROR_SDK_COMPONENT_MISSING && (flags & D3D11_CREATE_DEVICE_DEBUG) != 0) {
        // The debug layer (Graphics Tools optional feature) is not installed.
        hr = ::D3D11CreateDevice(nullptr, type, nullptr, flags & ~D3D11_CREATE_DEVICE_DEBUG, kFeatureLevels,
                                 static_cast<UINT>(std::size(kFeatureLevels)), D3D11_SDK_VERSION,
                                 device.ReleaseAndGetAddressOf(), &level, context.ReleaseAndGetAddressOf());
    }
    return hr;
}

} // namespace

GraphicsDevice::GraphicsDevice(const GraphicsDeviceOptions& options) {
    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    if (options.debugLayer) {
        flags |= D3D11_CREATE_DEVICE_DEBUG;
    }

    HRESULT hr = E_FAIL;
    if (!options.forceWarp) {
        hr = createDevice(D3D_DRIVER_TYPE_HARDWARE, flags, m_device, m_context, m_featureLevel);
        m_adapterKind = AdapterKind::Hardware;
    }
    if (FAILED(hr)) {
        // Virtual machines, remote sessions and ARM devices without a suitable driver.
        hr = createDevice(D3D_DRIVER_TYPE_WARP, flags, m_device, m_context, m_featureLevel);
        m_adapterKind = AdapterKind::Warp;
    }
    THROW_IF_FAILED(hr);

    ComPtr<IDXGIDevice> dxgiDevice;
    THROW_IF_FAILED(m_device.As(&dxgiDevice));
    ComPtr<IDXGIAdapter> adapter;
    THROW_IF_FAILED(dxgiDevice->GetAdapter(&adapter));
    THROW_IF_FAILED(adapter->GetParent(IID_PPV_ARGS(&m_factory)));
    THROW_IF_FAILED(::DCompositionCreateDevice(dxgiDevice.Get(), IID_PPV_ARGS(&m_composition)));
}

} // namespace vp
