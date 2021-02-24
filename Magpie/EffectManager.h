#pragma once
#include "pch.h"
#include "AdaptiveSharpenEffect.h"
#include "Anime4KUpscaleEffect.h"
#include "Anime4KUpscaleDeblurEffect.h"
#include "Anime4KUpscaleDenoiseEffect.h"
#include "Jinc2ScaleEffect.h"
#include "MitchellNetravaliScaleEffect.h"
#include "json.hpp"
#include <unordered_set>

class EffectManager {
public:
	EffectManager(
		ComPtr<ID2D1Factory1> d2dFactory, 
		ComPtr<ID2D1DeviceContext> d2dDC,
		const std::wstring_view& effectsJson,
		const SIZE &srcSize,
		const SIZE &maxSize
	): _maxSize(maxSize), _d2dFactory(d2dFactory), _d2dDC(d2dDC) {
		assert(srcSize.cx > 0 && srcSize.cy > 0);
		assert(maxSize.cx > 0 && maxSize.cy > 0);
		assert(d2dFactory != nullptr && d2dDC != nullptr);

		_CreateSourceEffect(srcSize);
		_ReadEffectsJson(effectsJson);
	}

	ComPtr<ID2D1Image> Apply(ComPtr<IWICBitmapSource> srcBmp) {
		assert(srcBmp != nullptr);

		Debug::ThrowIfFailed(
			_d2dSourceEffect->SetValue(D2D1_BITMAPSOURCE_PROP_WIC_BITMAP_SOURCE, srcBmp.Get()),
			L"设置 D2D1BitmapSource 源失败"
		);

		ComPtr<ID2D1Image> outputImg = nullptr;
		_outputEffect->GetOutput(&outputImg);

		return outputImg;
	}

private:
	void _CreateSourceEffect(const SIZE& srcSize) {
		// 创建 Source effect
		Debug::ThrowIfFailed(
			_d2dDC->CreateEffect(CLSID_D2D1BitmapSource, &_d2dSourceEffect),
			L"创建 D2D1BitmapSource 失败"
		);

		// 初始时输出为 Source effect
		_outputEffect = _d2dSourceEffect;

		_SetDestSize(srcSize);
	}

	void _ReadEffectsJson(const std::wstring_view& effectsJson) {
		const auto& effects = nlohmann::json::parse(effectsJson);
		Debug::ThrowIfFalse(effects.is_array(), L"json 格式错误");

		for (const auto &effect : effects) {
			Debug::ThrowIfFalse(effect.is_object(), L"json 格式错误");

			const auto &effectType = effect.value("effect", "");
			
			if (effectType == "scale") {
				const auto& subType = effect.value("type", "");

				if (subType == "anime4K") {
					_AddAnime4KEffect();
				} else if (subType == "anime4KxDeblur") {
					_AddAnime4KxDeblurEffect();
				} else if (subType == "anime4KxDenoise") {
					_AddAnime4KxDenoiseEffect();
				} else if (subType == "jinc2") {
					_AddJinc2ScaleEffect(effect);
				} else if (subType == "mitchell") {
					_AddMitchellNetravaliScaleEffect(effect);
				} else if (subType == "highQualityCubic") {
					_AddHighQualityCubicScaleEffect(effect);
				} else {
					Debug::ThrowIfFalse(false, L"未知的 scale effect");
				}
			} else if (effectType == "sharpen") {
				const auto& subType = effect.value("type", "");

				if (subType == "adaptive") {
					_AddAdaptiveSharpenEffect(effect);
				} else if (subType == "builtIn") {
					_AddBuiltInSharpenEffect(effect);
				} else {
					Debug::ThrowIfFalse(false, L"未知的 sharpen effect");
				}
			} else {
				Debug::ThrowIfFalse(false, L"未知的 effect");
			}
		}

	}

