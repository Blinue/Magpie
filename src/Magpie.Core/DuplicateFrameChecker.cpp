#include "pch.h"
#include "DuplicateFrameChecker.h"
#include "DebugInfo.h"
#include "DirectXHelper.h"
#include "Logger.h"
#include "ScalingWindow.h"
#include "shaders/DuplicateFrameCS.h"
#include "shaders/DuplicateFrameCS_NoBoundsChecking.h"

namespace Magpie {

static constexpr uint16_t INITIAL_CHECK_COUNT = 16;
static constexpr uint16_t INITIAL_SKIP_COUNT = 1;
static constexpr uint16_t MAX_SKIP_COUNT = 16;

DuplicateFrameChecker::DuplicateFrameChecker() noexcept :
	_nextSkipCount(INITIAL_SKIP_COUNT), _framesLeft(INITIAL_CHECK_COUNT) {}

// 使用 D3D11 而不是 D3D12 检查重复帧。有两个原因：
// 1. D3D11 支持 IDXGIDevice::SetGPUThreadPriority，可以提高 GPU 优先级，
// 而 D3D12 没有等价接口。
// 2. 对于小任务 D3D11 启动渲染的耗时比 D3D12 短，差距可以达到 50us 以上。
// 
// 对于不支持脏矩形且捕获帧右下两边没有多余像素的捕获方式，可以禁用边界检查获得
// 性能提升。
bool DuplicateFrameChecker::Initialize(
	ID3D11Device5* d3d11Device,
	ID3D11DeviceContext4* d3d11DC,
	const ColorInfo& colorInfo,
	SizeU frameSize,
	uint32_t captureFrameCount,
	bool disableBoundsChecking
) noexcept {
	assert(ScalingWindow::Get().Options().duplicateFrameDetectionMode !=
		DuplicateFrameDetectionMode::Never);

	_device = d3d11Device;
	_deviceContext = d3d11DC;
	_isScRGB = colorInfo.kind != winrt::AdvancedColorKind::StandardDynamicRange;
	_frameSize = frameSize;
#ifdef _DEBUG
	_isBoundsCheckingDisabled = disableBoundsChecking;
#endif

	_frameSrvs.resize(captureFrameCount);

	HRESULT hr = d3d11Device->CreateComputeShader(
		disableBoundsChecking ? DuplicateFrameCS_NoBoundsChecking : DuplicateFrameCS,
		disableBoundsChecking ? sizeof(DuplicateFrameCS_NoBoundsChecking) : sizeof(DuplicateFrameCS),
		nullptr,
		_dupFrameCS.put()
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateComputeShader 失败", hr);
		return false;
	}

	{
		D3D11_BUFFER_DESC desc = {
			// CSSetConstantBuffers1 要求偏移量以 256 字节对齐
			.ByteWidth = (MAX_CAPTURE_DIRTY_RECT_COUNT - 1) * 256 + 8 * sizeof(uint32_t),
			.Usage = D3D11_USAGE_DYNAMIC,
			.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
			.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
			.StructureByteStride = desc.ByteWidth
		};
		hr = d3d11Device->CreateBuffer(&desc, nullptr, _constantBuffer.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateBuffer 失败", hr);
			return false;
		}

		desc.ByteWidth = MAX_CAPTURE_DIRTY_RECT_COUNT * sizeof(uint32_t);
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
		desc.StructureByteStride = desc.ByteWidth;
		hr = d3d11Device->CreateBuffer(&desc, nullptr, _resultBuffer.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateBuffer 失败", hr);
			return false;
		}

		desc.Usage = D3D11_USAGE_STAGING;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		desc.BindFlags = 0;
		hr = d3d11Device->CreateBuffer(&desc, nullptr, _readBackBuffer.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateBuffer 失败", hr);
			return false;
		}
	}

	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC desc = {
			.Format = DXGI_FORMAT_R32_UINT,
			.ViewDimension = D3D11_UAV_DIMENSION_BUFFER,
			.Buffer = {
				.NumElements = MAX_CAPTURE_DIRTY_RECT_COUNT
			}
		};
		hr = d3d11Device->CreateUnorderedAccessView(_resultBuffer.get(), &desc, _resultBufferUav.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateUnorderedAccessView 失败", hr);
			return false;
		}
	}

