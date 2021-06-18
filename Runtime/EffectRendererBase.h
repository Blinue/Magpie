#pragma once
#include "pch.h"
#include "D2DContext.h"
#include "AdaptiveSharpenEffect.h"
#include "Anime4KEffect.h"
#include "Anime4KDarkLinesEffect.h"
#include "Anime4KThinLinesEffect.h"
#include "JincScaleEffect.h"
#include "MitchellNetravaliScaleEffect.h"
#include "LanczosScaleEffect.h"
#include "PixelScaleEffect.h"
#include "ACNetEffect.h"
#include "Anime4KDenoiseBilateralEffect.h"
#include "RavuLiteEffect.h"
#include "RavuZoomEffect.h"
#include "nlohmann/json.hpp"
#include <unordered_set>
#include "Env.h"


// 取决于不同的捕获方式，会有不同种类的输入，此类包含它们通用的部分
// 继承此类需要实现 _PushAsOutputEffect、Apply
// 并在构造函数中调用 _Init
class EffectRendererBase {
public:
	EffectRendererBase() :
		_d2dDC(Env::$instance->GetD2DDC()),
		_d2dFactory(Env::$instance->GetD2DFactory())
	{
	}

	virtual ~EffectRendererBase() {}

	// 不可复制，不可移动
	EffectRendererBase(const EffectRendererBase&) = delete;
	EffectRendererBase(EffectRendererBase&&) = delete;

	virtual ComPtr<ID2D1Image> Apply(IUnknown* inputImg) = 0;

protected:
	void _Init() {
		_ReadEffectsJson(Env::$instance->GetScaleModel());

		const RECT hostClient = Env::$instance->GetHostClient();
		const RECT srcClient = Env::$instance->GetSrcClient();

		float width = (srcClient.right - srcClient.left) * _scale.first;
		float height = (srcClient.bottom - srcClient.top) * _scale.second;
		float left = roundf((hostClient.right - hostClient.left - width) / 2);
		float top = roundf((hostClient.bottom - hostClient.top - height) / 2);
		Env::$instance->SetDestRect({left, top, left + width, top + height});
	}

	// 将 effect 添加到 effect 链作为输出
	virtual void _PushAsOutputEffect(ComPtr<ID2D1Effect> effect) = 0;

private:
	void _ReadEffectsJson(const std::string_view& scaleModel) {
		const auto& models = nlohmann::json::parse(scaleModel);
		Debug::Assert(models.is_array(), L"json 格式错误");

		for (const auto &model : models) {
			Debug::Assert(model.is_object(), L"json 格式错误");

			const auto &effectType = model.value("effect", "");
			const auto& subType = model.value("type", "");

			if (effectType == "scale") {
				if (subType == "Anime4K") {
					_AddAnime4KEffect(model);
				} else if (subType == "ACNet") {
					_AddACNetEffect();
				} else if (subType == "jinc") {
					_AddJincScaleEffect(model);
				} else if (subType == "mitchell") {
					_AddMitchellNetravaliScaleEffect(model);
				} else if (subType == "HQBicubic") {
					_AddHQBicubicScaleEffect(model);
				} else if (subType == "lanczos") {
					_AddLanczosScaleEffect(model);
				} else if (subType == "pixel") {
					_AddPixelScaleEffect(model);
				} else if (subType == "ravuLite") {
					_AddRavuLiteEffect(model);
				} else if (subType == "ravuZoom") {
					_AddRavuZoomEffect(model);
				} else {
					Debug::Assert(false, L"未知的 scale effect");
				}
			} else if (effectType == "sharpen") {
				if (subType == "adaptive") {
					_AddAdaptiveSharpenEffect(model);
				} else if (subType == "builtIn") {
					_AddBuiltInSharpenEffect(model);
				} else {
					Debug::Assert(false, L"未知的 sharpen effect");
				}
			} else if (effectType == "misc") {
				if (subType == "Anime4KDarkLines") {
					_AddAnime4KDarkLinesEffect(model);
				} else if (subType == "Anime4KThinLines") {
					_AddAnime4KThinLinesEffect(model);
				} else if (subType == "Anime4KDenoiseBilateral") {
					_AddAnime4KDenoiseBilateral(model);
				} else {
					Debug::Assert(false, L"未知的 misc effect");
				}
			} else {
				Debug::Assert(false, L"未知的 effect");
			}
		}
	}

