#pragma once
#include "pch.h"
#include <SimpleDrawTransform.h>
#include <EffectBase.h>
#include "EffectDefines.h"
#include "FSRCNNXAggregationTransform.h"


// FSRCNNX 超采样算法，可将图像放大至两倍
class FSRCNNXEffect : public EffectBase {
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
            &_featureMap1Transform,
            MAGPIE_FSRCNNX_8041_FEATURE_MAP_1_SHADER,
            GUID_MAGPIE_FSRCNNX_8041_FEATURE_MAP_1_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform<>::Create(
            effectContext,
            &_featureMap2Transform,
            MAGPIE_FSRCNNX_8041_FEATURE_MAP_2_SHADER,
            GUID_MAGPIE_FSRCNNX_8041_FEATURE_MAP_2_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform<2>::Create(
            effectContext,
            &_mapping1aTransform,
            MAGPIE_FSRCNNX_8041_MAPPING_1A_SHADER,
            GUID_MAGPIE_FSRCNNX_8041_MAPPING_1A_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform<2>::Create(
            effectContext,
            &_mapping1bTransform,
            MAGPIE_FSRCNNX_8041_MAPPING_1B_SHADER,
            GUID_MAGPIE_FSRCNNX_8041_MAPPING_1B_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform<2>::Create(
            effectContext,
            &_mapping2aTransform,
            MAGPIE_FSRCNNX_8041_MAPPING_2A_SHADER,
            GUID_MAGPIE_FSRCNNX_8041_MAPPING_2A_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform<2>::Create(
            effectContext,
            &_mapping2bTransform,
            MAGPIE_FSRCNNX_8041_MAPPING_2B_SHADER,
            GUID_MAGPIE_FSRCNNX_8041_MAPPING_2B_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform<2>::Create(
            effectContext,
            &_mapping3aTransform,
            MAGPIE_FSRCNNX_8041_MAPPING_3A_SHADER,
            GUID_MAGPIE_FSRCNNX_8041_MAPPING_3A_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform<2>::Create(
            effectContext,
            &_mapping3bTransform,
            MAGPIE_FSRCNNX_8041_MAPPING_3B_SHADER,
            GUID_MAGPIE_FSRCNNX_8041_MAPPING_3B_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform<2>::Create(
            effectContext,
            &_mapping4aTransform,
            MAGPIE_FSRCNNX_8041_MAPPING_4A_SHADER,
            GUID_MAGPIE_FSRCNNX_8041_MAPPING_4A_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform<2>::Create(
            effectContext,
            &_mapping4bTransform,
            MAGPIE_FSRCNNX_8041_MAPPING_4B_SHADER,
            GUID_MAGPIE_FSRCNNX_8041_MAPPING_4B_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform<3>::Create(
            effectContext,
            &_subBandResiduals1Transform,
            MAGPIE_FSRCNNX_8041_SUBBAND_RESIDUALS_1_SHADER,
            GUID_MAGPIE_FSRCNNX_8041_SUBBAND_RESIDUALS_1_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform<3>::Create(
            effectContext,
            &_subBandResiduals2Transform,
            MAGPIE_FSRCNNX_8041_SUBBAND_RESIDUALS_2_SHADER,
            GUID_MAGPIE_FSRCNNX_8041_SUBBAND_RESIDUALS_2_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform<2>::Create(
            effectContext,
            &_subPixelConvTransform,
            MAGPIE_FSRCNNX_8041_SUBPIXEL_CONV_SHADER,
            GUID_MAGPIE_FSRCNNX_8041_SUBPIXEL_CONV_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = FSRCNNXAggregationTransform::Create(
            effectContext,
            &_aggregationTransform
        );
        if (FAILED(hr)) {
            return hr;
        }
        

        hr = transformGraph->AddNode(_rgb2yuvTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_featureMap1Transform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_featureMap2Transform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_mapping1aTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_mapping1bTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_mapping2aTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_mapping2bTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_mapping3aTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_mapping3bTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_mapping4aTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_mapping4bTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_subBandResiduals1Transform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_subBandResiduals2Transform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_subPixelConvTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_aggregationTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }


        hr = transformGraph->ConnectToEffectInput(0, _rgb2yuvTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_rgb2yuvTransform.Get(), _featureMap1Transform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_rgb2yuvTransform.Get(), _featureMap2Transform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }

        hr = transformGraph->ConnectNode(_featureMap1Transform.Get(), _mapping1aTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_featureMap2Transform.Get(), _mapping1aTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_featureMap1Transform.Get(), _mapping1bTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_featureMap2Transform.Get(), _mapping1bTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }

        hr = transformGraph->ConnectNode(_mapping1aTransform.Get(), _mapping2aTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_mapping1bTransform.Get(), _mapping2aTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_mapping1aTransform.Get(), _mapping2bTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_mapping1bTransform.Get(), _mapping2bTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }

        hr = transformGraph->ConnectNode(_mapping2aTransform.Get(), _mapping3aTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_mapping2bTransform.Get(), _mapping3aTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_mapping2aTransform.Get(), _mapping3bTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_mapping2bTransform.Get(), _mapping3bTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }

        hr = transformGraph->ConnectNode(_mapping3aTransform.Get(), _mapping4aTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_mapping3bTransform.Get(), _mapping4aTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_mapping3aTransform.Get(), _mapping4bTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_mapping3bTransform.Get(), _mapping4bTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }

        hr = transformGraph->ConnectNode(_mapping4aTransform.Get(), _subBandResiduals1Transform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_mapping4bTransform.Get(), _subBandResiduals1Transform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_featureMap1Transform.Get(), _subBandResiduals1Transform.Get(), 2);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_mapping4aTransform.Get(), _subBandResiduals2Transform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_mapping4bTransform.Get(), _subBandResiduals2Transform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_featureMap2Transform.Get(), _subBandResiduals2Transform.Get(), 2);
        if (FAILED(hr)) {
            return hr;
        }
        
        hr = transformGraph->ConnectNode(_subBandResiduals1Transform.Get(), _subPixelConvTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_subBandResiduals2Transform.Get(), _subPixelConvTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }

        hr = transformGraph->ConnectNode(_rgb2yuvTransform.Get(), _aggregationTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_subPixelConvTransform.Get(), _aggregationTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }

        hr = transformGraph->SetOutputNode(_aggregationTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }

        return S_OK;
    }

    static HRESULT Register(_In_ ID2D1Factory1* pFactory) {
        HRESULT hr = pFactory->RegisterEffectFromString(CLSID_MAGPIE_FSRCNNX_EFFECT, XML(
            <?xml version='1.0'?>
            <Effect>
                <!--System Properties-->
                <Property name='DisplayName' type='string' value='FSRCNNX Upscale'/>
                <Property name='Author' type='string' value='Blinue'/>
                <Property name='Category' type='string' value='FSRCNNX'/>
                <Property name='Description' type='string' value='FSRCNNX Upscale'/>
                <Inputs>
                    <Input name='Source'/>
                </Inputs>
            </Effect>
        ), nullptr, 0, CreateEffect);

        return hr;
    }

    static HRESULT CALLBACK CreateEffect(_Outptr_ IUnknown** ppEffectImpl) {
        // This code assumes that the effect class initializes its reference count to 1.
        *ppEffectImpl = static_cast<ID2D1EffectImpl*>(new FSRCNNXEffect());

        if (*ppEffectImpl == nullptr) {
            return E_OUTOFMEMORY;
        }

        return S_OK;
    }

private:
    // Constructor should be private since it should never be called externally.
    FSRCNNXEffect() {}

    ComPtr<SimpleDrawTransform<>> _rgb2yuvTransform = nullptr;
    ComPtr<SimpleDrawTransform<>> _featureMap1Transform = nullptr;
    ComPtr<SimpleDrawTransform<>> _featureMap2Transform = nullptr;
    ComPtr<SimpleDrawTransform<2>> _mapping1aTransform = nullptr;
    ComPtr<SimpleDrawTransform<2>> _mapping1bTransform = nullptr;
    ComPtr<SimpleDrawTransform<2>> _mapping2aTransform = nullptr;
    ComPtr<SimpleDrawTransform<2>> _mapping2bTransform = nullptr;
    ComPtr<SimpleDrawTransform<2>> _mapping3aTransform = nullptr;
    ComPtr<SimpleDrawTransform<2>> _mapping3bTransform = nullptr;
    ComPtr<SimpleDrawTransform<2>> _mapping4aTransform = nullptr;
    ComPtr<SimpleDrawTransform<2>> _mapping4bTransform = nullptr;
    ComPtr<SimpleDrawTransform<3>> _subBandResiduals1Transform = nullptr;
    ComPtr<SimpleDrawTransform<3>> _subBandResiduals2Transform = nullptr;
    ComPtr<SimpleDrawTransform<2>> _subPixelConvTransform = nullptr;
    ComPtr<FSRCNNXAggregationTransform> _aggregationTransform = nullptr;
};
