// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include <Initguid.h>
#include "Anime4KEffect.h"
#include "Anime4KDenoiseBilateralEffect.h"
#include "Anime4KDarkLinesEffect.h"
#include "Anime4KThinLinesEffect.h"
#include <nlohmann/json.hpp>


HRESULT CreateAnime4KEffect(
	ID2D1Factory1* d2dFactory,
	ID2D1DeviceContext* d2dDC,
	const nlohmann::json& props,
	ComPtr<ID2D1Effect>& effect,
	std::pair<float, float>& scale
) {
	bool isRegistered;
	HRESULT hr = EffectUtils::IsEffectRegistered(d2dFactory, CLSID_MAGPIE_ANIME4K_EFFECT, isRegistered);
	if (FAILED(hr)) {
		return hr;
	}

	if (!isRegistered) {
		hr = Anime4KEffect::Register(d2dFactory);
		if (FAILED(hr)) {
			return hr;
		}
	}

	ComPtr<ID2D1Effect> result;
	hr = d2dDC->CreateEffect(CLSID_MAGPIE_ANIME4K_EFFECT, &result);
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
		if (curveHeight < 0) {
			return E_INVALIDARG;
		}

		hr = result->SetValue(Anime4KEffect::PROP_CURVE_HEIGHT, curveHeight);
		if (FAILED(hr)) {
			return hr;
		}
	}

	// useDenoiseVersion 属性
	it = props.find("useDenoiseVersion");
	if (it != props.end()) {
		const auto& val = *it;
		if (!val.is_boolean()) {
			return E_INVALIDARG;
		}

		hr = result->SetValue(Anime4KEffect::PROP_USE_DENOISE_VERSION, (BOOL)val.get<bool>());
		if (FAILED(hr)) {
			return hr;
		}
	}

	effect = std::move(result);
	scale = { 2.0f,2.0f };

	return S_OK;
}

HRESULT CreateDarkLinesEffect(
	ID2D1Factory1* d2dFactory,
	ID2D1DeviceContext* d2dDC,
	const nlohmann::json& props,
	ComPtr<ID2D1Effect>& effect,
	std::pair<float, float>& scale
) {
	bool isRegistered;
	HRESULT hr = EffectUtils::IsEffectRegistered(d2dFactory, CLSID_MAGPIE_ANIME4K_DARKLINES_EFFECT, isRegistered);
	if (FAILED(hr)) {
		return hr;
	}

	if (!isRegistered) {
		hr = Anime4KDarkLinesEffect::Register(d2dFactory);
		if (FAILED(hr)) {
			return hr;
		}
	}

	ComPtr<ID2D1Effect> result;
	hr = d2dDC->CreateEffect(CLSID_MAGPIE_ANIME4K_DARKLINES_EFFECT, &result);
	if (FAILED(hr)) {
		return hr;
	}

	// strength 属性
	auto it = props.find("strength");
	if (it != props.end()) {
		const auto& value = *it;
		if (!value.is_number()) {
			return E_INVALIDARG;
		}

		float strength = value.get<float>();
		if (strength <= 0) {
			return E_INVALIDARG;
		}

		hr = result->SetValue(Anime4KDarkLinesEffect::PROP_STRENGTH, strength);
		if (FAILED(hr)) {
			return hr;
		}
	}

	effect = std::move(result);
	scale = { 1.0f,1.0f };

	return S_OK;
}

