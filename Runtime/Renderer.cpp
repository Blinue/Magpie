#include "pch.h"
#include "Renderer.h"
#include "App.h"
#include "Utils.h"
#include "StrUtils.h"
#include "EffectCompiler.h"
#include "FrameSourceBase.h"
#include "DeviceResources.h"
#include "GPUTimer.h"
#include "EffectDrawer.h"
#include "OverlayDrawer.h"
#include "Logger.h"
#include "CursorManager.h"
#include "Config.h"
#include "WindowsMessages.h"

#pragma push_macro("GetObject")
#undef GetObject
#include <rapidjson/document.h>


static std::optional<LRESULT> WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WindowsMessages::WM_TOGGLE_OVERLAY) {
		Renderer& renderer = App::Get().GetRenderer();
		renderer.SetUIVisibility(!renderer.IsUIVisiable());
		return 0;
	}

	return std::nullopt;
}

Renderer::Renderer() {}

Renderer::~Renderer() {
	if (_handlerID != 0) {
		App::Get().UnregisterWndProcHandler(_handlerID);
	}
}

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
	
	if (App::Get().GetConfig().IsShowFPS()) {
		_overlayDrawer.reset(new OverlayDrawer());
		if (!_overlayDrawer->Initialize()) {
			Logger::Get().Error("初始化 OverlayDrawer 失败");
			return false;
		}
	}

	// 初始化所有效果共用的动态常量缓冲区
	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bd.ByteWidth = 4 * (UINT)_dynamicConstants.size();
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	HRESULT hr = App::Get().GetDeviceResources().GetD3DDevice()
		->CreateBuffer(&bd, nullptr, _dynamicCB.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateBuffer 失败", hr);
		return false;
	}

	_handlerID = App::Get().RegisterWndProcHandler(WndProcHandler);

	return true;
}


void Renderer::Render() {
	if (!_CheckSrcState()) {
		Logger::Get().Info("源窗口状态改变，退出全屏");
		App::Get().Quit();
		return;
	}

	DeviceResources& dr = App::Get().GetDeviceResources();

	if (!_waitingForNextFrame) {
		dr.BeginFrame();
		_gpuTimer->OnBeginFrame();
	}

	// 首先处理配置改变产生的回调
	App::Get().GetConfig().OnBeginFrame();

	auto state = App::Get().GetFrameSource().Update();
	_waitingForNextFrame = state == FrameSourceBase::UpdateState::Waiting
		|| state == FrameSourceBase::UpdateState::Error;
	if (_waitingForNextFrame) {
		return;
	}
	
	App::Get().GetCursorManager().OnBeginFrame();

	if (!_UpdateDynamicConstants()) {
		Logger::Get().Error("_UpdateDynamicConstants 失败");
	}

	auto d3dDC = dr.GetD3DDC();

	{
		ID3D11Buffer* t = _dynamicCB.get();
		d3dDC->CSSetConstantBuffers(0, 1, &t);
	}

	_gpuTimer->OnBeginEffects();

	UINT idx = 0;
	if (state == FrameSourceBase::UpdateState::NoUpdate) {
		// 此帧内容无变化
		// 从第一个使用动态常量的效果开始渲染
		// 如果没有则只渲染最后一个效果的最后一个通道

		size_t i = 0;
		for (size_t end = _effects.size() - 1; i < end; ++i) {
			if (_effects[i]->IsUseDynamic()) {
				break;
			} else {
				for (UINT j = (UINT)_effects[i]->GetDesc().passes.size(); j > 0; --j) {
					_gpuTimer->OnEndPass(idx++);
				}
			}
		}

		if (i == _effects.size()) {
			// 只渲染最后一个 Effect 的最后一个 pass
			_effects.back()->Draw(idx, true);
		} else {
			for (; i < _effects.size(); ++i) {
				_effects[i]->Draw(idx);
			}
		}
	} else {
		for (auto& effect : _effects) {
			effect->Draw(idx);
		}
	}

	_gpuTimer->OnEndEffects();

	if (_overlayDrawer) {
		_overlayDrawer->Draw();
	}

	dr.EndFrame();
}

bool Renderer::IsUIVisiable() const noexcept {
	return _overlayDrawer ? _overlayDrawer->IsUIVisiable() : false;
}

void Renderer::SetUIVisibility(bool value) {
	if (!value) {
		if (_overlayDrawer && _overlayDrawer->IsUIVisiable()) {
			_overlayDrawer->SetUIVisibility(false);
			_gpuTimer->StopProfiling();
		}
		return;
	}

	if (!_overlayDrawer) {
		_overlayDrawer.reset(new OverlayDrawer());
		if (!_overlayDrawer->Initialize()) {
			Logger::Get().Error("初始化 OverlayDrawer 失败");
			return;
		}
	}

	if (!_overlayDrawer->IsUIVisiable()) {
		_overlayDrawer->SetUIVisibility(true);

		UINT passCount = 0;
		for (const auto& effect : _effects) {
			passCount += (UINT)effect->GetDesc().passes.size();
		}

		// StartProfiling 必须在 OnBeginFrame 之前调用
		_gpuTimer->StartProfiling(std::chrono::milliseconds(500), passCount);
	}
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

	// 排除开始菜单
	return Utils::IsStartMenu(hwndForeground);
}

