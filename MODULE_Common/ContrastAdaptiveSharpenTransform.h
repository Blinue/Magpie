#pragma once
#include "pch.h"
#include <SimpleDrawTransform.h>
#include "EffectDefines.h"



class ContrastAdaptiveSharpenTransform : public SimpleDrawTransform<1> {
private:
    ContrastAdaptiveSharpenTransform() : SimpleDrawTransform<1>(GUID_MAGPIE_CONTRAST_ADAPTIVE_SHARPEN_SHADER) {}
public:
    static HRESULT Create(_In_ ID2D1EffectContext* d2dEC, _Outptr_ ContrastAdaptiveSharpenTransform** ppOutput) {
        *ppOutput = nullptr;

        HRESULT hr = LoadShader(
            d2dEC,
            MAGPIE_CONTRAST_ADAPTIVE_SHARPEN_SHADER,
            GUID_MAGPIE_CONTRAST_ADAPTIVE_SHARPEN_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }

        *ppOutput = new ContrastAdaptiveSharpenTransform();

        return S_OK;
    }

    void SetSharpness(float value) {
        assert(value >= 0 && value <= 1);
        _sharpness = value;
    }

    float GetSharpness() {
        return _sharpness;
    }
protected:
    void _SetShaderContantBuffer(const SIZE& srcSize) override {
        struct {
            INT32 srcWidth;
            INT32 srcHeight;
            FLOAT sharpness;
        } shaderConstants{
            srcSize.cx,
            srcSize.cy,
            _sharpness
        };

        _drawInfo->SetPixelShaderConstantBuffer((BYTE*)&shaderConstants, sizeof(shaderConstants));
    }

private:
    float _sharpness = 0.4f;
};
