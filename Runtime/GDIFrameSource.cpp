#include "pch.h"
#include "GDIFrameSource.h"
#include "App.h"


extern std::shared_ptr<spdlog::logger> logger;

GDIFrameSource::~GDIFrameSource() {
	DeleteBitmap(_hbmMem);
	DeleteDC(_hdcMem);
}

bool GDIFrameSource::Initialize() {
	_hwndSrc = App::GetInstance().GetHwndSrc();
	_d3dDC = App::GetInstance().GetRenderer().GetD3DDC();

	const RECT& srcClientRect = App::GetInstance().GetSrcClientRect();
	SIZE srcClientSize = { srcClientRect.right - srcClientRect.left, srcClientRect.bottom - srcClientRect.top };

	RECT windowRect;
	GetWindowRect(_hwndSrc, &windowRect);

	
	D3D11_TEXTURE2D_DESC desc{};
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.Width = srcClientSize.cx;
	desc.Height = srcClientSize.cy;
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

	HDC hdcScreen = GetDC(NULL);
	_hdcMem = CreateCompatibleDC(hdcScreen);
	_hbmMem = CreateCompatibleBitmap(hdcScreen, srcClientSize.cx, srcClientSize.cy);
	ReleaseDC(NULL, hdcScreen);

	return true;
}

ComPtr<ID3D11Texture2D> GDIFrameSource::GetOutput() {
	return _output;
}

bool GDIFrameSource::Update() {
	D3D11_MAPPED_SUBRESOURCE ms{};
	_d3dDC->Map(_output.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);

	HDC hdcSrc = GetDCEx(_hwndSrc, NULL, DCX_LOCKWINDOWUPDATE);
	
	const RECT& srcClientRect = App::GetInstance().GetSrcClientRect();
	SIZE srcClientSize = { srcClientRect.right - srcClientRect.left, srcClientRect.bottom - srcClientRect.top };

	HBITMAP hbmOld = SelectBitmap(_hdcMem, _hbmMem);
	BitBlt(_hdcMem, 0, 0, srcClientSize.cx, srcClientSize.cy, hdcSrc, 0, 0, SRCCOPY);
	SelectBitmap(_hdcMem, hbmOld);

	BITMAPINFO bi{};
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth = srcClientSize.cx;
	bi.bmiHeader.biHeight = -srcClientSize.cy;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biCompression = BI_RGB;
	bi.bmiHeader.biBitCount = 32;
	bi.bmiHeader.biSizeImage = srcClientSize.cx * srcClientSize.cy * 4;

	std::vector<BYTE> pixels(bi.bmiHeader.biSizeImage);
	GetDIBits(_hdcMem, _hbmMem, 0, srcClientRect.top - srcClientRect.bottom, pixels.data(), &bi, DIB_RGB_COLORS);
	for (int i = 0; i < srcClientSize.cy; ++i) {
		std::memcpy((BYTE*)ms.pData + i * ms.RowPitch, &pixels[0] + i * srcClientSize.cx * 4, srcClientSize.cx * 4);
	}

	ReleaseDC(_hwndSrc, hdcSrc);
	_d3dDC->Unmap(_output.Get(), 0);

	return true;
}
