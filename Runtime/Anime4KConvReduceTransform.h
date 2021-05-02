#pragma once
#include "pch.h"
#include "SimpleDrawTransform.h"


class Anime4KConvReduceTransform : public SimpleDrawTransform {
    Anime4KConvReduceTransform(const GUID& shaderID) : SimpleDrawTransform(shaderID) {}
public:
    virtual ~Anime4KConvReduceTransform() {}

    static HRESULT Create(
        _In_ ID2D1EffectContext* d2dEC,
        _Outptr_ Anime4KConvReduceTransform** ppOutput,
        bool useDenoise
    ) {
        *ppOutput = nullptr;

        const wchar_t* shaderPath = useDenoise ? MAGPIE_ANIME4K_DENOISE_CONV_REDUCE_SHADER : MAGPIE_ANIME4K_CONV_REDUCE_SHADER;
        const GUID& shaderID = useDenoise 
            ? GUID_MAGPIE_ANIME4K_DENOISE_CONV_REDUCE_SHADER
            : GUID_MAGPIE_ANIME4K_CONV_REDUCE_SHADER;

        HRESULT hr = DrawTransformBase::LoadShader(d2dEC, shaderPath, shaderID);
        if (FAILED(hr)) {
            return hr;
        }

        *ppOutput = new Anime4KConvReduceTransform(shaderID);

        return S_OK;
    }

    // ID2D1TransformNode Methods:
    IFACEMETHODIMP_(UINT32) GetInputCount() const {
        return 5;
    }

    // ID2D1Transform Methods:
    IFACEMETHODIMP MapInputRectsToOutputRect(
        _In_reads_(inputRectCount) const D2D1_RECT_L* pInputRects,
        _In_reads_(inputRectCount) const D2D1_RECT_L* pInputOpaqueSubRects,
        UINT32 inputRectCount,
        _Out_ D2D1_RECT_L* pOutputRect,
        _Out_ D2D1_RECT_L* pOutputOpaqueSubRect
    ) {
        if (inputRectCount != 5) {
            return E_INVALIDARG;
        }
        if (pInputRects[0].right - pInputRects[0].left != pInputRects[1].right - pInputRects[1].left
            || pInputRects[0].bottom - pInputRects[0].top != pInputRects[1].bottom - pInputRects[1].top
            || pInputRects[0].right - pInputRects[0].left != pInputRects[2].right - pInputRects[2].left
            || pInputRects[0].bottom - pInputRects[0].top != pInputRects[2].bottom - pInputRects[2].top
            || pInputRects[0].right - pInputRects[0].left != pInputRects[3].right - pInputRects[3].left
            || pInputRects[0].bottom - pInputRects[0].top != pInputRects[3].bottom - pInputRects[3].top
            || pInputRects[0].right - pInputRects[0].left != pInputRects[4].right - pInputRects[4].left
            || pInputRects[0].bottom - pInputRects[0].top != pInputRects[4].bottom - pInputRects[4].top
            ) {
            return E_INVALIDARG;
        }

        *pOutputRect = pInputRects[0];
        _inputRect = pInputRects[0];
        *pOutputOpaqueSubRect = { 0,0,0,0 };

        return S_OK;
    }

    IFACEMETHODIMP MapOutputRectToInputRects(
        _In_ const D2D1_RECT_L* pOutputRect,
        _Out_writes_(inputRectCount) D2D1_RECT_L* pInputRects,
        UINT32 inputRectCount
    ) const {
        if (inputRectCount != 5) {
            return E_INVALIDARG;
        }

        // The input needed for the transform is the same as the visible output.
        for (int i = 0; i < 5; ++i) {
            pInputRects[i] = _inputRect;
        }

        return S_OK;
    }

    IFACEMETHODIMP MapInvalidRect(
        UINT32 inputIndex,
        D2D1_RECT_L invalidInputRect,
        _Out_ D2D1_RECT_L* pInvalidOutputRect
    ) const {
        // This transform is designed to only accept one input.
        if (inputIndex >= 5) {
            return E_INVALIDARG;
        }

        // If part of the transform's input is invalid, mark the corresponding
        // output region as invalid. 
        *pInvalidOutputRect = D2D1::RectL(LONG_MIN, LONG_MIN, LONG_MAX, LONG_MAX);

        return S_OK;
    }
};