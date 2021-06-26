#pragma once
#include "pch.h"
#include <SimpleDrawTransform.h>
#include "EffectDefines.h"



class ACNetL10Transform : public SimpleDrawTransform<3> {
private:
    ACNetL10Transform() : SimpleDrawTransform<3>(GUID_MAGPIE_ACNET_L10_SHADER) {}

public:
    static HRESULT Create(_In_ ID2D1EffectContext* d2dEC, _Outptr_ ACNetL10Transform** ppOutput) {
        *ppOutput = nullptr;

        HRESULT hr = LoadShader(
            d2dEC,
            MAGPIE_ACNET_L10_SHADER,
            GUID_MAGPIE_ACNET_L10_SHADER
        );
        if (FAILED(hr)) {
            return hr;
        }

        *ppOutput = new ACNetL10Transform();

        return S_OK;
    }


    IFACEMETHODIMP MapInputRectsToOutputRect(
        _In_reads_(inputRectCount) const D2D1_RECT_L* pInputRects,
        _In_reads_(inputRectCount) const D2D1_RECT_L* pInputOpaqueSubRects,
        UINT32 inputRectCount,
        _Out_ D2D1_RECT_L* pOutputRect,
        _Out_ D2D1_RECT_L* pOutputOpaqueSubRect
    ) override {
        if (inputRectCount != 3) {
            return E_INVALIDARG;
        }
        for (int i = 0; i < 3; ++i) {
            if (pInputRects[0].right - pInputRects[0].left != pInputRects[i].right - pInputRects[i].left
                || pInputRects[0].bottom - pInputRects[0].top != pInputRects[i].bottom - pInputRects[i].top) {
                return E_INVALIDARG;
            }

            _inputRects[i] = pInputRects[i];
        }

        *pOutputRect = {
            0,
            0,
            _inputRects[0].right * 2,
            _inputRects[0].bottom * 2,
        };
        *pOutputOpaqueSubRect = {};

        return S_OK;
    }
};
