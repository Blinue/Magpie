#include "pch.h"
#include <initguid.h>
#include "EffectDefines.h"
#include "FsrEasuEffect.h"
#include "FsrRcasEffect.h"
#include "FfxCasEffect.h"
#include <fmt/xchar.h>

API_DECLSPEC HRESULT CreateCASEffect(
	ID2D1Factory1* d2dFactory,
	ID2D1DeviceContext* d2dDC,
	const nlohmann::json& props,
	ComPtr<ID2D1Effect>& effect
) {
	bool isRegistered;
	HRESULT hr = EffectUtils::IsEffectRegistered(d2dFactory, CLSID_MAGPIE_FFX_CAS_EFFECT, isRegistered);
	if (FAILED(hr)) {
		return hr;
	}

	if (!isRegistered) {
		hr = FfxCasEffect::Register(d2dFactory);
		if (FAILED(hr)) {
			return hr;
		}
	}

	ComPtr<ID2D1Effect> result;
	hr = d2dDC->CreateEffect(CLSID_MAGPIE_FFX_CAS_EFFECT, &result);
	if (FAILED(hr)) {
		return hr;
	}

	// sharpness 属性
	auto it = props.find("sharpness");
	if (it != props.end()) {
		const auto& value = *it;
		if (!value.is_number()) {
			return E_INVALIDARG;
		}

		float sharpness = value.get<float>();
		if (sharpness < 0 || sharpness > 1) {
			return E_INVALIDARG;
		}

		hr = result->SetValue(FfxCasEffect::PROP_SHARPNESS, sharpness);
		if (FAILED(hr)) {
			return hr;
		}
	}

	effect = std::move(result);
	return S_OK;
}

API_DECLSPEC HRESULT CreateFsrEasuEffect(
	ID2D1Factory1* d2dFactory,
	ID2D1DeviceContext* d2dDC,
	const nlohmann::json& props,
	float fillScale,
	std::pair<float, float>& scale,
	ComPtr<ID2D1Effect>& effect
) {
	bool isRegistered;
	HRESULT hr = EffectUtils::IsEffectRegistered(d2dFactory, CLSID_MAGPIE_FSR_EASU_EFFECT, isRegistered);
	if (FAILED(hr)) {
		return hr;
	}

	if (!isRegistered) {
		hr = FsrEasuEffect::Register(d2dFactory);
		if (FAILED(hr)) {
			return hr;
		}
	}

	ComPtr<ID2D1Effect> result;
	hr = d2dDC->CreateEffect(CLSID_MAGPIE_FSR_EASU_EFFECT, &result);
	if (FAILED(hr)) {
		return hr;
	}

	std::pair<float, float> scaleResult(1.0f, 1.0f);
	// scale 属性
	auto it = props.find("scale");
	if (it != props.end()) {
		hr = EffectUtils::ReadScaleProp(*it, fillScale, scale, scaleResult);
		if (FAILED(hr)) {
			return hr;
		}

		hr = result->SetValue(
			FsrEasuEffect::PROP_SCALE,
			D2D1_VECTOR_2F{ scaleResult.first, scaleResult.second }
		);
		if (FAILED(hr)) {
			return hr;
		}
	}

	effect = std::move(result);
	scale.first *= scaleResult.first;
	scale.second *= scaleResult.second;
	return S_OK;
}

API_DECLSPEC HRESULT CreateFsrRcasEffect(
	ID2D1Factory1* d2dFactory,
	ID2D1DeviceContext* d2dDC,
	const nlohmann::json& props,
	ComPtr<ID2D1Effect>& effect
) {
	bool isRegistered;
	HRESULT hr = EffectUtils::IsEffectRegistered(d2dFactory, CLSID_MAGPIE_FSR_RCAS_EFFECT, isRegistered);
	if (FAILED(hr)) {
		return hr;
	}

	if (!isRegistered) {
		hr = FsrRcasEffect::Register(d2dFactory);
		if (FAILED(hr)) {
			return hr;
		}
	}

	ComPtr<ID2D1Effect> result;
	hr = d2dDC->CreateEffect(CLSID_MAGPIE_FSR_RCAS_EFFECT, &result);
	if (FAILED(hr)) {
		return hr;
	}

	// sharpness 属性
	auto it = props.find("sharpness");
	if (it != props.end()) {
		const auto& value = *it;
		if (!value.is_number()) {
			return E_INVALIDARG;
		}

		float sharpness = value.get<float>();
		if (sharpness < 0 || sharpness > 1) {
			return E_INVALIDARG;
		}

		hr = result->SetValue(FsrRcasEffect::PROP_SHARPNESS, sharpness);
		if (FAILED(hr)) {
			return hr;
		}
	}

	effect = std::move(result);
	return S_OK;
}

API_DECLSPEC HRESULT CreateEffect(
	ID2D1Factory1* d2dFactory,
	ID2D1DeviceContext* d2dDC,
	IWICImagingFactory2* wicImgFactory,
	const nlohmann::json& props,
	float fillScale,
	std::pair<float, float>& scale,
	ComPtr<ID2D1Effect>& effect
) {
	const auto& e = props.value("effect", "");
	if (e == "CAS") {
		return CreateCASEffect(d2dFactory, d2dDC, props, effect);
	} else if (e == "FsrEasu") {
		return CreateFsrEasuEffect(d2dFactory, d2dDC, props, fillScale, scale, effect);
	} else if (e == "FsrRcas") {
		return CreateFsrRcasEffect(d2dFactory, d2dDC, props, effect);
	} else {
		return E_INVALIDARG;
	}
}
