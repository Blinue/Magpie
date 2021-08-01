#pragma once
#include "pch.h"
#include <SimpleDrawTransform.h>
#include "EffectDefines.h"


class MonochromeCursorTransform : public SimpleDrawTransform<2> {
private:
	MonochromeCursorTransform() : SimpleDrawTransform<2>(GUID_MAGPIE_MONOCHROME_CURSOR_SHADER) {}

public:
	static HRESULT Create(
		_In_ ID2D1EffectContext* d2dEC,
		_Outptr_ MonochromeCursorTransform** ppOutput
	) {
		*ppOutput = nullptr;

		HRESULT hr = LoadShader(
			d2dEC,
			MAGPIE_MONOCHROME_CURSOR_SHADER,
			GUID_MAGPIE_MONOCHROME_CURSOR_SHADER
		);
		if (FAILED(hr)) {
			return hr;
		}

		*ppOutput = new MonochromeCursorTransform();

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

		*pOutputRect = pInputRects[0];
		_inputRects[0] = pInputRects[0];
		_inputRects[1] = pInputRects[1];

		*pOutputOpaqueSubRect = { 0,0,0,0 };

		RECT shaderConstants{
			_cursorPos.x,
			_cursorPos.y,
			_cursorPos.x + pInputRects[1].right - pInputRects[1].left,
			_cursorPos.y + (pInputRects[1].bottom - pInputRects[1].top) / 2
		};

		_drawInfo->SetPixelShaderConstantBuffer((BYTE*)&shaderConstants, sizeof(shaderConstants));

		return S_OK;
	}

	const POINT& GetCursorPos() const {
		return _cursorPos;
	}

	void SetCursorPos(const POINT& value) {
		_cursorPos = value;
	}

private:
	POINT _cursorPos{};
};