	void _AddAdaptiveSharpenEffect(const nlohmann::json& props) {
		_CheckAndRegisterEffect(
			CLSID_MAGPIE_ADAPTIVE_SHARPEN_EFFECT,
			&AdaptiveSharpenEffect::Register
		);

		ComPtr<ID2D1Effect> adaptiveSharpenEffect = nullptr;
		Debug::ThrowIfFailed(
			_d2dDC->CreateEffect(CLSID_MAGPIE_ADAPTIVE_SHARPEN_EFFECT, &adaptiveSharpenEffect),
			L"创建 Adaptive sharpen effect 失败"
		);

		// strength 属性
		auto it = props.find("strength");
		if (it != props.end()) {
			const auto& value = *it;
			Debug::ThrowIfFalse(value.is_number(), L"非法的 strength 属性值");

			float strength = value.get<float>();
			Debug::ThrowIfFalse(
				strength >= 0 && strength <= 1,
				L"非法的 strength 属性值"
			);

			Debug::ThrowIfFailed(
				adaptiveSharpenEffect->SetValue(AdaptiveSharpenEffect::PROP_STRENGTH, strength),
				L"设置 strength 属性失败"
			);
		}

		// 替换 output effect
		_PushAsOutputEffect(adaptiveSharpenEffect);
	}

	void _AddBuiltInSharpenEffect(const nlohmann::json& props) {
		ComPtr<ID2D1Effect> d2dSharpenEffect = nullptr;
		Debug::ThrowIfFailed(
			_d2dDC->CreateEffect(CLSID_D2D1Sharpen, &d2dSharpenEffect),
			L"创建 sharpen effect 失败"
		);

		// sharpness 属性
		auto it = props.find("sharpness");
		if (it != props.end()) {
			const auto& value = *it;
			Debug::ThrowIfFalse(value.is_number(), L"非法的 sharpness 属性值");

			float sharpness = value.get<float>();
			Debug::ThrowIfFalse(
				sharpness >= 0 && sharpness <= 10,
				L"非法的 sharpness 属性值"
			);

			Debug::ThrowIfFailed(
				d2dSharpenEffect->SetValue(D2D1_SHARPEN_PROP_SHARPNESS, sharpness),
				L"设置 sharpness 属性失败"
			);
		}

		// threshold 属性
		it = props.find("threshold");
		if (it != props.end()) {
			const auto& value = *it;
			Debug::ThrowIfFalse(value.is_number(), L"非法的 threshold 属性值");

			float threshold = value.get<float>();
			Debug::ThrowIfFalse(
				threshold >= 0 && threshold <= 1,
				L"非法的 threshold 属性值"
			);

			Debug::ThrowIfFailed(
				d2dSharpenEffect->SetValue(D2D1_SHARPEN_PROP_THRESHOLD, threshold),
				L"设置 threshold 属性失败"
			);
		}

		// 替换 output effect
		_PushAsOutputEffect(d2dSharpenEffect);
	}

	void _AddAnime4KEffect() {
		_CheckAndRegisterEffect(
			CLSID_MAGIPE_ANIME4K_UPSCALE_EFFECT,
			&Anime4KUpscaleEffect::Register
		);

		ComPtr<ID2D1Effect> anime4KEffect = nullptr;
		Debug::ThrowIfFailed(
			_d2dDC->CreateEffect(CLSID_MAGIPE_ANIME4K_UPSCALE_EFFECT, &anime4KEffect),
			L"创建 Anime4K Effect 失败"
		);

		// 输出图像的长和宽变为 2 倍
		_SetDestSize(SIZE{ _destSize.cx * 2, _destSize.cy * 2 });

		_PushAsOutputEffect(anime4KEffect);
	}

	void _AddAnime4KxDeblurEffect() {
		_CheckAndRegisterEffect(
			CLSID_MAGIPE_ANIME4K_UPSCALE_DEBLUR_EFFECT,
			&Anime4KUpscaleDeblurEffect::Register
		);

		ComPtr<ID2D1Effect> anime4KxDeblurEffect = nullptr;
		Debug::ThrowIfFailed(
			_d2dDC->CreateEffect(CLSID_MAGIPE_ANIME4K_UPSCALE_DEBLUR_EFFECT, &anime4KxDeblurEffect),
			L"创建 Anime4K Effect 失败"
		);

		// 输出图像的长和宽变为 2 倍
		_SetDestSize(SIZE{ _destSize.cx * 2, _destSize.cy * 2 });

		_PushAsOutputEffect(anime4KxDeblurEffect);
	}

