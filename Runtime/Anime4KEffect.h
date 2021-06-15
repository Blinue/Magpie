#pragma once
#include "pch.h"
#include "Anime4KSharpenCombineTransform.h"
#include "Anime4KConvReduceTransform.h"
#include "SimpleDrawTransform.h"
#include "EffectBase.h"


// Anime4K 超采样算法，可将动漫图像放大至两倍
// 移植自 https://github.com/bloc97/Anime4K/blob/master/glsl/Upscale/Anime4K_Upscale_CNN_M_x2.glsl
class Anime4KEffect : public EffectBase {
public:
    IFACEMETHODIMP Initialize(
        _In_ ID2D1EffectContext* effectContext,
        _In_ ID2D1TransformGraph* transformGraph
    ) {
        _d2dEffectContext = effectContext;
        _d2dTransformGraph = transformGraph;

        return _MakeGraph();
    }

    HRESULT SetCurveHeight(FLOAT value) {
        if (value < 0) {
            return E_INVALIDARG;
        }

        _sharpenCombineTransform->SetCurveHeight(value);
        return S_OK;
    }

    FLOAT GetCurveHeight() const {
        return _sharpenCombineTransform->GetCurveHeight();
    }

    HRESULT SetUseDenoiseVersion(BOOL value) {
        if (_useDenoiseVersion != static_cast<bool>(value)) {
            _useDenoiseVersion = !_useDenoiseVersion;

            HRESULT hr = _MakeGraph();
            if (FAILED(hr)) {
                // 失败时还原状态
                _useDenoiseVersion = !_useDenoiseVersion;
            }

            return hr;
        }
        
        return S_OK;
    }

    BOOL IsUseDenoiseVersion() const {
        return static_cast<BOOL>(_useDenoiseVersion);
    }

    enum PROPS {
        PROP_CURVE_HEIGHT = 0,
        PROP_USE_DENOISE_VERSION = 1
    };

    static HRESULT Register(_In_ ID2D1Factory1* pFactory) {
        const D2D1_PROPERTY_BINDING bindings[] =
        {
            D2D1_VALUE_TYPE_BINDING(L"CurveHeight", &SetCurveHeight, &GetCurveHeight),
            D2D1_VALUE_TYPE_BINDING(L"UseDenoiseVersion", &SetUseDenoiseVersion, &IsUseDenoiseVersion)
        };

        HRESULT hr = pFactory->RegisterEffectFromString(CLSID_MAGPIE_ANIME4K_EFFECT, XML(
            <?xml version='1.0'?>
            <Effect>
                <!--System Properties-->
                <Property name='DisplayName' type='string' value='Anime4K Upscale'/>
                <Property name='Author' type='string' value='Blinue'/>
                <Property name='Category' type='string' value='Scale'/>
                <Property name='Description' type='string' value='Anime4K Upscale'/>
                <Inputs>
                    <Input name='Source'/>
                </Inputs>
                <Property name='CurveHeight' type='float'>
                    <Property name='DisplayName' type='string' value='CurveHeight'/>
                    <Property name='Default' type='float' value='0'/>
                </Property>
                <Property name='UseDenoiseVersion' type='bool'>
                    <Property name='DisplayName' type='string' value='Use Denoise Version'/>
                    <Property name='Default' type='bool' value='false'/>
                </Property>
            </Effect>
        ), bindings, ARRAYSIZE(bindings), CreateEffect);

        return hr;
    }

    static HRESULT CALLBACK CreateEffect(_Outptr_ IUnknown** ppEffectImpl) {
        // This code assumes that the effect class initializes its reference count to 1.
        *ppEffectImpl = static_cast<ID2D1EffectImpl*>(new Anime4KEffect());

        if (*ppEffectImpl == nullptr) {
            return E_OUTOFMEMORY;
        }

        return S_OK;
    }

private:
    // Constructor should be private since it should never be called externally.
    Anime4KEffect() {}

