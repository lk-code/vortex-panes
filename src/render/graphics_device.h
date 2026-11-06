#pragma once

#include <d3d11.h>
#include <dcomp.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

namespace vp {

struct GraphicsDeviceOptions {
    bool forceWarp = false;  // skip the hardware adapter (testing the fallback)
    bool debugLayer = false; // D3D11 debug layer; Debug builds only, ignored if not installed
};

enum class AdapterKind {
    Hardware,
    Warp,
};

// One D3D11 device, its immediate context and one DirectComposition device for all overlays.
// The constructor throws HResultError if neither the hardware adapter nor WARP is usable.
class GraphicsDevice {
public:
    explicit GraphicsDevice(const GraphicsDeviceOptions& options);

    ID3D11Device* device() const noexcept { return m_device.Get(); }
    ID3D11DeviceContext* context() const noexcept { return m_context.Get(); }
    IDXGIFactory2* dxgiFactory() const noexcept { return m_factory.Get(); }
    IDCompositionDevice* composition() const noexcept { return m_composition.Get(); }

    AdapterKind adapterKind() const noexcept { return m_adapterKind; }
    D3D_FEATURE_LEVEL featureLevel() const noexcept { return m_featureLevel; }

private:
    Microsoft::WRL::ComPtr<ID3D11Device> m_device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;
    Microsoft::WRL::ComPtr<IDXGIFactory2> m_factory;
    Microsoft::WRL::ComPtr<IDCompositionDevice> m_composition;
    AdapterKind m_adapterKind = AdapterKind::Hardware;
    D3D_FEATURE_LEVEL m_featureLevel = D3D_FEATURE_LEVEL_10_0;
};

} // namespace vp
