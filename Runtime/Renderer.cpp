#include "pch.h"
#include "Renderer.h"
#include "App.h"
#include "Utils.h"
#include "shaders/FillVS.h"
#include "shaders/CopyPS.h"
#include "shaders/SimpleVS.h"
#include <VertexTypes.h>
#include "EffectCompiler.h"
#include <rapidjson/document.h>


extern std::shared_ptr<spdlog::logger> logger;



Renderer::~Renderer() {
	CloseHandle(_frameLatencyWaitableObject);
}

bool Renderer::Initialize() {
	if (!_InitD3D()) {
		return false;
	}

	int frameRate = App::GetInstance().GetFrameRate();
	
	if (frameRate > 0) {
		_timer.SetFixedTimeStep(true);
		_timer.SetTargetElapsedSeconds(1.0 / frameRate);
	} else {
		_timer.SetFixedTimeStep(false);
	}
	
	_timer.ResetElapsedTime();

	return true;
}

bool Renderer::InitializeEffectsAndCursor(const std::string& effectsJson) {
	RECT destRect;
	if (!_ResolveEffectsJson(effectsJson, destRect)) {
		return false;
	}
	
	if (App::GetInstance().IsShowFPS()) {
		if (!_frameRateDrawer.Initialize(_backBuffer, destRect)) {
			return false;
		}
	}

	if (!_cursorRenderer.Initialize(_backBuffer, destRect)) {
		SPDLOG_LOGGER_CRITICAL(logger, "构建 CursorDrawer 失败");
		return false;
	}

	return true;
}


void Renderer::Render() {
	if (_waitingForNextFrame) {
		_Render();
	} else {
		_timer.Tick(std::bind(&Renderer::_Render, this));
	}
}

bool Renderer::GetRenderTargetView(ID3D11Texture2D* texture, ID3D11RenderTargetView** result) {
	auto it = _rtvMap.find(texture);
	if (it != _rtvMap.end()) {
		*result = it->second.Get();
		return true;
	}

	ComPtr<ID3D11RenderTargetView>& r = _rtvMap[texture];
	HRESULT hr = _d3dDevice->CreateRenderTargetView(texture, nullptr, &r);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("CreateRenderTargetView 失败", hr));
		return false;
	} else {
		*result = r.Get();
		return true;
	}
}

bool Renderer::GetShaderResourceView(ID3D11Texture2D* texture, ID3D11ShaderResourceView** result) {
	auto it = _srvMap.find(texture);
	if (it != _srvMap.end()) {
		*result = it->second.Get();
		return true;
	}

	ComPtr<ID3D11ShaderResourceView>& r = _srvMap[texture];
	HRESULT hr = _d3dDevice->CreateShaderResourceView(texture, nullptr, &r);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("CreateShaderResourceView 失败", hr));
		return false;
	} else {
		*result = r.Get();
		return true;
	}
}

bool Renderer::SetFillVS() {
	if (!_fillVS) {
		HRESULT hr = _d3dDevice->CreateVertexShader(FillVSShaderByteCode, sizeof(FillVSShaderByteCode), nullptr, &_fillVS);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_CRITICAL(logger, MakeComErrorMsg("创建 FillVS 失败", hr));
			return false;
		}
	}
	
	_d3dDC->IASetInputLayout(nullptr);
	_d3dDC->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	_d3dDC->VSSetShader(_fillVS.Get(), nullptr, 0);

	return true;
}


bool Renderer::SetCopyPS(ID3D11SamplerState* sampler, ID3D11ShaderResourceView* input) {
	if (!_copyPS) {
		HRESULT hr = _d3dDevice->CreatePixelShader(CopyPSShaderByteCode, sizeof(CopyPSShaderByteCode), nullptr, &_copyPS);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_CRITICAL(logger, MakeComErrorMsg("创建 CopyPS 失败", hr));
			return false;
		}
	}

	_d3dDC->PSSetShader(_copyPS.Get(), nullptr, 0);
	_d3dDC->PSSetConstantBuffers(0, 0, nullptr);
	_d3dDC->PSSetShaderResources(0, 1, &input);
	_d3dDC->PSSetSamplers(0, 1, &sampler);

	return true;
}

