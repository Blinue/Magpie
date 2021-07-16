#pragma once
#include "pch.h"
#include <SimpleDrawTransform.h>
#include "EffectDefines.h"



class FfxRcasTransform : public SimpleDrawTransform<1> {
private:
    FfxRcasTransform() : SimpleDrawTransform<1>(GUID_MAGPIE_FFX_RCAS_SHADER) {}
public:
    static HRESULT Create(_In_ ID2D1EffectContext* d2dEC, _Outptr_ FfxRcasTransform** ppOutput) {
        *ppOutput = nullptr;

        HRESULT hr = LoadShader(
            d2dEC,
            MAGPIE_FFX_RCAS_SHADER,
            GUID_MAGPIE_FFX_RCAS_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }

        *ppOutput = new FfxRcasTransform();

        return S_OK;
    }

    void SetSharpness(float value) {
        assert(value >= 0);
        _sharpness = value;
    }

    float GetSharpness() {
        return _sharpness;
    }
protected:
    void _SetShaderContantBuffer(const SIZE& srcSize) override {
        struct {
            UINT32 srcWidth;
            UINT32 srcHeight;
            FLOAT sharpness;
        } shaderConstants{
            (UINT32)srcSize.cx,
            (UINT32)srcSize.cy,
            _sharpness
        };

        _drawInfo->SetPixelShaderConstantBuffer((BYTE*)&shaderConstants, sizeof(shaderConstants));
    }

private:
    float _sharpness = 0.0f;
};
