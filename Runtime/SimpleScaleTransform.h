#pragma once
#include "pch.h"
#include "SimpleDrawTransform.h"
#include "Utils.h"
#include "GUIDs.h"


// 通用的 scale transform
// 只支持 scale 属性，默认值为 1.0
class SimpleScaleTransform : public SimpleDrawTransform<1> {
protected:
    SimpleScaleTransform(const GUID& shaderID) : SimpleDrawTransform<1>(shaderID) {}

public:
    virtual ~SimpleScaleTransform() {}

    static HRESULT Create(
        _In_ ID2D1EffectContext* d2dEC, 
        _Outptr_ SimpleScaleTransform** ppOutput,
        _In_ const wchar_t* shaderPath,
        const GUID& shaderID
    ) {
        if (!ppOutput) {
            return E_INVALIDARG;
        }

        HRESULT hr = LoadShader(d2dEC, shaderPath, shaderID);
        if (FAILED(hr)) {
            return hr;
        }

        *ppOutput = new SimpleScaleTransform(shaderID);
        return S_OK;
    }

    void SetScale(const D2D1_VECTOR_2F& scale) {
        assert(scale.x > 0 && scale.y > 0);
        _scale = scale;
    }

    D2D1_VECTOR_2F GetScale() const {
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
            lroundf(srcSize.cx * _scale.x),
            lroundf(srcSize.cy * _scale.y)
        };

        *pOutputRect = { 0, 0, destSize.cx, destSize.cy };
        *pOutputOpaqueSubRect = {};

        _SetShaderContantBuffer(srcSize, destSize);

        return S_OK;
    }
protected:
    // 继承的类可以覆盖此方法向着色器传递参数
    virtual void _SetShaderContantBuffer(const SIZE& srcSize, const SIZE& destSize) {
        struct {
            INT32 srcWidth;
            INT32 srcHeight;
            INT32 destWidth;
            INT32 destHeight;
        } shaderConstants{
            srcSize.cx,
            srcSize.cy,
            destSize.cx,
            destSize.cy
        };

        _drawInfo->SetPixelShaderConstantBuffer((BYTE*)&shaderConstants, sizeof(shaderConstants));
    }

private:
    // 缩放倍数
    D2D1_VECTOR_2F _scale{ 1,1 };
};
