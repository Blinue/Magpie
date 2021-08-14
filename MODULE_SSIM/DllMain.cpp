// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include <initguid.h>
#include "EffectDefines.h"
#include "SSimDownscalerEffect.h"
#include "SSimSuperResEffect.h"
#include <EffectUtils.h>
#include <fmt/xchar.h>


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

	// DownScaleEffect 属性
	hr = SSimDownscalerEffect::CreateAndSetDownScaleEffect(result.Get(), d2dFactory, d2dDC, scaleResult);
	if (FAILED(hr)) {
		return hr;
	}

	effect = std::move(result);
	scale.first *= scaleResult.first;
	scale.second *= scaleResult.second;
	return S_OK;
}

HRESULT CreateSSimSuperResEffect(
	ID2D1Factory1* d2dFactory,
	ID2D1DeviceContext* d2dDC,
	IWICImagingFactory2* wicImgFactory,
	const nlohmann::json& props,
	float fillScale,
	std::pair<float, float>& scale,
	ComPtr<ID2D1Effect>& effect
) {
	bool isRegistered;
	HRESULT hr = EffectUtils::IsEffectRegistered(d2dFactory, CLSID_MAGPIE_SSIM_SUPERRES_EFFECT, isRegistered);
	if (FAILED(hr)) {
		return hr;
	}

	if (!isRegistered) {
		hr = SSimSuperResEffect::Register(d2dFactory);
		if (FAILED(hr)) {
			return hr;
		}
	}

	ComPtr<ID2D1Effect> result;
	hr = d2dDC->CreateEffect(CLSID_MAGPIE_SSIM_SUPERRES_EFFECT, &result);
	if (FAILED(hr)) {
		return hr;
	}

	// upScaleEffect 属性
	auto it = props.find("upScaleEffect");
	if (it == props.end()) {
		return E_INVALIDARG;
	}

	const auto& moduleName = it->value("module", "");
	if (moduleName.empty()) {
		return E_INVALIDARG;
	}

	std::wstring moduleNameW;
	hr = EffectUtils::UTF8ToUTF16(moduleName, moduleNameW);
	
	HMODULE dll = LoadLibrary((fmt::format(L"effects\\{}", moduleNameW)).c_str());
	if (!dll) {
		return E_INVALIDARG;
	}

	using EffectCreateFunc = HRESULT(
		ID2D1Factory1* d2dFactory,
		ID2D1DeviceContext* d2dDC,
		IWICImagingFactory2* wicImgFactory,
		const nlohmann::json& props,
		float fillScale,
		std::pair<float, float>& scale,
		ComPtr<ID2D1Effect>& effect
	);
	auto createEffect = (EffectCreateFunc*)GetProcAddress(dll, "CreateEffect");
	if (createEffect == NULL) {
		return E_FAIL;
	}

	std::pair<float, float> scaleResult = scale;	// 缩放倍率为 upScaleEffect 的缩放倍率
	ComPtr<ID2D1Effect> upScaleEffect;
	hr = createEffect(d2dFactory, d2dDC, wicImgFactory, *it, fillScale, scaleResult, upScaleEffect);
	if (FAILED(hr)) {
		return hr;
	}

	hr = result->SetValue(SSimSuperResEffect::PROP_UP_SCALE_EFFECT, upScaleEffect.Get());
	if (FAILED(hr)) {
		return hr;
	}

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
	} else if (e == "SSimSuperRes") {
		return CreateSSimSuperResEffect(d2dFactory, d2dDC, wicImgFactory, props, fillScale, scale, effect);
	} else {
		return E_INVALIDARG;
	}
}
