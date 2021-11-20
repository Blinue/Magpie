#include "pch.h"
#include "MagCallbackFrameSource.h"
#include "App.h"


extern std::shared_ptr<spdlog::logger> logger;


MagCallbackFrameSource::~MagCallbackFrameSource() {
	MagSetImageScalingCallback(_hwndMag, nullptr);
	DestroyWindow(_hwndMag);
}

bool MagCallbackFrameSource::Initialize() {
	HWND hwndHost = App::GetInstance().GetHwndHost();
	const RECT& srcClientRect = App::GetInstance().GetSrcClientRect();

	D3D11_TEXTURE2D_DESC desc{};
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.Width = srcClientRect.right - srcClientRect.left;
	desc.Height = srcClientRect.bottom - srcClientRect.top;
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

	MagUninitialize();
	MagInitialize();
	_hwndMag = FindWindowEx(hwndHost, NULL, WC_MAGNIFIER, L"MagnifierWindow");
	if (!_hwndMag) {
		_hwndMag = CreateWindow(
			WC_MAGNIFIER,
			L"MagnifierWindow",
			WS_CHILD,
			0,
			0,
			srcClientRect.right - srcClientRect.left,
			srcClientRect.bottom - srcClientRect.top,
			hwndHost,
			NULL,
			App::GetInstance().GetHInstance(),
			NULL
		);
	}
	
	if (!_hwndMag) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("创建放大镜控件失败"));
		return false;
	}

	
	if (!MagSetImageScalingCallback(_hwndMag, &_ImageScalingCallback)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("MagSetImageScalingCallback 失败"));
		return false;
	}

	SPDLOG_LOGGER_INFO(logger, "MagCallbackFrameSource 初始化完成");
	return true;
}

ComPtr<ID3D11Texture2D> MagCallbackFrameSource::GetOutput() {
	return _output;
}

bool MagCallbackFrameSource::Update() {
	if (!MagSetWindowSource(_hwndMag, App::GetInstance().GetSrcClientRect())) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("MagSetWindowSource 失败"));
		return false;
	}
	return true;
}

BOOL MagCallbackFrameSource::_ImageScalingCallback(
	HWND hWnd,
	void* srcdata,
	MAGIMAGEHEADER srcheader,
	void* destdata,
	MAGIMAGEHEADER destheader,
	RECT unclipped,
	RECT clipped,
	HRGN dirty
) {
	if (srcheader.cbSize / srcheader.width / srcheader.height != 4) {
		SPDLOG_LOGGER_ERROR(logger, "srcdata 不是BGRA格式");
		return FALSE;
	}

	ComPtr<ID3D11DeviceContext> d3dDC = App::GetInstance().GetRenderer().GetD3DDC();
	ComPtr<ID3D11Texture2D> output = App::GetInstance().GetFrameSource().GetOutput();

	D3D11_MAPPED_SUBRESOURCE ms{};
	HRESULT hr = d3dDC->Map(output.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("Map 失败", hr));
		return FALSE;
	}

	BYTE* pPixels = (BYTE*)srcdata;
	BYTE* pData = (BYTE*)ms.pData;
	for (UINT i = 0; i < srcheader.height; ++i) {
		std::memcpy(pData, pPixels, static_cast<size_t>(srcheader.width) * 4);

		pPixels += srcheader.stride;
		pData += ms.RowPitch;
	}

	d3dDC->Unmap(output.Get(), 0);

	return TRUE;
}
