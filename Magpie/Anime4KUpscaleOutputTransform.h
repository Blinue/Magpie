#pragma once
#include "pch.h"
#include "GUIDs.h"
#include "DrawTransformBase.h"


class Anime4KUpscaleOutputTransform : public DrawTransformBase {
private:
    Anime4KUpscaleOutputTransform() {}
public:
    static HRESULT Create(_In_ ID2D1EffectContext* d2dEC, _Outptr_ Anime4KUpscaleOutputTransform** ppOutput) {
        *ppOutput = nullptr;

        HRESULT hr = DrawTransformBase::LoadShader(
            d2dEC, 
            MAGPIE_ANIME4K_UPSCALE_OUTPUT_SHADER, 
            GUID_MAGPIE_ANIME4K_UPSCALE_OUTPUT_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }

        *ppOutput = new Anime4KUpscaleOutputTransform();

        return S_OK;
    }

    // ID2D1TransformNode Methods:
    IFACEMETHODIMP_(UINT32) GetInputCount() const {
        return 2;
    }

    IFACEMETHODIMP MapInputRectsToOutputRect(
        _In_reads_(inputRectCount) const D2D1_RECT_L* pInputRects,
        _In_reads_(inputRectCount) const D2D1_RECT_L* pInputOpaqueSubRects,
        UINT32 inputRectCount,
        _Out_ D2D1_RECT_L* pOutputRect,
        _Out_ D2D1_RECT_L* pOutputOpaqueSubRect
    ) {
        if (inputRectCount != 2) {
            return E_INVALIDARG;
        }
        if (pInputRects[0].right - pInputRects[0].left != pInputRects[1].right - pInputRects[1].left
            || pInputRects[0].bottom - pInputRects[0].top != pInputRects[1].bottom - pInputRects[1].top
            ) {
            return E_INVALIDARG;
        }

        _inputRect = *pInputRects;
        *pOutputRect = { 
            pInputRects[0].left,
            pInputRects[0].top,
            2 * pInputRects[0].right - pInputRects[0].left,
            2 * pInputRects[0].bottom - pInputRects[0].top,
        };
        *pOutputOpaqueSubRect = { 0,0,0,0 };

        _shaderConstants = {
            pInputRects[0].right - pInputRects[0].left,
            pInputRects[0].bottom - pInputRects[0].top
        };
        _drawInfo->SetPixelShaderConstantBuffer((BYTE*)&_shaderConstants, sizeof(_shaderConstants));

        return S_OK;
    }

    IFACEMETHODIMP MapOutputRectToInputRects(
        _In_ const D2D1_RECT_L* pOutputRect,
        _Out_writes_(inputRectCount) D2D1_RECT_L* pInputRects,
        UINT32 inputRectCount
    ) const {
        if (inputRectCount != 2) {
            return E_INVALIDARG;
        }

        // The input needed for the transform is the same as the visible output.
        pInputRects[0] = _inputRect;
        pInputRects[1] = _inputRect;

        return S_OK;
    }

    IFACEMETHODIMP MapInvalidRect(
        UINT32 inputIndex,
        D2D1_RECT_L invalidInputRect,
        _Out_ D2D1_RECT_L* pInvalidOutputRect
    ) const {
        // This transform is designed to only accept one input.
        if (inputIndex >= 2) {
            return E_INVALIDARG;
        }

        // If part of the transform's input is invalid, mark the corresponding
        // output region as invalid. 
        *pInvalidOutputRect = D2D1::RectL(LONG_MIN, LONG_MIN, LONG_MAX, LONG_MAX);

        return S_OK;
    }

    IFACEMETHODIMP SetDrawInfo(ID2D1DrawInfo* pDrawInfo) {
        _drawInfo = pDrawInfo;
        return pDrawInfo->SetPixelShader(GUID_MAGPIE_ANIME4K_UPSCALE_OUTPUT_SHADER);
    }

private:
    D2D1_RECT_L _inputRect;

    ComPtr<ID2D1DrawInfo> _drawInfo = nullptr;

    struct {
        INT32 width;
        INT32 height;
    } _shaderConstants{};
};