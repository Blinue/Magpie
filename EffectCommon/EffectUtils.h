#pragma once
#include <d2d1_3.h>
#include <vector>
#include <wincodec.h>
#include <nlohmann/json.hpp>
#include <string>

using namespace Microsoft::WRL;


class EffectUtils {
public:
	static HRESULT IsEffectRegistered(ID2D1Factory1* d2dFactory, const CLSID& effectID, bool& result) {
		UINT32 n;
		HRESULT hr = d2dFactory->GetRegisteredEffects(nullptr, 0, nullptr, &n);
		if (FAILED(hr)) {
			return hr;
		}

		std::vector<CLSID> effects(n);
		hr = d2dFactory->GetRegisteredEffects(effects.data(), n, &n, nullptr);
		if (FAILED(hr)) {
			return hr;
		}

		auto it = std::find(effects.begin(), effects.end(), effectID);
		result = it != effects.end();

		return S_OK;
	}

	static HRESULT ReadScaleProp(
		const nlohmann::json& props,
		float fillScale,
		const std::pair<float, float>& scale,
		std::pair<float, float>& result
	) {
		if (!props.is_array() || props.size() != 2 || !props[0].is_number() || !props[1].is_number()) {
			return E_INVALIDARG;
		}

		std::pair<float, float> origin = { props[0], props[1] };
		if (origin.first == 0 || origin.second == 0) {
			return E_INVALIDARG;
		}

		result = {
			origin.first > 0 ? origin.first : -origin.first * fillScale / scale.first,
			origin.second > 0 ? origin.second : -origin.second * fillScale / scale.second
		};
		return S_OK;
	}

	static HRESULT LoadBitmapFromHBmp(IWICImagingFactory2* wicImgFactory, ID2D1DeviceContext* d2dDC, HBITMAP hBmp, ComPtr<ID2D1Bitmap>& d2dBmp) {
		ComPtr<IWICBitmap> wicBmp;
		HRESULT hr = wicImgFactory->CreateBitmapFromHBITMAP(hBmp, nullptr, WICBitmapIgnoreAlpha, &wicBmp);
		if (FAILED(hr)) {
			return hr;
		}
		
		ComPtr<ID2D1Bitmap1> result;
		hr = d2dDC->CreateBitmapFromWicBitmap(wicBmp.Get(), &result);
		if (FAILED(hr)) {
			return hr;
		}
		
		d2dBmp = std::move(result);
		return S_OK;
	}

	static HRESULT UTF8ToUTF16(std::string_view str, std::wstring& result) {
		assert(str.size() > 0);

		int convertResult = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0);
		if (convertResult <= 0) {
			return E_FAIL;
		}

		std::wstring r(convertResult + 10, L'\0');
		convertResult = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), &r[0], (int)r.size());
		if (convertResult <= 0) {
			return E_FAIL;
		}

		result = std::wstring(r.begin(), r.begin() + convertResult);
		return S_OK;
	}
};
