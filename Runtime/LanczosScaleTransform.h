#pragma once
#include "pch.h"
#include "SimpleScaleTransform.h"
#include "GUIDs.h"


// 为 LanczosScaleShader.hlsl 提供参数
// 参数：
//   ARStrength：抗振铃强度。必须在 0~1 之间。默认值为 0.5
class LanczosScaleTransform : public SimpleScaleTransform {
private:
    LanczosScaleTransform() : SimpleScaleTransform(GUID_MAGPIE_LANCZOS6_SCALE_SHADER) {}
public:
    static HRESULT Create(_In_ ID2D1EffectContext* d2dEC, _Outptr_ LanczosScaleTransform** ppOutput) {
        if (!ppOutput) {
            return E_INVALIDARG;
        }

        HRESULT hr = LoadShader(d2dEC, MAGPIE_LANCZOS6_SCALE_SHADER, GUID_MAGPIE_LANCZOS6_SCALE_SHADER);
        if (FAILED(hr)) {
            return hr;
        }

        *ppOutput = new LanczosScaleTransform();
        return hr;
    }

    void SetARStrength(float value) {
        assert(value >= 0 && value <= 1);
        _ARStrength = value;
    }

    float GetARStrength() {
        return _ARStrength;
    }

protected:
    void _SetShaderContantBuffer(const SIZE& srcSize, const SIZE& destSize) override {
        struct {
            INT32 srcWidth;
            INT32 srcHeight;
            INT32 destWidth;
            INT32 destHeight;
            float arStrength;
        } shaderConstants{
            srcSize.cx,
            srcSize.cy,
            destSize.cx,
            destSize.cy,
            _ARStrength
        };

        _drawInfo->SetPixelShaderConstantBuffer((BYTE*)&shaderConstants, sizeof(shaderConstants));
    }
private:
    float _ARStrength = 0.5f;
};
