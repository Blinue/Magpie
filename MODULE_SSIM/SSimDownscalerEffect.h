#pragma once
#include <EffectBase.h>
#include <SimpleDrawTransform.h>
#include <SimpleScaleTransform.h>
#include <nlohmann/json.hpp>
#include <boost/format.hpp>
#include "EffectDefines.h"


class SSimDownscalerEffect : public EffectBase {
public:
    IFACEMETHODIMP Initialize(
        _In_ ID2D1EffectContext* pEffectContext,
        _In_ ID2D1TransformGraph* pTransformGraph
    ) {
        _effectContext = pEffectContext;
        _transformGraph = pTransformGraph;

        HRESULT hr = SimpleScaleTransform::Create(
            pEffectContext,
            &_l2Pass1Transform,
            MAGPIE_SSIM_DOWNSCALER_L2_PASS1_SHADER,
            GUID_MAGPIE_SSIM_DOWNSCALER_L2_PASS1_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }

        hr = SimpleDrawTransform<1>::Create(
            pEffectContext,
            &_l2Pass2Transform,
            MAGPIE_SSIM_DOWNSCALER_L2_PASS2_SHADER,
            GUID_MAGPIE_SSIM_DOWNSCALER_L2_PASS2_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }

        hr = SimpleDrawTransform<1>::Create(
            pEffectContext,
            &_meanTransform,
            MAGPIE_SSIM_DOWNSCALER_MEAN_SHADER,
            GUID_MAGPIE_SSIM_DOWNSCALER_MEAN_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }

        hr = SimpleDrawTransform<3>::Create(
            pEffectContext,
            &_rTransform,
            MAGPIE_SSIM_DOWNSCALER_R_SHADER,
            GUID_MAGPIE_SSIM_DOWNSCALER_R_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }

        hr = SimpleDrawTransform<3>::Create(
            pEffectContext,
            &_finalTransform,
            MAGPIE_SSIM_DOWNSCALER_FINAL_SHADER,
            GUID_MAGPIE_SSIM_DOWNSCALER_FINAL_SHADER
        );

        return S_OK;
    }

    HRESULT SetScale(D2D_VECTOR_2F value) {
        if (value.x <= 0 || value.y <= 0) {
            return E_INVALIDARG;
        }

        _l2Pass1Transform->SetScale(value);
        return S_OK;
    }

    D2D_VECTOR_2F GetScale() const {
        return _l2Pass1Transform->GetScale();
    }

    HRESULT SetDownScaleEffect(IUnknown* value) {
        HRESULT hr = value->QueryInterface<ID2D1Effect>(&_downScaleEffect);
        if (FAILED(hr)) {
            return hr;
        }

        hr = _effectContext->CreateTransformNodeFromEffect(_downScaleEffect.Get(), &_downScaleTransform);
        if (FAILED(hr)) {
            return hr;
        }

        return _MakeGraph();
    }

    IUnknown* GetDownScaleEffect() const {
        return _downScaleEffect.Get();
    }

    enum PROPS {
        PROP_SCALE = 0,
        PROP_DOWN_SCALE_EFFECT = 1
    };

    static HRESULT Register(_In_ ID2D1Factory1* pFactory) {
        const D2D1_PROPERTY_BINDING bindings[] =
        {
            D2D1_VALUE_TYPE_BINDING(L"Scale", &SetScale, &GetScale),
            D2D1_VALUE_TYPE_BINDING(L"DownScaleEffect", &SetDownScaleEffect, &GetDownScaleEffect)
        };

        HRESULT hr = pFactory->RegisterEffectFromString(CLSID_MAGPIE_SSIM_DOWNSCALER_EFFECT, XML(
            <?xml version='1.0'?>
            <Effect>
                <!--System Properties-->
                <Property name='DisplayName' type='string' value='SSimDownscaler'/>
                <Property name='Author' type='string' value='Blinue'/>
                <Property name='Category' type='string' value='Scale'/>
                <Property name='Description' type='string' value='SSimDownscaler'/>
                <Inputs>
                    <Input name='Source'/>
                </Inputs>
                <Property name='Scale' type='vector2'>
                    <Property name='DisplayName' type='string' value='Scale'/>
                    <Property name='Default' type='vector2' value='(1,1)'/>
                </Property>
                <Property name='DownScaleEffect' type='iunknown'>
                    <Property name='DisplayName' type='string' value='DownScaleEffect'/>
                </Property>
            </Effect>
        ), bindings, ARRAYSIZE(bindings), CreateEffect);

        return hr;
    }

