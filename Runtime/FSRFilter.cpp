#include "pch.h"
#include "FSRFilter.h"
#include "App.h"
#include "DeviceResources.h"
#include "Renderer.h"
#include "FrameSourceBase.h"
#include "Utils.h"


extern std::shared_ptr<spdlog::logger> logger;

bool FSRFilter::Initialize() {
	auto& dr = App::GetInstance().GetDeviceResources();
	auto& renderer = App::GetInstance().GetRenderer();
	auto d3dDevice = dr.GetD3DDevice();

	const RECT& srcFrameRect = App::GetInstance().GetFrameSource().GetSrcFrameRect();
	const RECT& hostRect = App::GetInstance().GetHostWndRect();

	SIZE frameSize = { srcFrameRect.right - srcFrameRect.left,srcFrameRect.bottom - srcFrameRect.top };
	SIZE outputSize = { hostRect.right - hostRect.left,hostRect.bottom - hostRect.top };

	if (frameSize.cx / (double)frameSize.cy < outputSize.cx / (double)outputSize.cy) {
		outputSize.cx = outputSize.cy * frameSize.cx / (double)frameSize.cy;
	} else {
		outputSize.cy = outputSize.cx * frameSize.cy / (double)frameSize.cx;
	}
	

	// 创建常量缓冲区
	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = 4 * 4;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	union Constant32 {
		FLOAT floatVal;
		UINT uintVal;
	};

	Constant32 data[8]{};
	data[0].floatVal = (FLOAT)frameSize.cx;
	data[1].floatVal = (FLOAT)frameSize.cy;
	data[2].floatVal = (FLOAT)outputSize.cx;
	data[3].floatVal = (FLOAT)outputSize.cy;

	D3D11_SUBRESOURCE_DATA initData{};
	initData.pSysMem = data;

	HRESULT hr = d3dDevice->CreateBuffer(&bd, &initData, _easuCB.put());
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("CreateBuffer 失败", hr));
		return false;
	}

	bd.ByteWidth = 4 * 8;
	data[2].floatVal = 0.87f;
	data[4].uintVal = (hostRect.right - hostRect.left - outputSize.cx) / 2;
	data[5].uintVal = (hostRect.bottom - hostRect.top - outputSize.cy) / 2;
	data[6].uintVal = data[4].uintVal + outputSize.cx;
	data[7].uintVal = data[5].uintVal + outputSize.cy;

	d3dDevice->CreateBuffer(&bd, &initData, _rcasCB.put());
	
	dr.GetSampler(D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, &_sam);

	std::string hlsl;
	Utils::ReadTextFile(L".\\effects\\FSR_EASU.hlsl", hlsl);
	
	winrt::com_ptr<ID3DBlob> blob;
	dr.CompileShader(hlsl, "main", blob.put());
	d3dDevice->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, _easuShader.put());

	Utils::ReadTextFile(L".\\effects\\FSR_RCAS.hlsl", hlsl);
	dr.CompileShader(hlsl, "main", blob.put());
	d3dDevice->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, _rcasShader.put());

	D3D11_TEXTURE2D_DESC desc{};
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	desc.Width = outputSize.cx;
	desc.Height = outputSize.cy;

	d3dDevice->CreateTexture2D(&desc, nullptr, _tex.put());

	dr.GetShaderResourceView(App::GetInstance().GetFrameSource().GetOutput().get(), &_srv1);
	dr.GetShaderResourceView(_tex.get(), &_srv2);

	dr.GetUnorderedAccessView(_tex.get(), &_uav1);
	dr.GetUnorderedAccessView(dr.GetBackBuffer(), &_uav2);

	static const int threadGroupWorkRegionDim = 16;
	_dispatchX = (outputSize.cx + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;
	_dispatchY = (outputSize.cy + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;

	return true;
}

void FSRFilter::Draw() {
	auto& dr = App::GetInstance().GetDeviceResources();
	auto d3dDC = dr.GetD3DDC();
	auto d3dDevice = dr.GetD3DDevice();

	ID3D11Buffer* buf = _easuCB.get();
	d3dDC->CSSetConstantBuffers(0, 1, &buf);
	d3dDC->CSSetSamplers(0, 1, &_sam);
	d3dDC->CSSetShader(_easuShader.get(), nullptr, 0);
	d3dDC->CSSetShaderResources(0, 1, &_srv1);
	d3dDC->CSSetUnorderedAccessViews(0, 1, &_uav1, nullptr);

	d3dDC->Dispatch(_dispatchX, _dispatchY, 1);

	ID3D11UnorderedAccessView* uav = nullptr;
	d3dDC->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);
	buf = _rcasCB.get();
	d3dDC->CSSetConstantBuffers(0, 1, &buf);
	d3dDC->CSSetShader(_rcasShader.get(), nullptr, 0);
	d3dDC->CSSetShaderResources(0, 1, &_srv2);
	d3dDC->CSSetUnorderedAccessViews(0, 1, &_uav2, nullptr);

	d3dDC->Dispatch(_dispatchX, _dispatchY, 1);
}