	void _AddAnime4KxDenoiseEffect() {
		_CheckAndRegisterEffect(
			CLSID_MAGIPE_ANIME4K_UPSCALE_DENOISE_EFFECT,
			&Anime4KUpscaleDenoiseEffect::Register
		);

		ComPtr<ID2D1Effect> effect = nullptr;
		Debug::ThrowIfFailed(
			_d2dDC->CreateEffect(CLSID_MAGIPE_ANIME4K_UPSCALE_DENOISE_EFFECT, &effect),
			L"创建 Anime4K Effect 失败"
		);

		// 输出图像的长和宽变为 2 倍
		_SetDestSize(SIZE{ _destSize.cx * 2, _destSize.cy * 2 });

		_PushAsOutputEffect(effect);
	}

	void _AddJinc2ScaleEffect(const nlohmann::json& props) {
		_CheckAndRegisterEffect(
			CLSID_MAGPIE_JINC2_SCALE_EFFECT,
			&Jinc2ScaleEffect::Register
		);

		ComPtr<ID2D1Effect> jinc2Effect = nullptr;
		Debug::ThrowIfFailed(
			_d2dDC->CreateEffect(CLSID_MAGPIE_JINC2_SCALE_EFFECT, &jinc2Effect),
			L"创建 Anime4K Effect 失败"
		);
		

		// scale 属性
		auto it = props.find("scale");
		if (it != props.end()) {
			const auto& scale = _ReadScaleProp(*it);
			
			Debug::ThrowIfFailed(
				jinc2Effect->SetValue(Jinc2ScaleEffect::PROP_SCALE, scale),
				L"设置 scale 属性失败"
			);

			// 存在 scale 则输出图像尺寸改变
			_SetDestSize(SIZE{ lroundf(_destSize.cx * scale.x), lroundf(_destSize.cy * scale.y) });
		}
		
		// 替换 output effect
		_PushAsOutputEffect(jinc2Effect);
	}

	void _AddMitchellNetravaliScaleEffect(const nlohmann::json& props) {
		_CheckAndRegisterEffect(
			CLSID_MAGPIE_MITCHELL_NETRAVALI_SCALE_EFFECT, 
			&MitchellNetravaliScaleEffect::Register
		);

		ComPtr<ID2D1Effect> effect = nullptr;
		Debug::ThrowIfFailed(
			_d2dDC->CreateEffect(CLSID_MAGPIE_MITCHELL_NETRAVALI_SCALE_EFFECT, &effect),
			L"创建 Mitchell-Netraval Scale Effect 失败"
		);

		// scale 属性
		auto it = props.find("scale");
		if (it != props.end()) {
			const auto& scale = _ReadScaleProp(*it);

			Debug::ThrowIfFailed(
				effect->SetValue(MitchellNetravaliScaleEffect::PROP_SCALE, scale),
				L"设置 scale 属性失败"
			);

			// 存在 scale 则输出图像尺寸改变
			_SetDestSize(SIZE{ lroundf(_destSize.cx * scale.x), lroundf(_destSize.cy * scale.y) });
		}

		// useSharperVersion 属性
		it = props.find("useSharperVersion");
		if (it != props.end()) {
			const auto& val = *it;
			Debug::ThrowIfFalse(val.is_boolean(), L"非法的 useSharperVersion 属性值");

			Debug::ThrowIfFailed(
				effect->SetValue(MitchellNetravaliScaleEffect::PROP_USE_SHARPER_VERSION, (BOOL)val.get<bool>()),
				L"设置 useSharperVersion 属性失败"
			);
		}

		// 替换 output effect
		_PushAsOutputEffect(effect);
	}