const EffectDesc& Renderer::GetEffectDesc(UINT idx) const noexcept {
	assert(idx < _effects.size());
	return _effects[idx]->GetDesc();
}

bool Renderer::_CheckSrcState() {
	HWND hwndSrc = App::Get().GetHwndSrc();

	if (!App::Get().GetConfig().IsBreakpointMode()) {
		HWND hwndForeground = GetForegroundWindow();
		// 在 3D 游戏模式下打开游戏内覆盖则全屏窗口可以接收焦点
		if (!App::Get().GetConfig().Is3DMode() || !IsUIVisiable() || hwndForeground != App::Get().GetHwndHost()) {
			if (hwndForeground && hwndForeground != hwndSrc && !CheckForeground(hwndForeground)) {
				Logger::Get().Info("前台窗口已改变");
				return false;
			}
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

	// 不得为空
	if (effectsArr.Empty()) {
		Logger::Get().Error("解析 json 失败：根元素为空");
		return false;
	}

	// 并行编译所有效果

	UINT effectCount = effectsArr.Size();
	std::vector<const char*> effectNames(effectCount);
	std::vector<EffectParams> effectParams(effectCount);
	std::vector<EffectDesc> effectDescs(effectCount);
	std::atomic<bool> allSuccess = true;

	int duration = Utils::Measure([&]() {
		Utils::RunParallel([&](UINT id) {
			const auto& effectJson = effectsArr[id];
			UINT effectFlag = (id == effectCount - 1) ? EFFECT_FLAG_LAST_EFFECT : 0;
			EffectParams& params = effectParams[id];

			if (!effectJson.IsObject()) {
				Logger::Get().Error("解析 json 失败：根数组中存在非法成员");
				allSuccess = false;
				return;
			}

			{
				auto effectName = effectJson.FindMember("effect");
				if (effectName == effectJson.MemberEnd() || !effectName->value.IsString()) {
					Logger::Get().Error(fmt::format("解析效果#{}失败：未找到 effect 属性或该属性的值不合法", id));
					allSuccess = false;
					return;
				}
				effectNames[id] = effectName->value.GetString();
			}

			for (const auto& prop : effectJson.GetObject()) {
				if (!prop.name.IsString()) {
					Logger::Get().Error(fmt::format("解析效果#{}失败：非法的效果名", id));
					allSuccess = false;
					return;
				}

				std::string_view name = prop.name.GetString();

				if (name == "effect") {
					continue;
				} else if (name == "inlineParams") {
					if (!prop.value.IsBool()) {
						Logger::Get().Error(fmt::format("解析效果#{}（{}）失败：成员 inlineParams 必须为 bool 类型", id, effectNames[id]));
						allSuccess = false;
						return;
					}

					if (prop.value.GetBool()) {
						effectFlag |= EFFECT_FLAG_INLINE_PARAMETERS;
					}
					continue;
				} else if (name == "fp16") {
					if (!prop.value.IsBool()) {
						Logger::Get().Error(fmt::format("解析效果#{}（{}）失败：成员 fp16 必须为 bool 类型", id, effectNames[id]));
						allSuccess = false;
						return;
					}

					if (prop.value.GetBool()) {
						effectFlag |= EFFECT_FLAG_FP16;
					}
					continue;
				} else if (name == "scale") {
					auto scaleProp = effectJson.FindMember("scale");
					if (scaleProp != effectJson.MemberEnd()) {
						if (!scaleProp->value.IsArray()) {
							Logger::Get().Error(fmt::format("解析效果#{}（{}）失败：成员 scale 必须为数组类型", id, effectNames[id]));
							allSuccess = false;
							return;
						}

						const auto& scale = scaleProp->value.GetArray();
						if (scale.Size() != 2 || !scale[0].IsNumber() || !scale[1].IsNumber()) {
							Logger::Get().Error(fmt::format("解析效果#{}（{}）失败：成员 scale 格式非法", id, effectNames[id]));
							allSuccess = false;
							return;
						}

						params.scale = std::make_pair(scale[0].GetFloat(), scale[1].GetFloat());
					}
				} else {
					auto& paramValue = params.params[std::string(name)];

					if (prop.value.IsFloat()) {
						paramValue = prop.value.GetFloat();
					} else if (prop.value.IsInt()) {
						paramValue = prop.value.GetInt();
					} else if (prop.value.IsBool()) {
						// bool 值视为 int
						paramValue = (int)prop.value.GetBool();
					} else {
						Logger::Get().Error(fmt::format("解析效果#{}（{}）失败：成员 {} 的类型非法", id, effectNames[id], name));
						allSuccess = false;
						return;
					}
				}
			}

			std::wstring fileName = (L"effects\\" + StrUtils::UTF8ToUTF16(effectNames[id]) + L".hlsl");

			bool success = true;
			int duration = Utils::Measure([&]() {
				success = !EffectCompiler::Compile(effectNames[id], effectFlag, effectParams[id].params, effectDescs[id]);
			});

			if (success) {
				Logger::Get().Info(fmt::format("编译 {} 用时 {} 毫秒", StrUtils::UTF16ToUTF8(fileName), duration / 1000.0f));
			} else {
				Logger::Get().Error(StrUtils::Concat("编译 ", StrUtils::UTF16ToUTF8(fileName), " 失败"));
				allSuccess = false;
			}
		}, effectCount);
	});

	if (allSuccess) {
		if (effectCount > 1) {
			Logger::Get().Info(fmt::format("编译着色器总计用时 {} 毫秒", duration / 1000.0f));
		}
	} else {
		return false;
	}

	ID3D11Texture2D* effectInput = App::Get().GetFrameSource().GetOutput();
	_effects.resize(effectCount);

	for (UINT i = 0; i < effectCount; ++i) {
		bool isLastEffect = i == effectCount - 1;

		_effects[i].reset(new EffectDrawer());
		if (!_effects[i]->Initialize(
			effectDescs[i], effectParams[i], effectInput, &effectInput,
			isLastEffect ? &_outputRect : nullptr,
			isLastEffect ? &_virtualOutputRect : nullptr
		)) {
			Logger::Get().Error(fmt::format("初始化效果#{} ({}) 失败", i, effectNames[i]));
			return false;
		}
	}

	return true;
}

bool Renderer::_UpdateDynamicConstants() {
	// cbuffer __CB1 : register(b0) {
	//     int4 __cursorRect;
	//     float2 __cursorPt;
	//     uint2 __cursorPos;
	//     uint __cursorType;
	//     uint __frameCount;
	// };

	CursorManager& cursorManager = App::Get().GetCursorManager();
	if (cursorManager.HasCursor() && !(App::Get().GetConfig().Is3DMode() && IsUIVisiable())) {
		const POINT* pos = cursorManager.GetCursorPos();
		const CursorManager::CursorInfo* ci = cursorManager.GetCursorInfo();

		ID3D11Texture2D* cursorTex;
		CursorManager::CursorType cursorType = CursorManager::CursorType::Color;
		if (!cursorManager.GetCursorTexture(&cursorTex, cursorType)) {
			Logger::Get().Error("GetCursorTexture 失败");
		}
		assert(pos && ci);

		float cursorZoomFactor = App::Get().GetConfig().GetCursorZoomFactor();
		if (cursorZoomFactor < 1e-5) {
			SIZE srcFrameSize = Utils::GetSizeOfRect(App::Get().GetFrameSource().GetSrcFrameRect());
			SIZE virtualOutputSize = Utils::GetSizeOfRect(_virtualOutputRect);
			cursorZoomFactor = (((float)virtualOutputSize.cx / srcFrameSize.cx) 
				+ ((float)virtualOutputSize.cy / srcFrameSize.cy)) / 2;
		}

		SIZE cursorSize = {
			std::lroundf(ci->size.cx * cursorZoomFactor),
			std::lroundf(ci->size.cy * cursorZoomFactor)
		};

		_dynamicConstants[0].intVal = pos->x - std::lroundf(ci->hotSpot.x * cursorZoomFactor);
		_dynamicConstants[1].intVal = pos->y - std::lroundf(ci->hotSpot.y * cursorZoomFactor);
		_dynamicConstants[2].intVal = _dynamicConstants[0].intVal + cursorSize.cx;
		_dynamicConstants[3].intVal = _dynamicConstants[1].intVal + cursorSize.cy;

		_dynamicConstants[4].floatVal = 1.0f / cursorSize.cx;
		_dynamicConstants[5].floatVal = 1.0f / cursorSize.cy;

		_dynamicConstants[6].uintVal = pos->x;
		_dynamicConstants[7].uintVal = pos->y;

		_dynamicConstants[8].uintVal = (UINT)cursorType;
	} else {
		_dynamicConstants[0].intVal = INT_MAX;
		_dynamicConstants[1].intVal = INT_MAX;
		_dynamicConstants[2].intVal = INT_MAX;
		_dynamicConstants[3].intVal = INT_MAX;
		_dynamicConstants[6].uintVal = UINT_MAX;
		_dynamicConstants[7].uintVal = UINT_MAX;
	}

	_dynamicConstants[9].uintVal = _gpuTimer->GetFrameCount();

	auto d3dDC = App::Get().GetDeviceResources().GetD3DDC();

	D3D11_MAPPED_SUBRESOURCE ms;
	HRESULT hr = d3dDC->Map(_dynamicCB.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
	if (SUCCEEDED(hr)) {
		std::memcpy(ms.pData, _dynamicConstants.data(), _dynamicConstants.size() * 4);
		d3dDC->Unmap(_dynamicCB.get(), 0);
	} else {
		Logger::Get().ComError("Map 失败", hr);
		return false;
	}

	return true;
}

#pragma pop_macro("GetObject")
