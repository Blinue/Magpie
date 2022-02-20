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

	_fsrFilter.reset(new FSRFilter());
	if (!_fsrFilter->Initialize(_outputRect)) {
		return false;
	}
	/*_a4ksFilter.reset(new A4KSFilter());
	if (!_a4ksFilter->Initialize()) {
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

	ID3D11RenderTargetView* rtv = nullptr;
	//dr.GetRenderTargetView()
	//d3dDC->ClearRenderTargetView()
	// 所有渲染都使用三角形带拓扑
	//d3dDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	_fsrFilter->Draw();
	//_a4ksFilter->Draw();
	/*

	// 更新常量
	if (!EffectDrawer::UpdateExprDynamicVars()) {
		SPDLOG_LOGGER_ERROR(logger, "UpdateExprDynamicVars 失败");
	}

	if (state == FrameSourceBase::UpdateState::NewFrame) {
		for (auto& effect : _effects) {
			effect->Draw();
		}
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
	winrt::com_ptr<ID3D11Texture2D> srcFrame = App::Get().GetFrameSource().GetOutput();
	D3D11_TEXTURE2D_DESC inputDesc;
	srcFrame->GetDesc(&inputDesc);

	const RECT& hostWndRect = App::Get().GetHostWndRect();
	SIZE hostSize = { hostWndRect.right - hostWndRect.left,hostWndRect.bottom - hostWndRect.top };

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

	std::vector<SIZE> texSizes;
	texSizes.push_back({ (LONG)inputDesc.Width, (LONG)inputDesc.Height });

	const auto& effectsArr = doc.GetArray();
	_effects.reserve(effectsArr.Size());
	texSizes.reserve(static_cast<size_t>(effectsArr.Size()) + 1);

	// 不得为空
	if (effectsArr.Empty()) {
		Logger::Get().Error("解析 json 失败：根元素为空");
		return false;
	}

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

		if (!effect.Initialize((L"effects\\" + StrUtils::UTF8ToUTF16(effectName->value.GetString()) + L".hlsl").c_str(), i == end - 1)) {
			Logger::Get().Error(fmt::format("初始化效果 {} 失败", effectName->value.GetString()));
			return false;
		}

		if (effect.CanSetOutputSize()) {
			// scale 属性可用
			auto scaleProp = effectJson.FindMember("scale");
			if (scaleProp != effectJson.MemberEnd()) {
				if (!scaleProp->value.IsArray()) {
					Logger::Get().Error("解析 json 失败：非法的 scale 属性");
					return false;
				}

				// scale 属性的值为两个元素组成的数组
				// [+, +]：缩放比例
				// [0, 0]：非等比例缩放到屏幕大小
				// [-, -]：相对于屏幕能容纳的最大等比缩放的比例

				const auto& scale = scaleProp->value.GetArray();
				if (scale.Size() != 2 || !scale[0].IsNumber() || !scale[1].IsNumber()) {
					Logger::Get().Error("解析 json 失败：非法的 scale 属性");
					return false;
				}

				float scaleX = scale[0].GetFloat();
				float scaleY = scale[1].GetFloat();

				static float DELTA = 1e-5f;

				SIZE outputSize = texSizes.back();;

				if (scaleX >= DELTA) {
					if (scaleY < DELTA) {
						Logger::Get().Error("解析 json 失败：非法的 scale 属性");
						return false;
					}

					outputSize = { std::lroundf(outputSize.cx * scaleX), std::lroundf(outputSize.cy * scaleY) };
				} else if (std::abs(scaleX) < DELTA) {
					if (std::abs(scaleY) >= DELTA) {
						Logger::Get().Error("解析 json 失败：非法的 scale 属性");
						return false;
					}

					outputSize = hostSize;
				} else {
					if (scaleY > -DELTA) {
						Logger::Get().Error("解析 json 失败：非法的 scale 属性");
						return false;
					}

					float fillScale = std::min(float(hostSize.cx) / outputSize.cx, float(hostSize.cy) / outputSize.cy);
					outputSize = {
						std::lroundf(outputSize.cx * fillScale * -scaleX),
						std::lroundf(outputSize.cy * fillScale * -scaleY)
					};
				}

				effect.SetOutputSize(outputSize);
			}
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

			if (name == "effect" || (effect.CanSetOutputSize() && name == "scale")) {
				continue;
			} else {
				auto type = effect.GetConstantType(name);
				if (type == EffectDrawer::ConstantType::Float) {
					if (!prop.value.IsNumber()) {
						Logger::Get().Error(fmt::format("解析 json 失败：成员 {} 的类型非法", name));
						return false;
					}

					if (!effect.SetConstant(name, prop.value.GetFloat())) {
						Logger::Get().Error(fmt::format("解析 json 失败：成员 {} 的值非法", name));
						return false;
					}
				} else if (type == EffectDrawer::ConstantType::Int) {
					int value;
					if (prop.value.IsInt()) {
						value = prop.value.GetInt();
					} else if (prop.value.IsBool()) {
						// bool 值视为 int
						value = (int)prop.value.GetBool();
					} else {
						Logger::Get().Error(fmt::format("解析 json 失败：成员 {} 的类型非法", name));
						return false;
					}

					if (!effect.SetConstant(name, value)) {
						Logger::Get().Error(fmt::format("解析 json 失败：成员 {} 的值非法", name));
						return false;
					}
				} else {
					Logger::Get().Error(fmt::format("解析 json 失败：非法成员 {}", name));
					return false;
				}
			}
		}

		SIZE& outputSize = texSizes.emplace_back();
		if (!effect.CalcOutputSize(texSizes[texSizes.size() - 2], outputSize)) {
			Logger::Get().Error("CalcOutputSize 失败");
			return false;
		}
	}

	auto& dr = App::Get().GetDeviceResources();
	auto d3dDevice = dr.GetD3DDevice();

	if (_effects.size() == 1) {
		if (!_effects.back()->Build(srcFrame.get(), dr.GetBackBuffer())) {
			Logger::Get().Error("构建效果失败");
			return false;
		}
	} else {
		// 创建效果间的中间纹理
		winrt::com_ptr<ID3D11Texture2D> curTex = srcFrame;

		D3D11_TEXTURE2D_DESC desc{};
		desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

		assert(texSizes.size() == _effects.size() + 1);
		for (size_t i = 0, end = _effects.size() - 1; i < end; ++i) {
			SIZE texSize = texSizes[i + 1];
			desc.Width = texSize.cx;
			desc.Height = texSize.cy;

			winrt::com_ptr<ID3D11Texture2D> outputTex;
			HRESULT hr = d3dDevice->CreateTexture2D(&desc, nullptr, outputTex.put());
			if (FAILED(hr)) {
				Logger::Get().ComError("CreateTexture2D 失败", hr);
				return false;
			}

			if (!_effects[i]->Build(curTex.get(), outputTex.get())) {
				Logger::Get().Error("构建效果失败");
				return false;
			}

			curTex = outputTex;
		}

		// 最后一个效果输出到后缓冲纹理
		if (!_effects.back()->Build(curTex.get(), dr.GetBackBuffer())) {
			Logger::Get().Error("构建效果失败");
			return false;
		}
	}

	SIZE outputSize = texSizes.back();
	_outputRect.left = (hostSize.cx - outputSize.cx) / 2;
	_outputRect.right = _outputRect.left + outputSize.cx;
	_outputRect.top = (hostSize.cy - outputSize.cy) / 2;
	_outputRect.bottom = _outputRect.top + outputSize.cy;

	return true;
}
