#pragma once
#include "pch.h"
#include "GUIDs.h"
#include "Shaders.h"
#include "SimpleDrawTransform.h"
#include "AdaptiveSharpenPass2Transform.h"
#include "EffectBase.h"
#include <d2d1effecthelpers.h>


// Adaptive sharpen 算法
class AdaptiveSharpenEffect : public EffectBase {
public:
    IFACEMETHODIMP Initialize(
        _In_ ID2D1EffectContext* pEffectContext,
        _In_ ID2D1TransformGraph* pTransformGraph
    ) {
        HRESULT hr = SimpleDrawTransform::Create(
            pEffectContext, 
            &_pass1Transform, 
            MAGPIE_ADAPTIVE_SHARPEN_PASS1_SHADER, 
            GUID_MAGPIE_ADAPTIVE_SHARPEN_PASS1_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = AdaptiveSharpenPass2Transform::Create(
            pEffectContext, 
            &_pass2Transform
        );
        if (FAILED(hr)) {
            return hr;
        }

        hr = pTransformGraph->AddNode(_pass1Transform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = pTransformGraph->AddNode(_pass2Transform.Get());
        if (FAILED(hr)) {
            return hr;
        }

        hr = pTransformGraph->ConnectToEffectInput(0, _pass1Transform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = pTransformGraph->ConnectNode(_pass1Transform.Get(), _pass2Transform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = pTransformGraph->SetOutputNode(_pass2Transform.Get());
        if (FAILED(hr)) {
            return hr;
        }

        return S_OK;
    }

    HRESULT SetStrength(FLOAT value) {
        if (value < 0 || value > 1) {
            return E_INVALIDARG;
        }

        // 将 0~1 映射到 0.3~2
        _pass2Transform->SetCurveHeight(value * 1.7f + 0.3f);
        return S_OK;
    }

    FLOAT GetStrength() const {
        // 将 0.3~2 映射到 0~1
        return (_pass2Transform->GetCurveHeight() - 0.3f) / 1.7f;
    }

    enum PROPS {
        // FLOAT 类型。指示锐化强度，取值范围为 0~1，默认值为 0
        PROP_STRENGTH = 0
    };

    static HRESULT Register(_In_ ID2D1Factory1* pFactory) {
        const D2D1_PROPERTY_BINDING bindings[] =
        {
            D2D1_VALUE_TYPE_BINDING(L"Strength", &SetStrength, &GetStrength),
        };

        HRESULT hr = pFactory->RegisterEffectFromString(CLSID_MAGPIE_ADAPTIVE_SHARPEN_EFFECT, XML(
            <?xml version='1.0'?>
            <Effect>
                <!--System Properties-->
                <Property name='DisplayName' type='string' value='Adaptive Sharpen' />
                <Property name='Author' type='string' value='Xu Liu' />
                <Property name='Category' type='string' value='Sharpen' />
                <Property name='Description' type='string' value='Adaptive Sharpen' />
                <Inputs>
                    <Input name='Source' />
                </Inputs>
                <Property name='Strength' type='float'>
                    <Property name='DisplayName' type='string' value='Strength' />
                    <Property name='Default' type='float' value='0' />
                    <Property name='Min' type='float' value='0' />
                    <Property name='Max' type='float' value='1' />
                </Property>
            </Effect>
        ), bindings, ARRAYSIZE(bindings), CreateEffect);

        return hr;
    }

    static HRESULT CALLBACK CreateEffect(_Outptr_ IUnknown** ppEffectImpl) {
        *ppEffectImpl = static_cast<ID2D1EffectImpl*>(new AdaptiveSharpenEffect());

        if (*ppEffectImpl == nullptr) {
            return E_OUTOFMEMORY;
        }

        return S_OK;
    }


private:
    AdaptiveSharpenEffect() {}

    ComPtr<SimpleDrawTransform> _pass1Transform = nullptr;
    ComPtr<AdaptiveSharpenPass2Transform> _pass2Transform = nullptr;
};
