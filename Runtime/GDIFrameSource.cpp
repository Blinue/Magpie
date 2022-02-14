#include "pch.h"
#include "GDIFrameSource.h"
#include "App.h"
#include "DeviceResources.h"


extern std::shared_ptr<spdlog::logger> logger;

bool GDIFrameSource::Initialize() {
	if (!FrameSourceBase::Initialize()) {
		SPDLOG_LOGGER_ERROR(logger, "初始化 FrameSourceBase 失败");
		return false;
	}

	if (!_UpdateSrcFrameRect()) {
		SPDLOG_LOGGER_ERROR(logger, "_UpdateSrcFrameRect 失败");
		return false;
	}

	HWND hwndSrc = App::GetInstance().GetHwndSrc();

	double a, bx, by;
	if (_GetMapToOriginDPI(hwndSrc, a, bx, by)) {
		SPDLOG_LOGGER_INFO(logger, fmt::format("源窗口 DPI 缩放为 {}", 1 / a));

		_frameRect = {
			std::lround(_srcFrameRect.left * a + bx),
			std::lround(_srcFrameRect.top * a + by),
			std::lround(_srcFrameRect.right * a + bx),
			std::lround(_srcFrameRect.bottom * a + by)
		};
	} else {
		SPDLOG_LOGGER_ERROR(logger, "_GetMapToOriginDPI 失败");

		// _GetMapToOriginDPI 失败则假设 DPI 缩放为 1
		RECT srcWindowRect;
		if (!GetWindowRect(hwndSrc, &srcWindowRect)) {
			SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetWindowRect 失败"));
			return false;
		}

		_frameRect = {
			_srcFrameRect.left - srcWindowRect.left,
			_srcFrameRect.top - srcWindowRect.top,
			_srcFrameRect.right - srcWindowRect.left,
			_srcFrameRect.bottom - srcWindowRect.top
		};
	}
	
	if (_frameRect.left < 0 || _frameRect.top < 0 || _frameRect.right < 0
		|| _frameRect.bottom < 0 || _frameRect.right - _frameRect.left <= 0
		|| _frameRect.bottom - _frameRect.top <= 0
	) {
		App::GetInstance().SetErrorMsg(ErrorMessages::FAILED_TO_CROP);
		SPDLOG_LOGGER_ERROR(logger, "裁剪失败");
		return false;
	}

	D3D11_TEXTURE2D_DESC desc{};
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.Width = _frameRect.right - _frameRect.left;
	desc.Height = _frameRect.bottom - _frameRect.top;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	desc.MiscFlags = D3D11_RESOURCE_MISC_GDI_COMPATIBLE;
	HRESULT hr = App::GetInstance().GetDeviceResources().GetD3DDevice()->CreateTexture2D(&desc, nullptr, _output.put());
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("创建 Texture2D 失败", hr));
		return false;
	}

	_dxgiSurface = _output.try_as<IDXGISurface1>();
	if (!_dxgiSurface) {
		SPDLOG_LOGGER_ERROR(logger, "从 Texture2D 获取 IDXGISurface1 失败");
		return false;
	}

	SPDLOG_LOGGER_INFO(logger, "GDIFrameSource 初始化完成");
	return true;
}

FrameSourceBase::UpdateState GDIFrameSource::Update() {
	HWND hwndSrc = App::GetInstance().GetHwndSrc();

	HDC hdcDest;
	HRESULT hr = _dxgiSurface->GetDC(TRUE, &hdcDest);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("从 Texture2D 获取 IDXGISurface1 失败", hr));
		return UpdateState::Error;
	}

	HDC hdcSrc = GetDCEx(hwndSrc, NULL, DCX_LOCKWINDOWUPDATE | DCX_WINDOW);
	if (!hdcSrc) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetDC 失败"));
		_dxgiSurface->ReleaseDC(nullptr);
		return UpdateState::Error;
	}

	if (!BitBlt(hdcDest, 0, 0, _frameRect.right-_frameRect.left, _frameRect.bottom-_frameRect.top,
		hdcSrc, _frameRect.left, _frameRect.top, SRCCOPY)
	) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("BitBlt 失败"));
	}

	ReleaseDC(hwndSrc, hdcSrc);
	_dxgiSurface->ReleaseDC(nullptr);

	return UpdateState::NewFrame;
}
