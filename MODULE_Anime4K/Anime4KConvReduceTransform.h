#pragma once
#include "pch.h"
#include <SimpleDrawTransform.h>
#include "EffectDefines.h"


class Anime4KConvReduceTransform : public SimpleDrawTransform<5> {
private:
    Anime4KConvReduceTransform(const GUID& shaderID) : SimpleDrawTransform<5>(shaderID) {}

public:
    static HRESULT Create(_In_ ID2D1EffectContext* d2dEC, _Outptr_ Anime4KConvReduceTransform** ppOutput, bool useDenoiseVersion) {
        *ppOutput = nullptr;

        const GUID& shaderID = useDenoiseVersion ? GUID_MAGPIE_ANIME4K_DENOISE_CONV_REDUCE_SHADER : GUID_MAGPIE_ANIME4K_CONV_REDUCE_SHADER;
        HRESULT hr = LoadShader(
            d2dEC,
            useDenoiseVersion ? MAGPIE_ANIME4K_DENOISE_CONV_REDUCE_SHADER : MAGPIE_ANIME4K_CONV_REDUCE_SHADER,
            shaderID
        );
        if (FAILED(hr)) {
            return hr;
        }

        *ppOutput = new Anime4KConvReduceTransform(shaderID);

        return S_OK;
    }


    IFACEMETHODIMP MapInputRectsToOutputRect(
        _In_reads_(inputRectCount) const D2D1_RECT_L* pInputRects,
        _In_reads_(inputRectCount) const D2D1_RECT_L* pInputOpaqueSubRects,
        UINT32 inputRectCount,
        _Out_ D2D1_RECT_L* pOutputRect,
        _Out_ D2D1_RECT_L* pOutputOpaqueSubRect
    ) override {
        if (inputRectCount != 5) {
            return E_INVALIDARG;
        }

        for (int i = 0; i < 5; ++i) {
            if (pInputRects[0].right - pInputRects[0].left != pInputRects[i].right - pInputRects[i].left
                || pInputRects[0].bottom - pInputRects[0].top != pInputRects[i].bottom - pInputRects[i].top) {
                return E_INVALIDARG;
            }

            _inputRects[i] = pInputRects[i];
        }

        *pOutputRect = pInputRects[0];
        *pOutputOpaqueSubRect = { 0,0,0,0 };

        return S_OK;
    }
};