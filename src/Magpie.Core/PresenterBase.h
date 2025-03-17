#pragma once

namespace Magpie {

class DeviceResources;

class PresenterBase {
public:
	virtual ~PresenterBase() noexcept {}

	bool Initialize(HWND hwndAttach, const DeviceResources& deviceResources) noexcept;

	virtual winrt::com_ptr<ID3D11RenderTargetView> BeginFrame(POINT& updateOffset) noexcept = 0;

	virtual void EndFrame() noexcept = 0;

	virtual bool Resize() noexcept = 0;

protected:
	virtual bool _Initialize(HWND hwndAttach) noexcept = 0;

	void _WaitForDwmAfterResize() noexcept;

	const DeviceResources* _deviceResources = nullptr;

private:
	winrt::com_ptr<ID3D11Fence> _fence;
	uint64_t _fenceValue = 0;
	wil::unique_event_nothrow _fenceEvent;
};

}
