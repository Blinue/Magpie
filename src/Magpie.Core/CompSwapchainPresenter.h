#pragma once
#include "PresenterBase.h"
#include <dcomp.h>
#include <Presentation.h>

namespace Magpie {

class CompSwapchainPresenter final : public PresenterBase {
protected:
	bool _Initialize(HWND hwndAttach) noexcept override;

public:
	bool BeginFrame(
		winrt::com_ptr<ID3D11Texture2D>& frameTex,
		winrt::com_ptr<ID3D11RenderTargetView>& frameRtv,
		POINT& drawOffset
	) noexcept override;

	void EndFrame(bool waitForRenderComplete = false) noexcept override;

	bool OnResize() noexcept override;

private:
	winrt::com_ptr<IDCompositionDesktopDevice> _dcompDevice;
	winrt::com_ptr<IDCompositionTarget> _dcompTarget;
	winrt::com_ptr<IDCompositionVisual2> _dcompVisual;
	winrt::com_ptr<IDCompositionSurface> _dcompSurface;

	winrt::com_ptr<IPresentationManager> _presentationManager;
	winrt::com_ptr<IPresentationSurface> _presentationSurface;
	winrt::com_ptr<ID3D11Fence> _presentationFence;

	std::vector<winrt::com_ptr<IPresentationBuffer>> _presentationBuffers;
	std::vector<wil::unique_event_nothrow> _presentationBufferAvailableEvents;
	std::vector<winrt::com_ptr<ID3D11Texture2D>> _bufferTextures;
	std::vector<winrt::com_ptr<ID3D11RenderTargetView>> _bufferRtvs;

	bool _isResized = false;
};

}
