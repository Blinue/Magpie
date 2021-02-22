#pragma once
#include "pch.h"
#include "GUIDs.h"
#include "Anime4KUpscaleConvReduceTransform.h"
#include "Anime4KUpscaleDeblurOutputTransform.h"
#include "SimpleDrawTransform.h"
#include "EffectBase.h"

// Anime4K 超采样算法，可将动漫图像放大至两倍
// https://github.com/bloc97/Anime4K
class Anime4KUpscaleDeblurEffect : public EffectBase {
public:
    IFACEMETHODIMP Initialize(
        _In_ ID2D1EffectContext* pEffectContext,
        _In_ ID2D1TransformGraph* pTransformGraph
    ) {
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
        //                 +--v---+
        //                 |output|
        //                 +--+---+
        //                    |
        //                  +-v-+
        //                  |out|
        //                  +---+
        //
        HRESULT hr = SimpleDrawTransform::Create(
            pEffectContext,
            &_conv4x3x3x1Transform,
            MAGPIE_ANIME4K_UPSCALE_CONV_4x3x3x1_SHADER,
            GUID_MAGPIE_ANIME4K_UPSCALE_CONV_4x3x3x1_SHADER);
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform::Create(
            pEffectContext,
            &_conv4x3x3x8Transform1,
            MAGPIE_ANIME4K_UPSCALE_CONV_4x3x3x8_SHADER1,
            GUID_MAGPIE_ANIME4K_UPSCALE_CONV_4x3x3x8_SHADER_1);
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform::Create(
            pEffectContext,
            &_conv4x3x3x8Transform2,
            MAGPIE_ANIME4K_UPSCALE_CONV_4x3x3x8_SHADER2,
            GUID_MAGPIE_ANIME4K_UPSCALE_CONV_4x3x3x8_SHADER_2
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform::Create(
            pEffectContext,
            &_conv4x3x3x8Transform3,
            MAGPIE_ANIME4K_UPSCALE_CONV_4x3x3x8_SHADER3,
            GUID_MAGPIE_ANIME4K_UPSCALE_CONV_4x3x3x8_SHADER_3
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform::Create(
            pEffectContext,
            &_conv4x3x3x8Transform4,
            MAGPIE_ANIME4K_UPSCALE_CONV_4x3x3x8_SHADER4,
            GUID_MAGPIE_ANIME4K_UPSCALE_CONV_4x3x3x8_SHADER_4
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform::Create(
            pEffectContext,
            &_conv4x3x3x8Transform5,
            MAGPIE_ANIME4K_UPSCALE_CONV_4x3x3x8_SHADER5,
            GUID_MAGPIE_ANIME4K_UPSCALE_CONV_4x3x3x8_SHADER_5
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = Anime4KUpscaleConvReduceTransform::Create(pEffectContext, &_convReduceTransform);
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform::Create(
            pEffectContext,
            &_deblurKernelXTransform,
            MAGPIE_ANIME4K_DEBLUR_KERNEL_X_SHADER,
            GUID_MAGPIE_ANIME4K_DEBLUR_KERNEL_X_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform::Create(
            pEffectContext,
            &_deblurKernelYTransform,
            MAGPIE_ANIME4K_DEBLUR_KERNEL_Y_SHADER,
            GUID_MAGPIE_ANIME4K_DEBLUR_KERNEL_Y_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = Anime4KUpscaleDeblurOutputTransform::Create(pEffectContext, &_outputTransform);
        if (FAILED(hr)) {
            return hr;
        }

        hr = pTransformGraph->AddNode(_conv4x3x3x1Transform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = pTransformGraph->AddNode(_conv4x3x3x8Transform1.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = pTransformGraph->AddNode(_conv4x3x3x8Transform2.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = pTransformGraph->AddNode(_conv4x3x3x8Transform3.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = pTransformGraph->AddNode(_conv4x3x3x8Transform4.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = pTransformGraph->AddNode(_conv4x3x3x8Transform5.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = pTransformGraph->AddNode(_convReduceTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = pTransformGraph->AddNode(_deblurKernelXTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = pTransformGraph->AddNode(_deblurKernelYTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = pTransformGraph->AddNode(_outputTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }

        hr = pTransformGraph->ConnectToEffectInput(0, _conv4x3x3x1Transform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = pTransformGraph->ConnectNode(_conv4x3x3x1Transform.Get(), _conv4x3x3x8Transform1.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = pTransformGraph->ConnectNode(_conv4x3x3x8Transform1.Get(), _conv4x3x3x8Transform2.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = pTransformGraph->ConnectNode(_conv4x3x3x8Transform2.Get(), _conv4x3x3x8Transform3.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = pTransformGraph->ConnectNode(_conv4x3x3x8Transform3.Get(), _conv4x3x3x8Transform4.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = pTransformGraph->ConnectNode(_conv4x3x3x8Transform4.Get(), _conv4x3x3x8Transform5.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }

        hr = pTransformGraph->ConnectNode(_conv4x3x3x8Transform1.Get(), _convReduceTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = pTransformGraph->ConnectNode(_conv4x3x3x8Transform2.Get(), _convReduceTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }
        hr = pTransformGraph->ConnectNode(_conv4x3x3x8Transform3.Get(), _convReduceTransform.Get(), 2);
        if (FAILED(hr)) {
            return hr;
        }
        hr = pTransformGraph->ConnectNode(_conv4x3x3x8Transform4.Get(), _convReduceTransform.Get(), 3);
        if (FAILED(hr)) {
            return hr;
        }
        hr = pTransformGraph->ConnectNode(_conv4x3x3x8Transform5.Get(), _convReduceTransform.Get(), 4);
        if (FAILED(hr)) {
            return hr;
        }

        hr = pTransformGraph->ConnectToEffectInput(0, _deblurKernelXTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = pTransformGraph->ConnectNode(_deblurKernelXTransform.Get(), _deblurKernelYTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }

        hr = pTransformGraph->ConnectToEffectInput(0, _outputTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = pTransformGraph->ConnectNode(_convReduceTransform.Get(), _outputTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }
        hr = pTransformGraph->ConnectNode(_deblurKernelYTransform.Get(), _outputTransform.Get(), 2);
        if (FAILED(hr)) {
            return hr;
        }

        hr = pTransformGraph->SetOutputNode(_outputTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }


        return S_OK;
    }

    static HRESULT Register(_In_ ID2D1Factory1* pFactory) {
        HRESULT hr = pFactory->RegisterEffectFromString(CLSID_MAGIPE_ANIME4K_UPSCALE_DEBLUR_EFFECT, XML(
            <?xml version='1.0'?>
            <Effect>
                <!--System Properties-->
                <Property name='DisplayName' type='string' value='Ripple'/>
                <Property name='Author' type='string' value='Microsoft Corporation'/>
                <Property name='Category' type='string' value='Stylize'/>
                <Property name='Description' type='string' value='Adds a ripple effect that can be animated'/>
                <Inputs>
                    <Input name='Source'/>
                </Inputs>
            </Effect>
        ), nullptr, 0, CreateEffect);

        return hr;
    }

    static HRESULT CALLBACK CreateEffect(_Outptr_ IUnknown** ppEffectImpl) {
        // This code assumes that the effect class initializes its reference count to 1.
        *ppEffectImpl = static_cast<ID2D1EffectImpl*>(new Anime4KUpscaleDeblurEffect());

        if (*ppEffectImpl == nullptr) {
            return E_OUTOFMEMORY;
        }

        return S_OK;
    }

private:
    // Constructor should be private since it should never be called externally.
    Anime4KUpscaleDeblurEffect() {}

    ComPtr<SimpleDrawTransform> _conv4x3x3x1Transform = nullptr;
    ComPtr<SimpleDrawTransform> _conv4x3x3x8Transform1 = nullptr;
    ComPtr<SimpleDrawTransform> _conv4x3x3x8Transform2 = nullptr;
    ComPtr<SimpleDrawTransform> _conv4x3x3x8Transform3 = nullptr;
    ComPtr<SimpleDrawTransform> _conv4x3x3x8Transform4 = nullptr;
    ComPtr<SimpleDrawTransform> _conv4x3x3x8Transform5 = nullptr;
    ComPtr<Anime4KUpscaleConvReduceTransform> _convReduceTransform = nullptr;
    ComPtr<SimpleDrawTransform> _deblurKernelXTransform = nullptr;
    ComPtr<SimpleDrawTransform> _deblurKernelYTransform = nullptr;
    ComPtr<Anime4KUpscaleDeblurOutputTransform> _outputTransform = nullptr;
};
