#include "pch.h"
#include "GDIFrameSource2.h"
#include "App.h"


extern std::shared_ptr<spdlog::logger> logger;

bool GDIFrameSource2::Initialize() {
	_hwndSrc = App::GetInstance().GetHwndSrc();
	const RECT& srcClientRect = App::GetInstance().GetSrcClientRect();

	RECT windowRect;
	GetWindowRect(_hwndSrc, &windowRect);


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
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	HRESULT hr = App::GetInstance().GetRenderer().GetD3DDevice()->CreateTexture2D(&desc, nullptr, &_output);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("创建 Texture2D 失败", hr));
		return false;
	}

	return true;
}

ComPtr<ID3D11Texture2D> GDIFrameSource2::GetOutput() {
	return _output;
}

bool GDIFrameSource2::Update() {

	return false;
}
