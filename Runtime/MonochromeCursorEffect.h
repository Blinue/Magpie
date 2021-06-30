#pragma once
#include "pch.h"
#include "MonochromeCursorTransform.h"
#include <EffectBase.h>
#include "EffectDefines.h"


class MonochromeCursorEffect : public EffectBase {
public:
    IFACEMETHODIMP Initialize(
        _In_ ID2D1EffectContext* pEffectContext,
        _In_ ID2D1TransformGraph* pTransformGraph
    ) {
        HRESULT hr = MonochromeCursorTransform::Create(pEffectContext, &_transform);
        if (FAILED(hr)) {
            return hr;
        }

        hr = pTransformGraph->AddNode(_transform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = pTransformGraph->ConnectToEffectInput(0, _transform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = pTransformGraph->ConnectToEffectInput(1, _transform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }
        hr = pTransformGraph->SetOutputNode(_transform.Get());
        if (FAILED(hr)) {
            return hr;
        }

        return S_OK;
    }

    HRESULT SetCursorPos(D2D_VECTOR_2F value) {
        _transform->SetCursorPos({ lroundf(value.x), lroundf(value.y) });
        return S_OK;
    }

    D2D_VECTOR_2F GetCursorPos() const {
        POINT pos = _transform->GetCursorPos();
        return { float(pos.x), float(pos.y) };
    }

    enum PROPS {
        PROP_CURSOR_POS = 0
    };

    static HRESULT Register(_In_ ID2D1Factory1* pFactory) {
        const D2D1_PROPERTY_BINDING bindings[] =
        {
            D2D1_VALUE_TYPE_BINDING(L"CursorPos", &SetCursorPos, &GetCursorPos),
        };

        HRESULT hr = pFactory->RegisterEffectFromString(CLSID_MAGPIE_MONOCHROME_CURSOR_EFFECT, XML(
            <?xml version='1.0'?>
            <Effect>
                <!--System Properties-->
                <Property name='DisplayName' type='string' value='Monochrome cursor'/>
                <Property name='Author' type='string' value='Blinue'/>
                <Property name='Category' type='string' value='Cursor'/>
                <Property name='Description' type='string' value='Monochrome cursor'/>
                <Inputs>
                    <Input name='Source'/>
                    <Input name='Cursor'/>
                </Inputs>
                <Property name='CursorPos' type='vector2'>
                    <Property name='DisplayName' type='string' value='Cursor Pos'/>
                    <Property name='Default' type='vector2' value='(0,0)'/>
                </Property>
            </Effect>
        ), bindings, ARRAYSIZE(bindings), CreateEffect);

        return hr;
    }

    static HRESULT CALLBACK CreateEffect(_Outptr_ IUnknown** ppEffectImpl) {
        *ppEffectImpl = static_cast<ID2D1EffectImpl*>(new MonochromeCursorEffect());

        if (*ppEffectImpl == nullptr) {
            return E_OUTOFMEMORY;
        }

        return S_OK;
    }

private:
    MonochromeCursorEffect() {}

    ComPtr<MonochromeCursorTransform> _transform = nullptr;
};
