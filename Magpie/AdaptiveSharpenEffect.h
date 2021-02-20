#pragma once
#include "pch.h"
#include "GUIDs.h"
#include "Shaders.h"
#include "SimpleDrawTransform.h"
#include "EffectBase.h"


// Adaptive sharpen Ëã·¨
class AdaptiveSharpenEffect : public EffectBase {
public:
    IFACEMETHODIMP Initialize(
        _In_ ID2D1EffectContext* pEffectContext,
        _In_ ID2D1TransformGraph* pTransformGraph
    ) {
        HRESULT hr = SimpleDrawTransform::Create(
            pEffectContext, 
            &_pass1Transform, 
            ADAPTIVE_SHARPEN_PASS1_SHADER, 
            GUID_MAGPIE_ADAPTIVE_SHARPEN_PASS1_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform::Create(
            pEffectContext, 
            &_pass2Transform,
            ADAPTIVE_SHARPEN_PASS2_SHADER,
            GUID_MAGPIE_ADAPTIVE_SHARPEN_PASS2_SHADER
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

    static HRESULT Register(_In_ ID2D1Factory1* pFactory) {
        HRESULT hr = pFactory->RegisterEffectFromString(CLSID_MAGPIE_ADAPTIVE_SHARPEN_EFFECT, XML(
            <?xml version='1.0'?>
            <Effect>
                <!--System Properties-->
                <Property name='DisplayName' type='string' value='Ripple' />
                <Property name='Author' type='string' value='Microsoft Corporation' />
                <Property name='Category' type='string' value='Stylize' />
                <Property name='Description' type='string' value='Adds a ripple effect that can be animated' />
                <Inputs>
                    <Input name='Source' />
                </Inputs>
            </Effect>
        ), nullptr, 0, CreateEffect);

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
    ComPtr<SimpleDrawTransform> _pass2Transform = nullptr;
};
