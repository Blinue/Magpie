#pragma once
#include "PresenterBase.h"
#include <dcomp.h>
#include <Presentation.h>

namespace Magpie {

class CompSwapchainPresenter : public PresenterBase {
protected:
	bool _Initialize(HWND hwndAttach) noexcept override;

public:
	bool BeginFrame(
		winrt::com_ptr<ID3D11Texture2D>& frameTex,
		winrt::com_ptr<ID3D11RenderTargetView>& frameRtv,
		POINT& drawOffset
	) noexcept override;

	void EndFrame() noexcept override;

	bool Resize() noexcept override;

private:
	winrt::com_ptr<IDCompositionDesktopDevice> _dcompDevice;
	winrt::com_ptr<IDCompositionTarget> _dcompTarget;
	winrt::com_ptr<IDCompositionVisual2> _dcompVisual;
	winrt::com_ptr<IDCompositionSurface> _dcompSurface;

	winrt::com_ptr<IPresentationManager> _presentationManager;
	winrt::com_ptr<IPresentationSurface> _presentationSurface;
	winrt::com_ptr<ID3D11Fence> _presentationFence;
	std::array<winrt::com_ptr<IPresentationBuffer>, 2> _presentationBuffers;
	std::array<winrt::com_ptr<ID3D11Texture2D>, 2> _bufferTextures;
	std::array<winrt::com_ptr<ID3D11RenderTargetView>, 2> _bufferRtvs;
	uint32_t _curBufferIdx = 0;

	bool _isResized = false;
};

}
