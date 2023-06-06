#pragma once

namespace Magpie::Core {

struct ScalingOptions;

class DeviceResources {
public:
	DeviceResources() noexcept = default;
	DeviceResources(const DeviceResources&) = delete;
	DeviceResources(DeviceResources&&) noexcept = default;

	bool Initialize(HWND hwndScaling, const ScalingOptions& options) noexcept;

	ID3D11Device5* GetD3DDevice() const noexcept { return _d3dDevice.get(); }
	IDXGIAdapter4* GetGraphicsAdapter() const noexcept { return _graphicsAdapter.get(); }

private:
	bool _ObtainAdapterAndDevice(IDXGIFactory7* dxgiFactory, int adapterIdx) noexcept;
	bool _TryCreateD3DDevice(const winrt::com_ptr<IDXGIAdapter1>& adapter) noexcept;

	winrt::com_ptr<IDXGIAdapter4> _graphicsAdapter;
	winrt::com_ptr<ID3D11Device5> _d3dDevice;
};

}
