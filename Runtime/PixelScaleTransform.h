#pragma once
#include "pch.h"
#include "SimpleDrawTransform.h"
#include "Utils.h"


class PixelScaleTransform : public SimpleDrawTransform<> {
protected:
    PixelScaleTransform() : SimpleDrawTransform<>(GUID_MAGPIE_PIXEL_SCALE_SHADER) {}

public:
    virtual ~PixelScaleTransform() {}

    static HRESULT Create(
        _In_ ID2D1EffectContext* d2dEC,
        _Outptr_ PixelScaleTransform** ppOutput
    ) {
        if (!ppOutput) {
            return E_INVALIDARG;
        }

        HRESULT hr = LoadShader(d2dEC, MAGPIE_PIXEL_SCALE_SHADER, GUID_MAGPIE_PIXEL_SCALE_SHADER);
        if (FAILED(hr)) {
            return hr;
        }

        *ppOutput = new PixelScaleTransform();
        return S_OK;
    }

    void SetScale(int scale) {
        assert(scale > 0);
        _scale = scale;
    }

    int GetScale() const {
        return _scale;
    }

    IFACEMETHODIMP MapInputRectsToOutputRect(
        _In_reads_(inputRectCount) const D2D1_RECT_L* pInputRects,
        _In_reads_(inputRectCount) const D2D1_RECT_L* pInputOpaqueSubRects,
        UINT32 inputRectCount,
        _Out_ D2D1_RECT_L* pOutputRect,
        _Out_ D2D1_RECT_L* pOutputOpaqueSubRect
    ) override {
        if (inputRectCount != 1) {
            return E_INVALIDARG;
        }

        _inputRects[0] = pInputRects[0];

        const auto& srcSize = Utils::GetSize(_inputRects[0]);
        SIZE destSize = {
            srcSize.cx * _scale,
            srcSize.cy * _scale
        };

        *pOutputRect = { 0, 0, destSize.cx, destSize.cy };
        *pOutputOpaqueSubRect = {};

        _SetShaderContantBuffer(srcSize);

        return S_OK;
    }

protected:
    void _SetShaderContantBuffer(const SIZE& srcSize) override {
        struct {
            INT32 srcWidth;
            INT32 srcHeight;
            INT32 scale;
        } shaderConstants{
            srcSize.cx,
            srcSize.cy,
            _scale
        };

        _drawInfo->SetPixelShaderConstantBuffer((BYTE*)&shaderConstants, sizeof(shaderConstants));
    }

private:
    // Ëõ·Å±¶Êý
    int _scale = 1;
};
