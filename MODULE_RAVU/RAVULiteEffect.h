#pragma once
#include "pch.h"
#include <SimpleDrawTransform.h>
#include "RavuLitePass2Transform.h"
#include <EffectBase.h>
#include "EffectDefines.h"


class RAVULiteEffect : public EffectBase {
public:
	IFACEMETHODIMP Initialize(
		_In_ ID2D1EffectContext* effectContext,
		_In_ ID2D1TransformGraph* transformGraph
	) {
		HRESULT hr = SimpleDrawTransform<>::Create(
			effectContext,
			&_rgb2yuvTransform,
			MAGPIE_RGB2YUV_SHADER,
			GUID_MAGPIE_RGB2YUV_SHADER
		);
		if (FAILED(hr)) {
			return hr;
		}
		hr = SimpleDrawTransform<>::Create(
			effectContext,
			&_pass1Transform,
			MAGPIE_RAVU_LITE_R3_PASS1_SHADER,
			GUID_MAGPIE_RAVU_LITE_R3_PASS1_SHADER
		);
		if (FAILED(hr)) {
			return hr;
		}
		hr = RAVULitePass2Transform::Create(effectContext, &_pass2Transform);
		if (FAILED(hr)) {
			return hr;
		}

		hr = transformGraph->AddNode(_rgb2yuvTransform.Get());
		if (FAILED(hr)) {
			return hr;
		}
		hr = transformGraph->AddNode(_pass1Transform.Get());
		if (FAILED(hr)) {
			return hr;
		}
		hr = transformGraph->AddNode(_pass2Transform.Get());
		if (FAILED(hr)) {
			return hr;
		}

		hr = transformGraph->ConnectToEffectInput(0, _rgb2yuvTransform.Get(), 0);
		if (FAILED(hr)) {
			return hr;
		}
		hr = transformGraph->ConnectNode(_rgb2yuvTransform.Get(), _pass1Transform.Get(), 0);
		if (FAILED(hr)) {
			return hr;
		}
		hr = transformGraph->ConnectNode(_rgb2yuvTransform.Get(), _pass2Transform.Get(), 0);
		if (FAILED(hr)) {
			return hr;
		}
		hr = transformGraph->ConnectNode(_pass1Transform.Get(), _pass2Transform.Get(), 1);
		if (FAILED(hr)) {
			return hr;
		}

		hr = transformGraph->SetOutputNode(_pass2Transform.Get());
		if (FAILED(hr)) {
			return hr;
		}

		return S_OK;
	}

	static HRESULT Register(_In_ ID2D1Factory1* pFactory) {
		HRESULT hr = pFactory->RegisterEffectFromString(CLSID_MAGPIE_RAVU_LITE_EFFECT, XML(
			<?xml version='1.0'?>
			<Effect>
				<!--System Properties-->
				<Property name='DisplayName' type='string' value='Ravu Upscale'/>
				<Property name='Author' type='string' value='Blinue'/>
				<Property name='Category' type='string' value='Scale'/>
				<Property name='Description' type='string' value='Ravu Upscale'/>
				<Inputs>
					<Input name='Source'/>
				</Inputs>
			</Effect>
		), nullptr, 0, CreateEffect);

		return hr;
	}

	static HRESULT CALLBACK CreateEffect(_Outptr_ IUnknown** ppEffectImpl) {
		// This code assumes that the effect class initializes its reference count to 1.
		*ppEffectImpl = static_cast<ID2D1EffectImpl*>(new RAVULiteEffect());

		if (*ppEffectImpl == nullptr) {
			return E_OUTOFMEMORY;
		}

		return S_OK;
	}

private:
	// Constructor should be private since it should never be called externally.
	RAVULiteEffect() {}

	ComPtr<SimpleDrawTransform<>> _rgb2yuvTransform = nullptr;
	ComPtr<SimpleDrawTransform<>> _pass1Transform = nullptr;
	ComPtr<RAVULitePass2Transform> _pass2Transform = nullptr;
};
