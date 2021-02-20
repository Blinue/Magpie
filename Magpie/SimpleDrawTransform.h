#pragma once
#include "pch.h"
#include "DrawTransformBase.h"

// 用来取代简单的 DrawTransform
// 该 DrawTransform 必须满足以下条件：
//  * 一个输入
//  * 输出与输入尺寸相同
//  * 只是简单地对输入应用了一个像素着色器
//  * 无状态
class SimpleDrawTransform : public DrawTransformBase {
private:
    SimpleDrawTransform(const GUID &shaderID): _shaderID(shaderID) {}

public:
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

    IFACEMETHODIMP MapInputRectsToOutputRect(
        _In_reads_(inputRectCount) const D2D1_RECT_L* pInputRects,
        _In_reads_(inputRectCount) const D2D1_RECT_L* pInputOpaqueSubRects,
        UINT32 inputRectCount,
        _Out_ D2D1_RECT_L* pOutputRect,
        _Out_ D2D1_RECT_L* pOutputOpaqueSubRect
    ) override {
        HRESULT hr = DrawTransformBase::MapInputRectsToOutputRect(
            pInputRects, 
            pInputOpaqueSubRects, 
            inputRectCount, 
            pOutputRect, 
            pOutputOpaqueSubRect
        );
        if (FAILED(hr)) {
            return hr;
        }

        _shaderConstants = {
            pInputRects[0].right - pInputRects[0].left,
            pInputRects[0].bottom - pInputRects[0].top,
        };
        _drawInfo->SetPixelShaderConstantBuffer((BYTE*)&_shaderConstants, sizeof(_shaderConstants));

        return S_OK;
    }

    IFACEMETHODIMP SetDrawInfo(ID2D1DrawInfo* pDrawInfo) override {
        _drawInfo = pDrawInfo;
        return pDrawInfo->SetPixelShader(_shaderID);
    }

private:
    ComPtr<ID2D1DrawInfo> _drawInfo = nullptr;

    // 输出图像的尺寸
    D2D1_SIZE_U _destSize{};

    struct {
        INT32 width;
        INT32 height;
    } _shaderConstants{};

    const GUID& _shaderID;
};
