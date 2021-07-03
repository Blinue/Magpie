// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include <Initguid.h>
#include "EffectDefines.h"
#include <nlohmann/json.hpp>
#include "RAVULiteEffect.h"
#include "RAVUZoomEffect.h"


HINSTANCE hInst = NULL;

// DLL 入口
BOOL APIENTRY DllMain(
    HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        hInst = hModule;
        break;
    case DLL_PROCESS_DETACH:
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}


HRESULT CreateLiteEffect(
    ID2D1Factory1* d2dFactory,
    ID2D1DeviceContext* d2dDC,
    const nlohmann::json& props,
    ComPtr<ID2D1Effect>& effect,
    std::pair<float, float>& scale
) {
    bool isRegistered;
    HRESULT hr = EffectUtils::IsEffectRegistered(d2dFactory, CLSID_MAGPIE_RAVU_LITE_EFFECT, isRegistered);
    if (FAILED(hr)) {
        return hr;
    }

    if (!isRegistered) {
        hr = RAVULiteEffect::Register(d2dFactory);
        if (FAILED(hr)) {
            return hr;
        }
    }

    ComPtr<ID2D1Effect> result;
    hr = d2dDC->CreateEffect(CLSID_MAGPIE_RAVU_LITE_EFFECT, &result);
    if (FAILED(hr)) {
        return hr;
    }

    effect = std::move(result);
    scale.first *= 2;
    scale.second *= 2;
    return S_OK;
}

HRESULT CreateZoomEffect(
    ID2D1Factory1* d2dFactory,
    ID2D1DeviceContext* d2dDC,
    IWICImagingFactory2* wicImgFactory,
    const nlohmann::json& props,
    float fillScale,
    std::pair<float, float>& scale,
    ComPtr<ID2D1Effect>& effect
) {
    bool isRegistered;
    HRESULT hr = EffectUtils::IsEffectRegistered(d2dFactory, CLSID_MAGPIE_RAVU_ZOOM_EFFECT, isRegistered);
    if (FAILED(hr)) {
        return hr;
    }

    if (!isRegistered) {
        hr = RAVUZoomEffect::Register(d2dFactory);
        if (FAILED(hr)) {
            return hr;
        }
    }

    ComPtr<ID2D1Effect> result;
    hr = d2dDC->CreateEffect(CLSID_MAGPIE_RAVU_ZOOM_EFFECT, &result);
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

        hr = result->SetValue(RAVUZoomEffect::PROP_SCALE, D2D1_VECTOR_2F{ scaleResult.first, scaleResult.second });
        if (FAILED(hr)) {
            return hr;
        }
    }

    // 设置权重纹理
    hr = RAVUZoomEffect::LoadWeights(result.Get(), hInst, wicImgFactory, d2dDC);
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
    if (e == "RAVULite") {
        return CreateLiteEffect(d2dFactory, d2dDC, props, effect, scale);
    } else if (e == "RAVUZoom") {
        return CreateZoomEffect(d2dFactory, d2dDC, wicImgFactory, props, fillScale, scale, effect);
    } else {
        return E_INVALIDARG;
    }
}
