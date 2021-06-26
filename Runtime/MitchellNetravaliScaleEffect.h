#pragma once
#include "pch.h"
#include "MitchellNetravaliScaleTransform.h"
#include "EffectBase.h"
#include <d2d1effecthelpers.h>
#include "GUIDs.h"


// Mitchell-Netravali 缩放算法，一种双三次插值，可以获得平滑的边缘
// 可选是否使用更锐利的版本，默认为否
// （经测试两种版本几乎没有区别）
class MitchellNetravaliScaleEffect : public EffectBase {
public:
    IFACEMETHODIMP Initialize(
        _In_ ID2D1EffectContext* pEffectContext,
        _In_ ID2D1TransformGraph* pTransformGraph
    ) {
        HRESULT hr = MitchellNetravaliScaleTransform::Create(pEffectContext, &_transform);
        if (FAILED(hr)) {
            return hr;
        }

        hr = pTransformGraph->SetSingleTransformNode(_transform.Get());
        if (FAILED(hr)) {
            return hr;
        }

        return S_OK;
    }

    HRESULT SetScale(D2D_VECTOR_2F value) {
        if (value.x <= 0 || value.y <= 0) {
            return E_INVALIDARG;
        }

        _transform->SetScale(value);
        return S_OK;
    }

    D2D_VECTOR_2F GetScale() const {
        return _transform->GetScale();
    }

    HRESULT SetUseSharperVersion(BOOL value) {
        _transform->SetUseSharpenVersion((bool)value);
        return S_OK;
    }

    BOOL IsUseSharperVersion() const {
        return (BOOL)_transform->IsUseSharpenVersion();
    }

    enum PROPS {
        PROP_SCALE = 0,
        PROP_USE_SHARPER_VERSION = 1
    };

    static HRESULT Register(_In_ ID2D1Factory1* pFactory) {
        const D2D1_PROPERTY_BINDING bindings[] =
        {
            D2D1_VALUE_TYPE_BINDING(L"Scale", &SetScale, &GetScale),
            D2D1_VALUE_TYPE_BINDING(L"UseSharperVersion", &SetUseSharperVersion, &IsUseSharperVersion)
        };

        HRESULT hr = pFactory->RegisterEffectFromString(CLSID_MAGPIE_MITCHELL_NETRAVALI_SCALE_EFFECT, XML(
            <?xml version='1.0'?>
                <Effect>
                <!--System Properties-->
                <Property name='DisplayName' type='string' value='Mitchell-Netravali Scale' />
                <Property name='Author' type='string' value='Blinue' />
                <Property name='Category' type='string' value='Scale' />
                <Property name='Description' type='string' value='Mitchell-Netravali scale algorithm' />
                <Inputs>
                    <Input name='Source' />
                </Inputs>
                <Property name='Scale' type='vector2'>
                    <Property name='DisplayName' type='string' value='Scale' />
                    <Property name='Default' type='vector2' value='(1,1)' />
                </Property>
                <Property name='UseSharperVersion' type='bool'>
                    <Property name='DisplayName' type='string' value='Use Sharper Version' />
                    <Property name='Default' type='bool' value='false' />
                </Property>
            </Effect>
        ), bindings, ARRAYSIZE(bindings), CreateEffect);

        return hr;
    }

    static HRESULT CALLBACK CreateEffect(_Outptr_ IUnknown** ppEffectImpl) {
        *ppEffectImpl = static_cast<ID2D1EffectImpl*>(new MitchellNetravaliScaleEffect());

        if (*ppEffectImpl == nullptr) {
            return E_OUTOFMEMORY;
        }

        return S_OK;
    }

private:
    MitchellNetravaliScaleEffect() {}

    ComPtr<MitchellNetravaliScaleTransform> _transform = nullptr;
};
