#include "pch.h"
#include "DesktopDuplicationFrameSource.h"
#include "App.h"


ComPtr<IDXGIOutput1> GetDXGIOutput(HMONITOR hMonitor) {
	ComPtr<IDXGIAdapter1> curAdapter = App::GetInstance().GetRenderer().GetGraphicsAdapter();

	// 首先在当前使用的图形适配器上查询显示器
	ComPtr<IDXGIOutput> output;
	for (UINT adapterIndex = 0;
			SUCCEEDED(curAdapter->EnumOutputs(adapterIndex,
				output.ReleaseAndGetAddressOf()));
			adapterIndex++
	) {
		DXGI_OUTPUT_DESC desc;
		output->GetDesc(&desc);

		if (desc.Monitor == hMonitor) {
			ComPtr<IDXGIOutput1> output1;
			output.As<IDXGIOutput1>(&output1);
			return output1;
		}
	}

	return nullptr;
}

bool DesktopDuplicationFrameSource::Initialize() {
	HMONITOR hMonitor = MonitorFromWindow(App::GetInstance().GetHwndSrc(), MONITOR_DEFAULTTONEAREST);

	ComPtr<IDXGIOutput1> output = GetDXGIOutput(hMonitor);
	if (!output) {
		return false;
	}

	HRESULT hr = output->DuplicateOutput(App::GetInstance().GetRenderer().GetD3DDevice().Get(), _outputDup.ReleaseAndGetAddressOf());
	if (FAILED(hr)) {
		return false;
	}

	const RECT& srcClientRect = App::GetInstance().GetSrcClientRect();

	D3D11_TEXTURE2D_DESC desc{};
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.Width = srcClientRect.right - srcClientRect.left;
	desc.Height = srcClientRect.bottom - srcClientRect.top;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	hr = App::GetInstance().GetRenderer().GetD3DDevice()->CreateTexture2D(&desc, nullptr, &_output);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("创建 Texture2D 失败", hr));
		return false;
	}

	SetWindowDisplayAffinity(App::GetInstance().GetHwndHost(), WDA_EXCLUDEFROMCAPTURE);

	MONITORINFO mi{};
	mi.cbSize = sizeof(mi);
	if (!GetMonitorInfo(hMonitor, &mi)) {
		SPDLOG_LOGGER_ERROR(logger, MakeWin32ErrorMsg("GetMonitorInfo 出错"));
		return false;
	}

	_frameInMonitor = {
		UINT(srcClientRect.left - mi.rcMonitor.left),
		UINT(srcClientRect.top - mi.rcMonitor.top),
		0,
		UINT(srcClientRect.right - mi.rcMonitor.left),
		UINT(srcClientRect.bottom - mi.rcMonitor.top),
		1
	};

	return true;
}

bool DesktopDuplicationFrameSource::Update() {
	DXGI_OUTDUPL_FRAME_INFO info;
	
	if (_dxgiRes) {
		_outputDup->ReleaseFrame();
	}
	HRESULT hr = _outputDup->AcquireNextFrame(0, &info, _dxgiRes.ReleaseAndGetAddressOf());

	if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
		return true;
	}

	if (FAILED(hr)) {
		return false;
	}

	ComPtr<ID3D11Resource> d3dRes;
	_dxgiRes.As<ID3D11Resource>(&d3dRes);

	App::GetInstance().GetRenderer().GetD3DDC()->CopySubresourceRegion(
		_output.Get(), 0, 0, 0, 0, d3dRes.Get(), 0, &_frameInMonitor);

	_outputDup->ReleaseFrame();
	
	return true;
}
