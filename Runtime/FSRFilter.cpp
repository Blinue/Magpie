#include "pch.h"
#include "FSRFilter.h"
#include "App.h"
#include "DeviceResources.h"
#include "Renderer.h"
#include "FrameSourceBase.h"
#include "Utils.h"
#include "Logger.h"
#include "CursorManager.h"


union Constant32 {
	FLOAT floatVal;
	UINT uintVal;
	INT intVal;
};

bool FSRFilter::Initialize(RECT& outputRect) {
	auto& dr = App::Get().GetDeviceResources();
	auto& renderer = App::Get().GetRenderer();
	auto d3dDevice = dr.GetD3DDevice();

	const RECT& srcFrameRect = App::Get().GetFrameSource().GetSrcFrameRect();
	const RECT& hostRect = App::Get().GetHostWndRect();

	SIZE frameSize = { srcFrameRect.right - srcFrameRect.left,srcFrameRect.bottom - srcFrameRect.top };
	SIZE outputSize = { hostRect.right - hostRect.left,hostRect.bottom - hostRect.top };

	if (frameSize.cx / (double)frameSize.cy < outputSize.cx / (double)outputSize.cy) {
		outputSize.cx = std::lround(outputSize.cy * frameSize.cx / (double)frameSize.cy);
	} else {
		outputSize.cy = std::lround(outputSize.cx * frameSize.cy / (double)frameSize.cx);
	}

	outputRect.left = (hostRect.right - hostRect.left - outputSize.cx) / 2;
	outputRect.top = (hostRect.bottom - hostRect.top - outputSize.cy) / 2;
	outputRect.right = outputRect.left + outputSize.cx;
	outputRect.bottom = outputRect.top + outputSize.cy;

	// 创建常量缓冲区
	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = 4 * 4;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	D3D11_TEXTURE2D_DESC desc1{};
	App::Get().GetFrameSource().GetOutput()->GetDesc(&desc1);

	Constant32 data[8]{};
	data[0].floatVal = (FLOAT)desc1.Width;
	data[1].floatVal = (FLOAT)desc1.Height;
	data[2].floatVal = (FLOAT)outputSize.cx;
	data[3].floatVal = (FLOAT)outputSize.cy;

	D3D11_SUBRESOURCE_DATA initData{};
	initData.pSysMem = data;

	HRESULT hr = d3dDevice->CreateBuffer(&bd, &initData, _easuCB.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateBuffer 失败", hr);
		return false;
	}

	bd.ByteWidth = 4 * 8;
	data[0].floatVal = 0.87f;
	data[1].uintVal = 0;
	data[2].uintVal = 0;
	data[4].uintVal = outputRect.left;
	data[5].uintVal = outputRect.top;
	data[6].uintVal = outputSize.cx;
	data[7].uintVal = outputSize.cy;

	d3dDevice->CreateBuffer(&bd, &initData, _rcasCB.put());

	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	d3dDevice->CreateBuffer(&bd, nullptr, _cursorCB.put());
	
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

	dr.GetShaderResourceView(App::Get().GetFrameSource().GetOutput(), &_srv1);
	dr.GetShaderResourceView(_tex.get(), &_srv2);

	dr.GetUnorderedAccessView(_tex.get(), &_uav1);
	dr.GetUnorderedAccessView(dr.GetBackBuffer(), &_uav2);

	static const int threadGroupWorkRegionDim = 16;
	_dispatchX = (outputSize.cx + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;
	_dispatchY = (outputSize.cy + (threadGroupWorkRegionDim - 1)) / threadGroupWorkRegionDim;

	return true;
}

void FSRFilter::Draw() {
	auto& dr = App::Get().GetDeviceResources();
	auto d3dDC = dr.GetD3DDC();
	auto d3dDevice = dr.GetD3DDevice();

	ID3D11Buffer* buf = _easuCB.get();
	d3dDC->CSSetConstantBuffers(0, 1, &buf);
	d3dDC->CSSetSamplers(0, 1, &_sam);
	d3dDC->CSSetShader(_easuShader.get(), nullptr, 0);
	d3dDC->CSSetShaderResources(0, 1, &_srv1);
	d3dDC->CSSetUnorderedAccessViews(0, 1, &_uav1, nullptr);

	d3dDC->Dispatch(_dispatchX, _dispatchY, 1);

	CursorManager& cm = App::Get().GetRenderer().GetCursorManager();

	bool hasCursor = cm.HasCursor();
	POINT cursorPos = POINT{ LONG_MIN, LONG_MIN };
	SIZE cursorSize{};
	POINT cursorHotSpot{};
	ID3D11ShaderResourceView* cursorRtv = nullptr;
	CursorManager::CursorType cursorType = CursorManager::CursorType::Color;

	if (hasCursor) {
		const POINT* pos = cm.GetCursorPos();
		const CursorManager::CursorInfo* ci = cm.GetCursorInfo();
		if (pos && ci) {
			cursorPos = *cm.GetCursorPos();
			cursorSize = ci->size;
			cursorHotSpot = ci->hotSpot;
		} else {
			assert(false);
			hasCursor = false;
		}

		ID3D11Texture2D* cursorTexture = nullptr;
		if (hasCursor && !cm.GetCursorTexture(cursorTexture, cursorType)) {
			hasCursor = false;
		}

		if (hasCursor && !dr.GetShaderResourceView(cursorTexture, &cursorRtv)) {
			hasCursor = false;
		}
	}
	
	D3D11_MAPPED_SUBRESOURCE ms;
	HRESULT hr = d3dDC->Map(_cursorCB.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
	if (SUCCEEDED(hr)) {
		Constant32* data = (Constant32*)ms.pData;

		if (hasCursor) {
			data[0].intVal = cursorPos.x - cursorHotSpot.x;
			data[1].intVal = cursorPos.y - cursorHotSpot.y;
			data[2].intVal = data[0].intVal + cursorSize.cx;
			data[3].intVal = data[1].intVal + cursorSize.cy;

			data[4].floatVal = 1.0f / cursorSize.cx;
			data[5].floatVal = 1.0f / cursorSize.cy;

			data[6].uintVal = (UINT)cursorType;
		} else {
			data[0].intVal = INT_MIN;
			data[1].intVal = INT_MIN;
			data[2].intVal = INT_MIN;
			data[3].intVal = INT_MIN;
		}

		d3dDC->Unmap(_cursorCB.get(), 0);
	} else {
		Logger::Get().ComError("Map 失败", hr);
	}

	ID3D11UnorderedAccessView* uav = nullptr;
	d3dDC->CSSetUnorderedAccessViews(0, 1, &uav, nullptr);
	ID3D11Buffer* t[2] = { _rcasCB.get(), _cursorCB.get() };
	d3dDC->CSSetConstantBuffers(0, 2, t);
	d3dDC->CSSetShader(_rcasShader.get(), nullptr, 0);
	ID3D11ShaderResourceView* t1[2] = { _srv2, cursorRtv };
	d3dDC->CSSetShaderResources(0, 2, t1);
	d3dDC->CSSetUnorderedAccessViews(0, 1, &_uav2, nullptr);

	d3dDC->Dispatch(_dispatchX, _dispatchY, 1);
}