	void _AddRavuZoomEffect(const nlohmann::json& props) {
		_CheckAndRegisterEffect(
			CLSID_MAGPIE_RAVU_ZOOM_EFFECT,
			&RavuZoomEffect::Register
		);

		ComPtr<ID2D1Effect> effect = nullptr;
		Debug::ThrowIfComFailed(
			_d2dDC->CreateEffect(CLSID_MAGPIE_RAVU_ZOOM_EFFECT, &effect),
			L"创建 ravu zoom effect 失败"
		);

		// scale 属性
		auto it = props.find("scale");
		if (it != props.end()) {
			const auto& scale = _ReadScaleProp(*it);

			Debug::ThrowIfComFailed(
				effect->SetValue(RavuZoomEffect::PROP_SCALE, scale),
				L"设置 scale 属性失败"
			);

			// 存在 scale 则输出图像尺寸改变
			_scale.first *= scale.x;
			_scale.second *= scale.y;
		}

		// 设置权重纹理
		effect->SetInput(1, 
			Utils::LoadBitmapFromFile(Env::$instance->GetWICImageFactory(), _d2dDC, L"RavuZoomR3Weights.png").Get());

		// 替换 output effect
		_PushAsOutputEffect(effect);
	}

	void _AddRavuLiteEffect(const nlohmann::json& props) {
		_CheckAndRegisterEffect(
			CLSID_MAGPIE_RAVU_LITE_EFFECT,
			&RavuLiteEffect::Register
		);

		ComPtr<ID2D1Effect> effect = nullptr;
		Debug::ThrowIfComFailed(
			_d2dDC->CreateEffect(CLSID_MAGPIE_RAVU_LITE_EFFECT, &effect),
			L"创建 ravu effect 失败"
		);

		// 输出图像的长和宽变为 2 倍
		_scale.first *= 2;
		_scale.second *= 2;

		// 替换 output effect
		_PushAsOutputEffect(effect);
	}

	void _AddAnime4KDenoiseBilateral(const nlohmann::json& props) {
		_CheckAndRegisterEffect(
			CLSID_MAGPIE_ANIME4K_DENOISE_BILATERAL_EFFECT,
			&Anime4KDenoiseBilateralEffect::Register
		);

		ComPtr<ID2D1Effect> effect = nullptr;
		Debug::ThrowIfComFailed(
			_d2dDC->CreateEffect(CLSID_MAGPIE_ANIME4K_DENOISE_BILATERAL_EFFECT, &effect),
			L"创建 Anime4K denoise bilateral effect 失败"
		);

		// variant 属性
		auto it = props.find("variant");
		if (it != props.end()) {
			Debug::Assert(it->is_string(), L"非法的variant属性值");
			std::string_view variant = *it;
			int v = 0;
			if (variant == "mode") {
				v = 0;
			} else if (variant == "median") {
				v = 1;
			} else if (variant == "mean") {
				v = 2;
			} else {
				Debug::Assert(false, L"非法的variant属性值");
			}
			
			Debug::ThrowIfComFailed(
				effect->SetValue(Anime4KDenoiseBilateralEffect::PROP_VARIANT, v),
				L"设置 scale 属性失败"
			);
		}

		// intensity 属性
		it = props.find("intensity");
		if (it != props.end()) {
			const auto& value = *it;
			Debug::Assert(value.is_number(), L"非法的intensity属性值");

			float intensity = value.get<float>();
			Debug::Assert(
				intensity > 0,
				L"非法的intensity属性值"
			);

			Debug::ThrowIfComFailed(
				effect->SetValue(Anime4KDenoiseBilateralEffect::PROP_INTENSITY, intensity),
				L"设置intensity属性失败"
			);
		}

		// 替换 output effect
		_PushAsOutputEffect(effect);
	}

