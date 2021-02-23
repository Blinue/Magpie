#pragma once
#include "pch.h"
#include "GUIDs.h"
#include "SimpleScaleTransform.h"

// Mitchell-Netravali 插值算法，一种双三次插值，可以获得平滑的边缘
// 可选是否使用更锐利的版本，默认为否
// （经测试两种版本几乎没有区别）
class MitchellNetravaliScaleTransform : public SimpleScaleTransform {
private:
    MitchellNetravaliScaleTransform() : SimpleScaleTransform(GUID_MAGPIE_MITCHELL_NETRAVALI_SCALE_SHADER) {}
public:
    static HRESULT Create(_In_ ID2D1EffectContext* d2dEC, _Outptr_ MitchellNetravaliScaleTransform** ppOutput) {
        if (!ppOutput) {
            return E_INVALIDARG;
        }

        HRESULT hr = DrawTransformBase::LoadShader(
            d2dEC, 
            MAGPIE_MITCHELL_NETRAVALI_SCALE_SHADER, 
            GUID_MAGPIE_MITCHELL_NETRAVALI_SCALE_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }

        *ppOutput = new MitchellNetravaliScaleTransform();
        return hr;
    }

    void SetUseSharpenVersion(bool value) {
        _useSharperVersion = value;
    }

    bool IsUseSharpenVersion() {
        return _useSharperVersion;
    }

protected:
    void _SetShaderContantBuffer(const SIZE& srcSize, const SIZE& destSize) override {
        struct {
            INT32 srcWidth;
            INT32 srcHeight;
            INT32 destWidth;
            INT32 destHeight;
            INT32 useSharperVersion;
        } shaderConstants{
            srcSize.cx,
            srcSize.cy,
            destSize.cx,
            destSize.cy,
            (INT32)_useSharperVersion
        };

        _drawInfo->SetPixelShaderConstantBuffer((BYTE*)&shaderConstants, sizeof(shaderConstants));
    }
private:
    bool _useSharperVersion = false;
};
