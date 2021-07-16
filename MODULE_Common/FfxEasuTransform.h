#pragma once
#include "pch.h"
#include <SimpleScaleTransform.h>
#include "EffectDefines.h"


class FfxEasuTransform : public SimpleScaleTransform {
private:
    FfxEasuTransform() : SimpleScaleTransform(GUID_MAGPIE_FFX_EASU_SHADER) {}
public:
    static HRESULT Create(_In_ ID2D1EffectContext* d2dEC, _Outptr_ FfxEasuTransform** ppOutput) {
        if (!ppOutput) {
            return E_INVALIDARG;
        }

        HRESULT hr = LoadShader(d2dEC, MAGPIE_FFX_EASU_SHADER, GUID_MAGPIE_FFX_EASU_SHADER);
        if (FAILED(hr)) {
            return hr;
        }

        *ppOutput = new FfxEasuTransform();
        return hr;
    }

protected:
    void _SetShaderContantBuffer(const SIZE& srcSize, const SIZE& destSize) override {
        struct {
            INT32 srcWidth;
            INT32 srcHeight;
            INT32 destWidth;
            INT32 destHeight;
        } shaderConstants{
            srcSize.cx,
            srcSize.cy,
            destSize.cx,
            destSize.cy
        };

        _drawInfo->SetPixelShaderConstantBuffer((BYTE*)&shaderConstants, sizeof(shaderConstants));
    }
};
