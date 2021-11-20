#include "pch.h"
#include "LegacyGDIFrameSource.h"
#include "App.h"
#include "Utils.h"


extern std::shared_ptr<spdlog::logger> logger;


bool LegacyGDIFrameSource::Initialize() {
	_hwndSrc = App::GetInstance().GetHwndSrc();
	_d3dDC = App::GetInstance().GetRenderer().GetD3DDC();

	const RECT& srcClientRect = App::GetInstance().GetSrcClientRect();

	RECT srcWndRect;
	if (!GetWindowRect(_hwndSrc, &srcWndRect)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetWindowRect 失败"));
		return false;
	}

	POINT clientOffset;
	float dpiScale;
	bool success = true;

	if (!_GetWindowDpiScale(_hwndSrc, dpiScale)) {
		SPDLOG_LOGGER_ERROR(logger, "_GetWindowDpiScale 失败");
		success = false;
	}

	SPDLOG_LOGGER_INFO(logger, fmt::format("源窗口 DPI 缩放为 {}", dpiScale));

	if (abs(dpiScale - 1.0f) < 1e-5f) {
		// 无 DPI 缩放，使用普通方式
		success = false;
	}

	if (success && !_GetDpiAwareWindowClientOffset(_hwndSrc, clientOffset)) {
		SPDLOG_LOGGER_ERROR(logger, "_GetDpiAwareWindowClientOffset 失败");
		success = false;
	}

	if (success) {
		SIZE frameSize = {
			(LONG)ceilf((srcClientRect.right - srcClientRect.left) / dpiScale),
			(LONG)ceilf((srcClientRect.bottom - srcClientRect.top) / dpiScale)
		};
		_frameInWindow = {
			clientOffset.x,
			clientOffset.y,
			clientOffset.x + frameSize.cx,
			clientOffset.y + frameSize.cy
		};
		
		_bi.bmiHeader.biWidth = std::lroundf((srcWndRect.right - srcWndRect.left) / dpiScale);
		_bi.bmiHeader.biHeight = std::lroundf((srcWndRect.top - srcWndRect.bottom) / dpiScale);
	} else {
		// 回落到普通方式
		_frameInWindow = {
			srcClientRect.left - srcWndRect.left,
			srcClientRect.top - srcWndRect.top,
			srcClientRect.right - srcWndRect.left,
			srcClientRect.bottom - srcWndRect.top
		};

		_bi.bmiHeader.biWidth = srcWndRect.right - srcWndRect.left;
		_bi.bmiHeader.biHeight = srcWndRect.top - srcWndRect.bottom;
	}

	_bi.bmiHeader.biSize = sizeof(_bi);
	_bi.bmiHeader.biPlanes = 1;
	_bi.bmiHeader.biCompression = BI_RGB;
	_bi.bmiHeader.biBitCount = 32;
	_bi.bmiHeader.biSizeImage = _bi.bmiHeader.biWidth * -_bi.bmiHeader.biHeight * 4;

	_pixels.reset(new BYTE[_bi.bmiHeader.biSizeImage]);
	
	D3D11_TEXTURE2D_DESC desc{};
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.Width = _frameInWindow.right - _frameInWindow.left;
	desc.Height = _frameInWindow.bottom - _frameInWindow.top;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	HRESULT hr = App::GetInstance().GetRenderer().GetD3DDevice()->CreateTexture2D(&desc, nullptr, &_output);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("创建 Texture2D 失败", hr));
		return false;
	}

	SPDLOG_LOGGER_INFO(logger, "LegacyGDIFrameSource 初始化完成");
	return true;
}

ComPtr<ID3D11Texture2D> LegacyGDIFrameSource::GetOutput() {
	return _output;
}

bool LegacyGDIFrameSource::Update() {
	HDC hdcSrc = GetWindowDC(_hwndSrc);
	if (!hdcSrc) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetWindowDC 失败"));
		return false;
	}
	HDC hdcScreen = GetDC(NULL);
	if (!hdcScreen) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetDC 失败"));
		ReleaseDC(_hwndSrc, hdcSrc);
		return false;
	}

	Utils::ScopeExit se([&]() {
		ReleaseDC(_hwndSrc, hdcSrc);
		ReleaseDC(NULL, hdcScreen);
	});
	
	// 直接获取 DC 中当前图像，而不是使用 BitBlt 复制
	HBITMAP hBmpDest = (HBITMAP)GetCurrentObject(hdcSrc, OBJ_BITMAP);
	if (!hBmpDest) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetCurrentObject 失败"));
		return false;
	}

	SIZE frameSize = { _frameInWindow.right - _frameInWindow.left, _frameInWindow.bottom - _frameInWindow.top };

	if (GetDIBits(hdcScreen, hBmpDest, 0, -_bi.bmiHeader.biHeight,
		_pixels.get(), &_bi, DIB_RGB_COLORS) != -_bi.bmiHeader.biHeight
	) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetDIBits 失败"));
		return false;
	}

	// 将客户区的像素复制到 Texture2D 中
	D3D11_MAPPED_SUBRESOURCE ms{};
	HRESULT hr = _d3dDC->Map(_output.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("Map 失败", hr));
		return false;
	}

	BYTE* pPixels = _pixels.get() + (_bi.bmiHeader.biWidth * _frameInWindow.top + _frameInWindow.left) * 4;
	BYTE* pData = (BYTE*)ms.pData;
	for (int i = 0; i < frameSize.cy; ++i) {
		std::memcpy(pData, pPixels, static_cast<size_t>(frameSize.cx * 4));

		pPixels += _bi.bmiHeader.biWidth * 4;
		pData += ms.RowPitch;
	}

	_d3dDC->Unmap(_output.Get(), 0);

	return true;
}
