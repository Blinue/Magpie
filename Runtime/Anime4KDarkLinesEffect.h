#pragma once
#include "pch.h"
#include "SimpleDrawTransform.h"
#include "SimpleTwoInputsDrawTransform.h"
#include "EffectBase.h"



class Anime4KDarkLinesEffect : public EffectBase {
public:
    IFACEMETHODIMP Initialize(
        _In_ ID2D1EffectContext* effectContext,
        _In_ ID2D1TransformGraph* transformGraph
    ) {
        _d2dEffectContext = effectContext;
        _d2dTransformGraph = transformGraph;

        return _MakeGraph();
    }


    static HRESULT Register(_In_ ID2D1Factory1* pFactory) {
        HRESULT hr = pFactory->RegisterEffectFromString(CLSID_MAGPIE_ANIME4K_DARKLINES_EFFECT, XML(
            <?xml version='1.0'?>
            <Effect>
                <!--System Properties-->
                <Property name='DisplayName' type='string' value='Anime4K DarkLines'/>
                <Property name='Author' type='string' value='Blinue'/>
                <Property name='Category' type='string' value='Scale'/>
                <Property name='Description' type='string' value='Anime4K DarkLines'/>
                <Inputs>
                    <Input name='Source'/>
                </Inputs>
            </Effect>
        ), nullptr, 0, CreateEffect);

        return hr;
    }

    static HRESULT CALLBACK CreateEffect(_Outptr_ IUnknown** ppEffectImpl) {
        // This code assumes that the effect class initializes its reference count to 1.
        *ppEffectImpl = static_cast<ID2D1EffectImpl*>(new Anime4KDarkLinesEffect());

        if (*ppEffectImpl == nullptr) {
            return E_OUTOFMEMORY;
        }

        return S_OK;
    }

private:
    // Constructor should be private since it should never be called externally.
    Anime4KDarkLinesEffect() {}


    HRESULT _MakeGraph() {
        HRESULT hr;

        hr = SimpleDrawTransform::Create(
            _d2dEffectContext.Get(),
            &_rgb2yuvTransform,
            MAGPIE_RGB2YUV_SHADER,
            GUID_RGB2YUV_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform::Create(
            _d2dEffectContext.Get(),
            &_pass1Transform,
            MAGPIE_ANIME4K_DARKLINES_PASS1_SHADER,
            GUID_MAGPIE_ANIME4K_DARKLINES_PASS1_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleTwoInputsDrawTransform::Create(
            _d2dEffectContext.Get(),
            &_pass2Transform,
            MAGPIE_ANIME4K_DARKLINES_PASS2_SHADER,
            GUID_MAGPIE_ANIME4K_DARKLINES_PASS2_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform::Create(
            _d2dEffectContext.Get(),
            &_pass3Transform,
            MAGPIE_ANIME4K_DARKLINES_PASS3_SHADER,
            GUID_MAGPIE_ANIME4K_DARKLINES_PASS3_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform::Create(
            _d2dEffectContext.Get(),
            &_pass4Transform,
            MAGPIE_ANIME4K_DARKLINES_PASS4_SHADER,
            GUID_MAGPIE_ANIME4K_DARKLINES_PASS4_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleTwoInputsDrawTransform::Create(
            _d2dEffectContext.Get(),
            &_pass5Transform,
            MAGPIE_ANIME4K_DARKLINES_PASS5_SHADER,
            GUID_MAGPIE_ANIME4K_DARKLINES_PASS5_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        

        hr = _d2dTransformGraph->AddNode(_rgb2yuvTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = _d2dTransformGraph->AddNode(_pass1Transform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = _d2dTransformGraph->AddNode(_pass2Transform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = _d2dTransformGraph->AddNode(_pass3Transform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = _d2dTransformGraph->AddNode(_pass4Transform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = _d2dTransformGraph->AddNode(_pass5Transform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        
        hr = _d2dTransformGraph->ConnectToEffectInput(0, _rgb2yuvTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _d2dTransformGraph->ConnectNode(_rgb2yuvTransform.Get(), _pass1Transform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _d2dTransformGraph->ConnectNode(_rgb2yuvTransform.Get(), _pass2Transform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _d2dTransformGraph->ConnectNode(_rgb2yuvTransform.Get(), _pass5Transform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _d2dTransformGraph->ConnectNode(_pass1Transform.Get(), _pass2Transform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _d2dTransformGraph->ConnectNode(_pass2Transform.Get(), _pass3Transform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _d2dTransformGraph->ConnectNode(_pass3Transform.Get(), _pass4Transform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _d2dTransformGraph->ConnectNode(_pass4Transform.Get(), _pass5Transform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }

        hr = _d2dTransformGraph->SetOutputNode(_pass5Transform.Get());
        if (FAILED(hr)) {
            return hr;
        }

        return S_OK;
    }

    ComPtr<SimpleDrawTransform> _rgb2yuvTransform = nullptr;
    ComPtr<SimpleDrawTransform> _pass1Transform = nullptr;
    ComPtr<SimpleTwoInputsDrawTransform> _pass2Transform = nullptr;
    ComPtr<SimpleDrawTransform> _pass3Transform = nullptr;
    ComPtr<SimpleDrawTransform> _pass4Transform = nullptr;
    ComPtr<SimpleTwoInputsDrawTransform> _pass5Transform = nullptr;

    ComPtr<ID2D1EffectContext> _d2dEffectContext = nullptr;
    ComPtr<ID2D1TransformGraph> _d2dTransformGraph = nullptr;
};
