#pragma once
#include "pch.h"
#include "DrawTransformBase.h"


// 用来取代简单的 DrawTransform
// 该 DrawTransform 必须满足以下条件：
//  * NINPUTS 个输入
//  * 输出与所有输入尺寸相同
//  * 只是简单地对输入应用了一个像素着色器
//  * 无状态
template <int NINPUTS = 1>
class SimpleDrawTransform : public DrawTransformBase {
protected:
    SimpleDrawTransform(const GUID &shaderID): _shaderID(shaderID) {}

public:
    virtual ~SimpleDrawTransform() {}

    static HRESULT Create(
        _In_ ID2D1EffectContext* d2dEC,
        _Outptr_ SimpleDrawTransform** ppOutput,
        _In_ const wchar_t* shaderPath,
        const GUID &shaderID
    ) {
        if (!ppOutput) {
            return E_INVALIDARG;
        }

        HRESULT hr = DrawTransformBase::LoadShader(d2dEC, shaderPath, shaderID);
        if (FAILED(hr)) {
            return hr;
        }

        *ppOutput = new SimpleDrawTransform(shaderID);
        return S_OK;
    }

    IFACEMETHODIMP_(UINT32) GetInputCount() const override {
        return NINPUTS;
    }

    IFACEMETHODIMP MapInputRectsToOutputRect(
        _In_reads_(inputRectCount) const D2D1_RECT_L* pInputRects,
        _In_reads_(inputRectCount) const D2D1_RECT_L* pInputOpaqueSubRects,
        UINT32 inputRectCount,
        _Out_ D2D1_RECT_L* pOutputRect,
        _Out_ D2D1_RECT_L* pOutputOpaqueSubRect
    ) override {
        if (inputRectCount != NINPUTS) {
            return E_INVALIDARG;
        }

        for (int i = 0; i < NINPUTS; ++i) {
            if (pInputRects[0].right - pInputRects[0].left != pInputRects[i].right - pInputRects[i].left
                || pInputRects[0].bottom - pInputRects[0].top != pInputRects[i].bottom - pInputRects[i].top)
            {
                return E_INVALIDARG;
            }
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

    IFACEMETHODIMP SetDrawInfo(ID2D1DrawInfo* pDrawInfo) override {
        _drawInfo = pDrawInfo;
        return pDrawInfo->SetPixelShader(_shaderID);
    }

    IFACEMETHODIMP MapOutputRectToInputRects(
        _In_ const D2D1_RECT_L* pOutputRect,
        _Out_writes_(inputRectCount) D2D1_RECT_L* pInputRects,
        UINT32 inputRectCount
    ) const override {
        if (inputRectCount != NINPUTS) {
            return E_INVALIDARG;
        }

        for (int i = 0; i < NINPUTS; ++i) {
            pInputRects[i] = _inputRect;
        }

        return S_OK;
    }

    IFACEMETHODIMP MapInvalidRect(
        UINT32 inputIndex,
        D2D1_RECT_L invalidInputRect,
        _Out_ D2D1_RECT_L* pInvalidOutputRect
    ) const override {
        // This transform is designed to only accept one input.
        if (inputIndex >= NINPUTS) {
            return E_INVALIDARG;
        }

        // If part of the transform's input is invalid, mark the corresponding
        // output region as invalid. 
        *pInvalidOutputRect = D2D1::RectL(LONG_MIN, LONG_MIN, LONG_MAX, LONG_MAX);

        return S_OK;
    }

protected:
    // 继承的类可以覆盖此方法向着色器传递参数
    virtual void _SetShaderContantBuffer(const SIZE& srcSize) {
        struct {
            INT32 srcWidth;
            INT32 srcHeight;
        } shaderConstants{
            srcSize.cx,
            srcSize.cy
        };

        _drawInfo->SetPixelShaderConstantBuffer((BYTE*)&shaderConstants, sizeof(shaderConstants));
    }

    ComPtr<ID2D1DrawInfo> _drawInfo = nullptr;

private:
    const GUID& _shaderID;
};
