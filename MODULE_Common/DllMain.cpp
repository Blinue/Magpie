// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include <Initguid.h>
#include "AdaptiveSharpenEffect.h"
#include "JincScaleEffect.h"
#include "LanczosScaleEffect.h"
#include "MitchellNetravaliScaleEffect.h"
#include "PixelScaleEffect.h"
#include "ContrastAdaptiveSharpenEffect.h"

#pragma comment(lib, "dxguid.lib")


API_DECLSPEC HRESULT CreateSharpenEffect(
	ID2D1Factory1* d2dFactory,
	ID2D1DeviceContext* d2dDC,
	const nlohmann::json& props,
	ComPtr<ID2D1Effect>& effect
) {
	ComPtr<ID2D1Effect> result;
	HRESULT hr = d2dDC->CreateEffect(CLSID_D2D1Sharpen, &result);
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
		if (sharpness < 0 || sharpness > 10) {
			return E_INVALIDARG;
		}

		hr = result->SetValue(D2D1_SHARPEN_PROP_SHARPNESS, sharpness);
		if (FAILED(hr)) {
			return hr;
		}
	}

	// threshold 属性
	it = props.find("threshold");
	if (it != props.end()) {
		const auto& value = *it;
		if (!value.is_number()) {
			return E_INVALIDARG;
		}

		float threshold = value.get<float>();
		if (threshold < 0 || threshold>1) {
			return E_INVALIDARG;
		}

		hr = result->SetValue(D2D1_SHARPEN_PROP_THRESHOLD, threshold);
		if (FAILED(hr)) {
			return hr;
		}
	}

	effect = std::move(result);
	return S_OK;
}

API_DECLSPEC HRESULT CreateAdaptiveSharpenEffect(
	ID2D1Factory1* d2dFactory,
	ID2D1DeviceContext* d2dDC,
	const nlohmann::json& props,
	ComPtr<ID2D1Effect>& effect
) {
	bool isRegistered;
	HRESULT hr = EffectUtils::IsEffectRegistered(d2dFactory, CLSID_MAGPIE_ADAPTIVE_SHARPEN_EFFECT, isRegistered);
	if (FAILED(hr)) {
		return hr;
	}

	if (!isRegistered) {
		hr = AdaptiveSharpenEffect::Register(d2dFactory);
		if (FAILED(hr)) {
			return hr;
		}
	}

	ComPtr<ID2D1Effect> result;
	hr = d2dDC->CreateEffect(CLSID_MAGPIE_ADAPTIVE_SHARPEN_EFFECT, &result);
	if (FAILED(hr)) {
		return hr;
	}

	// curveHeight 属性
	auto it = props.find("curveHeight");
	if (it != props.end()) {
		const auto& value = *it;
		if (!value.is_number()) {
			return E_INVALIDARG;
		}

		float curveHeight = value.get<float>();
		if (curveHeight <= 0) {
			return E_INVALIDARG;
		}

		hr = result->SetValue(AdaptiveSharpenEffect::PROP_CURVE_HEIGHT, curveHeight);
		if (FAILED(hr)) {
			return hr;
		}
	}

	effect = std::move(result);
	return S_OK;
}

API_DECLSPEC HRESULT CreateLanczosEffect(
	ID2D1Factory1* d2dFactory,
	ID2D1DeviceContext* d2dDC,
	const nlohmann::json& props,
	float fillScale,
	std::pair<float, float>& scale,
	ComPtr<ID2D1Effect>& effect
) {
	bool isRegistered;
	HRESULT hr = EffectUtils::IsEffectRegistered(d2dFactory, CLSID_MAGPIE_LANCZOS_SCALE_EFFECT, isRegistered);
	if (FAILED(hr)) {
		return hr;
	}

	if (!isRegistered) {
		hr = LanczosScaleEffect::Register(d2dFactory);
		if (FAILED(hr)) {
			return hr;
		}
	}

	ComPtr<ID2D1Effect> result;
	hr = d2dDC->CreateEffect(CLSID_MAGPIE_LANCZOS_SCALE_EFFECT, &result);
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
			LanczosScaleEffect::PROP_SCALE,
			D2D1_VECTOR_2F{ scaleResult.first, scaleResult.second }
		);
		if (FAILED(hr)) {
			return hr;
		}
	}

	// ARStrength 属性
	it = props.find("ARStrength");
	if (it != props.end()) {
		const auto& value = *it;
		if (!value.is_number()) {
			return E_INVALIDARG;
		}

		float ARStrength = value.get<float>();
		if (ARStrength < 0 || ARStrength > 1) {
			return E_INVALIDARG;
		}

		hr = result->SetValue(LanczosScaleEffect::PROP_AR_STRENGTH, ARStrength);
		if (FAILED(hr)) {
			return hr;
		}
	}

	effect = std::move(result);
	scale.first *= scaleResult.first;
	scale.second *= scaleResult.second;
	return S_OK;
}