	void _AddAdaptiveSharpenEffect(const nlohmann::json& props) {
		_CheckAndRegisterEffect(
			CLSID_MAGPIE_ADAPTIVE_SHARPEN_EFFECT,
			&AdaptiveSharpenEffect::Register
		);

		ComPtr<ID2D1Effect> adaptiveSharpenEffect = nullptr;
		Debug::ThrowIfComFailed(
			_d2dDC->CreateEffect(CLSID_MAGPIE_ADAPTIVE_SHARPEN_EFFECT, &adaptiveSharpenEffect),
			L"创建 Adaptive sharpen effect 失败"
		);

		// curveHeight 属性
		auto it = props.find("curveHeight");
		if (it != props.end()) {
			const auto& value = *it;
			Debug::Assert(value.is_number(), L"非法的 curveHeight 属性值");

			float curveHeight = value.get<float>();
			Debug::Assert(
				curveHeight > 0,
				L"非法的 curveHeight 属性值"
			);

			Debug::ThrowIfComFailed(
				adaptiveSharpenEffect->SetValue(AdaptiveSharpenEffect::PROP_CURVE_HEIGHT, curveHeight),
				L"设置 curveHeight 属性失败"
			);
		}

		// 替换 output effect
		_PushAsOutputEffect(adaptiveSharpenEffect);
	}

	void _AddBuiltInSharpenEffect(const nlohmann::json& props) {
		ComPtr<ID2D1Effect> d2dSharpenEffect = nullptr;
		Debug::ThrowIfComFailed(
			_d2dDC->CreateEffect(CLSID_D2D1Sharpen, &d2dSharpenEffect),
			L"创建 sharpen effect 失败"
		);

		// sharpness 属性
		auto it = props.find("sharpness");
		if (it != props.end()) {
			const auto& value = *it;
			Debug::Assert(value.is_number(), L"非法的 sharpness 属性值");

			float sharpness = value.get<float>();
			Debug::Assert(
				sharpness >= 0 && sharpness <= 10,
				L"非法的 sharpness 属性值"
			);

			Debug::ThrowIfComFailed(
				d2dSharpenEffect->SetValue(D2D1_SHARPEN_PROP_SHARPNESS, sharpness),
				L"设置 sharpness 属性失败"
			);
		}

		// threshold 属性
		it = props.find("threshold");
		if (it != props.end()) {
			const auto& value = *it;
			Debug::Assert(value.is_number(), L"非法的 threshold 属性值");

			float threshold = value.get<float>();
			Debug::Assert(
				threshold >= 0 && threshold <= 1,
				L"非法的 threshold 属性值"
			);

			Debug::ThrowIfComFailed(
				d2dSharpenEffect->SetValue(D2D1_SHARPEN_PROP_THRESHOLD, threshold),
				L"设置 threshold 属性失败"
			);
		}

		// 替换 output effect
		_PushAsOutputEffect(d2dSharpenEffect);
	}

	void _AddACNetEffect() {
		_CheckAndRegisterEffect(
			CLSID_MAGPIE_ACNET_EFFECT,
			&ACNetEffect::Register
		);

		ComPtr<ID2D1Effect> effect = nullptr;
		Debug::ThrowIfComFailed(
			_d2dDC->CreateEffect(CLSID_MAGPIE_ACNET_EFFECT, &effect),
			L"创建 ACNet Effect 失败"
		);

		// 输出图像的长和宽变为 2 倍
		_scale.first *= 2;
		_scale.second *= 2;

		_PushAsOutputEffect(effect);
	}

	void _AddAnime4KEffect(const nlohmann::json& props) {
		_CheckAndRegisterEffect(
			CLSID_MAGPIE_ANIME4K_EFFECT,
			&Anime4KEffect::Register
		);

		ComPtr<ID2D1Effect> anime4KEffect = nullptr;
		Debug::ThrowIfComFailed(
			_d2dDC->CreateEffect(CLSID_MAGPIE_ANIME4K_EFFECT, &anime4KEffect),
			L"创建 Anime4K Effect 失败"
		);

		// curveHeight 属性
		auto it = props.find("curveHeight");
		if (it != props.end()) {
			const auto& value = *it;
			Debug::Assert(value.is_number(), L"非法的 curveHeight 属性值");

			float curveHeight = value.get<float>();
			Debug::Assert(
				curveHeight >= 0,
				L"非法的 curveHeight 属性值"
			);

			Debug::ThrowIfComFailed(
				anime4KEffect->SetValue(Anime4KEffect::PROP_CURVE_HEIGHT, curveHeight),
				L"设置 curveHeight 属性失败"
			);
		}

		// useDenoiseVersion 属性
		it = props.find("useDenoiseVersion");
		if (it != props.end()) {
			const auto& val = *it;
			Debug::Assert(val.is_boolean(), L"非法的 useSharperVersion 属性值");

			Debug::ThrowIfComFailed(
				anime4KEffect->SetValue(Anime4KEffect::PROP_USE_DENOISE_VERSION, (BOOL)val.get<bool>()),
				L"设置 useSharperVersion 属性失败"
			);
		}

		// 输出图像的长和宽变为 2 倍
		_scale.first *= 2;
		_scale.second *= 2;

		_PushAsOutputEffect(anime4KEffect);
	}

