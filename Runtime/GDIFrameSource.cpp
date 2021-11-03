#include "pch.h"
#include "GDIFrameSource.h"
#include "App.h"
#include "Utils.h"


extern std::shared_ptr<spdlog::logger> logger;


bool GDIFrameSource::Initialize(SIZE& frameSize) {
	_hwndSrc = App::GetInstance().GetHwndSrc();
	_d3dDC = App::GetInstance().GetRenderer().GetD3DDC();

	_srcClientRect = App::GetInstance().GetSrcClientRect();
	_srcClientSize = { _srcClientRect.right - _srcClientRect.left, _srcClientRect.bottom - _srcClientRect.top };

	if (!GetWindowRect(_hwndSrc, &_srcWndRect)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetWindowRect 失败"));
		return false;
	}
	_srcWndSize = { _srcWndRect.right - _srcWndRect.left, _srcWndRect.bottom - _srcWndRect.top };

	_pixels.resize(static_cast<size_t>(_srcWndSize.cx) * _srcWndSize.cy * 4);
	
	D3D11_TEXTURE2D_DESC desc{};
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.Width = _srcClientSize.cx;
	desc.Height = _srcClientSize.cy;
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

	return true;
}

ComPtr<ID3D11Texture2D> GDIFrameSource::GetOutput() {
	return _output;
}

bool GDIFrameSource::Update() {
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
	
	BITMAPINFO bi{};
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth = _srcWndSize.cx;
	bi.bmiHeader.biHeight = -_srcWndSize.cy;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biCompression = BI_RGB;
	bi.bmiHeader.biBitCount = 32;
	bi.bmiHeader.biSizeImage = (DWORD)_pixels.size();

	if (GetDIBits(hdcScreen, hBmpDest, 0, _srcWndSize.cy, _pixels.data(), &bi, DIB_RGB_COLORS) != _srcWndSize.cy) {
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

	BYTE* pPixels = _pixels.data() + (_srcWndSize.cx * (_srcClientRect.top - _srcWndRect.top) 
		+ _srcClientRect.left - _srcWndRect.left) * 4;
	BYTE* pData = (BYTE*)ms.pData;
	for (int i = 0; i < _srcClientSize.cy; ++i) {
		std::memcpy(pData, pPixels, static_cast<size_t>(_srcClientSize.cx) * 4);

		pPixels += _srcWndSize.cx * 4;
		pData += ms.RowPitch;
	}

	_d3dDC->Unmap(_output.Get(), 0);

	return true;
}
