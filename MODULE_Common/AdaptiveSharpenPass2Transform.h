#pragma once
#include "pch.h"
#include <SimpleDrawTransform.h>
#include "EffectDefines.h"


// Adaptive Sharpen Pass 2 需要 curveHeight 参数
// curveHeight 越大，锐化程度越大
// curveHeight 的取值在 0.3~2 之间，默认值为 0.3
class AdaptiveSharpenPass2Transform : public SimpleDrawTransform<> {
private:
    AdaptiveSharpenPass2Transform(): SimpleDrawTransform<>(GUID_MAGPIE_ADAPTIVE_SHARPEN_PASS2_SHADER) {}
public:
    static HRESULT Create(_In_ ID2D1EffectContext* d2dEC, _Outptr_ AdaptiveSharpenPass2Transform** ppOutput) {
        *ppOutput = nullptr;

        HRESULT hr = LoadShader(
            d2dEC,
            MAGPIE_ADAPTIVE_SHARPEN_PASS2_SHADER,
            GUID_MAGPIE_ADAPTIVE_SHARPEN_PASS2_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }

        *ppOutput = new AdaptiveSharpenPass2Transform();

        return S_OK;
    }

    void SetCurveHeight(float value) {
        assert(value > 0);
        _curveHeight = value;
    }

    float GetCurveHeight() {
        return _curveHeight;
    }
protected:
    void _SetShaderConstantBuffer(const SIZE& srcSize) override {
        struct {
            INT32 srcWidth;
            INT32 srcHeight;
            FLOAT curveHeight;
        } shaderConstants {
            srcSize.cx,
            srcSize.cy,
            _curveHeight
        };

        _drawInfo->SetPixelShaderConstantBuffer((BYTE*)&shaderConstants, sizeof(shaderConstants));
    }

private:
    float _curveHeight = 0.3f;
};