	void _AddAnime4KDarkLinesEffect(const nlohmann::json& props) {
		_CheckAndRegisterEffect(
			CLSID_MAGPIE_ANIME4K_DARKLINES_EFFECT,
			&Anime4KDarkLinesEffect::Register
		);

		ComPtr<ID2D1Effect> effect = nullptr;
		Debug::ThrowIfComFailed(
			_d2dDC->CreateEffect(CLSID_MAGPIE_ANIME4K_DARKLINES_EFFECT, &effect),
			L"创建 Anime4K Effect 失败"
		);

		// strength 属性
		auto it = props.find("strength");
		if (it != props.end()) {
			const auto& value = *it;
			Debug::Assert(value.is_number(), L"非法的 strength 属性值");

			float strength = value.get<float>();
			Debug::Assert(
				strength > 0,
				L"非法的 strength 属性值"
			);

			Debug::ThrowIfComFailed(
				effect->SetValue(Anime4KDarkLinesEffect::PROP_STRENGTH, strength),
				L"设置 strength 属性失败"
			);
		}

		_PushAsOutputEffect(effect);
	}

	void _AddAnime4KThinLinesEffect(const nlohmann::json& props) {
		_CheckAndRegisterEffect(
			CLSID_MAGPIE_ANIME4K_THINLINES_EFFECT,
			&Anime4KThinLinesEffect::Register
		);

		ComPtr<ID2D1Effect> effect = nullptr;
		Debug::ThrowIfComFailed(
			_d2dDC->CreateEffect(CLSID_MAGPIE_ANIME4K_THINLINES_EFFECT, &effect),
			L"创建 Anime4K Effect 失败"
		);

		// strength 属性
		auto it = props.find("strength");
		if (it != props.end()) {
			const auto& value = *it;
			Debug::Assert(value.is_number(), L"非法的 strength 属性值");

			float strength = value.get<float>();
			Debug::Assert(
				strength > 0,
				L"非法的 strength 属性值"
			);

			Debug::ThrowIfComFailed(
				effect->SetValue(Anime4KThinLinesEffect::PROP_STRENGTH, strength),
				L"设置 strength 属性失败"
			);
		}

		_PushAsOutputEffect(effect);
	}

	
	void _AddJincScaleEffect(const nlohmann::json& props) {
		_CheckAndRegisterEffect(
			CLSID_MAGPIE_JINC_SCALE_EFFECT,
			&JincScaleEffect::Register
		);

		ComPtr<ID2D1Effect> effect = nullptr;
		Debug::ThrowIfComFailed(
			_d2dDC->CreateEffect(CLSID_MAGPIE_JINC_SCALE_EFFECT, &effect),
			L"创建 Jinc Effect 失败"
		);
		

		// scale 属性
		auto it = props.find("scale");
		if (it != props.end()) {
			const auto& scale = _ReadScaleProp(*it);
			
			Debug::ThrowIfComFailed(
				effect->SetValue(JincScaleEffect::PROP_SCALE, scale),
				L"设置 scale 属性失败"
			);

			// 存在 scale 则输出图像尺寸改变
			_scale.first *= scale.x;
			_scale.second *= scale.y;
		}

		// windowSinc 属性
		it = props.find("windowSinc");
		if (it != props.end()) {
			const auto& value = *it;
			Debug::Assert(value.is_number(), L"非法的 windowSinc 属性值");

			float windowSinc = value.get<float>();
			Debug::Assert(
				windowSinc > 0,
				L"非法的 windowSinc 属性值"
			);

			Debug::ThrowIfComFailed(
				effect->SetValue(JincScaleEffect::PROP_WINDOW_SINC, windowSinc),
				L"设置 windowSinc 属性失败"
			);
		}

		// sinc 属性
		it = props.find("sinc");
		if (it != props.end()) {
			const auto& value = *it;
			Debug::Assert(value.is_number(), L"非法的 sinc 属性值");

			float sinc = value.get<float>();
			Debug::Assert(
				sinc > 0,
				L"非法的 sinc 属性值"
			);

			Debug::ThrowIfComFailed(
				effect->SetValue(JincScaleEffect::PROP_SINC, sinc),
				L"设置 sinc 属性失败"
			);
		}

		// ARStrength 属性
		it = props.find("ARStrength");
		if (it != props.end()) {
			const auto& value = *it;
			Debug::Assert(value.is_number(), L"非法的 ARStrength 属性值");

			float ARStrength = value.get<float>();
			Debug::Assert(
				ARStrength >= 0 && ARStrength <= 1,
				L"非法的 ARStrength 属性值"
			);

			Debug::ThrowIfComFailed(
				effect->SetValue(JincScaleEffect::PROP_AR_STRENGTH, ARStrength),
				L"设置 ARStrength 属性失败"
			);
		}
		
		// 替换 output effect
		_PushAsOutputEffect(effect);
	}

