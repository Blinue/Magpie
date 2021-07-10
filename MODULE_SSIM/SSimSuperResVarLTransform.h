#pragma once
#include "pch.h"
#include <SimpleDrawTransform.h>
#include "EffectDefines.h"


class SSimSuperResVarLTransform : public SimpleDrawTransform<2> {
private:
    SSimSuperResVarLTransform() : SimpleDrawTransform<2>(GUID_MAGPIE_SSIM_SUPERRES_VARL_SHADER) {}

public:
    static HRESULT Create(_In_ ID2D1EffectContext* d2dEC, _Outptr_ SSimSuperResVarLTransform** ppOutput) {
        *ppOutput = nullptr;

        HRESULT hr = LoadShader(
            d2dEC,
            MAGPIE_SSIM_SUPERRES_VARL_SHADER,
            GUID_MAGPIE_SSIM_SUPERRES_VARL_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }

        *ppOutput = new SSimSuperResVarLTransform();

        return S_OK;
    }


    IFACEMETHODIMP MapInputRectsToOutputRect(
        _In_reads_(inputRectCount) const D2D1_RECT_L* pInputRects,
        _In_reads_(inputRectCount) const D2D1_RECT_L* pInputOpaqueSubRects,
        UINT32 inputRectCount,
        _Out_ D2D1_RECT_L* pOutputRect,
        _Out_ D2D1_RECT_L* pOutputOpaqueSubRect
    ) override {
        if (inputRectCount != 2) {
            return E_INVALIDARG;
        }

        _inputRects[0] = pInputRects[0];
        _inputRects[1] = pInputRects[1];

        *pOutputRect = _inputRects[1];
        *pOutputOpaqueSubRect = {};

        struct {
            INT32 srcWidth;
            INT32 srcHeight;
            INT32 destWidth;
            INT32 destHeight;
        } shaderConstants{
            _inputRects[0].right - _inputRects[0].left,
            _inputRects[0].bottom - _inputRects[0].top,
            _inputRects[1].right - _inputRects[1].left,
            _inputRects[1].bottom - _inputRects[1].top
        };

        _drawInfo->SetPixelShaderConstantBuffer((BYTE*)&shaderConstants, sizeof(shaderConstants));

        return S_OK;
    }
};