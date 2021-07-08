// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include <initguid.h>
#include "EffectDefines.h"
#include "SSimDownscalerEffect.h"
#include <boost/format.hpp>


using EffectCreateFunc = HRESULT(
	ID2D1Factory1* d2dFactory,
	ID2D1DeviceContext* d2dDC,
	IWICImagingFactory2* wicImgFactory,
	const nlohmann::json& props,
	float fillScale,
	std::pair<float, float>& scale,
	ComPtr<ID2D1Effect>& effect
);


HRESULT CreateSSimDownscalerEffect(
	ID2D1Factory1* d2dFactory,
	ID2D1DeviceContext* d2dDC,
	const nlohmann::json& props,
	float fillScale,
	std::pair<float, float>& scale,
	ComPtr<ID2D1Effect>& effect
) {
	bool isRegistered;
	HRESULT hr = EffectUtils::IsEffectRegistered(d2dFactory, CLSID_MAGPIE_SSIM_DOWNSCALER_EFFECT, isRegistered);
	if (FAILED(hr)) {
		return hr;
	}

	if (!isRegistered) {
		hr = SSimDownscalerEffect::Register(d2dFactory);
		if (FAILED(hr)) {
			return hr;
		}
	}

	ComPtr<ID2D1Effect> result;
	hr = d2dDC->CreateEffect(CLSID_MAGPIE_SSIM_DOWNSCALER_EFFECT, &result);
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
			SSimDownscalerEffect::PROP_SCALE,
			D2D1_VECTOR_2F{ scaleResult.first, scaleResult.second }
		);
		if (FAILED(hr)) {
			return hr;
		}
	}

	// DownScale 属性
	HMODULE dll = LoadLibrary(L"effects\\Common");
	if (dll == NULL) {
		return E_FAIL;
	}
	auto createEffect = (EffectCreateFunc*)GetProcAddress(dll, "CreateEffect");
	if (createEffect == NULL) {
		return E_FAIL;
	}

	std::pair<float, float> t = scale;
	ComPtr<ID2D1Effect> mitchellEffect = nullptr;
	auto s = nlohmann::to_string(*it);
	s = (boost::format(R"({
		"effect" : "mitchell",
		"scale" : %1%,
		"variant": "mitchell"
	})") % s).str();
	auto json = nlohmann::json::parse(s);
	hr = createEffect(d2dFactory, d2dDC, nullptr, json, fillScale, t, mitchellEffect);
	if (FAILED(hr)) {
		return hr;
	}

	assert(t.first == scale.first * scaleResult.first && t.second == scale.second * scaleResult.second);

	hr = result->SetValue(SSimDownscalerEffect::PROP_DOWN_SCALE_EFFECT, mitchellEffect.Get());
	if (FAILED(hr)) {
		return hr;
	}

	/*
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
	}*/

	effect = std::move(result);
	scale.first *= scaleResult.first;
	scale.second *= scaleResult.second;
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
	if (e == "SSimDownscaler") {
		return CreateSSimDownscalerEffect(d2dFactory, d2dDC, props, fillScale, scale, effect);
	} else {
		return E_INVALIDARG;
	}
}