	void _AddMitchellNetravaliScaleEffect(const nlohmann::json& props) {
		_CheckAndRegisterEffect(
			CLSID_MAGPIE_MITCHELL_NETRAVALI_SCALE_EFFECT, 
			&MitchellNetravaliScaleEffect::Register
		);

		ComPtr<ID2D1Effect> effect = nullptr;
		Debug::ThrowIfComFailed(
			_d2dDC->CreateEffect(CLSID_MAGPIE_MITCHELL_NETRAVALI_SCALE_EFFECT, &effect),
			L"创建 Mitchell-Netraval Scale Effect 失败"
		);

		// scale 属性
		auto it = props.find("scale");
		if (it != props.end()) {
			const auto& scale = _ReadScaleProp(*it);

			Debug::ThrowIfComFailed(
				effect->SetValue(MitchellNetravaliScaleEffect::PROP_SCALE, scale),
				L"设置 scale 属性失败"
			);

			// 存在 scale 则输出图像尺寸改变
			_scale.first *= scale.x;
			_scale.second *= scale.y;
		}

		// useSharperVersion 属性
		it = props.find("useSharperVersion");
		if (it != props.end()) {
			const auto& val = *it;
			Debug::Assert(val.is_boolean(), L"非法的 useSharperVersion 属性值");

			Debug::ThrowIfComFailed(
				effect->SetValue(MitchellNetravaliScaleEffect::PROP_USE_SHARPER_VERSION, (BOOL)val.get<bool>()),
				L"设置 useSharperVersion 属性失败"
			);
		}

		// 替换 output effect
		_PushAsOutputEffect(effect);
	}

	// 内置的 HIGH_QUALITY_CUBIC 缩放算法
	void _AddHQBicubicScaleEffect(const nlohmann::json& props) {
		ComPtr<ID2D1Effect> effect = nullptr;
		Debug::ThrowIfComFailed(
			_d2dDC->CreateEffect(CLSID_D2D1Scale, &effect),
			L"创建 Anime4K Effect 失败"
		);

		effect->SetValue(D2D1_SCALE_PROP_INTERPOLATION_MODE, D2D1_SCALE_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC);
		effect->SetValue(D2D1_SCALE_PROP_BORDER_MODE, D2D1_BORDER_MODE_HARD);

		// scale 属性
		auto it = props.find("scale");
		if (it != props.end()) {
			const auto& scale = _ReadScaleProp(*it);
			Debug::ThrowIfComFailed(
				effect->SetValue(D2D1_SCALE_PROP_SCALE, scale),
				L"设置 scale 属性失败"
			);

			// 存在 scale 则输出图像尺寸改变
			_scale.first *= scale.x;
			_scale.second *= scale.y;
		}

		// sharpness 属性
		it = props.find("sharpness");
		if (it != props.end()) {
			const auto& value = *it;
			Debug::Assert(value.is_number(), L"非法的 sharpness 属性值");

			float sharpness = value.get<float>();
			Debug::Assert(
				sharpness >= 0 && sharpness <= 1,
				L"非法的 sharpness 属性值"
			);

			Debug::ThrowIfComFailed(
				effect->SetValue(D2D1_SCALE_PROP_SHARPNESS, sharpness),
				L"设置 sharpness 属性失败"
			);
		}

		_PushAsOutputEffect(effect);
	}

