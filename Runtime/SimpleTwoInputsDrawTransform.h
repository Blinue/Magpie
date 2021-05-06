#pragma once
#include "pch.h"
#include "SimpleDrawTransform.h"


// 类似 SimpleDrawTransform，但有两个相同大小的输入
class SimpleTwoInputsDrawTransform : public SimpleDrawTransform {
protected:
    SimpleTwoInputsDrawTransform(const GUID& shaderID) : SimpleDrawTransform(shaderID) {}

public:
    virtual ~SimpleTwoInputsDrawTransform() {}

    static HRESULT Create(
        _In_ ID2D1EffectContext* d2dEC,
        _Outptr_ SimpleTwoInputsDrawTransform** ppOutput,
        _In_ const wchar_t* shaderPath,
        const GUID& shaderID
    ) {
        *ppOutput = nullptr;

        HRESULT hr = DrawTransformBase::LoadShader(
            d2dEC, shaderPath, shaderID
        );
        if (FAILED(hr)) {
            return hr;
        }

        *ppOutput = new SimpleTwoInputsDrawTransform(shaderID);

        return S_OK;
    }

    // ID2D1TransformNode Methods:
    IFACEMETHODIMP_(UINT32) GetInputCount() const override {
        return 2;
    }

    // ID2D1Transform Methods:
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
        if (pInputRects[0].right - pInputRects[0].left != pInputRects[1].right - pInputRects[1].left
            || pInputRects[0].bottom - pInputRects[0].top != pInputRects[1].bottom - pInputRects[1].top
            ) {
            return E_INVALIDARG;
        }

        *pOutputRect = pInputRects[0];
        _inputRect = pInputRects[0];
        *pOutputOpaqueSubRect = { 0,0,0,0 };

        _SetShaderContantBuffer(SIZE{
            pInputRects->right - pInputRects->left,
            pInputRects->bottom - pInputRects->top
            });

        return S_OK;
    }

    IFACEMETHODIMP MapOutputRectToInputRects(
        _In_ const D2D1_RECT_L* pOutputRect,
        _Out_writes_(inputRectCount) D2D1_RECT_L* pInputRects,
        UINT32 inputRectCount
    ) const override {
        if (inputRectCount != 2) {
            return E_INVALIDARG;
        }

        pInputRects[0] = _inputRect;
        pInputRects[1] = _inputRect;

        return S_OK;
    }

    IFACEMETHODIMP MapInvalidRect(
        UINT32 inputIndex,
        D2D1_RECT_L invalidInputRect,
        _Out_ D2D1_RECT_L* pInvalidOutputRect
    ) const override {
        // This transform is designed to only accept one input.
        if (inputIndex >= 2) {
            return E_INVALIDARG;
        }

        // If part of the transform's input is invalid, mark the corresponding
        // output region as invalid. 
        *pInvalidOutputRect = D2D1::RectL(LONG_MIN, LONG_MIN, LONG_MAX, LONG_MAX);

        return S_OK;
    }
};