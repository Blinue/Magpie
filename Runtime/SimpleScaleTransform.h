#pragma once
#include "pch.h"
#include "GUIDs.h"
#include "SimpleDrawTransform.h"
#include "Utils.h"


// 通用的 scale transform
// 只支持 scale 属性，默认值为 1.0
class SimpleScaleTransform : public SimpleDrawTransform {
protected:
    SimpleScaleTransform(const GUID& shaderID) : SimpleDrawTransform(shaderID) {}

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

        HRESULT hr = DrawTransformBase::LoadShader(d2dEC, shaderPath, shaderID);
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

        _inputRect = pInputRects[0];

        const auto& srcSize = Utils::GetSize(_inputRect);
        SIZE destSize = {
            lroundf(srcSize.cx * _scale.x),
            lroundf(srcSize.cy * _scale.y)
        };

        *pOutputRect = {
            _inputRect.left,
            _inputRect.top,
            _inputRect.left + destSize.cx,
            _inputRect.top + destSize.cy
        };
        *pOutputOpaqueSubRect = { 0,0,0,0 };

        _SetShaderContantBuffer(srcSize, destSize);

        return S_OK;
    }

    IFACEMETHODIMP MapOutputRectToInputRects(
        _In_ const D2D1_RECT_L* pOutputRect,
        _Out_writes_(inputRectCount) D2D1_RECT_L* pInputRects,
        UINT32 inputRectCount
    ) const override {
        if (inputRectCount != 1) {
            return E_INVALIDARG;
        }

        *pInputRects = _inputRect;

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
    // 保存输入图像的尺寸
    D2D1_RECT_L _inputRect{};

    // 缩放倍数
    D2D1_VECTOR_2F _scale{ 1,1 };
};
