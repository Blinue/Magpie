#pragma once
#include "pch.h"
#include <EffectBase.h>
#include "EffectDefines.h"
#include "FfxEasuTransform.h"
#include "FfxRcasTransform.h"


class FSREffect : public EffectBase {
public:
    IFACEMETHODIMP Initialize(
        _In_ ID2D1EffectContext* pEffectContext,
        _In_ ID2D1TransformGraph* pTransformGraph
    ) {
        HRESULT hr = FfxEasuTransform::Create(pEffectContext, &_easuTransform);
        if (FAILED(hr)) {
            return hr;
        }
        hr = FfxRcasTransform::Create(pEffectContext, &_rcasTransform);
        if (FAILED(hr)) {
            return hr;
        }

        hr = pTransformGraph->AddNode(_easuTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = pTransformGraph->AddNode(_rcasTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }

        hr = pTransformGraph->ConnectToEffectInput(0, _easuTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = pTransformGraph->ConnectNode(_easuTransform.Get(), _rcasTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = pTransformGraph->SetOutputNode(_rcasTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }

        return S_OK;
    }

    HRESULT SetScale(D2D_VECTOR_2F value) {
        if (value.x <= 0 || value.y <= 0) {
            return E_INVALIDARG;
        }

        _easuTransform->SetScale(value);
        return S_OK;
    }

    D2D_VECTOR_2F GetScale() const {
        return _easuTransform->GetScale();
    }

    enum PROPS {
        PROP_SCALE = 0,
    };

    static HRESULT Register(_In_ ID2D1Factory1* pFactory) {
        const D2D1_PROPERTY_BINDING bindings[] =
        {
            D2D1_VALUE_TYPE_BINDING(L"Scale", &SetScale, &GetScale)
        };

        HRESULT hr = pFactory->RegisterEffectFromString(CLSID_MAGPIE_FSR_EFFECT, XML(
            <?xml version='1.0'?>
            <Effect>
            <!--System Properties-->
            <Property name='DisplayName' type='string' value='FSR'/>
            <Property name='Author' type='string' value='Blinue'/>
            <Property name='Category' type='string' value='FFX'/>
            <Property name='Description' type='string' value='FSR'/>
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
        *ppEffectImpl = static_cast<ID2D1EffectImpl*>(new FSREffect());

        if (*ppEffectImpl == nullptr) {
            return E_OUTOFMEMORY;
        }

        return S_OK;
    }

private:
    FSREffect() {}

    ComPtr<FfxEasuTransform> _easuTransform = nullptr;
    ComPtr<FfxRcasTransform> _rcasTransform = nullptr;
};
