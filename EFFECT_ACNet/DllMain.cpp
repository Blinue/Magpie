// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "ACNetEffect.h"
#include <nlohmann/json.hpp>



API_DECLSPEC HRESULT Initialize(ID2D1Factory1* factory) {
    return ACNetEffect::Register(factory);
}

API_DECLSPEC HRESULT Create(ID2D1DeviceContext* d2dDC, const nlohmann::json& props, ID2D1Effect** effect, std::pair<float,float>& scale) {
    HRESULT hr = d2dDC->CreateEffect(CLSID_MAGPIE_ACNET_EFFECT, effect);
    if (FAILED(hr)) {
        return hr;
    }

    scale = { 2.0f,2.0f };
    return S_OK;
}
