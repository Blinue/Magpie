#include "pch.h"
#include "Renderer.h"
#include "App.h"
#include "Utils.h"
#include "StrUtils.h"
#include <VertexTypes.h>
#include "EffectCompiler.h"
#include <rapidjson/document.h>
#include "FrameSourceBase.h"
#include "DeviceResources.h"
#include "GPUTimer.h"
#include "EffectDrawer.h"
#include "UIDrawer.h"
#include "FSRFilter.h"
#include "A4KSFilter.h"
#include "Logger.h"
#include "CursorManager.h"


Renderer::Renderer() {}

Renderer::~Renderer() {}

bool Renderer::Initialize(const std::string& effectsJson) {
	_gpuTimer.reset(new GPUTimer());
	
	if (!GetWindowRect(App::Get().GetHwndSrc(), &_srcWndRect)) {
		Logger::Get().Win32Error("GetWindowRect 失败");
		return false;
	}

	if (!_ResolveEffectsJson(effectsJson)) {
		Logger::Get().Error("_ResolveEffectsJson 失败");
		return false;
	}
	/*
	ID3D11Texture2D* backBuffer = App::Get().GetDeviceResources().GetBackBuffer();

	_UIDrawer.reset(new UIDrawer());
	if (!_UIDrawer->Initialize(backBuffer)) {
		return false;
	}*/

	_cursorManager.reset(new CursorManager());
	if (!_cursorManager->Initialize()) {
		Logger::Get().Error("初始化 CursorManager 失败");
		return false;
	}

	return true;
}


void Renderer::Render() {
	if (!_CheckSrcState()) {
		Logger::Get().Info("源窗口状态改变，退出全屏");
		App::Get().Quit();
		return;
	}

	DeviceResources& dr = App::Get().GetDeviceResources();
	ID3D11DeviceContext3* d3dDC = dr.GetD3DDC();

	if (!_waitingForNextFrame) {
		dr.BeginFrame();
	}

	auto state = App::Get().GetFrameSource().Update();
	_waitingForNextFrame = state == FrameSourceBase::UpdateState::Waiting
		|| state == FrameSourceBase::UpdateState::Error;
	if (_waitingForNextFrame) {
		return;
	}

	_gpuTimer->BeginFrame();
	_cursorManager->BeginFrame();

	for (auto& effect : _effects) {
		effect->Draw();
	}

	/*
	// 更新常量
	if (!EffectDrawer::UpdateExprDynamicVars()) {
		SPDLOG_LOGGER_ERROR(logger, "UpdateExprDynamicVars 失败");
	}

	if (state == FrameSourceBase::UpdateState::NewFrame) {
		
	} else {
		// 此帧内容无变化
		// 从第一个有动态常量的 Effect 开始渲染
		// 如果没有则只渲染最后一个 Effect 的最后一个 pass

		size_t i = 0;
		for (; i < _effects.size(); ++i) {
			if (_effects[i]->HasDynamicConstants()) {
				break;
			}
		}

		if (i == _effects.size()) {
			// 只渲染最后一个 Effect 的最后一个 pass
			_effects.back()->Draw(true);
		} else {
			for (; i < _effects.size(); ++i) {
				_effects[i]->Draw();
			}
		}
	}

	_UIDrawer->Draw();*/


	dr.EndFrame();
}

