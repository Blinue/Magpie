#pragma once
#include "pch.h"
#include <SimpleDrawTransform.h>
#include "ACNetL10Transform.h"
#include <EffectBase.h>
#include "EffectDefines.h"



// ACNet 超采样算法，可将动漫图像放大至两倍
// 移植自 https://github.com/TianZerL/ACNetGLSL/blob/master/glsl/ACNet.glsl
class ACNetEffect : public EffectBase {
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
            &_l1aTransform,
            MAGPIE_ACNET_L1A_SHADER,
            GUID_MAGPIE_ACNET_L1A_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform<>::Create(
            effectContext,
            &_l1bTransform,
            MAGPIE_ACNET_L1B_SHADER,
            GUID_MAGPIE_ACNET_L1B_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform<2>::Create(
            effectContext,
            &_l2aTransform,
            MAGPIE_ACNET_L2A_SHADER,
            GUID_MAGPIE_ACNET_L2A_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform<2>::Create(
            effectContext,
            &_l2bTransform,
            MAGPIE_ACNET_L2B_SHADER,
            GUID_MAGPIE_ACNET_L2B_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform<2>::Create(
            effectContext,
            &_l3aTransform,
            MAGPIE_ACNET_L3A_SHADER,
            GUID_MAGPIE_ACNET_L3A_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform<2>::Create(
            effectContext,
            &_l3bTransform,
            MAGPIE_ACNET_L3B_SHADER,
            GUID_MAGPIE_ACNET_L3B_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform<2>::Create(
            effectContext,
            &_l4aTransform,
            MAGPIE_ACNET_L4A_SHADER,
            GUID_MAGPIE_ACNET_L4A_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform<2>::Create(
            effectContext,
            &_l4bTransform,
            MAGPIE_ACNET_L4B_SHADER,
            GUID_MAGPIE_ACNET_L4B_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform<2>::Create(
            effectContext,
            &_l5aTransform,
            MAGPIE_ACNET_L5A_SHADER,
            GUID_MAGPIE_ACNET_L5A_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform<2>::Create(
            effectContext,
            &_l5bTransform,
            MAGPIE_ACNET_L5B_SHADER,
            GUID_MAGPIE_ACNET_L5B_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform<2>::Create(
            effectContext,
            &_l6aTransform,
            MAGPIE_ACNET_L6A_SHADER,
            GUID_MAGPIE_ACNET_L6A_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform<2>::Create(
            effectContext,
            &_l6bTransform,
            MAGPIE_ACNET_L6B_SHADER,
            GUID_MAGPIE_ACNET_L6B_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform<2>::Create(
            effectContext,
            &_l7aTransform,
            MAGPIE_ACNET_L7A_SHADER,
            GUID_MAGPIE_ACNET_L7A_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform<2>::Create(
            effectContext,
            &_l7bTransform,
            MAGPIE_ACNET_L7B_SHADER,
            GUID_MAGPIE_ACNET_L7B_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform<2>::Create(
            effectContext,
            &_l8aTransform,
            MAGPIE_ACNET_L8A_SHADER,
            GUID_MAGPIE_ACNET_L8A_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform<2>::Create(
            effectContext,
            &_l8bTransform,
            MAGPIE_ACNET_L8B_SHADER,
            GUID_MAGPIE_ACNET_L8B_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform<2>::Create(
            effectContext,
            &_l9aTransform,
            MAGPIE_ACNET_L9A_SHADER,
            GUID_MAGPIE_ACNET_L9A_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = SimpleDrawTransform<2>::Create(
            effectContext,
            &_l9bTransform,
            MAGPIE_ACNET_L9B_SHADER,
            GUID_MAGPIE_ACNET_L9B_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }
        hr = ACNetL10Transform::Create(effectContext, &_l10Transform);
        if (FAILED(hr)) {
            return hr;
        }

        hr = transformGraph->AddNode(_rgb2yuvTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_l1aTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_l1bTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_l2aTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_l2bTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_l3aTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_l3bTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_l4aTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_l4bTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_l5aTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_l5bTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_l6aTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_l6bTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_l7aTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_l7bTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_l8aTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_l8bTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_l9aTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_l9bTransform.Get());
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->AddNode(_l10Transform.Get());
        if (FAILED(hr)) {
            return hr;
        }


        hr = transformGraph->ConnectToEffectInput(0, _rgb2yuvTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_rgb2yuvTransform.Get(), _l1aTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_rgb2yuvTransform.Get(), _l1bTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }

        hr = transformGraph->ConnectNode(_l1aTransform.Get(), _l2aTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_l1bTransform.Get(), _l2aTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_l1aTransform.Get(), _l2bTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_l1bTransform.Get(), _l2bTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }

        hr = transformGraph->ConnectNode(_l2aTransform.Get(), _l3aTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_l2bTransform.Get(), _l3aTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_l2aTransform.Get(), _l3bTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_l2bTransform.Get(), _l3bTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }

        hr = transformGraph->ConnectNode(_l3aTransform.Get(), _l4aTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_l3bTransform.Get(), _l4aTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_l3aTransform.Get(), _l4bTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_l3bTransform.Get(), _l4bTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }

        hr = transformGraph->ConnectNode(_l4aTransform.Get(), _l5aTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_l4bTransform.Get(), _l5aTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_l4aTransform.Get(), _l5bTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_l4bTransform.Get(), _l5bTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }

        hr = transformGraph->ConnectNode(_l5aTransform.Get(), _l6aTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_l5bTransform.Get(), _l6aTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_l5aTransform.Get(), _l6bTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_l5bTransform.Get(), _l6bTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }

        hr = transformGraph->ConnectNode(_l6aTransform.Get(), _l7aTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_l6bTransform.Get(), _l7aTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_l6aTransform.Get(), _l7bTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_l6bTransform.Get(), _l7bTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }

        hr = transformGraph->ConnectNode(_l7aTransform.Get(), _l8aTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_l7bTransform.Get(), _l8aTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_l7aTransform.Get(), _l8bTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_l7bTransform.Get(), _l8bTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }

        hr = transformGraph->ConnectNode(_l8aTransform.Get(), _l9aTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_l8bTransform.Get(), _l9aTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_l8aTransform.Get(), _l9bTransform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_l8bTransform.Get(), _l9bTransform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }

        hr = transformGraph->ConnectNode(_rgb2yuvTransform.Get(), _l10Transform.Get(), 0);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_l9aTransform.Get(), _l10Transform.Get(), 1);
        if (FAILED(hr)) {
            return hr;
        }
        hr = transformGraph->ConnectNode(_l9bTransform.Get(), _l10Transform.Get(), 2);
        if (FAILED(hr)) {
            return hr;
        }

        hr = transformGraph->SetOutputNode(_l10Transform.Get());
        if (FAILED(hr)) {
            return hr;
        }

        return S_OK;
    }

    static HRESULT Register(_In_ ID2D1Factory1* pFactory) {
        HRESULT hr = pFactory->RegisterEffectFromString(CLSID_MAGPIE_ACNET_EFFECT, XML(
            <?xml version='1.0'?>
            <Effect>
                <!--System Properties-->
                <Property name='DisplayName' type='string' value='ACNet Upscale'/>
                <Property name='Author' type='string' value='Blinue'/>
                <Property name='Category' type='string' value='Scale'/>
                <Property name='Description' type='string' value='ACNet Upscale'/>
                <Inputs>
                    <Input name='Source'/>
                </Inputs>
            </Effect>
        ), nullptr, 0, CreateEffect);

        return hr;
    }

    static HRESULT CALLBACK CreateEffect(_Outptr_ IUnknown** ppEffectImpl) {
        // This code assumes that the effect class initializes its reference count to 1.
        *ppEffectImpl = static_cast<ID2D1EffectImpl*>(new ACNetEffect());

        if (*ppEffectImpl == nullptr) {
            return E_OUTOFMEMORY;
        }

        return S_OK;
    }

private:
    // Constructor should be private since it should never be called externally.
    ACNetEffect() {}

    ComPtr<SimpleDrawTransform<>> _rgb2yuvTransform = nullptr;
    ComPtr<SimpleDrawTransform<>> _l1aTransform = nullptr;
    ComPtr<SimpleDrawTransform<>> _l1bTransform = nullptr;
    ComPtr<SimpleDrawTransform<2>> _l2aTransform = nullptr;
    ComPtr<SimpleDrawTransform<2>> _l2bTransform = nullptr;
    ComPtr<SimpleDrawTransform<2>> _l3aTransform = nullptr;
    ComPtr<SimpleDrawTransform<2>> _l3bTransform = nullptr;
    ComPtr<SimpleDrawTransform<2>> _l4aTransform = nullptr;
    ComPtr<SimpleDrawTransform<2>> _l4bTransform = nullptr;
    ComPtr<SimpleDrawTransform<2>> _l5aTransform = nullptr;
    ComPtr<SimpleDrawTransform<2>> _l5bTransform = nullptr;
    ComPtr<SimpleDrawTransform<2>> _l6aTransform = nullptr;
    ComPtr<SimpleDrawTransform<2>> _l6bTransform = nullptr;
    ComPtr<SimpleDrawTransform<2>> _l7aTransform = nullptr;
    ComPtr<SimpleDrawTransform<2>> _l7bTransform = nullptr;
    ComPtr<SimpleDrawTransform<2>> _l8aTransform = nullptr;
    ComPtr<SimpleDrawTransform<2>> _l8bTransform = nullptr;
    ComPtr<SimpleDrawTransform<2>> _l9aTransform = nullptr;
    ComPtr<SimpleDrawTransform<2>> _l9bTransform = nullptr;
    ComPtr<ACNetL10Transform> _l10Transform = nullptr;
};
