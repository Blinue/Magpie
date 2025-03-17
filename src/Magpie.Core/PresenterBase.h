#pragma once

namespace Magpie {

class DeviceResources;

class PresenterBase {
public:
	virtual ~PresenterBase() noexcept {}
	virtual bool Initialize(HWND hwndAttach, const DeviceResources& deviceResources) noexcept = 0;
	virtual winrt::com_ptr<ID3D11RenderTargetView> BeginFrame(POINT& updateOffset) noexcept = 0;
	virtual void EndFrame() noexcept = 0;
	virtual bool Resize() noexcept = 0;

protected:
	static void _WaitForDwmComposition() noexcept;
};

}