API_DECLSPEC HRESULT CreateMitchellEffect(
	ID2D1Factory1* d2dFactory,
	ID2D1DeviceContext* d2dDC,
	const nlohmann::json& props,
	float fillScale,
	std::pair<float, float>& scale,
	ComPtr<ID2D1Effect>& effect
) {
	bool isRegistered;
	HRESULT hr = EffectUtils::IsEffectRegistered(d2dFactory, CLSID_MAGPIE_MITCHELL_NETRAVALI_SCALE_EFFECT, isRegistered);
	if (FAILED(hr)) {
		return hr;
	}

	if (!isRegistered) {
		hr = MitchellNetravaliScaleEffect::Register(d2dFactory);
		if (FAILED(hr)) {
			return hr;
		}
	}

	ComPtr<ID2D1Effect> result;
	hr = d2dDC->CreateEffect(CLSID_MAGPIE_MITCHELL_NETRAVALI_SCALE_EFFECT, &result);
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
			MitchellNetravaliScaleEffect::PROP_SCALE,
			D2D1_VECTOR_2F{ scaleResult.first, scaleResult.second }
		);
		if (FAILED(hr)) {
			return hr;
		}
	}

	// variant 属性
	it = props.find("variant");
	if (it != props.end()) {
		if (!it->is_string()) {
			return E_INVALIDARG;
		}

		std::string_view variant = *it;
		int v = 0;
		if (variant == "mitchell") {
			v = 0;
		} else if (variant == "catrom") {
			v = 1;
		} else if (variant == "sharper") {
			v = 2;
		} else {
			return E_INVALIDARG;
		}

		hr = result->SetValue(MitchellNetravaliScaleEffect::PROP_VARIANT, v);
		if (FAILED(hr)) {
			return hr;
		}
	}

	effect = std::move(result);
	scale.first *= scaleResult.first;
	scale.second *= scaleResult.second;
	return S_OK;
}

API_DECLSPEC HRESULT CreatePixelScaleEffect(
	ID2D1Factory1* d2dFactory,
	ID2D1DeviceContext* d2dDC,
	const nlohmann::json& props,
	ComPtr<ID2D1Effect>& effect,
	std::pair<float, float>& scale
) {
	bool isRegistered;
	HRESULT hr = EffectUtils::IsEffectRegistered(d2dFactory, CLSID_MAGPIE_PIXEL_SCALE_EFFECT, isRegistered);
	if (FAILED(hr)) {
		return hr;
	}

	if (!isRegistered) {
		hr = PixelScaleEffect::Register(d2dFactory);
		if (FAILED(hr)) {
			return hr;
		}
	}

	ComPtr<ID2D1Effect> result;
	hr = d2dDC->CreateEffect(CLSID_MAGPIE_PIXEL_SCALE_EFFECT, &result);
	if (FAILED(hr)) {
		return hr;
	}

	int scaleResult = 1;
	// scale 属性
	auto it = props.find("scale");
	if (it != props.end()) {
		if (!it->is_number()) {
			return E_INVALIDARG;
		}
		scaleResult = *it;
		if (scaleResult <= 0) {
			return E_INVALIDARG;
		}

		hr = result->SetValue(PixelScaleEffect::PROP_SCALE, scaleResult);
		if (FAILED(hr)) {
			return hr;
		}
	}

	effect = std::move(result);
	scale.first *= scaleResult;
	scale.second *= scaleResult;
	return S_OK;
}

API_DECLSPEC HRESULT CreateContrastAdaptiveSharpenEffect(
	ID2D1Factory1* d2dFactory,
	ID2D1DeviceContext* d2dDC,
	const nlohmann::json& props,
	ComPtr<ID2D1Effect>& effect
) {
	bool isRegistered;
	HRESULT hr = EffectUtils::IsEffectRegistered(d2dFactory, CLSID_MAGPIE_CONTRAST_ADAPTIVE_SHARPEN_EFFECT, isRegistered);
	if (FAILED(hr)) {
		return hr;
	}

	if (!isRegistered) {
		hr = ContrastAdaptiveSharpenEffect::Register(d2dFactory);
		if (FAILED(hr)) {
			return hr;
		}
	}

	ComPtr<ID2D1Effect> result;
	hr = d2dDC->CreateEffect(CLSID_MAGPIE_CONTRAST_ADAPTIVE_SHARPEN_EFFECT, &result);
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

		hr = result->SetValue(ContrastAdaptiveSharpenEffect::PROP_SHARPNESS, sharpness);
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
	if (e == "sharpen") {
		return CreateSharpenEffect(d2dFactory, d2dDC, props, effect);
	} else if (e == "adaptiveSharpen") {
		return CreateAdaptiveSharpenEffect(d2dFactory, d2dDC, props, effect);
	} else if (e == "CAS") {
		return CreateContrastAdaptiveSharpenEffect(d2dFactory, d2dDC, props, effect);
	} else if (e == "lanczos") {
		return CreateLanczosEffect(d2dFactory, d2dDC, props, fillScale, scale, effect);
	} else if (e == "mitchell") {
		return CreateMitchellEffect(d2dFactory, d2dDC, props, fillScale, scale, effect);
	} else if (e == "pixelScale") {
		return CreatePixelScaleEffect(d2dFactory, d2dDC, props, effect, scale);
	} else  {
		return E_INVALIDARG;
	}
}
