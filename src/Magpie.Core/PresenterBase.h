#pragma once

namespace Magpie {

class DeviceResources;

class PresenterBase {
public:
	virtual ~PresenterBase() noexcept {}

	bool Initialize(HWND hwndAttach, const DeviceResources& deviceResources) noexcept;

	virtual bool BeginFrame(
		winrt::com_ptr<ID3D11Texture2D>& frameTex,
		winrt::com_ptr<ID3D11RenderTargetView>& frameRtv,
		POINT& drawOffset
	) noexcept = 0;

	void EndFrame() noexcept;

	bool Resize() noexcept;

protected:
	virtual bool _Initialize(HWND hwndAttach) noexcept = 0;

	virtual void _EndDraw() noexcept {}

	virtual void _Present() noexcept = 0;

	virtual bool _Resize() noexcept = 0;

	void _WaitForRenderComplete() noexcept;

	const DeviceResources* _deviceResources = nullptr;

private:
	winrt::com_ptr<ID3D11Fence> _fence;
	uint64_t _fenceValue = 0;
	wil::unique_event_nothrow _fenceEvent;

	bool _isResized = false;
};

}
