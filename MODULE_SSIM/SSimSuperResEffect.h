#pragma once
#include <EffectBase.h>
#include <SimpleDrawTransform.h>
#include <SimpleScaleTransform.h>
#include "SSimSuperResWithScaleTransform.h"
#include "SSimSuperResFinalTransform.h"
#include "SSimSuperResVarLTransform.h"
#include "EffectDefines.h"


class SSimSuperResEffect : public EffectBase {
public:
	IFACEMETHODIMP Initialize(
		_In_ ID2D1EffectContext* pEffectContext,
		_In_ ID2D1TransformGraph* pTransformGraph
	) {
		_effectContext = pEffectContext;
		_transformGraph = pTransformGraph;

		HRESULT hr = SSimSuperResWithScaleTransform::Create(
			_effectContext,
			&_downscaling1Transform,
			MAGPIE_SSIM_SUPERRES_DOWNSCALING1_SHADER,
			GUID_MAGPIE_SSIM_SUPERRES_DOWNSCALING_1_SHADER
		);
		if (FAILED(hr)) {
			return hr;
		}

		hr = SSimSuperResWithScaleTransform::Create(
			_effectContext,
			&_downscaling2Transform,
			MAGPIE_SSIM_SUPERRES_DOWNSCALING2_SHADER,
			GUID_MAGPIE_SSIM_SUPERRES_DOWNSCALING_2_SHADER
		);
		if (FAILED(hr)) {
			return hr;
		}

		hr = SSimSuperResVarLTransform::Create(
			_effectContext,
			&_varLTransform
		);
		if (FAILED(hr)) {
			return hr;
		}

		hr = SSimSuperResWithScaleTransform::Create(
			_effectContext,
			&_varHTransform,
			MAGPIE_SSIM_SUPERRES_VARH_SHADER,
			GUID_MAGPIE_SSIM_SUPERRES_VARH_SHADER
		);
		if (FAILED(hr)) {
			return hr;
		}

		hr = SSimSuperResFinalTransform::Create(_effectContext, &_finalTransform);
		if (FAILED(hr)) {
			return hr;
		}

		return S_OK;
	}

	HRESULT SetUpScaleEffect(IUnknown* value) {
		HRESULT hr = value->QueryInterface<ID2D1Effect>(&_upScaleEffect);
		if (FAILED(hr)) {
			return hr;
		}

		hr = _effectContext->CreateTransformNodeFromEffect(_upScaleEffect.Get(), &_upScaleTransform);
		if (FAILED(hr)) {
			return hr;
		}

		return _MakeGraph();
	}

	IUnknown* GetUpScaleEffect() const {
		return _upScaleEffect.Get();
	}

	enum PROPS {
		PROP_UP_SCALE_EFFECT = 0
	};

	static HRESULT Register(_In_ ID2D1Factory1* pFactory) {
		const D2D1_PROPERTY_BINDING bindings[] =
		{
			D2D1_VALUE_TYPE_BINDING(L"UpScaleEffect", &SetUpScaleEffect, &GetUpScaleEffect)
		};

		HRESULT hr = pFactory->RegisterEffectFromString(CLSID_MAGPIE_SSIM_SUPERRES_EFFECT, XML(
			<?xml version='1.0'?>
			<Effect>
				<!--System Properties-->
				<Property name='DisplayName' type='string' value='SSimSuperRes'/>
				<Property name='Author' type='string' value='Blinue'/>
				<Property name='Category' type='string' value='SSIM'/>
				<Property name='Description' type='string' value='SSimSuperRes'/>
				<Inputs>
					<Input name='Source'/>
				</Inputs>
				<Property name='UpScaleEffect' type='iunknown'>
					<Property name='DisplayName' type='string' value='UpScaleEffect'/>
				</Property>
			</Effect>
		), bindings, ARRAYSIZE(bindings), CreateEffect);

		return hr;
	}

	static HRESULT CALLBACK CreateEffect(_Outptr_ IUnknown** ppEffectImpl) {
		*ppEffectImpl = static_cast<ID2D1EffectImpl*>(new SSimSuperResEffect());

		if (*ppEffectImpl == nullptr) {
			return E_OUTOFMEMORY;
		}

		return S_OK;
	}

private:
	SSimSuperResEffect() {}

