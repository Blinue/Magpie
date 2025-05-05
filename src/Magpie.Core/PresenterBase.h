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

	virtual void EndFrame() noexcept = 0;

	virtual bool Resize() noexcept = 0;

	virtual void EndResize(bool& shouldRedraw) noexcept {
		shouldRedraw = false;
	}

protected:
	virtual bool _Initialize(HWND hwndAttach) noexcept = 0;

	void _WaitForRenderComplete() noexcept;

	// 比 DwmFlush 更准确
	static void _WaitForDwmComposition() noexcept;

	const DeviceResources* _deviceResources = nullptr;

	wil::unique_event_nothrow _fenceEvent;

private:
	winrt::com_ptr<ID3D11Fence> _fence;
	uint64_t _fenceValue = 0;
};

}