    static HRESULT CALLBACK CreateEffect(_Outptr_ IUnknown** ppEffectImpl) {
        *ppEffectImpl = static_cast<ID2D1EffectImpl*>(new SSimDownscalerEffect());

        if (*ppEffectImpl == nullptr) {
            return E_OUTOFMEMORY;
        }

        return S_OK;
    }

    static HRESULT CreateAndSetDownScaleEffect(
        ID2D1Effect* result,
        ID2D1Factory1* d2dFactory,
        ID2D1DeviceContext* d2dDC,
        std::pair<float, float> scale
    ) {
        assert(scale.first > 0 && scale.second > 0);

        HMODULE dll = LoadLibrary(L"effects\\Common");
        if (dll == NULL) {
            return E_FAIL;
        }

        using EffectCreateFunc = HRESULT(
            ID2D1Factory1* d2dFactory,
            ID2D1DeviceContext* d2dDC,
            IWICImagingFactory2* wicImgFactory,
            const nlohmann::json& props,
            float fillScale,
            std::pair<float, float>& scale,
            ComPtr<ID2D1Effect>& effect
        );
        auto createEffect = (EffectCreateFunc*)GetProcAddress(dll, "CreateEffect");
        if (createEffect == NULL) {
            return E_FAIL;
        }

        std::pair<float, float> t = scale;
        ComPtr<ID2D1Effect> mitchellEffect = nullptr;
        auto json = nlohmann::json::parse((boost::format(R"({
		    "effect" : "mitchell",
		    "scale" : [%1%, %2%],
		    "variant": "mitchell"
	    })") % scale.first % scale.second).str());
        HRESULT hr = createEffect(d2dFactory, d2dDC, nullptr, json, {}, t, mitchellEffect);
        if (FAILED(hr)) {
            return hr;
        }

        hr = result->SetValue(SSimDownscalerEffect::PROP_DOWN_SCALE_EFFECT, mitchellEffect.Get());
        if (FAILED(hr)) {
            return hr;
        }

        return S_OK;
    }
private:
    SSimDownscalerEffect() {}

    HRESULT _MakeGraph() {
        assert(_downScaleTransform);

        _transformGraph->Clear();

        HRESULT hr = _transformGraph->AddNode(_l2Pass1Transform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->AddNode(_l2Pass2Transform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->AddNode(_downScaleTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->AddNode(_meanTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->AddNode(_rTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->AddNode(_finalTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }

        hr = _transformGraph->ConnectToEffectInput(0, _l2Pass1Transform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->ConnectToEffectInput(0, _downScaleTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->ConnectNode(_l2Pass1Transform.Get(), _l2Pass2Transform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->ConnectNode(_downScaleTransform.Get(), _meanTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->ConnectNode(_downScaleTransform.Get(), _rTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->ConnectNode(_l2Pass2Transform.Get(), _rTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->ConnectNode(_meanTransform.Get(), _rTransform.Get(), 2);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->ConnectNode(_downScaleTransform.Get(), _finalTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->ConnectNode(_meanTransform.Get(), _finalTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->ConnectNode(_rTransform.Get(), _finalTransform.Get(), 2);
        if (FAILED(hr)) {
            return hr;
        }

        hr = _transformGraph->SetOutputNode(_finalTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }

        return S_OK;
    }

    ComPtr<SimpleScaleTransform> _l2Pass1Transform = nullptr;
    ComPtr<SimpleDrawTransform<1>> _l2Pass2Transform = nullptr;
    ComPtr<ID2D1TransformNode> _downScaleTransform = nullptr;
    ComPtr<SimpleDrawTransform<1>> _meanTransform = nullptr;
    ComPtr<SimpleDrawTransform<3>> _rTransform = nullptr;
    ComPtr<SimpleDrawTransform<3>> _finalTransform = nullptr;

    ComPtr<ID2D1Effect> _downScaleEffect = nullptr;

    ID2D1EffectContext* _effectContext = nullptr;
    ID2D1TransformGraph* _transformGraph = nullptr;
};