	HRESULT _MakeGraph() {
		assert(_upScaleTransform);

		_transformGraph->Clear();


		HRESULT hr = _transformGraph->AddNode(_upScaleTransform.Get());
		if (FAILED(hr)) {
			return hr;
		}
		hr = _transformGraph->AddNode(_downscaling1Transform.Get());
		if (FAILED(hr)) {
			return hr;
		}
		hr = _transformGraph->AddNode(_downscaling2Transform.Get());
		if (FAILED(hr)) {
			return hr;
		}
		hr = _transformGraph->AddNode(_varLTransform.Get());
		if (FAILED(hr)) {
			return hr;
		}
		hr = _transformGraph->AddNode(_varHTransform.Get());
		if (FAILED(hr)) {
			return hr;
		}
		hr = _transformGraph->AddNode(_finalTransform.Get());
		if (FAILED(hr)) {
			return hr;
		}
		
		hr = _transformGraph->ConnectToEffectInput(0, _upScaleTransform.Get(), 0);
		if (FAILED(hr)) {
			return hr;
		}
		hr = _transformGraph->ConnectToEffectInput(0, _varLTransform.Get(), 0);
		if (FAILED(hr)) {
			return hr;
		}
		hr = _transformGraph->ConnectToEffectInput(0, _downscaling1Transform.Get(), 1);
		if (FAILED(hr)) {
			return hr;
		}
		hr = _transformGraph->ConnectToEffectInput(0, _downscaling2Transform.Get(), 1);
		if (FAILED(hr)) {
			return hr;
		}
		hr = _transformGraph->ConnectToEffectInput(0, _varHTransform.Get(), 1);
		if (FAILED(hr)) {
			return hr;
		}
		hr = _transformGraph->ConnectToEffectInput(0, _finalTransform.Get(), 0);
		if (FAILED(hr)) {
			return hr;
		}
		hr = _transformGraph->ConnectNode(_upScaleTransform.Get(), _downscaling1Transform.Get(), 0);
		if (FAILED(hr)) {
			return hr;
		}
		hr = _transformGraph->ConnectNode(_downscaling1Transform.Get(), _downscaling2Transform.Get(), 0);
		if (FAILED(hr)) {
			return hr;
		}
		hr = _transformGraph->ConnectNode(_upScaleTransform.Get(), _varLTransform.Get(), 1);
		if (FAILED(hr)) {
			return hr;
		}
		hr = _transformGraph->ConnectNode(_downscaling2Transform.Get(), _varHTransform.Get(), 0);
		if (FAILED(hr)) {
			return hr;
		}
		hr = _transformGraph->ConnectNode(_upScaleTransform.Get(), _finalTransform.Get(), 1);
		if (FAILED(hr)) {
			return hr;
		}
		hr = _transformGraph->ConnectNode(_downscaling2Transform.Get(), _finalTransform.Get(), 2);
		if (FAILED(hr)) {
			return hr;
		}
		hr = _transformGraph->ConnectNode(_varLTransform.Get(), _finalTransform.Get(), 3);
		if (FAILED(hr)) {
			return hr;
		}
		hr = _transformGraph->ConnectNode(_varHTransform.Get(), _finalTransform.Get(), 4);
		if (FAILED(hr)) {
			return hr;
		}
		hr = _transformGraph->SetOutputNode(_finalTransform.Get());
		if (FAILED(hr)) {
			return hr;
		}

		return S_OK;
	}

	ComPtr<ID2D1TransformNode> _upScaleTransform = nullptr;
	ComPtr<SSimSuperResWithScaleTransform> _downscaling1Transform = nullptr;
	ComPtr<SSimSuperResWithScaleTransform> _downscaling2Transform = nullptr;
	ComPtr<SSimSuperResVarLTransform> _varLTransform = nullptr;
	ComPtr<SSimSuperResWithScaleTransform> _varHTransform = nullptr;
	ComPtr<SSimSuperResFinalTransform> _finalTransform = nullptr;

	ComPtr<ID2D1Effect> _upScaleEffect = nullptr;

	ID2D1EffectContext* _effectContext = nullptr;
	ID2D1TransformGraph* _transformGraph = nullptr;
};