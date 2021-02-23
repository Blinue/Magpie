#pragma once
#include "pch.h"
#include "GUIDs.h"
#include "Jinc2Transform.h"
#include "EffectBase.h"
#include <d2d1effecthelpers.h>


class Jinc2ScaleEffect : public EffectBase {
public:
    IFACEMETHODIMP Initialize(
        _In_ ID2D1EffectContext* pEffectContext,
        _In_ ID2D1TransformGraph* pTransformGraph
    ) {
        HRESULT hr = SimpleScaleTransform::Create(
            pEffectContext, &_transform, 
            MAGPIE_JINC2_SCALE_SHADER, 
            GUID_MAGPIE_JINC2_SCALE_SHADER);
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

    enum PROPS {
        PROP_SCALE = 0
    };

    static HRESULT Register(_In_ ID2D1Factory1* pFactory) {
        const D2D1_PROPERTY_BINDING bindings[] =
        {
            D2D1_VALUE_TYPE_BINDING(L"Scale", &SetScale, &GetScale),
        };

        HRESULT hr = pFactory->RegisterEffectFromString(CLSID_MAGPIE_JINC2_SCALE_EFFECT, XML(
            <?xml version='1.0'?>
            <Effect>
                <!--System Properties-->
                <Property name='DisplayName' type='string' value='Ripple'/>
                <Property name='Author' type='string' value='Microsoft Corporation'/>
                <Property name='Category' type='string' value='Stylize'/>
                <Property name='Description' type='string' value='Adds a ripple effect that can be animated'/>
                <Inputs>
                    <Input name='Source'/>
                </Inputs>
                <Property name='Scale' type='vector2'>
                    <Property name='DisplayName' type='string' value='Scale'/>
                    <Property name='Default' type='vector2' value='(1,1)'/>
                </Property>
            </Effect>
        ), bindings, ARRAYSIZE(bindings), CreateEffect);

        return hr;
    }

    static HRESULT CALLBACK CreateEffect(_Outptr_ IUnknown** ppEffectImpl) {
        *ppEffectImpl = static_cast<ID2D1EffectImpl*>(new Jinc2ScaleEffect());

        if (*ppEffectImpl == nullptr) {
            return E_OUTOFMEMORY;
        }

        return S_OK;
    }

private:
    Jinc2ScaleEffect() {}

    ComPtr<SimpleScaleTransform> _transform = nullptr;
};