bool CheckForeground(HWND hwndForeground) {
	wchar_t className[256]{};
	if (!GetClassName(hwndForeground, (LPWSTR)className, 256)) {
		Logger::Get().Win32Error("GetClassName 失败");
		return false;
	}

	// 排除桌面窗口和 Alt+Tab 窗口
	if (!std::wcscmp(className, L"WorkerW") || !std::wcscmp(className, L"ForegroundStaging") ||
		!std::wcscmp(className, L"MultitaskingViewFrame") || !std::wcscmp(className, L"XamlExplorerHostIslandWindow")
	) {
		return true;
	}

	RECT rectForground{};

	// 如果捕获模式可以捕获到弹窗，则允许小的弹窗
	if (App::Get().GetFrameSource().IsScreenCapture()
		&& GetWindowStyle(hwndForeground) & (WS_POPUP | WS_CHILD)
	) {
		if (!Utils::GetWindowFrameRect(hwndForeground, rectForground)) {
			Logger::Get().Error("GetWindowFrameRect 失败");
			return false;
		}

		// 弹窗如果完全在源窗口客户区内则不退出全屏
		const RECT& srcFrameRect = App::Get().GetFrameSource().GetSrcFrameRect();
		if (rectForground.left >= srcFrameRect.left
			&& rectForground.right <= srcFrameRect.right
			&& rectForground.top >= srcFrameRect.top
			&& rectForground.bottom <= srcFrameRect.bottom
		) {
			return true;
		}
	}

	// 非多屏幕模式下退出全屏
	if (!App::Get().IsMultiMonitorMode()) {
		return false;
	}

	if (rectForground == RECT{}) {
		if (!Utils::GetWindowFrameRect(hwndForeground, rectForground)) {
			Logger::Get().Error("GetWindowFrameRect 失败");
			return false;
		}
	}

	IntersectRect(&rectForground, &App::Get().GetHostWndRect(), &rectForground);

	// 允许稍微重叠，否则前台窗口最大化时会意外退出
	if (rectForground.right - rectForground.left < 10 || rectForground.right - rectForground.top < 10) {
		return true;
	}

	// 排除开始菜单，它的类名是 CoreWindow
	if (std::wcscmp(className, L"Windows.UI.Core.CoreWindow")) {
		// 记录新的前台窗口
		Logger::Get().Info(fmt::format("新的前台窗口：\n\t类名：{}", StrUtils::UTF16ToUTF8(className)));
		return false;
	}

	DWORD dwProcId = 0;
	if (!GetWindowThreadProcessId(hwndForeground, &dwProcId)) {
		Logger::Get().Win32Error("GetWindowThreadProcessId 失败");
		return false;
	}

	Utils::ScopedHandle hProc(Utils::SafeHandle(OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcId)));
	if (!hProc) {
		Logger::Get().Win32Error("OpenProcess 失败");
		return false;
	}

	wchar_t fileName[MAX_PATH] = { 0 };
	if (!GetModuleFileNameEx(hProc.get(), NULL, fileName, MAX_PATH)) {
		Logger::Get().Win32Error("GetModuleFileName 失败");
		return false;
	}

	std::string exeName = StrUtils::UTF16ToUTF8(fileName);
	exeName = exeName.substr(exeName.find_last_of(L'\\') + 1);
	StrUtils::ToLowerCase(exeName);

	// win10: searchapp.exe 和 startmenuexperiencehost.exe
	// win11: searchhost.exe 和 startmenuexperiencehost.exe
	if (exeName == "searchapp.exe" || exeName == "searchhost.exe" || exeName == "startmenuexperiencehost.exe") {
		return true;
	}

	return false;
}

bool Renderer::_CheckSrcState() {
	HWND hwndSrc = App::Get().GetHwndSrc();

	if (!App::Get().IsBreakpointMode()) {
		HWND hwndForeground = GetForegroundWindow();
		if (hwndForeground && hwndForeground != hwndSrc && !CheckForeground(hwndForeground)) {
			Logger::Get().Info("前台窗口已改变");
			return false;
		}
	}

	if (Utils::GetWindowShowCmd(hwndSrc) != SW_NORMAL) {
		Logger::Get().Info("源窗口显示状态改变");
		return false;
	}

	RECT rect;
	if (!GetWindowRect(hwndSrc, &rect)) {
		Logger::Get().Error("GetWindowRect 失败");
		return false;
	}

	if (_srcWndRect != rect) {
		Logger::Get().Info("源窗口位置或大小改变");
		return false;
	}

	return true;
}

