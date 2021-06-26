#pragma once
#include "pch.h"
#include "EffectBase.h"
#include "RavuZoomTransform.h"
#include "RavuZoomWeightsTransform.h"
#include <d2d1effecthelpers.h>


class RavuZoomEffect : public EffectBase {
public:
    IFACEMETHODIMP Initialize(
        _In_ ID2D1EffectContext* pEffectContext,
        _In_ ID2D1TransformGraph* pTransformGraph
    ) {
        HRESULT hr = SimpleDrawTransform<>::Create(
            pEffectContext,
            &_rgb2yuvTransform,
            MAGPIE_RGB2YUV_SHADER,
            GUID_MAGPIE_RGB2YUV_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = RavuZoomTransform::Create(pEffectContext, &_ravuZoomTransform);
        if (FAILED(hr)) {
            return hr;
        }
        hr = RavuZoomWeightsTransform::Create(pEffectContext, &_ravuZoomWeightsTransform);
        if (FAILED(hr)) {
            return hr;
        }

        hr = pTransformGraph->AddNode(_rgb2yuvTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = pTransformGraph->AddNode(_ravuZoomTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = pTransformGraph->AddNode(_ravuZoomWeightsTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }

        hr = pTransformGraph->ConnectToEffectInput(0, _rgb2yuvTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = pTransformGraph->ConnectToEffectInput(1, _ravuZoomWeightsTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = pTransformGraph->ConnectNode(_rgb2yuvTransform.Get(), _ravuZoomTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = pTransformGraph->ConnectNode(_ravuZoomWeightsTransform.Get(), _ravuZoomTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }
        hr = pTransformGraph->SetOutputNode(_ravuZoomTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }

        return S_OK;
    }

    HRESULT SetScale(D2D_VECTOR_2F value) {
        if (value.x <= 0 || value.y <= 0) {
            return E_INVALIDARG;
        }

        _ravuZoomTransform->SetScale(value);
        return S_OK;
    }

    D2D_VECTOR_2F GetScale() const {
        return _ravuZoomTransform->GetScale();
    }


    enum PROPS {
        PROP_SCALE = 0
    };

    static HRESULT Register(_In_ ID2D1Factory1* pFactory) {
        const D2D1_PROPERTY_BINDING bindings[] =
        {
            D2D1_VALUE_TYPE_BINDING(L"Scale", &SetScale, &GetScale)
        };

        HRESULT hr = pFactory->RegisterEffectFromString(CLSID_MAGPIE_RAVU_ZOOM_EFFECT, XML(
            <?xml version='1.0'?>
            <Effect>
                <!--System Properties-->
                <Property name='DisplayName' type='string' value='RAVU Zoom Scale'/>
                <Property name='Author' type='string' value='Blinue'/>
                <Property name='Category' type='string' value='Scale'/>
                <Property name='Description' type='string' value='RAVU Zoom Scale'/>
                <Inputs>
                    <Input name='Source'/>
                    <Input name='Weights'/>
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
        *ppEffectImpl = static_cast<ID2D1EffectImpl*>(new RavuZoomEffect());

        if (*ppEffectImpl == nullptr) {
            return E_OUTOFMEMORY;
        }

        return S_OK;
    }

    static void LoadWeights(ID2D1Effect* effect) {
        effect->SetInput(1,
            Utils::LoadBitmapFromFile(Env::$instance->GetWICImageFactory(), Env::$instance->GetD2DDC(), L"RavuZoomR3Weights.png").Get());
    }

private:
    RavuZoomEffect() {}

    ComPtr<SimpleDrawTransform<>> _rgb2yuvTransform = nullptr;
    ComPtr<RavuZoomTransform> _ravuZoomTransform = nullptr;
    ComPtr<RavuZoomWeightsTransform> _ravuZoomWeightsTransform = nullptr;
};
