#pragma once
#include "pch.h"
#include <SimpleDrawTransform.h>
#include "EffectDefines.h"


class SSimSuperResFinalTransform : public SimpleDrawTransform<5> {
private:
	SSimSuperResFinalTransform() : SimpleDrawTransform<5>(GUID_MAGPIE_SSIM_SUPERRES_FINAL_SHADER) {}

public:
	static HRESULT Create(_In_ ID2D1EffectContext* d2dEC, _Outptr_ SSimSuperResFinalTransform** ppOutput) {
		*ppOutput = nullptr;

		HRESULT hr = LoadShader(
			d2dEC,
			MAGPIE_SSIM_SUPERRES_FINAL_SHADER,
			GUID_MAGPIE_SSIM_SUPERRES_FINAL_SHADER
		);
		if (FAILED(hr)) {
			return hr;
		}

		*ppOutput = new SSimSuperResFinalTransform();

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

		_inputRects[0] = pInputRects[0];
		_inputRects[1] = pInputRects[1];
		for (int i = 2; i < 5; ++i) {
			if (pInputRects[1].right - pInputRects[1].left != pInputRects[i].right - pInputRects[i].left
				|| pInputRects[1].bottom - pInputRects[1].top != pInputRects[i].bottom - pInputRects[i].top) {
				return E_INVALIDARG;
			}

			_inputRects[i] = pInputRects[i];
		}

		*pOutputRect = pInputRects[1];
		*pOutputOpaqueSubRect = { 0,0,0,0 };

		struct {
			INT32 srcWidth;
			INT32 srcHeight;
			INT32 destWidth;
			INT32 destHeight;
		} shaderConstants{
			_inputRects[0].right - _inputRects[0].left,
			_inputRects[0].bottom - _inputRects[0].top,
			_inputRects[1].right - _inputRects[1].left,
			_inputRects[1].bottom - _inputRects[1].top
		};

		_drawInfo->SetPixelShaderConstantBuffer((BYTE*)&shaderConstants, sizeof(shaderConstants));

		return S_OK;
	}
};