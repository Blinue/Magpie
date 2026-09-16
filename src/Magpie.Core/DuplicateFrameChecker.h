#pragma once
#include "SmallVector.h"
#include <d3d11_4.h>

namespace Magpie {

class DuplicateFrameChecker {
public:
	DuplicateFrameChecker() noexcept;
	DuplicateFrameChecker(const DuplicateFrameChecker&) = delete;
	DuplicateFrameChecker(DuplicateFrameChecker&&) = delete;

	~DuplicateFrameChecker() = default;

	bool Initialize(
		ID3D11Device5* d3d11Device,
		ID3D11DeviceContext4* d3d11DC,
		const ColorInfo& colorInfo,
		SizeU frameSize,
		uint32_t captureFrameCount,
		bool disableBoundsChecking
	) noexcept;

	HRESULT CheckFrame(
		ID3D11Texture2D* frameResource,
		uint32_t frameIdx,
		SmallVectorImpl<RectU>& dirtyRects
	) noexcept;

	void OnFrameAdopted(uint32_t frameIdx) noexcept;

	void OnCaptureStopped() noexcept;

private:
	HRESULT _CheckDirtyRects(uint32_t newFrameIdx, SmallVectorImpl<RectU>& dirtyRects) noexcept;

	ID3D11Device5* _device = nullptr;
	ID3D11DeviceContext4* _deviceContext = nullptr;

	SizeU _frameSize{};

	winrt::com_ptr<ID3D11ComputeShader> _dupFrameCS;
	winrt::com_ptr<ID3D11Buffer> _constantBuffer;
	winrt::com_ptr<ID3D11Buffer> _resultBuffer;
	winrt::com_ptr<ID3D11UnorderedAccessView> _resultBufferUav;
	winrt::com_ptr<ID3D11Buffer> _readBackBuffer;
	winrt::com_ptr<ID3D11SamplerState> _sampler;
	std::vector<winrt::com_ptr<ID3D11ShaderResourceView>> _frameSrvs;

	uint32_t _oldFrameIdx = std::numeric_limits<uint32_t>::max();
	uint32_t _curTargetValue = 0;

	// 用于检查重复帧
	uint16_t _nextSkipCount;
	uint16_t _framesLeft;

	bool _isScRGB = false;
#ifdef _DEBUG
	bool _isBoundsCheckingDisabled = false;
#endif
	bool _isCheckingForDuplicateFrame = true;
};

}
