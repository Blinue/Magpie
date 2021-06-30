// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include <Initguid.h>
#include "ACNetEffect.h"
#include <nlohmann/json.hpp>


API_DECLSPEC HRESULT CreateEffect(
    ID2D1Factory1* d2dFactory,
    ID2D1DeviceContext* d2dDC,
    IWICImagingFactory2* wicImgFactory,
    const nlohmann::json& props,
    ComPtr<ID2D1Effect>& effect,
    std::pair<float, float>& scale
) {
    bool isRegistered;
    HRESULT hr = EffectUtils::IsEffectRegistered(d2dFactory, CLSID_MAGPIE_ACNET_EFFECT, isRegistered);
    if (FAILED(hr)) {
        return hr;
    }

    if (!isRegistered) {
        hr = ACNetEffect::Register(d2dFactory);
        if (FAILED(hr)) {
            return hr;
        }
    }

    ComPtr<ID2D1Effect> result;
    hr = d2dDC->CreateEffect(CLSID_MAGPIE_ACNET_EFFECT, &result);
    if (FAILED(hr)) {
        return hr;
    }

    effect = std::move(result);
    scale = { 2.0f, 2.0f };
    return S_OK;
}
