#pragma once
#include "pch.h"
#include <SimpleDrawTransform.h>
#include "EffectDefines.h"


class RAVUZoomWeightsTransform : public SimpleDrawTransform<1> {
private:
	RAVUZoomWeightsTransform() : SimpleDrawTransform<1>(GUID_MAGPIE_RAVU_ZOOM_R3_WEIGHTS_SHADER) {}
public:
	static HRESULT Create(_In_ ID2D1EffectContext* d2dEC, _Outptr_ RAVUZoomWeightsTransform** ppOutput) {
		if (!ppOutput) {
			return E_INVALIDARG;
		}

		HRESULT hr = LoadShader(
			d2dEC,
			MAGPIE_RAVU_ZOOM_R3_WEIGHTS_SHADER,
			GUID_MAGPIE_RAVU_ZOOM_R3_WEIGHTS_SHADER
		);
		if (FAILED(hr)) {
			return hr;
		}

		*ppOutput = new RAVUZoomWeightsTransform();
		return hr;
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

		// 权重纹理为固定大小
		if (pInputRects[0].right - pInputRects[0].left != 45 * 4
			|| pInputRects[0].bottom - pInputRects[0].top != 2592
			) {
			return E_INVALIDARG;
		}

		_inputRects[0] = pInputRects[0];
		pOutputRect[0] = { 0, 0, 45, 2592 };
		pOutputOpaqueSubRect[0] = { 0,0,0,0 };

		return S_OK;
	}
};
