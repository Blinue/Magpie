// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include <Initguid.h>
#include "EffectDefines.h"
#include <nlohmann/json.hpp>
#include "RAVULiteEffect.h"
#include "RAVUZoomEffect.h"
#include "resource.h"


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
    scale = { 2.0f,2.0f };
    return S_OK;
}

HRESULT CreateZoomEffect(
    ID2D1Factory1* d2dFactory,
    ID2D1DeviceContext* d2dDC,
    IWICImagingFactory2* wicImgFactory,
    const nlohmann::json& props,
    ComPtr<ID2D1Effect>& effect,
    std::pair<float, float>& scale
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

    std::pair<float, float> scaleResult{ 1.0f,1.0f };
    // scale 属性
    auto it = props.find("scale");
    if (it != props.end()) {
        hr = EffectUtils::ReadScaleProp(*it, scaleResult);
        if (FAILED(hr)) {
            return hr;
        }

        hr = result->SetValue(RAVUZoomEffect::PROP_SCALE, scaleResult);
        if (FAILED(hr)) {
            return hr;
        }
    }

    // 设置权重纹理
    HBITMAP hBmp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_RAVU_ZOOM_R3_WEIGHTS));
    ComPtr<ID2D1Bitmap> weights;
    hr = EffectUtils::LoadBitmapFromHBmp(wicImgFactory, d2dDC, hBmp, weights);
    if (FAILED(hr)) {
        return hr;
    }
    result->SetInput(1, weights.Get());

    effect = std::move(result);
    scale = scaleResult;
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
    if (e == "lite") {
        return CreateLiteEffect(d2dFactory, d2dDC, props, effect, scale);
    } else if (e == "zoom") {
        return CreateZoomEffect(d2dFactory, d2dDC, wicImgFactory, props, effect, scale);
    } else {
        return E_INVALIDARG;
    }
}
