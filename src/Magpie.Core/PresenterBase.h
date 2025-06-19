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

	virtual void EndFrame(bool waitForRenderComplete = false) noexcept = 0;

	virtual bool OnResize() noexcept = 0;

	virtual void OnEndResize(bool& shouldRedraw) noexcept {
		shouldRedraw = false;
	}

protected:
	virtual bool _Initialize(HWND hwndAttach) noexcept = 0;

	void _WaitForRenderComplete() noexcept;

	// 和 DwmFlush 效果相同但更准确
	static void _WaitForDwmComposition() noexcept;

	static uint32_t _CalcBufferCount() noexcept;

	const DeviceResources* _deviceResources = nullptr;

private:
	winrt::com_ptr<ID3D11Fence> _fence;
	uint64_t _fenceValue = 0;
	wil::unique_event_nothrow _fenceEvent;
};

}
