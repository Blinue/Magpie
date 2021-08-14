#pragma once
#include "pch.h"
#include <EffectBase.h>
#include "EffectDefines.h"
#include "FsrRcasTransform.h"


class FsrRcasEffect : public EffectBase {
public:
	IFACEMETHODIMP Initialize(
		_In_ ID2D1EffectContext* pEffectContext,
		_In_ ID2D1TransformGraph* pTransformGraph
	) {
		HRESULT hr = FsrRcasTransform::Create(pEffectContext, &_transform);
		if (FAILED(hr)) {
			return hr;
		}
		hr = pTransformGraph->SetSingleTransformNode(_transform.Get());
		if (FAILED(hr)) {
			return hr;
		}

		return S_OK;
	}

	HRESULT SetSharpness(FLOAT value) {
		if (value <= 0) {
			return E_INVALIDARG;
		}

		_transform->SetSharpness(value);
		return S_OK;
	}

	FLOAT GetSharpness() const {
		return _transform->GetSharpness();
	}

	HRESULT SetRemoveNoise(BOOL value) {
		_transform->SetRemoveNoise(value);
		return S_OK;
	}

	BOOL IsRemoveNoise() const {
		return _transform->IsRemoveNoise();
	}

	enum PROPS {
		PROP_SHARPNESS = 0,  // 锐化强度，必须在0~1之间。默认值为 0.87
		PROP_REMOVE_NOISE = 1	// 是否降噪，默认为否
	};

	static HRESULT Register(_In_ ID2D1Factory1* pFactory) {
		const D2D1_PROPERTY_BINDING bindings[] = {
			D2D1_VALUE_TYPE_BINDING(L"Sharpness", &SetSharpness, &GetSharpness),
			D2D1_VALUE_TYPE_BINDING(L"RemoveNoise", &SetRemoveNoise, &IsRemoveNoise)
		};

		HRESULT hr = pFactory->RegisterEffectFromString(CLSID_MAGPIE_FSR_RCAS_EFFECT, XML(
			<?xml version='1.0'?>
			<Effect>
				<!--System Properties-->
				<Property name='DisplayName' type='string' value='FSR RCAS'/>
				<Property name='Author' type='string' value='Blinue'/>
				<Property name='Category' type='string' value='FFX'/>
				<Property name='Description' type='string' value='FSR RCAS'/>
				<Inputs>
					<Input name='Source'/>
				</Inputs>
				<Property name='Sharpness' type='float'>
					<Property name='DisplayName' type='string' value='Sharpness'/>
					<Property name='Default' type='float' value='0.87'/>
				</Property>
				<Property name="RemoveNoise" type='bool'>
					<Property name='DisplayName' type='string' value='Remove Noise'/>
					<Property name='Default' type='bool' value='false'/>
				</Property>
			</Effect>
		), bindings, ARRAYSIZE(bindings), CreateEffect);

		return hr;
	}

	static HRESULT CALLBACK CreateEffect(_Outptr_ IUnknown** ppEffectImpl) {
		*ppEffectImpl = static_cast<ID2D1EffectImpl*>(new FsrRcasEffect());

		if (*ppEffectImpl == nullptr) {
			return E_OUTOFMEMORY;
		}

		return S_OK;
	}

private:
	FsrRcasEffect() {}

	ComPtr<FsrRcasTransform> _transform = nullptr;
};
