#pragma once
#include "pch.h"
#include <EffectBase.h>
#include "PixelScaleTransform.h"
#include "EffectDefines.h"


class PixelScaleEffect : public EffectBase {
public:
	IFACEMETHODIMP Initialize(
		_In_ ID2D1EffectContext* pEffectContext,
		_In_ ID2D1TransformGraph* pTransformGraph
	) {
		HRESULT hr = PixelScaleTransform::Create(pEffectContext, &_transform);
		if (FAILED(hr)) {
			return hr;
		}

		hr = pTransformGraph->SetSingleTransformNode(_transform.Get());
		if (FAILED(hr)) {
			return hr;
		}

		return S_OK;
	}

	HRESULT SetScale(INT value) {
		if (value <= 0) {
			return E_INVALIDARG;
		}

		_transform->SetScale(value);
		return S_OK;
	}

	INT GetScale() const {
		return _transform->GetScale();
	}

	enum PROPS {
		// 缩放倍数。默认值为 1
		PROP_SCALE = 0,
	};

	static HRESULT Register(_In_ ID2D1Factory1* pFactory) {
		const D2D1_PROPERTY_BINDING bindings[] =
		{
			D2D1_VALUE_TYPE_BINDING(L"Scale", &SetScale, &GetScale)
		};

		HRESULT hr = pFactory->RegisterEffectFromString(CLSID_MAGPIE_PIXEL_SCALE_EFFECT, XML(
			<?xml version='1.0'?>
			<Effect>
				<!--System Properties-->
				<Property name='DisplayName' type='string' value='Pixel Scale' />
				<Property name='Author' type='string' value='Blinue' />
				<Property name='Category' type='string' value='Scale' />
				<Property name='Description' type='string' value='Pixel Scale' />
				<Inputs>
					<Input name='Source' />
				</Inputs>
				<Property name='Scale' type='int32'>
					<Property name='DisplayName' type='string' value='Scale' />
					<Property name='Default' type='int32' value='1' />
				</Property>
			</Effect>
		), bindings, ARRAYSIZE(bindings), CreateEffect);

		return hr;
	}

	static HRESULT CALLBACK CreateEffect(_Outptr_ IUnknown** ppEffectImpl) {
		*ppEffectImpl = static_cast<ID2D1EffectImpl*>(new PixelScaleEffect());

		if (*ppEffectImpl == nullptr) {
			return E_OUTOFMEMORY;
		}

		return S_OK;
	}

private:
	PixelScaleEffect() {}

	ComPtr<PixelScaleTransform> _transform = nullptr;
};
