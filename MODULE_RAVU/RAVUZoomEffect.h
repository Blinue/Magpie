#pragma once
#include "pch.h"
#include <EffectBase.h>
#include "RavuZoomTransform.h"
#include "RavuZoomWeightsTransform.h"
#include <d2d1effecthelpers.h>
#include "EffectDefines.h"
#include "resource.h"


class RAVUZoomEffect : public EffectBase {
public:
	IFACEMETHODIMP Initialize(
		_In_ ID2D1EffectContext* pEffectContext,
		_In_ ID2D1TransformGraph* pTransformGraph
	) {
		HRESULT hr = SimpleDrawTransform<>::Create(
			pEffectContext,
			&_rgb2yuvTransform,
			MAGPIE_RGB2YUV_SHADER,
			GUID_MAGPIE_RGB2YUV_SHADER
		);
		if (FAILED(hr)) {
			return hr;
		}
		hr = RAVUZoomTransform::Create(pEffectContext, &_ravuZoomTransform);
		if (FAILED(hr)) {
			return hr;
		}
		hr = RAVUZoomWeightsTransform::Create(pEffectContext, &_ravuZoomWeightsTransform);
		if (FAILED(hr)) {
			return hr;
		}

		hr = pTransformGraph->AddNode(_rgb2yuvTransform.Get());
		if (FAILED(hr)) {
			return hr;
		}
		hr = pTransformGraph->AddNode(_ravuZoomTransform.Get());
		if (FAILED(hr)) {
			return hr;
		}
		hr = pTransformGraph->AddNode(_ravuZoomWeightsTransform.Get());
		if (FAILED(hr)) {
			return hr;
		}

		hr = pTransformGraph->ConnectToEffectInput(0, _rgb2yuvTransform.Get(), 0);
		if (FAILED(hr)) {
			return hr;
		}
		hr = pTransformGraph->ConnectToEffectInput(1, _ravuZoomWeightsTransform.Get(), 0);
		if (FAILED(hr)) {
			return hr;
		}
		hr = pTransformGraph->ConnectNode(_rgb2yuvTransform.Get(), _ravuZoomTransform.Get(), 0);
		if (FAILED(hr)) {
			return hr;
		}
		hr = pTransformGraph->ConnectNode(_ravuZoomWeightsTransform.Get(), _ravuZoomTransform.Get(), 1);
		if (FAILED(hr)) {
			return hr;
		}
		hr = pTransformGraph->SetOutputNode(_ravuZoomTransform.Get());
		if (FAILED(hr)) {
			return hr;
		}

		return S_OK;
	}

	HRESULT SetScale(D2D_VECTOR_2F value) {
		if (value.x <= 0 || value.y <= 0) {
			return E_INVALIDARG;
		}

		_ravuZoomTransform->SetScale(value);
		return S_OK;
	}

	D2D_VECTOR_2F GetScale() const {
		return _ravuZoomTransform->GetScale();
	}


	enum PROPS {
		PROP_SCALE = 0
	};

	static HRESULT Register(_In_ ID2D1Factory1* pFactory) {
		const D2D1_PROPERTY_BINDING bindings[] =
		{
			D2D1_VALUE_TYPE_BINDING(L"Scale", &SetScale, &GetScale)
		};

		HRESULT hr = pFactory->RegisterEffectFromString(CLSID_MAGPIE_RAVU_ZOOM_EFFECT, XML(
			<?xml version='1.0'?>
			<Effect>
				<!--System Properties-->
				<Property name='DisplayName' type='string' value='RAVU Zoom Scale'/>
				<Property name='Author' type='string' value='Blinue'/>
				<Property name='Category' type='string' value='Scale'/>
				<Property name='Description' type='string' value='RAVU Zoom Scale'/>
				<Inputs>
					<Input name='Source'/>
					<Input name='Weights'/>
				</Inputs>
				<Property name='Scale' type='vector2'>
					<Property name='DisplayName' type='string' value='Scale'/>
					<Property name='Default' type='vector2' value='(1,1)'/>
				</Property>
			</Effect>
		), bindings, ARRAYSIZE(bindings), CreateEffect);

		return hr;
	}

	static HRESULT CALLBACK CreateEffect(_Outptr_ IUnknown** ppEffectImpl) {
		*ppEffectImpl = static_cast<ID2D1EffectImpl*>(new RAVUZoomEffect());

		if (*ppEffectImpl == nullptr) {
			return E_OUTOFMEMORY;
		}

		return S_OK;
	}

	static HRESULT LoadWeights(ID2D1Effect* effect, HINSTANCE hInst, IWICImagingFactory2* wicImgFactory, ID2D1DeviceContext* d2dDC) {
		HBITMAP hBmp = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(IDB_RAVU_ZOOM_R3_WEIGHTS), IMAGE_BITMAP, 0, 0, 0);
		if (hBmp == NULL) {
			return E_FAIL;
		}
		ComPtr<ID2D1Bitmap> weights;
		HRESULT hr = EffectUtils::LoadBitmapFromHBmp(wicImgFactory, d2dDC, hBmp, weights);
		if (FAILED(hr)) {
			return hr;
		}
		if (!DeleteObject(hBmp)) {
			return E_FAIL;
		}

		effect->SetInput(1, weights.Get());
		return S_OK;
	}
private:
	RAVUZoomEffect() {}

	ComPtr<SimpleDrawTransform<>> _rgb2yuvTransform = nullptr;
	ComPtr<RAVUZoomTransform> _ravuZoomTransform = nullptr;
	ComPtr<RAVUZoomWeightsTransform> _ravuZoomWeightsTransform = nullptr;
};
