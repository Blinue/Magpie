#pragma once
#include "pch.h"
#include <SimpleDrawTransform.h>
#include "EffectDefines.h"


class RAVUZoomTransform
	: public SimpleDrawTransform<2> {
private:
	RAVUZoomTransform() : SimpleDrawTransform<2>(GUID_MAGPIE_RAVU_ZOOM_R3_SHADER) {}
public:
	static HRESULT Create(_In_ ID2D1EffectContext* d2dEC, _Outptr_ RAVUZoomTransform** ppOutput) {
		if (!ppOutput) {
			return E_INVALIDARG;
		}

		HRESULT hr = LoadShader(
			d2dEC,
			MAGPIE_RAVU_ZOOM_R3_SHADER,
			GUID_MAGPIE_RAVU_ZOOM_R3_SHADER
		);
		if (FAILED(hr)) {
			return hr;
		}

		*ppOutput = new RAVUZoomTransform();
		return hr;
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

		// 权重纹理为固定大小
		if (pInputRects[1].right - pInputRects[1].left != 45
			|| pInputRects[1].bottom - pInputRects[1].top != 2592
		) {
			return E_INVALIDARG;
		}

		_inputRects[0] = pInputRects[0];
		_inputRects[1] = pInputRects[1];

		SIZE srcSize = { _inputRects[0].right - _inputRects[0].left, _inputRects[0].bottom - _inputRects[0].top };
		SIZE destSize = {
			lroundf(srcSize.cx * _scale.x),
			lroundf(srcSize.cy * _scale.y)
		};

		pOutputRect[0] = { 0, 0, destSize.cx, destSize.cy };
		pOutputOpaqueSubRect[0] = { 0,0,0,0 };

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
		return S_OK;
	}

	void SetScale(const D2D1_VECTOR_2F& scale) {
		assert(scale.x > 0 && scale.y > 0);
		_scale = scale;
	}

	D2D1_VECTOR_2F GetScale() const {
		return _scale;
	}

private:
	// 缩放倍数
	D2D1_VECTOR_2F _scale{ 1,1 };
};