bool Renderer::_ResolveEffectsJson(const std::string& effectsJson) {
	rapidjson::Document doc;
	if (doc.Parse(effectsJson.c_str(), effectsJson.size()).HasParseError()) {
		// 解析 json 失败
		Logger::Get().Error(fmt::format("解析 json 失败\n\t错误码：{}", doc.GetParseError()));
		return false;
	}

	if (!doc.IsArray()) {
		Logger::Get().Error("解析 json 失败：根元素不为数组");
		return false;
	}

	const auto& effectsArr = doc.GetArray();
	_effects.reserve(effectsArr.Size());

	// 不得为空
	if (effectsArr.Empty()) {
		Logger::Get().Error("解析 json 失败：根元素为空");
		return false;
	}

	ID3D11Texture2D* effectInput = App::Get().GetFrameSource().GetOutput();
	SIZE outputSize{};

	for (int i = 0, end = effectsArr.Size(); i < end; ++i) {
		const auto& effectJson = effectsArr[i];

		if (!effectJson.IsObject()) {
			Logger::Get().Error("解析 json 失败：根数组中存在非法成员");
			return false;
		}

		EffectDrawer& effect = *_effects.emplace_back(new EffectDrawer());

		auto effectName = effectJson.FindMember("effect");
		if (effectName == effectJson.MemberEnd() || !effectName->value.IsString()) {
			Logger::Get().Error("解析 json 失败：未找到 effect 属性或该属性的值不合法");
			return false;
		}

		EffectDesc effectDesc;
		EffectParams effectParams;
		std::wstring fileName = (L"effects\\" + StrUtils::UTF8ToUTF16(effectName->value.GetString()) + L".hlsl");

		bool result = false;
		int duration = Utils::Measure([&]() {
			result = !EffectCompiler::Compile(
				fileName.c_str(),
				effectDesc,
				(i == end - 1) ? EFFECT_FLAG_LAST_EFFECT : 0
			);
		});

		if (!result) {
			Logger::Get().Error(fmt::format("编译 {} 失败", StrUtils::UTF16ToUTF8(fileName)));
			return false;
		} else {
			Logger::Get().Info(fmt::format("编译 {} 用时 {} 毫秒", StrUtils::UTF16ToUTF8(fileName), duration / 1000.0f));
		}

#pragma push_macro("GetObject")
#undef GetObject
		for (const auto& prop : effectJson.GetObject()) {
#pragma pop_macro("GetObject")
			if (!prop.name.IsString()) {
				Logger::Get().Error("解析 json 失败：非法的效果名");
				return false;
			}

			std::string_view name = prop.name.GetString();

			if (name == "effect") {
				continue;
			} else if (name == "scale" && effectDesc.outSizeExpr.first.empty()) {
				auto scaleProp = effectJson.FindMember("scale");
				if (scaleProp != effectJson.MemberEnd()) {
					if (!scaleProp->value.IsArray()) {
						Logger::Get().Error("解析 json 失败：非法的 scale 属性");
						return false;
					}

					const auto& scale = scaleProp->value.GetArray();
					if (scale.Size() != 2 || !scale[0].IsNumber() || !scale[1].IsNumber()) {
						Logger::Get().Error("解析 json 失败：非法的 scale 属性");
						return false;
					}

					effectParams.scale.first = scale[0].GetFloat();
					effectParams.scale.second = scale[1].GetFloat();
				}
			} else {
				auto it = std::find_if(
					effectDesc.params.begin(),
					effectDesc.params.end(),
					[name](const EffectParameterDesc& desc) { return desc.name == name; }
				);

				if (it == effectDesc.params.end()) {
					Logger::Get().Error(fmt::format("解析 json 失败：非法成员 {}", name));
					return false;
				}

				auto pair = effectParams.params.emplace(UINT(it - effectDesc.params.begin()), EffectConstant32{});
				if (!pair.second) {
					Logger::Get().Error(fmt::format("重复的成员：{}", name));
					return false;
				}

				EffectConstant32& value = pair.first->second;

				auto type = it->type;
				if (type == EffectConstantType::Float) {
					if (!prop.value.IsNumber()) {
						Logger::Get().Error(fmt::format("解析 json 失败：成员 {} 的类型非法", name));
						return false;
					}

					value.floatVal = prop.value.GetFloat();
				} else {
					if (prop.value.IsInt()) {
						value.intVal = prop.value.GetInt();
					} else if (prop.value.IsBool()) {
						// bool 值视为 int
						value.intVal = (int)prop.value.GetBool();
					} else {
						Logger::Get().Error(fmt::format("解析 json 失败：成员 {} 的类型非法", name));
						return false;
					}
				}
			}
		}

		if (!effect.Initialize(effectDesc, effectParams, effectInput, &effectInput, i == end - 1 ? &_outputRect : nullptr)) {
			Logger::Get().Error(fmt::format("初始化效果#{} ({}) 失败", i, effectName->value.GetString()));
			return false;
		}
	}

	return true;
}
