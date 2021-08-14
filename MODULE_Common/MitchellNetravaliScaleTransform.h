#pragma once
#include "pch.h"
#include <SimpleScaleTransform.h>
#include "EffectDefines.h"


// 为 MitchellNetravaliScaleShader.hlsl 提供参数
class MitchellNetravaliScaleTransform : public SimpleScaleTransform {
private:
	MitchellNetravaliScaleTransform() : SimpleScaleTransform(GUID_MAGPIE_MITCHELL_NETRAVALI_SCALE_SHADER) {}
public:
	static HRESULT Create(_In_ ID2D1EffectContext* d2dEC, _Outptr_ MitchellNetravaliScaleTransform** ppOutput) {
		if (!ppOutput) {
			return E_INVALIDARG;
		}

		HRESULT hr = LoadShader(
			d2dEC, 
			MAGPIE_MITCHELL_NETRAVALI_SCALE_SHADER, 
			GUID_MAGPIE_MITCHELL_NETRAVALI_SCALE_SHADER
		);
		if (FAILED(hr)) {
			return hr;
		}

		*ppOutput = new MitchellNetravaliScaleTransform();
		return hr;
	}

	void SetVariant(int value) {
		assert(value >= 0 && value <= 2);
		_variant = value;
	}

	int GetVariant() {
		return _variant;
	}

protected:
	void _SetShaderConstantBuffer(const SIZE& srcSize, const SIZE& destSize) override {
		struct {
			INT32 srcWidth;
			INT32 srcHeight;
			INT32 destWidth;
			INT32 destHeight;
			INT32 variant;
		} shaderConstants{
			srcSize.cx,
			srcSize.cy,
			destSize.cx,
			destSize.cy,
			_variant
		};

		_drawInfo->SetPixelShaderConstantBuffer((BYTE*)&shaderConstants, sizeof(shaderConstants));
	}

private:
	int _variant = 0;
};