bool Renderer::SetSimpleVS(ID3D11Buffer* simpleVB) {
	if (!_simpleVS) {
		HRESULT hr = _d3dDevice->CreateVertexShader(SimpleVSShaderByteCode, sizeof(SimpleVSShaderByteCode), nullptr, &_simpleVS);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_CRITICAL(logger, MakeComErrorMsg("创建 SimpleVS 失败", hr));
			return false;
		}

		hr = _d3dDevice->CreateInputLayout(
			VertexPositionTexture::InputElements,
			VertexPositionTexture::InputElementCount,
			SimpleVSShaderByteCode,
			sizeof(SimpleVSShaderByteCode),
			&_simpleIL
		);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_CRITICAL(logger, fmt::sprintf("创建 SimpleVS 输入布局失败\n\tHRESULT：0x%X", hr));
			return false;
		}
	}

	_d3dDC->IASetInputLayout(_simpleIL.Get());

	UINT stride = sizeof(VertexPositionTexture);
	UINT offset = 0;
	_d3dDC->IASetVertexBuffers(0, 1, &simpleVB, &stride, &offset);

	_d3dDC->VSSetShader(_simpleVS.Get(), nullptr, 0);

	return true;
}

bool Renderer::_InitD3D() {
	HRESULT hr;
	const SIZE hostSize = App::GetInstance().GetHostWndSize();

	{
		UINT createDeviceFlags = 0;
#ifdef _DEBUG
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		D3D_FEATURE_LEVEL featureLevels[] = {
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0
		};

		ComPtr<ID3D11Device> d3dDevice;
		ComPtr<ID3D11DeviceContext> d3dDC;
		D3D_FEATURE_LEVEL featureLevel;
		hr = D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			createDeviceFlags,
			featureLevels,
			1,
			D3D11_SDK_VERSION,
			&d3dDevice,
			&featureLevel,
			&d3dDC
		);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_CRITICAL(logger, MakeComErrorMsg("D3D11CreateDevice 失败", hr));
			return false;
		}
		SPDLOG_LOGGER_INFO(logger, fmt::format("已创建 D3D Device\n\t功能级别：{}", featureLevel));

		hr = d3dDevice.As<ID3D11Device1>(&_d3dDevice);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_CRITICAL(logger, MakeComErrorMsg("获取 ID3D11Device1 失败", hr));
			return false;
		}

		hr = d3dDC.As<ID3D11DeviceContext1>(&_d3dDC);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_CRITICAL(logger, MakeComErrorMsg("获取 ID3D11DeviceContext1 失败", hr));
			return false;
		}
	}

	{
		ComPtr<IDXGIFactory2> dxgiFactory;

		hr = _d3dDevice.As<IDXGIDevice1>(&_dxgiDevice);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_CRITICAL(logger, MakeComErrorMsg("获取 IDXGIDevice 失败", hr));
			return false;
		}

		// 将 GPU 优先级设为最高，不一定有用
		hr = _dxgiDevice->SetGPUThreadPriority(7);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("SetGPUThreadPriority", hr));
		}

		ComPtr<IDXGIAdapter> dxgiAdapter;
		hr = _dxgiDevice->GetAdapter(&dxgiAdapter);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_CRITICAL(logger, MakeComErrorMsg("获取 IDXGIAdapter 失败", hr));
			return false;
		}

		hr = dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));
		if (FAILED(hr)) {
			SPDLOG_LOGGER_CRITICAL(logger, MakeComErrorMsg("获取 IDXGIFactory2 失败", hr));
			return false;
		}

		// 检查可变帧率支持
		BOOL supportTearing = FALSE;
		ComPtr<IDXGIFactory5> dxgiFactory5;
		hr = dxgiFactory.As<IDXGIFactory5>(&dxgiFactory5);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_WARN(logger, MakeComErrorMsg("获取 IDXGIFactory5 失败", hr));
		} else {
			hr = dxgiFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &supportTearing, sizeof(supportTearing));
			if (FAILED(hr)) {
				SPDLOG_LOGGER_WARN(logger, MakeComErrorMsg("CheckFeatureSupport 失败", hr));
			}
		}
		SPDLOG_LOGGER_INFO(logger, fmt::format("可变刷新率支持：{}", supportTearing ? "是" : "否"));

		int frameRate = App::GetInstance().GetFrameRate();
		if (frameRate != 0 && !supportTearing) {
			SPDLOG_LOGGER_CRITICAL(logger, "当前显示器不支持可变刷新率，初始化失败");
			App::GetInstance().SetErrorMsg(ErrorMessages::VSYNC_OFF_NOT_SUPPORTED);
			return false;
		}

		DXGI_SWAP_CHAIN_DESC1 sd = {};
		sd.Width = hostSize.cx;
		sd.Height = hostSize.cy;
		sd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		sd.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
		sd.BufferCount = 2;
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		sd.Flags = frameRate != 0 ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

		ComPtr<IDXGISwapChain1> dxgiSwapChain = nullptr;
		hr = dxgiFactory->CreateSwapChainForHwnd(
			_d3dDevice.Get(),
			App::GetInstance().GetHwndHost(),
			&sd,
			nullptr,
			nullptr,
			&dxgiSwapChain
		);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_CRITICAL(logger, MakeComErrorMsg("创建交换链失败", hr));
			return false;
		}

		hr = dxgiSwapChain.As<IDXGISwapChain2>(&_dxgiSwapChain);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_CRITICAL(logger, MakeComErrorMsg("获取 IDXGISwapChain2 失败", hr));
			return false;
		}
		
		if (frameRate != 0) {
			hr = _dxgiDevice->SetMaximumFrameLatency(1);
			if (FAILED(hr)) {
				SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("SetMaximumFrameLatency 失败", hr));
			}
		} else {
			_frameLatencyWaitableObject = _dxgiSwapChain->GetFrameLatencyWaitableObject();
		}
		
		hr = dxgiFactory->MakeWindowAssociation(App::GetInstance().GetHwndHost(), DXGI_MWA_NO_ALT_ENTER);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("MakeWindowAssociation 失败", hr));
		}

		// 检查 Multiplane Overlay 支持
		BOOL supportMPO = FALSE;
		ComPtr<IDXGIOutput> output;
		hr = _dxgiSwapChain->GetContainingOutput(&output);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_WARN(logger, MakeComErrorMsg("获取 IDXGIOutput 失败", hr));
		} else {
			ComPtr<IDXGIOutput2> output2;
			hr = output.As<IDXGIOutput2>(&output2);
			if (FAILED(hr)) {
				SPDLOG_LOGGER_WARN(logger, MakeComErrorMsg("获取 IDXGIOutput2 失败", hr));
			} else {
				supportMPO = output2->SupportsOverlays();
			}
		}
		SPDLOG_LOGGER_INFO(logger, fmt::format("Multiplane Overlay 支持：{}", supportMPO ? "是" : "否"));

		// 检查 Hardware Composition 支持
		BOOL supportHardwareComposition = FALSE;
		ComPtr<IDXGIOutput6> output6;
		hr = output.As<IDXGIOutput6>(&output6);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_WARN(logger, MakeComErrorMsg("获取 IDXGIOutput6 失败", hr));
		} else {
			UINT flags;
			hr = output6->CheckHardwareCompositionSupport(&flags);
			if (FAILED(hr)) {
				SPDLOG_LOGGER_WARN(logger, MakeComErrorMsg("CheckHardwareCompositionSupport 失败", hr));
			} else {
				supportHardwareComposition = flags & DXGI_HARDWARE_COMPOSITION_SUPPORT_FLAG_WINDOWED;
			}
		}
		SPDLOG_LOGGER_INFO(logger, fmt::format("Hardware Composition 支持：{}", supportHardwareComposition ? "是" : "否"));
	}

	hr = _dxgiSwapChain->GetBuffer(0, IID_PPV_ARGS(&_backBuffer));
	if (FAILED(hr)) {
		SPDLOG_LOGGER_CRITICAL(logger, MakeComErrorMsg("获取后缓冲区失败", hr));
		return false;
	}

	SPDLOG_LOGGER_INFO(logger, "Renderer 初始化完成");
	return true;
}