	void _AddHighQualityCubicScaleEffect(const nlohmann::json& props) {
		ComPtr<ID2D1Effect> effect = nullptr;
		Debug::ThrowIfFailed(
			_d2dDC->CreateEffect(CLSID_D2D1Scale, &effect),
			L"创建 Anime4K Effect 失败"
		);

		effect->SetValue(D2D1_SCALE_PROP_INTERPOLATION_MODE, D2D1_SCALE_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC);
		effect->SetValue(D2D1_SCALE_PROP_BORDER_MODE, D2D1_BORDER_MODE_HARD);

		// scale 属性
		auto it = props.find("scale");
		if (it != props.end()) {
			const auto& scale = _ReadScaleProp(*it);

			Debug::ThrowIfFailed(
				effect->SetValue(D2D1_SCALE_PROP_SCALE, scale),
				L"设置 scale 属性失败"
			);

			// 存在 scale 则输出图像尺寸改变
			_SetDestSize(SIZE{ lroundf(_destSize.cx * scale.x), lroundf(_destSize.cy * scale.y) });
		}

		// sharpness 属性
		it = props.find("sharpness");
		if (it != props.end()) {
			const auto& value = *it;
			Debug::ThrowIfFalse(value.is_number(), L"非法的 sharpness 属性值");

			float sharpness = value.get<float>();
			Debug::ThrowIfFalse(
				sharpness >= 0 && sharpness <= 1,
				L"非法的 sharpness 属性值"
			);

			Debug::ThrowIfFailed(
				effect->SetValue(D2D1_SCALE_PROP_SHARPNESS, sharpness),
				L"设置 sharpness 属性失败"
			);
		}

		_PushAsOutputEffect(effect);
	}

	D2D1_VECTOR_2F _ReadScaleProp(const nlohmann::json& prop) {
		Debug::ThrowIfFalse(
			prop.is_array() && prop.size() == 2
			&& prop[0].is_number() && prop[1].is_number(),
			L"读取 scale 属性失败"
		);

		D2D1_VECTOR_2F scale{ prop[0], prop[1] };
		Debug::ThrowIfFalse(
			scale.x >= 0 && scale.y >= 0,
			L"scale 属性的值非法"
		);

		if (scale.x == 0 || scale.y == 0) {
			// 输出图像充满屏幕
			scale.x = min((FLOAT)_maxSize.cx / _destSize.cx, (FLOAT)_maxSize.cy / _destSize.cy);
			scale.y = scale.x;
		}

		return scale;
	}

	// 将 effect 添加到 effect 链作为输出
	void _PushAsOutputEffect(ComPtr<ID2D1Effect> effect) {
		effect->SetInputEffect(0, _outputEffect.Get());
		_outputEffect = effect;
	}

	// 设置 destSize 的同时增大 tile 的大小以容纳图像
	void _SetDestSize(SIZE value) {
		if (value.cx > _destSize.cx || value.cy > _destSize.cy) {
			// 需要更大的 tile
			rc.tileSize.width = max(value.cx, _destSize.cx);
			rc.tileSize.height = max(value.cy, _destSize.cy);
			_d2dDC->SetRenderingControls(rc);
		}

		_destSize = value;
	}


	// 必要时注册 effect
	void _CheckAndRegisterEffect(const GUID& effectID, std::function<HRESULT(ID2D1Factory1*)> registerFunc) {
		if (_registeredEffects.find(effectID) == _registeredEffects.end()) {
			// 未注册
			Debug::ThrowIfFailed(
				registerFunc(_d2dFactory.Get()),
				L"注册 Effect 失败"
			);
			
			_registeredEffects.insert(effectID);
		}
	}

	ComPtr<ID2D1Factory1> _d2dFactory;
	ComPtr<ID2D1DeviceContext> _d2dDC;

	ComPtr<ID2D1Effect> _d2dSourceEffect = nullptr;
	ComPtr<ID2D1Effect> _outputEffect = nullptr;

	// 输出图像尺寸
	SIZE _destSize{};
	// 全屏窗口尺寸
	SIZE _maxSize;

	// 存储已注册的 effect 的 GUID
	std::unordered_set<GUID> _registeredEffects;

	// 用于确定 tile 的大小
	SIZE _maxDestSize{};

	D2D1_RENDERING_CONTROLS rc{
		D2D1_BUFFER_PRECISION_32BPC_FLOAT,
		1024,
		1024
	};
};