	void _AddLanczosScaleEffect(const nlohmann::json& props) {
		_CheckAndRegisterEffect(
			CLSID_MAGPIE_LANCZOS_SCALE_EFFECT,
			&LanczosScaleEffect::Register
		);

		ComPtr<ID2D1Effect> effect = nullptr;
		Debug::ThrowIfComFailed(
			_d2dDC->CreateEffect(CLSID_MAGPIE_LANCZOS_SCALE_EFFECT, &effect),
			L"创建 Lanczos Effect 失败"
		);

		// scale 属性
		auto it = props.find("scale");
		if (it != props.end()) {
			const auto& scale = _ReadScaleProp(*it);

			Debug::ThrowIfComFailed(
				effect->SetValue(LanczosScaleEffect::PROP_SCALE, scale),
				L"设置 scale 属性失败"
			);

			// 存在 scale 则输出图像尺寸改变
			_scale.first *= scale.x;
			_scale.second *= scale.y;
		}

		// ARStrength 属性
		it = props.find("ARStrength");
		if (it != props.end()) {
			const auto& value = *it;
			Debug::Assert(value.is_number(), L"非法的 ARStrength 属性值");

			float ARStrength = value.get<float>();
			Debug::Assert(
				ARStrength >= 0 && ARStrength <= 1,
				L"非法的 ARStrength 属性值"
			);

			Debug::ThrowIfComFailed(
				effect->SetValue(LanczosScaleEffect::PROP_AR_STRENGTH, ARStrength),
				L"设置 ARStrengthc 属性失败"
			);
		}

		// 替换 output effect
		_PushAsOutputEffect(effect);
	}

	void _AddPixelScaleEffect(const nlohmann::json& props) {
		_CheckAndRegisterEffect(
			CLSID_MAGPIE_PIXEL_SCALE_EFFECT,
			&PixelScaleEffect::Register
		);

		ComPtr<ID2D1Effect> effect = nullptr;
		Debug::ThrowIfComFailed(
			_d2dDC->CreateEffect(CLSID_MAGPIE_PIXEL_SCALE_EFFECT, &effect),
			L"创建 Pixel Scale Effect 失败"
		);

		// scale 属性
		auto it = props.find("scale");
		if (it != props.end()) {
			Debug::Assert(it->is_number_integer(), L"非法的Scale属性值");
			int scale = *it;

			Debug::Assert(scale > 0, L"非法的Scale属性值");
			Debug::ThrowIfComFailed(
				effect->SetValue(PixelScaleEffect::PROP_SCALE, scale),
				L"设置 scale 属性失败"
			);

			_scale.first *= scale;
			_scale.second *= scale;
		}

		// 替换 output effect
		_PushAsOutputEffect(effect);
	}

	D2D1_VECTOR_2F _ReadScaleProp(const nlohmann::json& prop) {
		Debug::Assert(
			prop.is_array() && prop.size() == 2
			&& prop[0].is_number() && prop[1].is_number(),
			L"读取 scale 属性失败"
		);

		D2D1_VECTOR_2F scale{ prop[0], prop[1] };
		Debug::Assert(
			scale.x >= 0 && scale.y >= 0,
			L"scale 属性的值非法"
		);

		if (scale.x == 0 || scale.y == 0) {
			SIZE hostSize = Utils::GetSize(Env::$instance->GetHostClient());
			SIZE srcSize = Utils::GetSize(Env::$instance->GetSrcClient());

			// 输出图像充满屏幕
			float x = float(hostSize.cx) / srcSize.cx / _scale.first;
			float y = float(hostSize.cy) / srcSize.cy / _scale.second;

			scale.x = min(x, y);
			scale.y = scale.x;
		}

		return scale;
	}
	

	// 必要时注册 effect
	void _CheckAndRegisterEffect(const GUID& effectID, std::function<HRESULT(ID2D1Factory1*)> registerFunc) {
		if (_registeredEffects.find(effectID) == _registeredEffects.end()) {
			// 未注册
			Debug::ThrowIfComFailed(
				registerFunc(_d2dFactory),
				L"注册 Effect 失败"
			);
			
			_registeredEffects.insert(effectID);
		}
	}

private:
	// 输出图像尺寸
	std::pair<float, float> _scale{ 1.0f,1.0f };

	// 存储已注册的 effect 的 GUID
	std::unordered_set<GUID> _registeredEffects;

	ID2D1Factory1* _d2dFactory;
	ID2D1DeviceContext* _d2dDC;
};
