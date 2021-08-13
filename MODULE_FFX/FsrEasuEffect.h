#pragma once
#include "pch.h"
#include <EffectBase.h>
#include "EffectDefines.h"
#include "FsrEasuTransform.h"


class FsrEasuEffect : public EffectBase {
public:
	IFACEMETHODIMP Initialize(
		_In_ ID2D1EffectContext* pEffectContext,
		_In_ ID2D1TransformGraph* pTransformGraph
	) {
		HRESULT hr = FsrEasuTransform::Create(pEffectContext, &_transform);
		if (FAILED(hr)) {
			return hr;
		}

		hr = pTransformGraph->SetSingleTransformNode(_transform.Get());
		if (FAILED(hr)) {
			return hr;
		}

		return S_OK;
	}

	HRESULT SetScale(D2D_VECTOR_2F value) {
		if (value.x <= 0 || value.y <= 0) {
			return E_INVALIDARG;
		}

		_transform->SetScale(value);
		return S_OK;
	}

	D2D_VECTOR_2F GetScale() const {
		return _transform->GetScale();
	}

	enum PROPS {
		PROP_SCALE = 0
	};

	static HRESULT Register(_In_ ID2D1Factory1* pFactory) {
		const D2D1_PROPERTY_BINDING bindings[] = {
			D2D1_VALUE_TYPE_BINDING(L"Scale", &SetScale, &GetScale)
		};

		HRESULT hr = pFactory->RegisterEffectFromString(CLSID_MAGPIE_FSR_EASU_EFFECT, XML(
			<?xml version='1.0'?>
			<Effect>
				<!--System Properties-->
				<Property name='DisplayName' type='string' value='FSR EASU'/>
				<Property name='Author' type='string' value='Blinue'/>
				<Property name='Category' type='string' value='FFX'/>
				<Property name='Description' type='string' value='FSR EASU'/>
				<Inputs>
					<Input name='Source'/>
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
		*ppEffectImpl = static_cast<ID2D1EffectImpl*>(new FsrEasuEffect());

		if (*ppEffectImpl == nullptr) {
			return E_OUTOFMEMORY;
		}

		return S_OK;
	}

private:
	FsrEasuEffect() {}

	ComPtr<FsrEasuTransform> _transform = nullptr;
};
