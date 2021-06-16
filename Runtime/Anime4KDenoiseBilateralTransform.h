#pragma once
#include "pch.h"
#include "SimpleDrawTransform.h"
#include "GUIDs.h"
#include "ShaderPaths.h"


class Anime4KDenoiseBilateralTransform : public SimpleDrawTransform<> {
private:
    Anime4KDenoiseBilateralTransform() : SimpleDrawTransform<>(GUID_MAGPIE_ANIME4K_DENOISE_BILATERAL_SHADER) {}

public:
    static HRESULT Create(
        _In_ ID2D1EffectContext* d2dEC,
        _Outptr_ Anime4KDenoiseBilateralTransform** ppOutput
    ) {
        *ppOutput = nullptr;

        HRESULT hr = LoadShader(
            d2dEC,
            MAGPIE_ANIME4K_DENOISE_BILATERAL_SHADER,
            GUID_MAGPIE_ANIME4K_DENOISE_BILATERAL_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }

        *ppOutput = new Anime4KDenoiseBilateralTransform();

        return S_OK;
    }

    void SetVariant(int value) {
        assert(value >= 0 && value <= 2);
        _variant = value;
    }

    int GetVariant() {
        return _variant;
    }

    void SetIntensitySigma(float value) {
        assert(value > 0);
        _intensitySigma = value;
    }

    float GetIntensitySigma() {
        return _intensitySigma;
    }

protected:
    void _SetShaderContantBuffer(const SIZE& srcSize) override {
        struct {
            INT32 srcWidth;
            INT32 srcHeight;
            INT32 variant;
            FLOAT intensitySigma;
        } shaderConstants{
            srcSize.cx,
            srcSize.cy,
            _variant,
            _intensitySigma
        };

        _drawInfo->SetPixelShaderConstantBuffer((BYTE*)&shaderConstants, sizeof(shaderConstants));
    }

private:
    int _variant = 0;
    float _intensitySigma = 0.1f;
};