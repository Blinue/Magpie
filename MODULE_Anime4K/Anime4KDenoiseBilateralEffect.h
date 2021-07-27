#pragma once
#include "pch.h"
#include <SimpleDrawTransform.h>
#include "Anime4KDenoiseBilateralTransform.h"
#include <EffectBase.h>
#include "EffectDefines.h"


class Anime4KDenoiseBilateralEffect : public EffectBase {
public:
    IFACEMETHODIMP Initialize(
        _In_ ID2D1EffectContext* effectContext,
        _In_ ID2D1TransformGraph* transformGraph
    ) {
        HRESULT hr = SimpleDrawTransform<>::Create(
            effectContext,
            &_rgb2yuvTransform,
            MAGPIE_RGB2YUV_SHADER,
            GUID_MAGPIE_RGB2YUV_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = Anime4KDenoiseBilateralTransform::Create(effectContext, &_denoiseBilateralTransform);
        if (FAILED(hr)) {
            return hr;
        }

        hr = transformGraph->AddNode(_rgb2yuvTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_denoiseBilateralTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }

        hr = transformGraph->ConnectToEffectInput(0, _rgb2yuvTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_rgb2yuvTransform.Get(), _denoiseBilateralTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->SetOutputNode(_denoiseBilateralTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }

        return S_OK;
    }

    HRESULT SetVariant(INT32 value) {
        if (value < 0 || value > 2) {
            return E_INVALIDARG;
        }

        _denoiseBilateralTransform->SetVariant(value);
        return S_OK;
    }

    INT32 GetVariant() const {
        return _denoiseBilateralTransform->GetVariant();
    }

    HRESULT SetIntensity(FLOAT value) {
        if (value <= 0) {
            return E_INVALIDARG;
        }

        _denoiseBilateralTransform->SetIntensitySigma(value);
        return S_OK;
    }

    FLOAT GetIntensity() const {
        return _denoiseBilateralTransform->GetIntensitySigma();
    }

    enum PROPS {
        PROP_VARIANT = 0,   // 降噪变体种类，0: mode, 1: median, 2: mean，默认值为 0
        PROP_INTENSITY = 1  // 降噪强度，越大强度越高，必须大于0，默认值为 0.1
    };

    static HRESULT Register(_In_ ID2D1Factory1* pFactory) {
        const D2D1_PROPERTY_BINDING bindings[] =
        {
            D2D1_VALUE_TYPE_BINDING(L"Variant", &SetVariant, &GetVariant),
            D2D1_VALUE_TYPE_BINDING(L"Intensity", &SetIntensity, &GetIntensity)
        };

        HRESULT hr = pFactory->RegisterEffectFromString(CLSID_MAGPIE_ANIME4K_DENOISE_BILATERAL_EFFECT, XML(
            <?xml version='1.0'?>
            <Effect>
                <!--System Properties-->
                <Property name='DisplayName' type='string' value='Anime4K Denoise Bilater'/>
                <Property name='Author' type='string' value='Blinue'/>
                <Property name='Category' type='string' value='Misc'/>
                <Property name='Description' type='string' value='Anime4K Denoise Bilater'/>
                <Inputs>
                    <Input name='Source'/>
                </Inputs>
                <Property name='Variant' type='int32'>
                    <Property name='DisplayName' type='string' value='Variant'/>
                    <Property name='Default' type='int32' value='0'/>
                    <Property name='Min' type='int32' value='0'/>
                    <Property name='Max' type='int32' value='2'/>
                </Property>
                <Property name='Intensity' type='float'>
                    <Property name='DisplayName' type='string' value='Intensity'/>
                    <Property name='Default' type='float' value='0.1' />
                </Property>
            </Effect>
        ), bindings, ARRAYSIZE(bindings), CreateEffect);

        return hr;
    }

    static HRESULT CALLBACK CreateEffect(_Outptr_ IUnknown** ppEffectImpl) {
        // This code assumes that the effect class initializes its reference count to 1.
        *ppEffectImpl = static_cast<ID2D1EffectImpl*>(new Anime4KDenoiseBilateralEffect());

        if (*ppEffectImpl == nullptr) {
            return E_OUTOFMEMORY;
        }

        return S_OK;
    }

private:
    // Constructor should be private since it should never be called externally.
    Anime4KDenoiseBilateralEffect() {}

    ComPtr<SimpleDrawTransform<>> _rgb2yuvTransform = nullptr;
    ComPtr<Anime4KDenoiseBilateralTransform> _denoiseBilateralTransform = nullptr;
};
