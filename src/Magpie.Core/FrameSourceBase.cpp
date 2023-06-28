#include "pch.h"
#include "FrameSourceBase.h"
#include "ScalingOptions.h"
#include "Logger.h"
#include "Win32Utils.h"
#include "CommonSharedConstants.h"
#include "Utils.h"
#include "SmallVector.h"
#include "DirectXHelper.h"
#include "DeviceResources.h"
#include "shaders/DuplicateFrameCS.h"

namespace Magpie::Core {

FrameSourceBase::~FrameSourceBase() noexcept {
	// 还原窗口圆角
	if (_roundCornerDisabled) {
		_roundCornerDisabled = false;

		INT attr = DWMWCP_DEFAULT;
		HRESULT hr = DwmSetWindowAttribute(
			_hwndSrc, DWMWA_WINDOW_CORNER_PREFERENCE, &attr, sizeof(attr));
		if (FAILED(hr)) {
			Logger::Get().ComError("取消禁用窗口圆角失败", hr);
		} else {
			Logger::Get().Info("已取消禁用窗口圆角");
		}
	}

	// 还原窗口大小调整
	if (_windowResizingDisabled) {
		// 缩放 Magpie 主窗口时会在 SetWindowLongPtr 中卡住，似乎是 Win11 的 bug
		// 将在 MagService::_MagRuntime_IsRunningChanged 还原主窗口样式
		if (Win32Utils::GetWndClassName(_hwndSrc) != CommonSharedConstants::MAIN_WINDOW_CLASS_NAME) {
			LONG_PTR style = GetWindowLongPtr(_hwndSrc, GWL_STYLE);
			if (!(style & WS_THICKFRAME)) {
				if (SetWindowLongPtr(_hwndSrc, GWL_STYLE, style | WS_THICKFRAME)) {
					if (!SetWindowPos(_hwndSrc, 0, 0, 0, 0, 0,
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
}

bool FrameSourceBase::Initialize(
	HWND hwndSrc,
	HWND hwndScaling,
	const ScalingOptions& options,
	DeviceResources& deviceResources
) noexcept {
	_hwndSrc = hwndSrc;
	_deviceResources = &deviceResources;

	// 禁用窗口大小调整
	if (options.IsDisableWindowResizing()) {
		LONG_PTR style = GetWindowLongPtr(hwndSrc, GWL_STYLE);
		if (style & WS_THICKFRAME) {
			if (SetWindowLongPtr(hwndSrc, GWL_STYLE, style ^ WS_THICKFRAME)) {
				// 不重绘边框，以防某些窗口状态不正确
				// if (!SetWindowPos(hwndSrc, 0, 0, 0, 0, 0,
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
		if (Win32Utils::GetOSVersion().IsWin11()) {
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

	if (!_Initialize(hwndScaling, options)) {
		Logger::Get().Error("_Initialize 失败");
		return false;
	}

	return true;
}

FrameSourceBase::UpdateState FrameSourceBase::Update() noexcept {
	UpdateState state = _Update();
	if (state != UpdateState::NewFrame) {
		return state;
	}

	ID3D11Device5* d3dDevice = _deviceResources->GetD3DDevice();
	ID3D11DeviceContext4* d3dDC = _deviceResources->GetD3DDC();

	if (_prevFrame) {
		// 检查是否和前一帧相同
		ID3D11ShaderResourceView* srvs[]{
			_deviceResources->GetShaderResourceView(_output.get()),
			_deviceResources->GetShaderResourceView(_prevFrame.get())
		};
		d3dDC->CSSetShaderResources(0, 2, srvs);

		ID3D11SamplerState* sam = _deviceResources->GetSampler(
			D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP);
		d3dDC->CSSetSamplers(0, 1, &sam);

		ID3D11UnorderedAccessView* uav = _deviceResources->GetUnorderedAccessView(
			_resultBuffer.get(), 1, DXGI_FORMAT_R32_UINT);
		// 将缓冲区置零
		static constexpr UINT ZERO[4]{};
		d3dDC->ClearUnorderedAccessViewUint(uav, ZERO);
		d3dDC->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);

		d3dDC->CSSetShader(_dupFrameCS.get(), nullptr, 0);

		d3dDC->Dispatch(_dispatchCount.first, _dispatchCount.second, 1);

		// 取回结果
		d3dDC->CopyResource(_readBackBuffer.get(), _resultBuffer.get());

		uint32_t result = 1;
		{
			D3D11_MAPPED_SUBRESOURCE ms;
			HRESULT hr = d3dDC->Map(_readBackBuffer.get(), 0, D3D11_MAP_READ, 0, &ms);
			if (SUCCEEDED(hr)) {
				result = *(uint32_t*)ms.pData;
				d3dDC->Unmap(_readBackBuffer.get(), 0);
			}
		}
		if (result == 0) {
			// 和前一帧相同
			return UpdateState::NoChange;
		}
	} else {
		D3D11_TEXTURE2D_DESC td;
		_output->GetDesc(&td);

		_prevFrame = DirectXHelper::CreateTexture2D(
			d3dDevice, td.Format, td.Width, td.Height, D3D11_BIND_SHADER_RESOURCE);

		if (!_prevFrame) {
			return UpdateState::NewFrame;
		}

		D3D11_BUFFER_DESC bd{};
		bd.ByteWidth = 4;
		bd.StructureByteStride = 4;
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
		HRESULT hr = d3dDevice->CreateBuffer(&bd, nullptr, _resultBuffer.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateBuffer 失败", hr);
			_prevFrame = nullptr;
			return UpdateState::NewFrame;
		}

		bd.Usage = D3D11_USAGE_STAGING;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		bd.BindFlags = 0;
		hr = d3dDevice->CreateBuffer(&bd, nullptr, _readBackBuffer.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateBuffer 失败", hr);
			_prevFrame = nullptr;
			return UpdateState::NewFrame;
		}

		hr = d3dDevice->CreateComputeShader(
			DuplicateFrameCS, sizeof(DuplicateFrameCS), nullptr, _dupFrameCS.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateComputeShader 失败", hr);
			_prevFrame = nullptr;
			return UpdateState::NewFrame;
		}

		static constexpr std::pair<uint32_t, uint32_t> BLOCK_SIZE{16, 16};
		_dispatchCount.first = (td.Width + BLOCK_SIZE.first - 1) / BLOCK_SIZE.first;
		_dispatchCount.second = (td.Height + BLOCK_SIZE.second - 1) / BLOCK_SIZE.second;
	}

	d3dDC->CopyResource(_prevFrame.get(), _output.get());
	return UpdateState::NewFrame;
}

struct EnumChildWndParam {
	const wchar_t* clientWndClassName = nullptr;
	SmallVector<HWND, 1> childWindows;
};

static BOOL CALLBACK EnumChildProc(
	_In_ HWND   hwnd,
	_In_ LPARAM lParam
) {
	std::wstring className = Win32Utils::GetWndClassName(hwnd);

	EnumChildWndParam* param = (EnumChildWndParam*)lParam;
	if (className == param->clientWndClassName) {
		param->childWindows.push_back(hwnd);
	}

	return TRUE;
}

static HWND FindClientWindowOfUWP(HWND hwndSrc, const wchar_t* clientWndClassName) {
	// 查找所有窗口类名为 ApplicationFrameInputSinkWindow 的子窗口
	// 该子窗口一般为客户区
	EnumChildWndParam param{};
	param.clientWndClassName = clientWndClassName;
	EnumChildWindows(hwndSrc, EnumChildProc, (LPARAM)&param);

	if (param.childWindows.empty()) {
		// 未找到符合条件的子窗口
		return hwndSrc;
	}

	if (param.childWindows.size() == 1) {
		return param.childWindows[0];
	}

	// 如果有多个匹配的子窗口，取最大的（一般不会出现）
	int maxSize = 0, maxIdx = 0;
	for (int i = 0; i < param.childWindows.size(); ++i) {
		RECT rect;
		if (!GetClientRect(param.childWindows[i], &rect)) {
			continue;
		}

		int size = rect.right - rect.left + rect.bottom - rect.top;
		if (size > maxSize) {
			maxSize = size;
			maxIdx = i;
		}
	}

	return param.childWindows[maxIdx];
}

bool FrameSourceBase::_CalcSrcRect(const Cropping& cropping, bool isCaptureTitleBar) noexcept {
	if (isCaptureTitleBar && _CanCaptureTitleBar()) {
		HRESULT hr = DwmGetWindowAttribute(_hwndSrc,
			DWMWA_EXTENDED_FRAME_BOUNDS, &_srcRect, sizeof(_srcRect));
		if (FAILED(hr)) {
			Logger::Get().ComError("DwmGetWindowAttribute 失败", hr);
		}
	} else {
		std::wstring className = Win32Utils::GetWndClassName(_hwndSrc);
		if (className == L"ApplicationFrameWindow" || className == L"Windows.UI.Core.CoreWindow") {
			// "Modern App"
			// 客户区窗口类名为 ApplicationFrameInputSinkWindow
			HWND hwndClient = FindClientWindowOfUWP(_hwndSrc, L"ApplicationFrameInputSinkWindow");
			if (hwndClient) {
				if (!Win32Utils::GetClientScreenRect(hwndClient, _srcRect)) {
					Logger::Get().Win32Error("GetClientScreenRect 失败");
				}
			}
		}
	}

	if (_srcRect == RECT{}) {
		if (!Win32Utils::GetClientScreenRect(_hwndSrc, _srcRect)) {
			Logger::Get().Win32Error("GetClientScreenRect 失败");
			return false;
		}
	}

	_srcRect = {
		std::lround(_srcRect.left + cropping.Left),
		std::lround(_srcRect.top + cropping.Top),
		std::lround(_srcRect.right - cropping.Right),
		std::lround(_srcRect.bottom - cropping.Bottom)
	};

	if (_srcRect.right - _srcRect.left <= 0 || _srcRect.bottom - _srcRect.top <= 0) {
		Logger::Get().Error("裁剪窗口失败");
		return false;
	}

	return true;
}

bool FrameSourceBase::_GetMapToOriginDPI(HWND hWnd, double& a, double& bx, double& by) noexcept {
	// HDC 中的 HBITMAP 尺寸为窗口的原始尺寸
	// 通过 GetWindowRect 获得的尺寸为窗口的 DPI 缩放后尺寸
	// 它们的商即为窗口的 DPI 缩放
	HDC hdcWindow = GetDCEx(hWnd, NULL, DCX_LOCKWINDOWUPDATE | DCX_WINDOW);
	if (!hdcWindow) {
		Logger::Get().Win32Error("GetDCEx 失败");
		return false;
	}

	HDC hdcClient = GetDCEx(hWnd, NULL, DCX_LOCKWINDOWUPDATE);
	if (!hdcClient) {
		Logger::Get().Win32Error("GetDCEx 失败");
		ReleaseDC(hWnd, hdcWindow);
		return false;
	}

	Utils::ScopeExit se([hWnd, hdcWindow, hdcClient]() {
		ReleaseDC(hWnd, hdcWindow);
		ReleaseDC(hWnd, hdcClient);
	});

	HGDIOBJ hBmpWindow = GetCurrentObject(hdcWindow, OBJ_BITMAP);
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
	if (!GetDCOrgEx(hdcClient, &ptClient)) {
		Logger::Get().Win32Error("GetDCOrgEx 失败");
		return false;
	}
	if (!GetDCOrgEx(hdcWindow, &ptWindow)) {
		Logger::Get().Win32Error("GetDCOrgEx 失败");
		return false;
	}

	if (!Win32Utils::GetClientScreenRect(hWnd, rect)) {
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
	RECT srcRect;
	if (!GetWindowRect(hWnd, &srcRect)) {
		Logger::Get().Win32Error("GetWindowRect 失败");
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

}