void Renderer::_Render() {
	int frameRate = App::GetInstance().GetFrameRate();

	if (!_waitingForNextFrame && frameRate == 0) {
		WaitForSingleObjectEx(_frameLatencyWaitableObject, 1000, TRUE);
	}

	if (!_CheckSrcState()) {
		SPDLOG_LOGGER_INFO(logger, "源窗口状态改变，退出全屏");
		DestroyWindow(App::GetInstance().GetHwndHost());
		return;
	}

	_waitingForNextFrame = !App::GetInstance().GetFrameSource().Update();
	if (_waitingForNextFrame) {
		return;
	}

	_d3dDC->ClearState();
	// 所有渲染都使用三角形带拓扑
	_d3dDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	for (EffectDrawer& effect : _effects) {
		effect.Draw();
	}

	if (App::GetInstance().IsShowFPS()) {
		_frameRateDrawer.Draw();
	}

	_cursorRenderer.Draw();

	if (frameRate != 0) {
		_dxgiSwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
	} else {
		_dxgiSwapChain->Present(1, 0);
	}
}

bool Renderer::_CheckSrcState() {
	HWND hwndSrc = App::GetInstance().GetHwndSrc();
	if (GetForegroundWindow() != hwndSrc) {
		SPDLOG_LOGGER_INFO(logger, "前台窗口已改变");
		return false;
	}

	RECT rect = Utils::GetClientScreenRect(hwndSrc);
	if (App::GetInstance().GetSrcClientRect() != rect) {
		SPDLOG_LOGGER_INFO(logger, "源窗口位置或大小改变");
		return false;
	}
	if (Utils::GetWindowShowCmd(hwndSrc) != SW_NORMAL) {
		SPDLOG_LOGGER_INFO(logger, "源窗口显示状态改变");
		return false;
	}

	return true;
}

