#pragma once
#include "pch.h"
#include <EffectBase.h>
#include "EffectDefines.h"
#include "FsrEasuTransform.h"
#include "FsrRcasTransform.h"


class FSREffect : public EffectBase {
public:
    IFACEMETHODIMP Initialize(
        _In_ ID2D1EffectContext* pEffectContext,
        _In_ ID2D1TransformGraph* pTransformGraph
    ) {
        HRESULT hr = FsrEasuTransform::Create(pEffectContext, &_easuTransform);
        if (FAILED(hr)) {
            return hr;
        }
        hr = FsrRcasTransform::Create(pEffectContext, &_rcasTransform);
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

    HRESULT SetSharpness(FLOAT value) {
        if (value < 0 || value > 1) {
            return E_INVALIDARG;
        }

        _rcasTransform->SetSharpness(value);
        return S_OK;
    }

    FLOAT GetSharpness() const {
        return _rcasTransform->GetSharpness();
    }

    enum PROPS {
        PROP_SCALE = 0,
        PROP_SHARPNESS = 1
    };

    static HRESULT Register(_In_ ID2D1Factory1* pFactory) {
        const D2D1_PROPERTY_BINDING bindings[] =
        {
            D2D1_VALUE_TYPE_BINDING(L"Scale", &SetScale, &GetScale),
            D2D1_VALUE_TYPE_BINDING(L"Sharpness", &SetSharpness, &GetSharpness)
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
            <Property name='Sharpness' type='float'>
                <Property name='DisplayName' type='string' value='Sharpness'/>
                <Property name='Default' type='float' value='0.8'/>
                <Property name='Min' type='float' value='0'/>
                <Property name='Max' type='float' value='1.0'/>
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

    ComPtr<FsrEasuTransform> _easuTransform = nullptr;
    ComPtr<FsrRcasTransform> _rcasTransform = nullptr;
};
