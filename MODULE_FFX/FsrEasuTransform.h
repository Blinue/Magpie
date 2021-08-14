#pragma once
#include "pch.h"
#include <SimpleScaleTransform.h>
#include "EffectDefines.h"


class FsrEasuTransform : public SimpleScaleTransform {
private:
	FsrEasuTransform() : SimpleScaleTransform(GUID_MAGPIE_FSR_EASU_SHADER) {}
public:
	static HRESULT Create(_In_ ID2D1EffectContext* d2dEC, _Outptr_ FsrEasuTransform** ppOutput) {
		if (!ppOutput) {
			return E_INVALIDARG;
		}

		HRESULT hr = LoadShader(d2dEC, MAGPIE_FSR_EASU_SHADER, GUID_MAGPIE_FSR_EASU_SHADER);
		if (FAILED(hr)) {
			return hr;
		}

		*ppOutput = new FsrEasuTransform();
		return hr;
	}

protected:
	void _SetShaderConstantBuffer(const SIZE& srcSize, const SIZE& destSize) override {
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
};
