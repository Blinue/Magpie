#include "pch.h"
#include <initguid.h>
#include <nlohmann/json.hpp>
#include "FSRCNNXEffect.h"


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
    if (e != "FSRCNNX" && e != "FSRCNNXLineArt") {
        return E_INVALIDARG;
    }

    bool isRegistered;
    HRESULT hr = EffectUtils::IsEffectRegistered(d2dFactory, CLSID_MAGPIE_FSRCNNX_EFFECT, isRegistered);
    if (FAILED(hr)) {
        return hr;
    }

    if (!isRegistered) {
        hr = FSRCNNXEffect::Register(d2dFactory);
        if (FAILED(hr)) {
            return hr;
        }
    }

    ComPtr<ID2D1Effect> result;
    hr = d2dDC->CreateEffect(CLSID_MAGPIE_FSRCNNX_EFFECT, &result);
    if (FAILED(hr)) {
        return hr;
    }

    if (e == "FSRCNNXLineArt") {
        hr = result->SetValue(FSRCNNXEffect::PROP_USE_LINE_ART_VERSION, TRUE);
        if (FAILED(hr)) {
            return hr;
        }
    }

    effect = std::move(result);
    scale.first *= 2;
    scale.second *= 2;
    return S_OK;
}

