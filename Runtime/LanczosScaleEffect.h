#pragma once
#include "pch.h"
#include "LanczosScaleTransform.h"
#include "EffectBase.h"
#include <d2d1effecthelpers.h>


class LanczosScaleEffect : public EffectBase {
public:
    IFACEMETHODIMP Initialize(
        _In_ ID2D1EffectContext* pEffectContext,
        _In_ ID2D1TransformGraph* pTransformGraph
    ) {
        HRESULT hr = LanczosScaleTransform::Create(pEffectContext, &_transform);
        if (FAILED(hr)) {
            return hr;
        }

        hr = pTransformGraph->SetSingleTransformNode(_transform.Get());
        if (FAILED(hr)) {
            return hr;
        }

        return S_OK;
    }

    HRESULT SetScale(D2D_VECTOR_2F value) {
        if (value.x <= 0 || value.y <= 0) {
            return E_INVALIDARG;
        }

        _transform->SetScale(value);
        return S_OK;
    }

    D2D_VECTOR_2F GetScale() const {
        return _transform->GetScale();
    }

    HRESULT SetARStrength(FLOAT value) {
        if (value < 0 || value > 1) {
            return E_INVALIDARG;
        }

        _transform->SetARStrength(value);
        return S_OK;
    }

    FLOAT GetARStrength() const {
        return _transform->GetARStrength();
    }

    enum PROPS {
        PROP_SCALE = 0,
        // 抗振铃强度。必须在 0~1 之间。默认值为 0.5
        PROP_AR_STRENGTH = 1
    };

    static HRESULT Register(_In_ ID2D1Factory1* pFactory) {
        const D2D1_PROPERTY_BINDING bindings[] =
        {
            D2D1_VALUE_TYPE_BINDING(L"Scale", &SetScale, &GetScale),
            D2D1_VALUE_TYPE_BINDING(L"ARStrength", &SetARStrength, &GetARStrength)
        };

        HRESULT hr = pFactory->RegisterEffectFromString(CLSID_MAGPIE_LANCZOS_SCALE_EFFECT, XML(
            <?xml version='1.0'?>
            <Effect>
                <!--System Properties-->
                <Property name='DisplayName' type='string' value='Lanczos6 Scale' />
                <Property name='Author' type='string' value='Blinue' />
                <Property name='Category' type='string' value='Scale' />
                <Property name='Description' type='string' value='Lanczos6 scale algorithm' />
                <Inputs>
                    <Input name='Source' />
                </Inputs>
                <Property name='Scale' type='vector2'>
                    <Property name='DisplayName' type='string' value='Scale' />
                    <Property name='Default' type='vector2' value='(1,1)' />
                </Property>
                <Property name='ARStrength' type='float'>
                    <Property name='DisplayName' type='string' value='ARStrength' />
                    <Property name='Default' type= 'float' value='0.5' />
                    <Property name='Min' type='float' value='0' />
                    <Property name='Max' type='float' value='1.0' />
                </Property>
            </Effect>
        ), bindings, ARRAYSIZE(bindings), CreateEffect);

        return hr;
    }

    static HRESULT CALLBACK CreateEffect(_Outptr_ IUnknown** ppEffectImpl) {
        *ppEffectImpl = static_cast<ID2D1EffectImpl*>(new LanczosScaleEffect());

        if (*ppEffectImpl == nullptr) {
            return E_OUTOFMEMORY;
        }

        return S_OK;
    }

private:
    LanczosScaleEffect() {}

    ComPtr<LanczosScaleTransform> _transform = nullptr;
};
