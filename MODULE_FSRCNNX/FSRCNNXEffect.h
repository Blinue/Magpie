#pragma once
#include "pch.h"
#include <SimpleDrawTransform.h>
#include <EffectBase.h>
#include "EffectDefines.h"
#include "FSRCNNXAggregationTransform.h"


// FSRCNNX 超采样算法，可将图像放大至两倍
// 移植自 https://github.com/igv/FSRCNN-TensorFlow
class FSRCNNXEffect : public EffectBase {
public:
    IFACEMETHODIMP Initialize(
        _In_ ID2D1EffectContext* effectContext,
        _In_ ID2D1TransformGraph* transformGraph
    ) {
        _effectContext = effectContext;
        _transformGraph = transformGraph;

        return _MakeGraph();
    }

    HRESULT SetUseLineArtVersion(BOOL value) {
        if (_useLineArtVersion != static_cast<bool>(value)) {
            _useLineArtVersion = !_useLineArtVersion;

            HRESULT hr = _MakeGraph();
            if (FAILED(hr)) {
                // 失败时还原状态
                _useLineArtVersion = !_useLineArtVersion;
                _MakeGraph();
            }

            return hr;
        }

        return S_OK;
    }

    BOOL IsUseLineArtVersion() const {
        return _useLineArtVersion;
    }

    enum PROPS {
        PROP_USE_LINE_ART_VERSION = 0
    };

