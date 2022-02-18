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
#include "CursorDrawer.h"
#include "EffectDrawer.h"
#include "UIDrawer.h"
#include "FSRFilter.h"
#include "A4KSFilter.h"
#include "Logger.h"


Renderer::Renderer() {}

Renderer::~Renderer() {}

bool Renderer::Initialize(const std::string& effectsJson) {
	_gpuTimer.reset(new GPUTimer());

	if (!GetWindowRect(App::GetInstance().GetHwndSrc(), &_srcWndRect)) {
		Logger::Get().Win32Error("GetWindowRect 失败");
		return false;
	}

	/*if (!_ResolveEffectsJson(effectsJson)) {
		SPDLOG_LOGGER_ERROR(logger, "_ResolveEffectsJson 失败");
		return false;
	}

	ID3D11Texture2D* backBuffer = App::GetInstance().GetDeviceResources().GetBackBuffer();

	_UIDrawer.reset(new UIDrawer());
	if (!_UIDrawer->Initialize(backBuffer)) {
		return false;
	}

	_cursorDrawer.reset(new CursorDrawer());
	if (!_cursorDrawer->Initialize(backBuffer)) {
		SPDLOG_LOGGER_ERROR(logger, "初始化 CursorDrawer 失败");
		return false;
	}*/

	/*_fsrFilter.reset(new FSRFilter());
	if (!_fsrFilter->Initialize()) {
		return false;
	}*/
	_a4ksFilter.reset(new A4KSFilter());
	if (!_a4ksFilter->Initialize()) {
		return false;
	}

	return true;
}


void Renderer::Render() {
	if (!_CheckSrcState()) {
		Logger::Get().Info("源窗口状态改变，退出全屏");
		App::GetInstance().Quit();
		return;
	}

	DeviceResources& dr = App::GetInstance().GetDeviceResources();
	ID3D11DeviceContext3* d3dDC = dr.GetD3DDC();

	if (!_waitingForNextFrame) {
		dr.BeginFrame();
		_gpuTimer->BeginFrame();
	}

	auto state = App::GetInstance().GetFrameSource().Update();
	_waitingForNextFrame = state == FrameSourceBase::UpdateState::Waiting
		|| state == FrameSourceBase::UpdateState::Error;
	if (_waitingForNextFrame) {
		return;
	}

	d3dDC->ClearState();

	ID3D11RenderTargetView* rtv = nullptr;
	//dr.GetRenderTargetView()
	//d3dDC->ClearRenderTargetView()
	// 所有渲染都使用三角形带拓扑
	//d3dDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	//_fsrFilter->Draw();
	_a4ksFilter->Draw();
	/*
	if (!_cursorDrawer->Update()) {
		SPDLOG_LOGGER_ERROR(logger, "更新光标位置失败");
	}

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

	_UIDrawer->Draw();
	_cursorDrawer->Draw();*/


	dr.EndFrame();
}

bool Renderer::SetFillVS() {
	auto& dr = App::GetInstance().GetDeviceResources();

	if (!_fillVS) {
		const char* src = "void m(uint i:SV_VERTEXID,out float4 p:SV_POSITION,out float2 c:TEXCOORD){c=float2(i&1,i>>1)*2;p=float4(c.x*2-1,-c.y*2+1,0,1);}";

		winrt::com_ptr<ID3DBlob> blob;
		if (!dr.CompileShader(true, src, "m", blob.put(), "FillVS")) {
			Logger::Get().Error("编译 FillVS 失败");
			return false;
		}

		HRESULT hr = dr.GetD3DDevice()->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, _fillVS.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("创建 FillVS 失败", hr);
			return false;
		}
	}
	
	auto d3dDC = dr.GetD3DDC();
	d3dDC->IASetInputLayout(nullptr);
	d3dDC->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	d3dDC->VSSetShader(_fillVS.get(), nullptr, 0);

	return true;
}


bool Renderer::SetCopyPS(ID3D11SamplerState* sampler, ID3D11ShaderResourceView* input) {
	auto& dr = App::GetInstance().GetDeviceResources();

	if (!_copyPS) {
		const char* src = "Texture2D t:register(t0);SamplerState s:register(s0);float4 m(float4 p:SV_POSITION,float2 c:TEXCOORD):SV_Target{return t.Sample(s,c);}";

		winrt::com_ptr<ID3DBlob> blob;
		if (!dr.CompileShader(false, src, "m", blob.put(), "CopyPS")) {
			Logger::Get().Error("编译 CopyPS 失败");
			return false;
		}

		HRESULT hr = dr.GetD3DDevice()->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, _copyPS.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("创建 CopyPS 失败", hr);
			return false;
		}
	}

	auto d3dDC = dr.GetD3DDC();
	d3dDC->PSSetShader(_copyPS.get(), nullptr, 0);
	d3dDC->PSSetConstantBuffers(0, 0, nullptr);
	d3dDC->PSSetShaderResources(0, 1, &input);
	d3dDC->PSSetSamplers(0, 1, &sampler);

	return true;
}

