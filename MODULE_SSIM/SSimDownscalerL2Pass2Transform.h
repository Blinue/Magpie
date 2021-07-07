#pragma once
#include <SimpleDrawTransform.h>
#include "EffectDefines.h"


class SSimDownscalerL2Pass2Transform : public SimpleDrawTransform<1> {
private:
    SSimDownscalerL2Pass2Transform() : SimpleDrawTransform<1>(GUID_MAGPIE_SSIM_DOWNSCALER_L2_PASS2_SHADER) {}
public:
    static HRESULT Create(_In_ ID2D1EffectContext* d2dEC, _Outptr_ SSimDownscalerL2Pass2Transform** ppOutput) {
        if (!ppOutput) {
            return E_INVALIDARG;
        }

        HRESULT hr = LoadShader(
            d2dEC,
            MAGPIE_SSIM_DOWNSCALER_L2_PASS2_SHADER,
            GUID_MAGPIE_SSIM_DOWNSCALER_L2_PASS2_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }

        *ppOutput = new SSimDownscalerL2Pass2Transform();
        return hr;
    }

    void SetVariant(int value) {
        assert(value >= 0 && value <= 2);
        _variant = value;
    }

    int GetVariant() {
        return _variant;
    }

protected:
    void _SetShaderContantBuffer(const SIZE& srcSize) override {
        struct {
            INT32 preWidth;
            INT32 preHeight;
            INT32 variant;
        } shaderConstants{
            srcSize.cx,
            srcSize.cy,
            _variant
        };

        _drawInfo->SetPixelShaderConstantBuffer((BYTE*)&shaderConstants, sizeof(shaderConstants));
    }

private:
    int _variant = 0;
};