HRESULT CreateThinLinesEffect(
	ID2D1Factory1* d2dFactory,
	ID2D1DeviceContext* d2dDC,
	const nlohmann::json& props,
	ComPtr<ID2D1Effect>& effect,
	std::pair<float, float>& scale
) {
	bool isRegistered;
	HRESULT hr = EffectUtils::IsEffectRegistered(d2dFactory, CLSID_MAGPIE_ANIME4K_THINLINES_EFFECT, isRegistered);
	if (FAILED(hr)) {
		return hr;
	}

	if (!isRegistered) {
		hr = Anime4KThinLinesEffect::Register(d2dFactory);
		if (FAILED(hr)) {
			return hr;
		}
	}

	ComPtr<ID2D1Effect> result;
	hr = d2dDC->CreateEffect(CLSID_MAGPIE_ANIME4K_THINLINES_EFFECT, &result);
	if (FAILED(hr)) {
		return hr;
	}

	// strength 属性
	auto it = props.find("strength");
	if (it != props.end()) {
		const auto& value = *it;
		if (!value.is_number()) {
			return E_INVALIDARG;
		}

		float strength = value.get<float>();
		if (strength <= 0) {
			return E_INVALIDARG;
		}

		hr = result->SetValue(Anime4KThinLinesEffect::PROP_STRENGTH, strength);
		if (FAILED(hr)) {
			return hr;
		}
	}

	effect = std::move(result);
	scale = { 1.0f,1.0f };

	return S_OK;
}

HRESULT CreateDenoiseBilateralEffect(
	ID2D1Factory1* d2dFactory,
	ID2D1DeviceContext* d2dDC,
	const nlohmann::json& props,
	ComPtr<ID2D1Effect>& effect,
	std::pair<float, float>& scale
) {
	bool isRegistered;
	HRESULT hr = EffectUtils::IsEffectRegistered(d2dFactory, CLSID_MAGPIE_ANIME4K_DENOISE_BILATERAL_EFFECT, isRegistered);
	if (FAILED(hr)) {
		return hr;
	}

	if (!isRegistered) {
		hr = Anime4KDenoiseBilateralEffect::Register(d2dFactory);
		if (FAILED(hr)) {
			return hr;
		}
	}

	ComPtr<ID2D1Effect> result;
	hr = d2dDC->CreateEffect(CLSID_MAGPIE_ANIME4K_DENOISE_BILATERAL_EFFECT, &result);
	if (FAILED(hr)) {
		return hr;
	}

	// variant 属性
	auto it = props.find("variant");
	if (it != props.end()) {
		if (!it->is_string()) {
			return E_INVALIDARG;
		}

		std::string_view variant = *it;
		int v = 0;
		if (variant == "mode") {
			v = 0;
		} else if (variant == "median") {
			v = 1;
		} else if (variant == "mean") {
			v = 2;
		} else {
			return E_INVALIDARG;
		}

		hr = result->SetValue(Anime4KDenoiseBilateralEffect::PROP_VARIANT, v);
		if (FAILED(hr)) {
			return hr;
		}
	}

	// intensity 属性
	it = props.find("intensity");
	if (it != props.end()) {
		const auto& value = *it;
		if (!it->is_number()) {
			return E_INVALIDARG;
		}

		float intensity = value.get<float>();
		if (intensity <= 0) {
			return E_INVALIDARG;
		}

		hr = result->SetValue(Anime4KDenoiseBilateralEffect::PROP_INTENSITY, intensity);
		if (FAILED(hr)) {
			return hr;
		}
	}
	
	effect = std::move(result);
	scale = { 1.0f,1.0f };

	return S_OK;
}


API_DECLSPEC HRESULT CreateEffect(
	ID2D1Factory1* d2dFactory,
	ID2D1DeviceContext* d2dDC,
	IWICImagingFactory2* wicImgFactory,
	const nlohmann::json& props,
	ComPtr<ID2D1Effect>& effect,
	std::pair<float, float>& scale
) {
	const auto& e = props.value("effect", "");
	if (e == "Anime4K") {
		return CreateAnime4KEffect(d2dFactory, d2dDC, props, effect, scale);
	} else if (e == "darkLines") {
		return CreateDarkLinesEffect(d2dFactory, d2dDC, props, effect, scale);
	} else if (e == "thinLines") {
		return CreateThinLinesEffect(d2dFactory, d2dDC, props, effect, scale);
	} else if (e == "denoiseBilateral") {
		return CreateDenoiseBilateralEffect(d2dFactory, d2dDC, props, effect, scale);
	} else {
		return E_INVALIDARG;
	}
}