bool Renderer::_ResolveEffectsJson(const std::string& effectsJson, RECT& destRect) {
	_effectInput = App::GetInstance().GetFrameSource().GetOutput();
	D3D11_TEXTURE2D_DESC inputDesc;
	_effectInput->GetDesc(&inputDesc);

	SIZE hostSize = App::GetInstance().GetHostWndSize();

	rapidjson::Document doc;
	if (doc.Parse(effectsJson.c_str(), effectsJson.size()).HasParseError()) {
		// 解析 json 失败
		return false;
	}

	if (!doc.IsArray()) {
		return false;
	}

	std::vector<SIZE> texSizes;
	texSizes.push_back({ (LONG)inputDesc.Width, (LONG)inputDesc.Height });

	for (const auto& effectJson : doc.GetArray()) {
		if (!effectJson.IsObject()) {
			return false;
		}

		EffectDrawer& effect = _effects.emplace_back();

		auto effectName = effectJson.FindMember("effect");
		if (effectName == effectJson.MemberEnd() || !effectName->value.IsString()) {
			// 未找到 effect 属性或该属性不合法
			return false;
		}

		if (!effect.Initialize((L"shaders\\" + StrUtils::UTF8ToUTF16(effectName->value.GetString()) + L".hlsl").c_str())) {
			return false;
		}

		if (effect.CanSetOutputSize()) {
			// scale 属性可用
			auto scaleProp = effectJson.FindMember("scale");
			if (scaleProp != effectJson.MemberEnd()) {
				if (!scaleProp->value.IsArray()) {
					return false;
				}

				// scale 属性的值为两个元素组成的数组
				// [+, +]：缩放比例
				// [0, 0]：非等比例缩放到屏幕大小
				// [-, -]：相对于屏幕能容纳的最大等比缩放的比例

				const auto& scale = scaleProp->value.GetArray();
				if (scale.Size() != 2 || !scale[0].IsNumber() || !scale[1].IsNumber()) {
					return false;
				}

				float scaleX = scale[0].GetFloat();
				float scaleY = scale[1].GetFloat();

				static float DELTA = 1e-5f;

				SIZE outputSize = texSizes.back();;

				if (scaleX >= DELTA) {
					if (scaleY < DELTA) {
						return false;
					}

					outputSize = { std::lroundf(outputSize.cx * scaleX), std::lroundf(outputSize.cy * scaleY) };
				} else if (std::abs(scaleX) < DELTA) {
					outputSize = hostSize;
				} else {
					if (scaleY > -DELTA) {
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

		for (const auto& prop : effectJson.GetObject()) {
			if (!prop.name.IsString()) {
				return false;
			}

			std::string_view name = prop.name.GetString();

			if (name == "effect" || (effect.CanSetOutputSize() && name == "scale")) {
				continue;
			} else {
				auto type = effect.GetConstantType(name);
				if (type == EffectDrawer::ConstantType::Float) {
					if (!prop.value.IsNumber()) {
						return false;
					}

					if (!effect.SetConstant(name, prop.value.GetFloat())) {
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
						return false;
					}

					if (!effect.SetConstant(name, value)) {
						return false;
					}
				} else {
					return false;
				}
			}
		}

		SIZE& outputSize = texSizes.emplace_back();
		if (!effect.CalcOutputSize(texSizes[texSizes.size() - 2], outputSize)) {
			return false;
		}
	}

	if (_effects.size() == 1) {
		if (!_effects.back().Build(_effectInput, _backBuffer)) {
			return false;
		}
	} else if(_effects.size() > 1) {
		// 创建效果间的中间纹理
		ComPtr<ID3D11Texture2D> curTex = _effectInput;

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

			ComPtr<ID3D11Texture2D> outputTex;
			HRESULT hr = _d3dDevice->CreateTexture2D(&desc, nullptr, &outputTex);
			if (FAILED(hr)) {
				return false;
			}

			if (!_effects[i].Build(curTex, outputTex)) {
				return false;
			}

			curTex = outputTex;
		}

		// 最后一个效果输出到后缓冲纹理
		if (!_effects.back().Build(curTex, _backBuffer)) {
			return false;
		}
	}

	SIZE outputSize = texSizes.back();
	destRect.left = (hostSize.cx - outputSize.cx) / 2;
	destRect.right = destRect.left + outputSize.cx;
	destRect.top = (hostSize.cy - outputSize.cy) / 2;
	destRect.bottom = destRect.top + outputSize.cy;

	return true;
}

bool Renderer::SetAlphaBlend(bool enable) {
	if (!enable) {
		_d3dDC->OMSetBlendState(nullptr, nullptr, 0xffffffff);
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

		HRESULT hr = _d3dDevice->CreateBlendState(&desc, &_alphaBlendState);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_CRITICAL(logger, MakeComErrorMsg("CreateBlendState 失败", hr));
			return false;
		}
	}
	
	_d3dDC->OMSetBlendState(_alphaBlendState.Get(), nullptr, 0xffffffff);
	return true;
}

bool Renderer::GetSampler(EffectSamplerFilterType filterType, ID3D11SamplerState** result) {
	if (filterType == EffectSamplerFilterType::Linear) {
		if (!_linearSampler) {
			D3D11_SAMPLER_DESC desc{};
			desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
			desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
			desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
			desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
			desc.MinLOD = 0;
			desc.MaxLOD = 0;
			HRESULT hr = _d3dDevice->CreateSamplerState(&desc, &_linearSampler);

			if (FAILED(hr)) {
				SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("创建 ID3D11SamplerState 出错", hr));
				return false;
			} else {
				SPDLOG_LOGGER_INFO(logger, "已创建 ID3D11SamplerState\n\tFilter：D3D11_FILTER_MIN_MAG_MIP_LINEAR");
			}
		}

		*result = _linearSampler.Get();
	} else {
		if (!_pointSampler) {
			D3D11_SAMPLER_DESC desc{};
			desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
			desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
			desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
			desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
			desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
			desc.MinLOD = 0;
			desc.MaxLOD = 0;
			HRESULT hr = _d3dDevice->CreateSamplerState(&desc, &_pointSampler);

			if (FAILED(hr)) {
				SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("创建 ID3D11SamplerState 出错", hr));
				return false;
			} else {
				SPDLOG_LOGGER_INFO(logger, "已创建 ID3D11SamplerState\n\tFilter：D3D11_FILTER_MIN_MAG_MIP_POINT");
			}
		}

		*result = _pointSampler.Get();
	}

	return true;
}

