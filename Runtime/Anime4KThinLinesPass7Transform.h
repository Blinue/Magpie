#pragma once
#include "pch.h"
#include "SimpleDrawTransform.h"
#include "GUIDs.h"
#include "ShaderPaths.h"


class Anime4KThinLinesPass7Transform : public SimpleDrawTransform<2> {
private:
	Anime4KThinLinesPass7Transform() : SimpleDrawTransform<2>(GUID_MAGPIE_ANIME4K_THINLINES_PASS7_SHADER) {}

public:
    static HRESULT Create(
        _In_ ID2D1EffectContext* d2dEC,
        _Outptr_ Anime4KThinLinesPass7Transform** ppOutput
    ) {
        *ppOutput = nullptr;

        HRESULT hr = DrawTransformBase::LoadShader(
            d2dEC,
            MAGPIE_ANIME4K_THINLINES_PASS7_SHADER,
            GUID_MAGPIE_ANIME4K_THINLINES_PASS7_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }

        *ppOutput = new Anime4KThinLinesPass7Transform();

        return S_OK;
    }

    void SetStrength(float value) {
        assert(value > 0);
        _strength = value;
    }

    float GetStrength() {
        return _strength;
    }

protected:
    void _SetShaderContantBuffer(const SIZE& srcSize) override {
        struct {
            INT32 srcWidth;
            INT32 srcHeight;
            FLOAT strength;
        } shaderConstants{
            srcSize.cx,
            srcSize.cy,
            _strength
        };

        _drawInfo->SetPixelShaderConstantBuffer((BYTE*)&shaderConstants, sizeof(shaderConstants));
    }

private:
    float _strength = 0.2f;
};