    static HRESULT Register(_In_ ID2D1Factory1* pFactory) {
        const D2D1_PROPERTY_BINDING bindings[] =
        {
            D2D1_VALUE_TYPE_BINDING(L"UseLineArtVersion", &SetUseLineArtVersion, &IsUseLineArtVersion)
        };

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
                <Property name='UseLineArtVersion' type='bool'>
                    <Property name='DisplayName' type='string' value='Use Denoise Version'/>
                    <Property name='Default' type='bool' value='false'/>
                </Property>
            </Effect>
        ), bindings, ARRAYSIZE(bindings), CreateEffect);

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

    HRESULT _MakeGraph() {
        HRESULT hr = SimpleDrawTransform<>::Create(
            _effectContext,
            &_rgb2yuvTransform,
            MAGPIE_RGB2YUV_SHADER,
            GUID_MAGPIE_RGB2YUV_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }

        if (_useLineArtVersion) {
            hr = SimpleDrawTransform<>::Create(
                _effectContext,
                &_featureMap1Transform,
                MAGPIE_FSRCNNX_8041_LINE_ART_FEATURE_MAP_1_SHADER,
                GUID_MAGPIE_FSRCNNX_8041_LINE_ART_FEATURE_MAP_1_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
            hr = SimpleDrawTransform<>::Create(
                _effectContext,
                &_featureMap2Transform,
                MAGPIE_FSRCNNX_8041_LINE_ART_FEATURE_MAP_2_SHADER,
                GUID_MAGPIE_FSRCNNX_8041_LINE_ART_FEATURE_MAP_2_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
            hr = SimpleDrawTransform<2>::Create(
                _effectContext,
                &_mapping1aTransform,
                MAGPIE_FSRCNNX_8041_LINE_ART_MAPPING_1A_SHADER,
                GUID_MAGPIE_FSRCNNX_8041_LINE_ART_MAPPING_1A_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
            hr = SimpleDrawTransform<2>::Create(
                _effectContext,
                &_mapping1bTransform,
                MAGPIE_FSRCNNX_8041_LINE_ART_MAPPING_1B_SHADER,
                GUID_MAGPIE_FSRCNNX_8041_LINE_ART_MAPPING_1B_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
            hr = SimpleDrawTransform<2>::Create(
                _effectContext,
                &_mapping2aTransform,
                MAGPIE_FSRCNNX_8041_LINE_ART_MAPPING_2A_SHADER,
                GUID_MAGPIE_FSRCNNX_8041_LINE_ART_MAPPING_2A_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
            hr = SimpleDrawTransform<2>::Create(
                _effectContext,
                &_mapping2bTransform,
                MAGPIE_FSRCNNX_8041_LINE_ART_MAPPING_2B_SHADER,
                GUID_MAGPIE_FSRCNNX_8041_LINE_ART_MAPPING_2B_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
            hr = SimpleDrawTransform<2>::Create(
                _effectContext,
                &_mapping3aTransform,
                MAGPIE_FSRCNNX_8041_LINE_ART_MAPPING_3A_SHADER,
                GUID_MAGPIE_FSRCNNX_8041_LINE_ART_MAPPING_3A_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
            hr = SimpleDrawTransform<2>::Create(
                _effectContext,
                &_mapping3bTransform,
                MAGPIE_FSRCNNX_8041_LINE_ART_MAPPING_3B_SHADER,
                GUID_MAGPIE_FSRCNNX_8041_LINE_ART_MAPPING_3B_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
            hr = SimpleDrawTransform<2>::Create(
                _effectContext,
                &_mapping4aTransform,
                MAGPIE_FSRCNNX_8041_LINE_ART_MAPPING_4A_SHADER,
                GUID_MAGPIE_FSRCNNX_8041_LINE_ART_MAPPING_4A_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
            hr = SimpleDrawTransform<2>::Create(
                _effectContext,
                &_mapping4bTransform,
                MAGPIE_FSRCNNX_8041_LINE_ART_MAPPING_4B_SHADER,
                GUID_MAGPIE_FSRCNNX_8041_LINE_ART_MAPPING_4B_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
            hr = SimpleDrawTransform<3>::Create(
                _effectContext,
                &_subBandResiduals1Transform,
                MAGPIE_FSRCNNX_8041_LINE_ART_SUBBAND_RESIDUALS_1_SHADER,
                GUID_MAGPIE_FSRCNNX_8041_LINE_ART_SUBBAND_RESIDUALS_1_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
            hr = SimpleDrawTransform<3>::Create(
                _effectContext,
                &_subBandResiduals2Transform,
                MAGPIE_FSRCNNX_8041_LINE_ART_SUBBAND_RESIDUALS_2_SHADER,
                GUID_MAGPIE_FSRCNNX_8041_LINE_ART_SUBBAND_RESIDUALS_2_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
            hr = SimpleDrawTransform<2>::Create(
                _effectContext,
                &_subPixelConvTransform,
                MAGPIE_FSRCNNX_8041_LINE_ART_SUBPIXEL_CONV_SHADER,
                GUID_MAGPIE_FSRCNNX_8041_LINE_ART_SUBPIXEL_CONV_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
        } else {
            hr = SimpleDrawTransform<>::Create(
                _effectContext,
                &_featureMap1Transform,
                MAGPIE_FSRCNNX_8041_FEATURE_MAP_1_SHADER,
                GUID_MAGPIE_FSRCNNX_8041_FEATURE_MAP_1_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
            hr = SimpleDrawTransform<>::Create(
                _effectContext,
                &_featureMap2Transform,
                MAGPIE_FSRCNNX_8041_FEATURE_MAP_2_SHADER,
                GUID_MAGPIE_FSRCNNX_8041_FEATURE_MAP_2_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
            hr = SimpleDrawTransform<2>::Create(
                _effectContext,
                &_mapping1aTransform,
                MAGPIE_FSRCNNX_8041_MAPPING_1A_SHADER,
                GUID_MAGPIE_FSRCNNX_8041_MAPPING_1A_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
            hr = SimpleDrawTransform<2>::Create(
                _effectContext,
                &_mapping1bTransform,
                MAGPIE_FSRCNNX_8041_MAPPING_1B_SHADER,
                GUID_MAGPIE_FSRCNNX_8041_MAPPING_1B_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
            hr = SimpleDrawTransform<2>::Create(
                _effectContext,
                &_mapping2aTransform,
                MAGPIE_FSRCNNX_8041_MAPPING_2A_SHADER,
                GUID_MAGPIE_FSRCNNX_8041_MAPPING_2A_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
            hr = SimpleDrawTransform<2>::Create(
                _effectContext,
                &_mapping2bTransform,
                MAGPIE_FSRCNNX_8041_MAPPING_2B_SHADER,
                GUID_MAGPIE_FSRCNNX_8041_MAPPING_2B_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
            hr = SimpleDrawTransform<2>::Create(
                _effectContext,
                &_mapping3aTransform,
                MAGPIE_FSRCNNX_8041_MAPPING_3A_SHADER,
                GUID_MAGPIE_FSRCNNX_8041_MAPPING_3A_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
            hr = SimpleDrawTransform<2>::Create(
                _effectContext,
                &_mapping3bTransform,
                MAGPIE_FSRCNNX_8041_MAPPING_3B_SHADER,
                GUID_MAGPIE_FSRCNNX_8041_MAPPING_3B_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
            hr = SimpleDrawTransform<2>::Create(
                _effectContext,
                &_mapping4aTransform,
                MAGPIE_FSRCNNX_8041_MAPPING_4A_SHADER,
                GUID_MAGPIE_FSRCNNX_8041_MAPPING_4A_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
            hr = SimpleDrawTransform<2>::Create(
                _effectContext,
                &_mapping4bTransform,
                MAGPIE_FSRCNNX_8041_MAPPING_4B_SHADER,
                GUID_MAGPIE_FSRCNNX_8041_MAPPING_4B_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
            hr = SimpleDrawTransform<3>::Create(
                _effectContext,
                &_subBandResiduals1Transform,
                MAGPIE_FSRCNNX_8041_SUBBAND_RESIDUALS_1_SHADER,
                GUID_MAGPIE_FSRCNNX_8041_SUBBAND_RESIDUALS_1_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
            hr = SimpleDrawTransform<3>::Create(
                _effectContext,
                &_subBandResiduals2Transform,
                MAGPIE_FSRCNNX_8041_SUBBAND_RESIDUALS_2_SHADER,
                GUID_MAGPIE_FSRCNNX_8041_SUBBAND_RESIDUALS_2_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
            hr = SimpleDrawTransform<2>::Create(
                _effectContext,
                &_subPixelConvTransform,
                MAGPIE_FSRCNNX_8041_SUBPIXEL_CONV_SHADER,
                GUID_MAGPIE_FSRCNNX_8041_SUBPIXEL_CONV_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
        }

        hr = FSRCNNXAggregationTransform::Create(
            _effectContext,
            &_aggregationTransform
        );
        if (FAILED(hr)) {
            return hr;
        }

        hr = _transformGraph->AddNode(_rgb2yuvTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->AddNode(_featureMap1Transform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->AddNode(_featureMap2Transform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->AddNode(_mapping1aTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->AddNode(_mapping1bTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->AddNode(_mapping2aTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->AddNode(_mapping2bTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->AddNode(_mapping3aTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->AddNode(_mapping3bTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->AddNode(_mapping4aTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->AddNode(_mapping4bTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->AddNode(_subBandResiduals1Transform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->AddNode(_subBandResiduals2Transform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->AddNode(_subPixelConvTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->AddNode(_aggregationTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }


        hr = _transformGraph->ConnectToEffectInput(0, _rgb2yuvTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->ConnectNode(_rgb2yuvTransform.Get(), _featureMap1Transform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->ConnectNode(_rgb2yuvTransform.Get(), _featureMap2Transform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }

        hr = _transformGraph->ConnectNode(_featureMap1Transform.Get(), _mapping1aTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->ConnectNode(_featureMap2Transform.Get(), _mapping1aTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->ConnectNode(_featureMap1Transform.Get(), _mapping1bTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->ConnectNode(_featureMap2Transform.Get(), _mapping1bTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }

        hr = _transformGraph->ConnectNode(_mapping1aTransform.Get(), _mapping2aTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->ConnectNode(_mapping1bTransform.Get(), _mapping2aTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->ConnectNode(_mapping1aTransform.Get(), _mapping2bTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->ConnectNode(_mapping1bTransform.Get(), _mapping2bTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }

        hr = _transformGraph->ConnectNode(_mapping2aTransform.Get(), _mapping3aTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->ConnectNode(_mapping2bTransform.Get(), _mapping3aTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->ConnectNode(_mapping2aTransform.Get(), _mapping3bTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->ConnectNode(_mapping2bTransform.Get(), _mapping3bTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }

        hr = _transformGraph->ConnectNode(_mapping3aTransform.Get(), _mapping4aTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->ConnectNode(_mapping3bTransform.Get(), _mapping4aTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->ConnectNode(_mapping3aTransform.Get(), _mapping4bTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->ConnectNode(_mapping3bTransform.Get(), _mapping4bTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }

        hr = _transformGraph->ConnectNode(_mapping4aTransform.Get(), _subBandResiduals1Transform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->ConnectNode(_mapping4bTransform.Get(), _subBandResiduals1Transform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->ConnectNode(_featureMap1Transform.Get(), _subBandResiduals1Transform.Get(), 2);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->ConnectNode(_mapping4aTransform.Get(), _subBandResiduals2Transform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->ConnectNode(_mapping4bTransform.Get(), _subBandResiduals2Transform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->ConnectNode(_featureMap2Transform.Get(), _subBandResiduals2Transform.Get(), 2);
        if (FAILED(hr)) {
            return hr;
        }

        hr = _transformGraph->ConnectNode(_subBandResiduals1Transform.Get(), _subPixelConvTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->ConnectNode(_subBandResiduals2Transform.Get(), _subPixelConvTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }

        hr = _transformGraph->ConnectNode(_rgb2yuvTransform.Get(), _aggregationTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _transformGraph->ConnectNode(_subPixelConvTransform.Get(), _aggregationTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }

        hr = _transformGraph->SetOutputNode(_aggregationTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }

        return S_OK;
    }

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

    ID2D1EffectContext* _effectContext = nullptr;
    ID2D1TransformGraph* _transformGraph = nullptr;

    bool _useLineArtVersion = false;
};
