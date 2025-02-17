#include "pch.h"
#include "FrameSourceBase.h"
#include "ScalingOptions.h"
#include "Logger.h"
#include "Win32Helper.h"
#include "SmallVector.h"
#include "DirectXHelper.h"
#include "DeviceResources.h"
#include "shaders/DuplicateFrameCS.h"
#include "ScalingWindow.h"
#include "BackendDescriptorStore.h"
#include <dwmapi.h>

namespace Magpie {

static constexpr uint16_t INITIAL_CHECK_COUNT = 16;
static constexpr uint16_t INITIAL_SKIP_COUNT = 1;
static constexpr uint16_t MAX_SKIP_COUNT = 16;

FrameSourceBase::FrameSourceBase() noexcept :
	_nextSkipCount(INITIAL_SKIP_COUNT), _framesLeft(INITIAL_CHECK_COUNT) {}

FrameSourceBase::~FrameSourceBase() noexcept {
	const HWND hwndSrc = ScalingWindow::Get().SrcInfo().Handle();

	// 还原窗口圆角
	if (_roundCornerDisabled) {
		_roundCornerDisabled = false;

		INT attr = DWMWCP_DEFAULT;
		HRESULT hr = DwmSetWindowAttribute(
			hwndSrc, DWMWA_WINDOW_CORNER_PREFERENCE, &attr, sizeof(attr));
		if (FAILED(hr)) {
			Logger::Get().ComError("取消禁用窗口圆角失败", hr);
		} else {
			Logger::Get().Info("已取消禁用窗口圆角");
		}
	}

	// 还原窗口大小调整
	if (_windowResizingDisabled) {
		LONG_PTR style = GetWindowLongPtr(hwndSrc, GWL_STYLE);
		if (!(style & WS_THICKFRAME)) {
			if (SetWindowLongPtr(hwndSrc, GWL_STYLE, style | WS_THICKFRAME)) {
				if (!SetWindowPos(hwndSrc, 0, 0, 0, 0, 0,
					SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED)) {
					Logger::Get().Win32Error("SetWindowPos 失败");
				}

				Logger::Get().Info("已取消禁用窗口大小调整");
			} else {
				Logger::Get().Win32Error("取消禁用窗口大小调整失败");
			}
		}
	}
}

bool FrameSourceBase::Initialize(DeviceResources& deviceResources, BackendDescriptorStore& descriptorStore) noexcept {
	_deviceResources = &deviceResources;
	_descriptorStore = &descriptorStore;

	const HWND hwndSrc = ScalingWindow::Get().SrcInfo().Handle();

	// 禁用窗口大小调整
	if (ScalingWindow::Get().Options().IsWindowResizingDisabled()) {
		LONG_PTR style = GetWindowLongPtr(hwndSrc, GWL_STYLE);
		if (style & WS_THICKFRAME) {
			if (SetWindowLongPtr(hwndSrc, GWL_STYLE, style ^ WS_THICKFRAME)) {
				// 不重绘边框，以防某些窗口状态不正确
				// if (!SetWindowPos(HwndSrc, 0, 0, 0, 0, 0,
				//	SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED)) {
				//	SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("SetWindowPos 失败"));
				// }

				Logger::Get().Info("已禁用窗口大小调整");
				_windowResizingDisabled = true;
			} else {
				Logger::Get().Win32Error("禁用窗口大小调整失败");
			}
		}
	}

	// 禁用窗口圆角
	if (_HasRoundCornerInWin11()) {
		if (Win32Helper::GetOSVersion().IsWin11()) {
			INT attr = DWMWCP_DONOTROUND;
			HRESULT hr = DwmSetWindowAttribute(
				hwndSrc, DWMWA_WINDOW_CORNER_PREFERENCE, &attr, sizeof(attr));
			if (FAILED(hr)) {
				Logger::Get().ComError("禁用窗口圆角失败", hr);
			} else {
				Logger::Get().Info("已禁用窗口圆角");
				_roundCornerDisabled = true;
			}
		}
	}

	if (!_Initialize()) {
		Logger::Get().Error("_Initialize 失败");
		return false;
	}

	assert(_output);
	_outputSrv = descriptorStore.GetShaderResourceView(_output.get());
	if (!_outputSrv) {
		Logger::Get().Error("GetShaderResourceView 失败");
		return false;
	}

	return true;
}

FrameSourceState FrameSourceBase::Update() noexcept {
	const FrameSourceState state = _Update();

	const ScalingOptions& options = ScalingWindow::Get().Options();
	const auto duplicateFrameDetectionMode = options.duplicateFrameDetectionMode;
	if (state != FrameSourceState::NewFrame || options.Is3DGameMode() ||
		duplicateFrameDetectionMode == DuplicateFrameDetectionMode::Never) {
		return state;
	}

	ID3D11DeviceContext4* d3dDC = _deviceResources->GetD3DDC();

	if (!_prevFrame) {
		if (_InitCheckingForDuplicateFrame()) {
			d3dDC->CopyResource(_prevFrame.get(), _output.get());
		} else {
			Logger::Get().Error("_InitCheckingForDuplicateFrame 失败");
			_prevFrame = nullptr;
			_prevFrameSrv = nullptr;
		}

		return FrameSourceState::NewFrame;
	}

	if (duplicateFrameDetectionMode == DuplicateFrameDetectionMode::Always) {
		// 总是检查重复帧
		if (_IsDuplicateFrame()) {
			return FrameSourceState::Waiting;
		} else {
			d3dDC->CopyResource(_prevFrame.get(), _output.get());
			return FrameSourceState::NewFrame;
		}
	}

	///////////////////////////////////////////////
	//
	// 动态检查重复帧，见 #787
	//
	///////////////////////////////////////////////

	const bool isStatisticsEnabled = options.IsStatisticsForDynamicDetectionEnabled();

	if (_isCheckingForDuplicateFrame) {
		if (--_framesLeft == 0) {
			_isCheckingForDuplicateFrame = false;
			_framesLeft = _nextSkipCount;
			if (_nextSkipCount < MAX_SKIP_COUNT) {
				// 增加下一次连续跳过检查的帧数
				++_nextSkipCount;
			}
		}

		if (_IsDuplicateFrame()) {
			_isCheckingForDuplicateFrame = true;
			_framesLeft = INITIAL_CHECK_COUNT;
			_nextSkipCount = INITIAL_SKIP_COUNT;
			return FrameSourceState::Waiting;
		} else {
			if (_isCheckingForDuplicateFrame || isStatisticsEnabled) {
				d3dDC->CopyResource(_prevFrame.get(), _output.get());
			}
			return FrameSourceState::NewFrame;
		}
	} else {
		if (--_framesLeft == 0) {
			_isCheckingForDuplicateFrame = true;
			// 第 2 次连续检查 10 帧，之后逐渐减少，从第 16 次开始只连续检查 2 帧
			_framesLeft = uint32_t((-4 * (int)_nextSkipCount + 78) / 7);
			
			if (!isStatisticsEnabled) {
				// 下一帧将检查重复帧，需要复制此帧
				d3dDC->CopyResource(_prevFrame.get(), _output.get());
			}
		}

		if (isStatisticsEnabled) {
			const bool isDuplicate = _IsDuplicateFrame();
			if (!isDuplicate) {
				d3dDC->CopyResource(_prevFrame.get(), _output.get());
			}

			std::pair<uint32_t, uint32_t> statistics = _statistics.load(std::memory_order_relaxed);
			if (isDuplicate) {
				// 预测错误
				++statistics.first;
			}
			// 总帧数
			++statistics.second;
			_statistics.store(statistics, std::memory_order_relaxed);
		}

		return FrameSourceState::NewFrame;
	}
}

std::pair<uint32_t, uint32_t> FrameSourceBase::GetStatisticsForDynamicDetection() const noexcept {
	return _statistics.load(std::memory_order_relaxed);
}

bool FrameSourceBase::_GetMapToOriginDPI(HWND hWnd, double& a, double& bx, double& by) noexcept {
	// HDC 中的 HBITMAP 尺寸为窗口的原始尺寸
	// 通过 GetWindowRect 获得的尺寸为窗口的 DPI 缩放后尺寸
	// 它们的商即为窗口的 DPI 缩放
	wil::unique_hdc_window hdcWindow(wil::window_dc(
		GetDCEx(hWnd, NULL, DCX_WINDOW), hWnd));
	if (!hdcWindow) {
		Logger::Get().Win32Error("GetDCEx 失败");
		return false;
	}

	wil::unique_hdc_window hdcClient(wil::window_dc(
		GetDCEx(hWnd, NULL, 0), hWnd));
	if (!hdcClient) {
		Logger::Get().Win32Error("GetDCEx 失败");
		return false;
	}

	HGDIOBJ hBmpWindow = GetCurrentObject(hdcWindow.get(), OBJ_BITMAP);
	if (!hBmpWindow) {
		Logger::Get().Win32Error("GetCurrentObject 失败");
		return false;
	}

	if (GetObjectType(hBmpWindow) != OBJ_BITMAP) {
		Logger::Get().Error("无法获取窗口的重定向表面");
		return false;
	}

	BITMAP bmp{};
	if (!GetObject(hBmpWindow, sizeof(bmp), &bmp)) {
		Logger::Get().Win32Error("GetObject 失败");
		return false;
	}

	RECT rect;
	if (!GetWindowRect(hWnd, &rect)) {
		Logger::Get().Win32Error("GetWindowRect 失败");
		return false;
	}

	a = bmp.bmWidth / double(rect.right - rect.left);

	// 使用 DPI 缩放无法可靠计算出窗口客户区的位置
	// 这里使用窗口 HDC 和客户区 HDC 的原点坐标差值
	// GetDCOrgEx 获得的是 DC 原点的屏幕坐标

	POINT ptClient{}, ptWindow{};
	if (!GetDCOrgEx(hdcClient.get(), &ptClient)) {
		Logger::Get().Win32Error("GetDCOrgEx 失败");
		return false;
	}
	if (!GetDCOrgEx(hdcWindow.get(), &ptWindow)) {
		Logger::Get().Win32Error("GetDCOrgEx 失败");
		return false;
	}

	if (!Win32Helper::GetClientScreenRect(hWnd, rect)) {
		Logger::Get().Error("GetClientScreenRect 失败");
		return false;
	}

	// 以窗口的客户区左上角为基准
	// 该点在坐标系 1 中坐标为 (rect.left, rect.top)
	// 在坐标系 2 中坐标为 (ptClient.x - ptWindow.x, ptClient.y - ptWindow.y)
	// 由此计算出 b
	bx = ptClient.x - ptWindow.x - rect.left * a;
	by = ptClient.y - ptWindow.y - rect.top * a;

	return true;
}

bool FrameSourceBase::_CenterWindowIfNecessary(HWND hWnd, const RECT& rcWork) noexcept {
	if (Win32Helper::GetWindowShowCmd(hWnd) == SW_SHOWMAXIMIZED) {
		return true;
	}

	RECT srcRect;
	if (!Win32Helper::GetWindowFrameRect(hWnd, srcRect)) {
		Logger::Get().Error("GetWindowFrameRect 失败");
		return false;
	}

	if (srcRect.left < rcWork.left || srcRect.top < rcWork.top
		|| srcRect.right > rcWork.right || srcRect.bottom > rcWork.bottom) {
		// 源窗口超越边界，将源窗口移到屏幕中央
		SIZE srcSize = { srcRect.right - srcRect.left, srcRect.bottom - srcRect.top };
		SIZE rcWorkSize = { rcWork.right - rcWork.left, rcWork.bottom - rcWork.top };
		if (srcSize.cx > rcWorkSize.cx || srcSize.cy > rcWorkSize.cy) {
			// 源窗口无法被当前屏幕容纳，因此无法捕获
			return false;
		}

		if (!SetWindowPos(
			hWnd,
			0,
			rcWork.left + (rcWorkSize.cx - srcSize.cx) / 2,
			rcWork.top + (rcWorkSize.cy - srcSize.cy) / 2,
			0,
			0,
			SWP_NOSIZE | SWP_NOZORDER
		)) {
			Logger::Get().Win32Error("SetWindowPos 失败");
		}
	}

	return true;
}

bool FrameSourceBase::_InitCheckingForDuplicateFrame() {
	ID3D11Device5* d3dDevice = _deviceResources->GetD3DDevice();

	D3D11_TEXTURE2D_DESC td;
	_output->GetDesc(&td);

	_prevFrame = DirectXHelper::CreateTexture2D(
		d3dDevice, td.Format, td.Width, td.Height, D3D11_BIND_SHADER_RESOURCE);
	if (!_prevFrame) {
		return false;
	}

	HRESULT hr = d3dDevice->CreateShaderResourceView(_prevFrame.get(), nullptr, _prevFrameSrv.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateShaderResourceView 失败", hr);
		return false;
	}

	D3D11_BUFFER_DESC bd{
		.ByteWidth = 4,
		.Usage = D3D11_USAGE_DEFAULT,
		.BindFlags = D3D11_BIND_UNORDERED_ACCESS,
		.StructureByteStride = 4
	};
	hr = d3dDevice->CreateBuffer(&bd, nullptr, _resultBuffer.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateBuffer 失败", hr);
		return false;
	}

	_resultBufferUav = _descriptorStore->GetUnorderedAccessView(
		_resultBuffer.get(), 1, DXGI_FORMAT_R32_UINT);
	if (!_resultBufferUav) {
		Logger::Get().ComError("GetUnorderedAccessView 失败", hr);
		return false;
	}

	bd.Usage = D3D11_USAGE_STAGING;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	bd.BindFlags = 0;
	hr = d3dDevice->CreateBuffer(&bd, nullptr, _readBackBuffer.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateBuffer 失败", hr);
		return false;
	}

	hr = d3dDevice->CreateComputeShader(
		DuplicateFrameCS, sizeof(DuplicateFrameCS), nullptr, _dupFrameCS.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateComputeShader 失败", hr);
		return false;
	}

	static constexpr std::pair<uint32_t, uint32_t> BLOCK_SIZE{ 16, 16 };
	_dispatchCount.first = (td.Width + BLOCK_SIZE.first - 1) / BLOCK_SIZE.first;
	_dispatchCount.second = (td.Height + BLOCK_SIZE.second - 1) / BLOCK_SIZE.second;
	
	return true;
}

bool FrameSourceBase::_IsDuplicateFrame() {
	// 检查是否和前一帧相同
	ID3D11DeviceContext4* d3dDC = _deviceResources->GetD3DDC();

	ID3D11ShaderResourceView* srvs[]{ _outputSrv, _prevFrameSrv.get() };
	d3dDC->CSSetShaderResources(0, 2, srvs);

	ID3D11SamplerState* sam = _deviceResources->GetSampler(
		D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP);
	d3dDC->CSSetSamplers(0, 1, &sam);

	// 将缓冲区置零
	static constexpr UINT ZERO[4]{};
	d3dDC->ClearUnorderedAccessViewUint(_resultBufferUav, ZERO);
	d3dDC->CSSetUnorderedAccessViews(0, 1, &_resultBufferUav, nullptr);

	d3dDC->CSSetShader(_dupFrameCS.get(), nullptr, 0);

	d3dDC->Dispatch(_dispatchCount.first, _dispatchCount.second, 1);

	// 取回结果
	d3dDC->CopyResource(_readBackBuffer.get(), _resultBuffer.get());

	uint32_t result = 1;
	D3D11_MAPPED_SUBRESOURCE ms;
	HRESULT hr = d3dDC->Map(_readBackBuffer.get(), 0, D3D11_MAP_READ, 0, &ms);
	if (SUCCEEDED(hr)) {
		result = *(uint32_t*)ms.pData;
		d3dDC->Unmap(_readBackBuffer.get(), 0);
	}
	return result == 0;
}

}