bool Renderer::SetSimpleVS(ID3D11Buffer* simpleVB) {
	auto& dr = App::GetInstance().GetDeviceResources();

	if (!_simpleVS) {
		const char* src = "void m(float4 p:SV_POSITION,float2 c:TEXCOORD,out float4 q:SV_POSITION,out float2 d:TEXCOORD) {q=p;d=c;}";

		winrt::com_ptr<ID3DBlob> blob;
		if (!dr.CompileShader(true, src, "m", blob.put(), "SimpleVS")) {
			Logger::Get().Error("编译 SimpleVS 失败");
			return false;
		}

		auto d3dDevice = dr.GetD3DDevice();

		HRESULT hr = d3dDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, _simpleVS.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("创建 SimpleVS 失败", hr);
			return false;
		}

		hr = d3dDevice->CreateInputLayout(
			DirectX::VertexPositionTexture::InputElements,
			DirectX::VertexPositionTexture::InputElementCount,
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
			_simpleIL.put()
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("创建 SimpleVS 输入布局失败", hr);
			return false;
		}
	}

	auto d3dDC = dr.GetD3DDC();
	d3dDC->IASetInputLayout(_simpleIL.get());

	UINT stride = sizeof(DirectX::VertexPositionTexture);
	UINT offset = 0;
	d3dDC->IASetVertexBuffers(0, 1, &simpleVB, &stride, &offset);

	d3dDC->VSSetShader(_simpleVS.get(), nullptr, 0);

	return true;
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
	if (App::GetInstance().GetFrameSource().IsScreenCapture()
		&& GetWindowStyle(hwndForeground) & (WS_POPUP | WS_CHILD)
	) {
		if (!Utils::GetWindowFrameRect(hwndForeground, rectForground)) {
			Logger::Get().Error("GetWindowFrameRect 失败");
			return false;
		}

		// 弹窗如果完全在源窗口客户区内则不退出全屏
		const RECT& srcFrameRect = App::GetInstance().GetFrameSource().GetSrcFrameRect();
		if (rectForground.left >= srcFrameRect.left
			&& rectForground.right <= srcFrameRect.right
			&& rectForground.top >= srcFrameRect.top
			&& rectForground.bottom <= srcFrameRect.bottom
		) {
			return true;
		}
	}

	// 非多屏幕模式下退出全屏
	if (!App::GetInstance().IsMultiMonitorMode()) {
		return false;
	}

	if (rectForground == RECT{}) {
		if (!Utils::GetWindowFrameRect(hwndForeground, rectForground)) {
			Logger::Get().Error("GetWindowFrameRect 失败");
			return false;
		}
	}

	IntersectRect(&rectForground, &App::GetInstance().GetHostWndRect(), &rectForground);

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
	HWND hwndSrc = App::GetInstance().GetHwndSrc();

	if (!App::GetInstance().IsBreakpointMode()) {
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
	_effectInput = App::GetInstance().GetFrameSource().GetOutput();
	D3D11_TEXTURE2D_DESC inputDesc;
	_effectInput->GetDesc(&inputDesc);

	const RECT& hostWndRect = App::GetInstance().GetHostWndRect();
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

	for (const auto& effectJson : effectsArr) {
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

		if (!effect.Initialize((L"effects\\" + StrUtils::UTF8ToUTF16(effectName->value.GetString()) + L".hlsl").c_str())) {
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

	auto& dr = App::GetInstance().GetDeviceResources();
	auto d3dDevice = dr.GetD3DDevice();

	if (_effects.size() == 1) {
		if (!_effects.back()->Build(_effectInput.get(), dr.GetBackBuffer())) {
			Logger::Get().Error("构建效果失败");
			return false;
		}
	} else {
		// 创建效果间的中间纹理
		winrt::com_ptr<ID3D11Texture2D> curTex = _effectInput;

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

bool Renderer::SetAlphaBlend(bool enable) {
	auto& dr = App::GetInstance().GetDeviceResources();

	if (!enable) {
		dr.GetD3DDC()->OMSetBlendState(nullptr, nullptr, 0xffffffff);
		return true;
	}
	
	if (!_alphaBlendState) {
		D3D11_BLEND_DESC desc{};
		desc.RenderTarget[0].BlendEnable = TRUE;
		desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		desc.RenderTarget[0].BlendOp = desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		HRESULT hr = dr.GetD3DDevice()->CreateBlendState(&desc, _alphaBlendState.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateBlendState 失败", hr);
			return false;
		}
	}
	
	dr.GetD3DDC()->OMSetBlendState(_alphaBlendState.get(), nullptr, 0xffffffff);
	return true;
}
