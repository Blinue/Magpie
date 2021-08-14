#pragma once
#include "pch.h"
#include <SimpleDrawTransform.h>
#include "EffectDefines.h"


class RAVULitePass2Transform : public SimpleDrawTransform<2> {
private:
	RAVULitePass2Transform() : SimpleDrawTransform<2>(GUID_MAGPIE_RAVU_LITE_R3_PASS2_SHADER) {}

public:
	static HRESULT Create(_In_ ID2D1EffectContext* d2dEC, _Outptr_ RAVULitePass2Transform** ppOutput) {
		*ppOutput = nullptr;

		HRESULT hr = LoadShader(
			d2dEC,
			MAGPIE_RAVU_LITE_R3_PASS2_SHADER,
			GUID_MAGPIE_RAVU_LITE_R3_PASS2_SHADER
		);
		if (FAILED(hr)) {
			return hr;
		}

		*ppOutput = new RAVULitePass2Transform();

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
		if (pInputRects[0].right - pInputRects[0].left != pInputRects[1].right - pInputRects[1].left
			|| pInputRects[0].bottom - pInputRects[0].top != pInputRects[1].bottom - pInputRects[1].top
			) {
			return E_INVALIDARG;
		}

		_inputRects[0] = pInputRects[0];
		_inputRects[1] = pInputRects[1];
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