	{
		D3D11_SAMPLER_DESC desc{
			.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT,
			.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP,
			.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP,
			.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP,
			.ComparisonFunc = D3D11_COMPARISON_NEVER
		};
		hr = d3d11Device->CreateSamplerState(&desc, _sampler.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateSamplerState 失败", hr);
			return false;
		}
	}

	_deviceContext->CSSetShader(_dupFrameCS.get(), nullptr, 0);

	{
		ID3D11UnorderedAccessView* uav = _resultBufferUav.get();
		_deviceContext->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);
	}

	{
		ID3D11SamplerState* t = _sampler.get();
		_deviceContext->CSSetSamplers(0, 1, &t);
	}

	return true;
}

HRESULT DuplicateFrameChecker::CheckFrame(
	ID3D11Texture2D* frameResource,
	uint32_t frameIdx,
	SmallVectorImpl<RectU>& dirtyRects
) noexcept {
	assert(!dirtyRects.empty() && dirtyRects.size() <= MAX_CAPTURE_DIRTY_RECT_COUNT);

#ifdef _DEBUG
	{
		D3D11_TEXTURE2D_DESC desc;
		frameResource->GetDesc(&desc);
		assert(desc.Width == _frameSize.width && desc.Height == _frameSize.height);

		if (_isBoundsCheckingDisabled) {
			// 确保捕获帧右下两边没有多余像素
			for (const RectU& rect : dirtyRects) {
				assert(rect.right == desc.Width && rect.bottom == desc.Height);
			}
		}
	}
#endif

	if (!_frameSrvs[frameIdx]) {
		HRESULT hr = _device->CreateShaderResourceView(frameResource, nullptr, _frameSrvs[frameIdx].put());
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateShaderResourceView 失败", hr);
			return hr;
		}
	}

	// 第一帧无需检查重复帧
	if (_oldFrameIdx == std::numeric_limits<uint32_t>::max()) {
		return S_OK;
	}

	if (ScalingWindow::Get().Options().duplicateFrameDetectionMode == DuplicateFrameDetectionMode::Always) {
		HRESULT hr = _CheckDirtyRects(frameIdx, dirtyRects);
		if (FAILED(hr)) {
			Logger::Get().ComError("_CheckDirtyRects 失败", hr);
			return hr;
		}

		return S_OK;
	}

	// 动态检查重复帧，见 #787
	if (_isCheckingForDuplicateFrame) {
		if (--_framesLeft == 0) {
			_isCheckingForDuplicateFrame = false;
			_framesLeft = _nextSkipCount;
			if (_nextSkipCount < MAX_SKIP_COUNT) {
				// 增加下一次连续跳过检查的帧数
				++_nextSkipCount;
			}
		}

		HRESULT hr = _CheckDirtyRects(frameIdx, dirtyRects);
		if (FAILED(hr)) {
			Logger::Get().ComError("_CheckDirtyRects 失败", hr);
			return hr;
		}

		if (dirtyRects.empty()) {
			_isCheckingForDuplicateFrame = true;
			_framesLeft = INITIAL_CHECK_COUNT;
			_nextSkipCount = INITIAL_SKIP_COUNT;
		}
	} else {
		if (--_framesLeft == 0) {
			_isCheckingForDuplicateFrame = true;
			// 第 2 次连续检查 10 帧，之后逐渐减少，从第 16 次开始只连续检查 2 帧
			_framesLeft = uint32_t((-4 * (int)_nextSkipCount + 78) / 7);
		}

#ifdef MP_DEBUG_INFO
		if (DEBUG_INFO.enableStatisticsForDynamicDuplicateFrameDetection) {
			// 预测此帧不会重复，验证是否正确
			SmallVector<RectU> tempRects(dirtyRects.begin(), dirtyRects.end());
			HRESULT hr = _CheckDirtyRects(frameIdx, tempRects);
			if (FAILED(hr)) {
				Logger::Get().ComError("_CheckDirtyRects 失败", hr);
				return hr;
			}

			auto lk = DEBUG_INFO.lock.lock_exclusive();
			++DEBUG_INFO.ddfdSkippedFrameCount;
			if (tempRects.empty()) {
				++DEBUG_INFO.ddfdWrongPredictionCount;
			}
		}
#endif
	}

	return S_OK;
}

