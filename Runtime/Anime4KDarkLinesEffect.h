#pragma once
#include "pch.h"
#include "SimpleDrawTransform.h"
#include "EffectBase.h"
#include "Anime4KDarkLinesPass5Transform.h"
#include "GUIDs.h"
#include <CommonEffectDefines.h>


class Anime4KDarkLinesEffect : public EffectBase {
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
        hr = SimpleDrawTransform<>::Create(
            effectContext,
            &_pass1Transform,
            MAGPIE_ANIME4K_DARKLINES_PASS1_SHADER,
            GUID_MAGPIE_ANIME4K_DARKLINES_PASS1_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform<2>::Create(
            effectContext,
            &_pass2Transform,
            MAGPIE_ANIME4K_DARKLINES_PASS2_SHADER,
            GUID_MAGPIE_ANIME4K_DARKLINES_PASS2_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform<>::Create(
            effectContext,
            &_pass3Transform,
            MAGPIE_ANIME4K_DARKLINES_PASS3_SHADER,
            GUID_MAGPIE_ANIME4K_DARKLINES_PASS3_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform<>::Create(
            effectContext,
            &_pass4Transform,
            MAGPIE_ANIME4K_DARKLINES_PASS4_SHADER,
            GUID_MAGPIE_ANIME4K_DARKLINES_PASS4_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = Anime4KDarkLinesPass5Transform::Create(
            effectContext,
            &_pass5Transform
        );
        if (FAILED(hr)) {
            return hr;
        }


        hr = transformGraph->AddNode(_rgb2yuvTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_pass1Transform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_pass2Transform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_pass3Transform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_pass4Transform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_pass5Transform.Get());
        if (FAILED(hr)) {
            return hr;
        }

        hr = transformGraph->ConnectToEffectInput(0, _rgb2yuvTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_rgb2yuvTransform.Get(), _pass1Transform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_rgb2yuvTransform.Get(), _pass2Transform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_rgb2yuvTransform.Get(), _pass5Transform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_pass1Transform.Get(), _pass2Transform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_pass2Transform.Get(), _pass3Transform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_pass3Transform.Get(), _pass4Transform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_pass4Transform.Get(), _pass5Transform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }

        hr = transformGraph->SetOutputNode(_pass5Transform.Get());
        if (FAILED(hr)) {
            return hr;
        }

        return S_OK;
    }

    HRESULT SetStrength(FLOAT value) {
        if (value <= 0) {
            return E_INVALIDARG;
        }

        _pass5Transform->SetStrength(value);
        return S_OK;
    }

    FLOAT GetStrength() const {
        return _pass5Transform->GetStrength();
    }

    enum PROPS {
        PROP_STRENGTH = 0   // 加深强度，值越大线条越深。默认值为 1
    };


    static HRESULT Register(_In_ ID2D1Factory1* pFactory) {
        const D2D1_PROPERTY_BINDING bindings[] =
        {
            D2D1_VALUE_TYPE_BINDING(L"Strength", &SetStrength, &GetStrength)
        };

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
                <Property name='Strength' type='float'>
                    <Property name='DisplayName' type='string' value='Strength'/>
                    <Property name='Default' type='float' value='1'/>
                </Property>
            </Effect>
        ), bindings, ARRAYSIZE(bindings), CreateEffect);

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


    ComPtr<SimpleDrawTransform<>> _rgb2yuvTransform = nullptr;
    ComPtr<SimpleDrawTransform<>> _pass1Transform = nullptr;
    ComPtr<SimpleDrawTransform<2>> _pass2Transform = nullptr;
    ComPtr<SimpleDrawTransform<>> _pass3Transform = nullptr;
    ComPtr<SimpleDrawTransform<>> _pass4Transform = nullptr;
    ComPtr<Anime4KDarkLinesPass5Transform> _pass5Transform = nullptr;
};
