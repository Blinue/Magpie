#include "pch.h"
#include "A4KSFilter.h"
#include "App.h"
#include "DeviceResources.h"
#include "FrameSourceBase.h"

static SIZE outputSize;

bool A4KSFilter::Initialize() {
	auto& dr = App::GetInstance().GetDeviceResources();
	auto d3dDevice = dr.GetD3DDevice();

	const RECT& srcFrameRect = App::GetInstance().GetFrameSource().GetSrcFrameRect();
	const RECT& hostRect = App::GetInstance().GetHostWndRect();

	SIZE frameSize = { srcFrameRect.right - srcFrameRect.left,srcFrameRect.bottom - srcFrameRect.top };
	SIZE hostWndSize = { hostRect.right - hostRect.left,hostRect.bottom - hostRect.top };

	// 创建常量缓冲区
	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = 4 * 8;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	union Constant32 {
		FLOAT floatVal;
		UINT uintVal;
	};

	Constant32 data[8]{};
	data[0].floatVal = 1.0f / frameSize.cx;
	data[1].floatVal = 1.0f / frameSize.cy;
	
	if (hostWndSize.cx >= frameSize.cx * 2) {
		data[2].uintVal = 0;
		data[4].uintVal = (hostWndSize.cx - frameSize.cx * 2) / 2;
		data[6].uintVal = UINT(data[4].uintVal + frameSize.cx * 2);
	} else {
		data[2].uintVal = (frameSize.cx * 2 - hostWndSize.cx) / 2;
		data[4].uintVal = 0;
		data[6].uintVal = hostWndSize.cx;
	}

	if (hostWndSize.cy >= frameSize.cy * 2) {
		data[3].uintVal = 0;
		data[5].uintVal = (hostWndSize.cy - frameSize.cy * 2) / 2;
		data[7].uintVal = UINT(data[5].uintVal + frameSize.cy * 2);
	} else {
		data[3].uintVal = (frameSize.cy * 2 - hostWndSize.cy) / 2;
		data[5].uintVal = 0;
		data[7].uintVal = hostWndSize.cy;
	}

	outputSize = { LONG(data[6].uintVal - data[4].uintVal), LONG(data[7].uintVal - data[5].uintVal) };

	D3D11_SUBRESOURCE_DATA initData{};
	initData.pSysMem = data;

	HRESULT hr = d3dDevice->CreateBuffer(&bd, &initData, _CB.put());
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("CreateBuffer 失败", hr));
		return false;
	}

	dr.GetSampler(D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_CLAMP, &_sams[0]);
	dr.GetSampler(D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP, &_sams[1]);

	std::string hlsl;
	Utils::ReadTextFile(L".\\effects\\A4KS1.hlsl", hlsl);

	winrt::com_ptr<ID3DBlob> blob;
	dr.CompileShader(hlsl, "main", blob.put());
	d3dDevice->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, _pass1Shader.put());

	Utils::ReadTextFile(L".\\effects\\A4KS2.hlsl", hlsl);
	dr.CompileShader(hlsl, "main", blob.put());
	d3dDevice->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, _pass2Shader.put());

	Utils::ReadTextFile(L".\\effects\\A4KS3.hlsl", hlsl);
	dr.CompileShader(hlsl, "main", blob.put());
	d3dDevice->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, _pass3Shader.put());

	Utils::ReadTextFile(L".\\effects\\A4KS4.hlsl", hlsl);
	dr.CompileShader(hlsl, "main", blob.put());
	d3dDevice->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, _pass4Shader.put());

	D3D11_TEXTURE2D_DESC desc{};
	desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	desc.Width = frameSize.cx;
	desc.Height = frameSize.cy;

	d3dDevice->CreateTexture2D(&desc, nullptr, _tex1.put());
	d3dDevice->CreateTexture2D(&desc, nullptr, _tex2.put());
	d3dDevice->CreateTexture2D(&desc, nullptr, _tex3.put());

	dr.GetShaderResourceView(App::GetInstance().GetFrameSource().GetOutput().get(), &_srv1);
	dr.GetShaderResourceView(_tex1.get(), &_srv2);
	dr.GetShaderResourceView(_tex2.get(), &_srv3);
	dr.GetShaderResourceView(_tex3.get(), &_srv4);

	dr.GetUnorderedAccessView(_tex1.get(), &_uav1);
	dr.GetUnorderedAccessView(_tex2.get(), &_uav2);
	dr.GetUnorderedAccessView(_tex3.get(), &_uav3);
	dr.GetUnorderedAccessView(dr.GetBackBuffer(), &_uav4);

	return true;
}

void A4KSFilter::Draw() {
	auto& dr = App::GetInstance().GetDeviceResources();
	auto d3dDC = dr.GetD3DDC();
	auto d3dDevice = dr.GetD3DDevice();

	const RECT& srcFrameRect = App::GetInstance().GetFrameSource().GetSrcFrameRect();
	SIZE frameSize = { srcFrameRect.right - srcFrameRect.left,srcFrameRect.bottom - srcFrameRect.top };


	int threadGroupWorkRegionDim = 16;
	int dispatchX = (frameSize.cx + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;
	int dispatchY = (frameSize.cy + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;

	ID3D11Buffer* buf = _CB.get();
	d3dDC->CSSetConstantBuffers(0, 1, &buf);
	d3dDC->CSSetSamplers(0, 2, _sams);
	d3dDC->CSSetShader(_pass1Shader.get(), nullptr, 0);
	d3dDC->CSSetShaderResources(0, 1, &_srv1);
	d3dDC->CSSetUnorderedAccessViews(0, 1, &_uav1, nullptr);
	d3dDC->Dispatch(dispatchX, dispatchY, 1);

	ID3D11UnorderedAccessView* emptyUav = nullptr;
	d3dDC->CSSetUnorderedAccessViews(0, 1, &emptyUav, nullptr);
	d3dDC->CSSetShader(_pass2Shader.get(), nullptr, 0);
	d3dDC->CSSetShaderResources(0, 1, &_srv2);
	d3dDC->CSSetUnorderedAccessViews(0, 1, &_uav2, nullptr);
	d3dDC->Dispatch(dispatchX, dispatchY, 1);
	
	d3dDC->CSSetUnorderedAccessViews(0, 1, &emptyUav, nullptr);
	d3dDC->CSSetShader(_pass3Shader.get(), nullptr, 0);
	d3dDC->CSSetShaderResources(0, 1, &_srv3);
	d3dDC->CSSetUnorderedAccessViews(0, 1, &_uav3, nullptr);
	d3dDC->Dispatch(dispatchX, dispatchY, 1);

	//threadGroupWorkRegionDim = 16;
	dispatchX = (outputSize.cx + (16 - 1)) / 16;
	dispatchY = (outputSize.cy + (16 - 1)) / 16;

	d3dDC->CSSetUnorderedAccessViews(0, 1, &emptyUav, nullptr);
	d3dDC->CSSetShader(_pass4Shader.get(), nullptr, 0);
	ID3D11ShaderResourceView* srvs[2]{ _srv4, _srv1 };
	d3dDC->CSSetShaderResources(0, 2, srvs);
	d3dDC->CSSetUnorderedAccessViews(0, 1, &_uav4, nullptr);
	d3dDC->Dispatch(dispatchX, dispatchY, 1);
}
