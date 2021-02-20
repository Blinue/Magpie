#pragma once
#include "pch.h"
#include "GUIDs.h"
#include "SimpleScaleTransform.h"

// Jinc2 ²åÖµËã·¨
class Jinc2Transform : public SimpleScaleTransform {
    Jinc2Transform() : SimpleScaleTransform(GUID_MAGPIE_JINC2_SCALE_SHADER) {}
public:
    static HRESULT Create(_In_ ID2D1EffectContext* d2dEC, _Outptr_ Jinc2Transform** ppOutput) {
        if (!ppOutput) {
            return E_INVALIDARG;
        }

        HRESULT hr = DrawTransformBase::LoadShader(d2dEC, JINC2_SCALE_SHADER, GUID_MAGPIE_JINC2_SCALE_SHADER);
        if (FAILED(hr)) {
            return hr;
        }

        *ppOutput = new Jinc2Transform();
        return hr;
    }
private:
    
};
