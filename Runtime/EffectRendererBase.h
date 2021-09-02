#pragma once
#include "pch.h"
#include "nlohmann/json.hpp"
#include "Env.h"


using EffectCreateFunc = HRESULT(
	ID2D1Factory1* d2dFactory,
	ID2D1DeviceContext* d2dDC,
	IWICImagingFactory2* wicImgFactory,
	const nlohmann::json& props,
	float fillScale,
	std::pair<float, float>& scale,
	ComPtr<ID2D1Effect>& effect
);


// 取决于不同的捕获方式，会有不同种类的输入，此类包含它们通用的部分
// 继承此类需要实现 _PushAsOutputEffect、Apply
// 并在构造函数中调用 _Init
class EffectRendererBase {
public:
	EffectRendererBase() {
		SIZE hostSize = Utils::GetSize(Env::$instance->GetHostClient());
		SIZE srcSize = Utils::GetSize(Env::$instance->GetSrcClient());

		// 输出图像充满屏幕时的缩放比例
		_fillScale = std::min(float(hostSize.cx) / srcSize.cx, float(hostSize.cy) / srcSize.cy);
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

			const auto &moduleName = model.value("module", "");
			Debug::Assert(!moduleName.empty(), L"json 格式错误");

			std::wstring moduleNameW = Utils::UTF8ToUTF16(moduleName);
			HMODULE dll = LoadLibrary(fmt::format(L"effects\\{}", moduleNameW).c_str());
			Debug::ThrowIfWin32Failed(dll, fmt::format(L"加载模块{}出错", moduleNameW));
			
			auto createEffect = (EffectCreateFunc*)GetProcAddress(dll, "CreateEffect");
			Debug::ThrowIfWin32Failed(createEffect, L"非法的dll");

			ComPtr<ID2D1Effect> effect;
			Debug::ThrowIfComFailed(
				createEffect(_d2dFactory, _d2dDC, Env::$instance->GetWICImageFactory(), model, _fillScale, _scale, effect),
				L"json格式错误"
			);

			// 替换 output effect
			_PushAsOutputEffect(effect);
		}
	}

private:
	// 输出图像尺寸
	std::pair<float, float> _scale{ 1.0f,1.0f };

	float _fillScale = 0;

	ID2D1Factory1* _d2dFactory;
	ID2D1DeviceContext* _d2dDC;
};