    //
    // +--+   +-+   +-+   +-+   +-+   +-+   +-+
    // |in+--->0+--->1+--->2+--->3+--->4+--->5|
    // +-++   +-+   +++   +++   +++   +++   +++
    //   |           |     |     |     |     |
    //   |           v-----v-----v---+-v-----v
    //   |                           |
    //   |                       +---v--+
    //   +--------------->+<-----+reduce|
    //                    |      +------+
    //                +---v---+
    //                |combine|
    //                +---+---+
    //                    |
    //                  +-v-+
    //                  |out|
    //                  +---+
    //
    HRESULT _MakeGraph() {
        HRESULT hr;
        _d2dTransformGraph->Clear();

        if (!_rgb2yuvTransform) {
            hr = SimpleDrawTransform<>::Create(
                _d2dEffectContext.Get(),
                &_rgb2yuvTransform,
                MAGPIE_RGB2YUV_SHADER,
                GUID_MAGPIE_RGB2YUV_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
        }

        if (_useDenoiseVersion) {
            hr = SimpleDrawTransform<>::Create(
                _d2dEffectContext.Get(),
                &_conv4x3x3x1Transform,
                MAGPIE_ANIME4K_DENOISE_CONV_4x3x3x1_SHADER,
                GUID_MAGPIE_ANIME4K_DENOISE_CONV_4x3x3x1_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
            hr = SimpleDrawTransform<>::Create(
                _d2dEffectContext.Get(),
                &_conv4x3x3x8Transform1,
                MAGPIE_ANIME4K_DENOISE_CONV_4x3x3x8_PASS1_SHADER,
                GUID_MAGPIE_ANIME4K_DENOISE_CONV_4x3x3x8_PASS1_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
            hr = SimpleDrawTransform<>::Create(
                _d2dEffectContext.Get(),
                &_conv4x3x3x8Transform2,
                MAGPIE_ANIME4K_DENOISE_CONV_4x3x3x8_PASS2_SHADER,
                GUID_MAGPIE_ANIME4K_DENOISE_CONV_4x3x3x8_PASS2_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
            hr = SimpleDrawTransform<>::Create(
                _d2dEffectContext.Get(),
                &_conv4x3x3x8Transform3,
                MAGPIE_ANIME4K_DENOISE_CONV_4x3x3x8_PASS3_SHADER,
                GUID_MAGPIE_ANIME4K_DENOISE_CONV_4x3x3x8_PASS3_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
            hr = SimpleDrawTransform<>::Create(
                _d2dEffectContext.Get(),
                &_conv4x3x3x8Transform4,
                MAGPIE_ANIME4K_DENOISE_CONV_4x3x3x8_PASS4_SHADER,
                GUID_MAGPIE_ANIME4K_DENOISE_CONV_4x3x3x8_PASS4_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
            hr = SimpleDrawTransform<>::Create(
                _d2dEffectContext.Get(),
                &_conv4x3x3x8Transform5,
                MAGPIE_ANIME4K_DENOISE_CONV_4x3x3x8_PASS5_SHADER,
                GUID_MAGPIE_ANIME4K_DENOISE_CONV_4x3x3x8_PASS5_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
        } else {
            hr = SimpleDrawTransform<>::Create(
                _d2dEffectContext.Get(),
                &_conv4x3x3x1Transform,
                MAGPIE_ANIME4K_CONV_4x3x3x1_SHADER,
                GUID_MAGPIE_ANIME4K_CONV_4x3x3x1_SHADER);
            if (FAILED(hr)) {
                return hr;
            }
            hr = SimpleDrawTransform<>::Create(
                _d2dEffectContext.Get(),
                &_conv4x3x3x8Transform1,
                MAGPIE_ANIME4K_CONV_4x3x3x8_PASS1_SHADER,
                GUID_MAGPIE_ANIME4K_CONV_4x3x3x8_PASS1_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
            hr = SimpleDrawTransform<>::Create(
                _d2dEffectContext.Get(),
                &_conv4x3x3x8Transform2,
                MAGPIE_ANIME4K_CONV_4x3x3x8_PASS2_SHADER,
                GUID_MAGPIE_ANIME4K_CONV_4x3x3x8_PASS2_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
            hr = SimpleDrawTransform<>::Create(
                _d2dEffectContext.Get(),
                &_conv4x3x3x8Transform3,
                MAGPIE_ANIME4K_CONV_4x3x3x8_PASS3_SHADER,
                GUID_MAGPIE_ANIME4K_CONV_4x3x3x8_PASS3_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
            hr = SimpleDrawTransform<>::Create(
                _d2dEffectContext.Get(),
                &_conv4x3x3x8Transform4,
                MAGPIE_ANIME4K_CONV_4x3x3x8_PASS4_SHADER,
                GUID_MAGPIE_ANIME4K_CONV_4x3x3x8_PASS4_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
            hr = SimpleDrawTransform<>::Create(
                _d2dEffectContext.Get(),
                &_conv4x3x3x8Transform5,
                MAGPIE_ANIME4K_CONV_4x3x3x8_PASS5_SHADER,
                GUID_MAGPIE_ANIME4K_CONV_4x3x3x8_PASS5_SHADER
            );
            if (FAILED(hr)) {
                return hr;
            }
        }

        hr = Anime4KConvReduceTransform::Create(
            _d2dEffectContext.Get(),
            &_convReduceTransform,
            _useDenoiseVersion
        );
        if (FAILED(hr)) {
            return hr;
        }

        if (!_sharpenCombineTransform) {
            // 不重新创建 sharpenCombineTransform
            hr = Anime4KSharpenCombineTransform::Create(_d2dEffectContext.Get(), &_sharpenCombineTransform);
            if (FAILED(hr)) {
                return hr;
            }
        }
        
        hr = _d2dTransformGraph->AddNode(_rgb2yuvTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = _d2dTransformGraph->AddNode(_conv4x3x3x1Transform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = _d2dTransformGraph->AddNode(_conv4x3x3x8Transform1.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = _d2dTransformGraph->AddNode(_conv4x3x3x8Transform2.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = _d2dTransformGraph->AddNode(_conv4x3x3x8Transform3.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = _d2dTransformGraph->AddNode(_conv4x3x3x8Transform4.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = _d2dTransformGraph->AddNode(_conv4x3x3x8Transform5.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = _d2dTransformGraph->AddNode(_convReduceTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = _d2dTransformGraph->AddNode(_sharpenCombineTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }


        hr = _d2dTransformGraph->ConnectToEffectInput(0, _rgb2yuvTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _d2dTransformGraph->ConnectNode(_rgb2yuvTransform.Get(), _conv4x3x3x1Transform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _d2dTransformGraph->ConnectNode(_conv4x3x3x1Transform.Get(), _conv4x3x3x8Transform1.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _d2dTransformGraph->ConnectNode(_conv4x3x3x8Transform1.Get(), _conv4x3x3x8Transform2.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _d2dTransformGraph->ConnectNode(_conv4x3x3x8Transform2.Get(), _conv4x3x3x8Transform3.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _d2dTransformGraph->ConnectNode(_conv4x3x3x8Transform3.Get(), _conv4x3x3x8Transform4.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _d2dTransformGraph->ConnectNode(_conv4x3x3x8Transform4.Get(), _conv4x3x3x8Transform5.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }

        hr = _d2dTransformGraph->ConnectNode(_conv4x3x3x8Transform1.Get(), _convReduceTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _d2dTransformGraph->ConnectNode(_conv4x3x3x8Transform2.Get(), _convReduceTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _d2dTransformGraph->ConnectNode(_conv4x3x3x8Transform3.Get(), _convReduceTransform.Get(), 2);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _d2dTransformGraph->ConnectNode(_conv4x3x3x8Transform4.Get(), _convReduceTransform.Get(), 3);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _d2dTransformGraph->ConnectNode(_conv4x3x3x8Transform5.Get(), _convReduceTransform.Get(), 4);
        if (FAILED(hr)) {
            return hr;
        }

        hr = _d2dTransformGraph->ConnectNode(_rgb2yuvTransform.Get(), _sharpenCombineTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = _d2dTransformGraph->ConnectNode(_convReduceTransform.Get(), _sharpenCombineTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }

        hr = _d2dTransformGraph->SetOutputNode(_sharpenCombineTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }

        return S_OK;
    }

    ComPtr<SimpleDrawTransform<>> _rgb2yuvTransform = nullptr;
    ComPtr<SimpleDrawTransform<>> _conv4x3x3x1Transform = nullptr;
    ComPtr<SimpleDrawTransform<>> _conv4x3x3x8Transform1 = nullptr;
    ComPtr<SimpleDrawTransform<>> _conv4x3x3x8Transform2 = nullptr;
    ComPtr<SimpleDrawTransform<>> _conv4x3x3x8Transform3 = nullptr;
    ComPtr<SimpleDrawTransform<>> _conv4x3x3x8Transform4 = nullptr;
    ComPtr<SimpleDrawTransform<>> _conv4x3x3x8Transform5 = nullptr;
    ComPtr<Anime4KConvReduceTransform> _convReduceTransform = nullptr;
    ComPtr<Anime4KSharpenCombineTransform> _sharpenCombineTransform = nullptr;

    ComPtr<ID2D1EffectContext> _d2dEffectContext = nullptr;
    ComPtr<ID2D1TransformGraph> _d2dTransformGraph = nullptr;

    bool _useDenoiseVersion = false;
};
