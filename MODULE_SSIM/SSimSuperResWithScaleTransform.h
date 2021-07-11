#pragma once
#include "pch.h"
#include <SimpleDrawTransform.h>
#include "EffectDefines.h"


// 第二个输入只是为了传入缩放倍数
class SSimSuperResWithScaleTransform : public SimpleDrawTransform<2> {
private:
    SSimSuperResWithScaleTransform(const GUID& shaderId) : SimpleDrawTransform<2>(shaderId) {}

public:
    static HRESULT Create(
        _In_ ID2D1EffectContext* d2dEC,
        _Outptr_ SSimSuperResWithScaleTransform** ppOutput,
        _In_ const wchar_t* path,
        const GUID& shaderId
    ) {
        *ppOutput = nullptr;

        HRESULT hr = LoadShader(d2dEC, path, shaderId);
        if (FAILED(hr)) {
            return hr;
        }

        *ppOutput = new SSimSuperResWithScaleTransform(shaderId);

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

        *pOutputRect = _inputRects[0];
        *pOutputOpaqueSubRect = {};

        int srcWidth = _inputRects[0].right - _inputRects[0].left;
        int srcHeight = _inputRects[0].bottom - _inputRects[0].top;
        struct {
            INT32 srcWidth;
            INT32 srcHeight;
            FLOAT scaleX;
            FLOAT scaleY;
        } shaderConstants{
            srcWidth,
            srcHeight,
            float(srcWidth) / (_inputRects[1].right - _inputRects[1].left),
            float(srcHeight) / (_inputRects[1].bottom - _inputRects[1].top)
        };

        _drawInfo->SetPixelShaderConstantBuffer((BYTE*)&shaderConstants, sizeof(shaderConstants));

        return S_OK;
    }
};