void DuplicateFrameChecker::OnFrameAdopted(uint32_t frameIdx) noexcept {
	_oldFrameIdx = frameIdx;
}

void DuplicateFrameChecker::OnCaptureStopped() noexcept {
	_oldFrameIdx = std::numeric_limits<uint32_t>::max();
	std::fill(_frameSrvs.begin(), _frameSrvs.end(), nullptr);
}

HRESULT DuplicateFrameChecker::_CheckDirtyRects(
	uint32_t newFrameIdx,
	SmallVectorImpl<RectU>& dirtyRects
) noexcept {
	assert(dirtyRects.size() <= MAX_CAPTURE_DIRTY_RECT_COUNT);

	{
		assert(_frameSrvs[_oldFrameIdx] && _frameSrvs[newFrameIdx]);
		ID3D11ShaderResourceView* srvs[]{ _frameSrvs[_oldFrameIdx].get(), _frameSrvs[newFrameIdx].get()};
		_deviceContext->CSSetShaderResources(0, 2, srvs);
	}

	const uint32_t dirtyRectCount = (uint32_t)dirtyRects.size();

	D3D11_MAPPED_SUBRESOURCE ms;
	HRESULT hr = _deviceContext->Map(_constantBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
	if (FAILED(hr)) {
		Logger::Get().ComError("ID3D11DeviceContext::Map 失败", hr);
		return hr;
	}

	++_curTargetValue;

	for (uint32_t i = 0; i < dirtyRectCount; ++i) {
		const RectU& dirtyRect = dirtyRects[i];

		alignas(32) DirectXHelper::Constant32 constants[] = {
			{.uintVal = dirtyRect.left},
			{.uintVal = dirtyRect.top},
			{.uintVal = dirtyRect.right},
			{.uintVal = dirtyRect.bottom},
			{.floatVal = 1.0f / _frameSize.width},
			{.floatVal = 1.0f / _frameSize.height},
			{.uintVal = _curTargetValue},
			{.uintVal = i}
		};
		// CSSetConstantBuffers1 要求偏移量以 256 字节对齐
		std::memcpy((uint8_t*)ms.pData + i * 256, constants, sizeof(constants));
	}

	_deviceContext->Unmap(_constantBuffer.get(), 0);

	for (uint32_t i = 0; i < dirtyRectCount; ++i) {
		{
			ID3D11Buffer* buffer = _constantBuffer.get();
			UINT firstConstant = i * 16;
			UINT numConstants = 16;
			_deviceContext->CSSetConstantBuffers1(0, 1, &buffer, &firstConstant, &numConstants);
		}
		
		const RectU& dirtyRect = dirtyRects[i];
		_deviceContext->Dispatch(
			(dirtyRect.right - dirtyRect.left + DUP_FRAME_DISPATCH_BLOCK_SIZE - 1) / DUP_FRAME_DISPATCH_BLOCK_SIZE,
			(dirtyRect.bottom - dirtyRect.top + DUP_FRAME_DISPATCH_BLOCK_SIZE - 1) / DUP_FRAME_DISPATCH_BLOCK_SIZE,
			1
		);
	}

	{
		D3D11_BOX box = {
			.right = dirtyRectCount * 4,
			.bottom = 1,
			.back = 1
		};
		_deviceContext->CopySubresourceRegion(_readBackBuffer.get(), 0, 0, 0, 0, _resultBuffer.get(), 0, &box);
	}
	
	// 读取结果
	SmallVector<uint32_t, 4> removeList;

	hr = _deviceContext->Map(_readBackBuffer.get(), 0, D3D11_MAP_READ, 0, &ms);
	if (FAILED(hr)) {
		Logger::Get().ComError("ID3D11DeviceContext::Map 失败", hr);
		return hr;
	}
	
	for (uint32_t i = 0; i < dirtyRectCount; ++i) {
		if (((uint32_t*)ms.pData)[i] != _curTargetValue) {
			// 此矩形内画面无变化
			removeList.push_back(i);
		}
	}

	_deviceContext->Unmap(_readBackBuffer.get(), 0);

	if (!removeList.empty()) {
		// 从后向前删除
		std::sort(removeList.begin(), removeList.end(), std::greater<uint32_t>());
		for (uint32_t idx : removeList) {
			dirtyRects.erase(dirtyRects.begin() + idx);
		}
	}

	return S_OK;
}

}
