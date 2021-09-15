#include "pch.h"
#include "Renderer.h"
#include "App.h"
#include "Utils.h"

extern std::shared_ptr<spdlog::logger> logger;

const char vertexShader[] = R"(

struct VS_OUTPUT {
	float4 Position : SV_POSITION;
	float4 TexCoord : TEXCOORD0;
};
	
VS_OUTPUT main(uint id : SV_VERTEXID) {
	VS_OUTPUT output;

	float2 texCoord = float2(id & 1, id >> 1) * 2.0;
	output.TexCoord = float4(texCoord, 0, 0);
	output.Position = float4(texCoord * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);

	return output;
}

)";


bool Renderer::Initialize() {
	if (!_InitD3D()) {
		return false;
	}

	return true;
}

bool Renderer::InitializeEffectsAndCursor() {
	// 编译顶点着色器
	ComPtr<ID3DBlob> errorMsgs = nullptr;
	ComPtr<ID3DBlob> blob = nullptr;
	HRESULT hr = D3DCompile(vertexShader, sizeof(vertexShader), nullptr, nullptr, nullptr,
		"main", "vs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &blob, &errorMsgs);
	if (FAILED(hr)) {
		if (errorMsgs) {
			SPDLOG_LOGGER_CRITICAL(logger, MakeComErrorMsg(
				fmt::format("编译顶点着色器失败：{}", (const char*)errorMsgs->GetBufferPointer()), hr));
		}
		return false;
	}

	hr = _d3dDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &_vertexShader);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_CRITICAL(logger, MakeComErrorMsg("创建顶点着色器失败", hr));
		return false;
	}

	Effect& effect = _effects.emplace_back();
	if (!effect.InitializeFsr()) {
		SPDLOG_LOGGER_CRITICAL(logger, "初始化 Effect 失败");
		return false;
	}

	_effectInput = App::GetInstance().GetFrameSource().GetOutput();
	D3D11_TEXTURE2D_DESC inputDesc;
	_effectInput->GetDesc(&inputDesc);
	SIZE hostSize = App::GetInstance().GetHostWndSize();

	// 等比缩放到最大
	float fillScale = std::min(float(hostSize.cx) / inputDesc.Width, float(hostSize.cy) / inputDesc.Height);
	SIZE outputSize = { lroundf(inputDesc.Width * fillScale), lroundf(inputDesc.Height * fillScale) };
	effect.SetOutputSize(outputSize);

	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = outputSize.cx;
	desc.Height = outputSize.cy;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	hr = _d3dDevice->CreateTexture2D(&desc, nullptr, &_effectOutput);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_CRITICAL(logger, MakeComErrorMsg("创建 Texture2D 失败", hr));
		return false;
	}

	if (!effect.Build(_effectInput, _effectOutput)) {
		SPDLOG_LOGGER_CRITICAL(logger, "构建 Effect 失败");
		return false;
	}

	if (!_cursorRenderer.Initialize(_effectOutput, _backBuffer)) {
		SPDLOG_LOGGER_CRITICAL(logger, "构建 CursorRenderer 失败");
		return false;
	}

	return true;
}


void Renderer::Render() {
	if (!_waitingForNextFrame) {
		WaitForSingleObjectEx(_frameLatencyWaitableObject, 1000, true);
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
	_d3dDC->VSSetShader(_vertexShader.Get(), nullptr, 0);

	for (Effect& effect : _effects) {
		effect.Draw();
	}

	_cursorRenderer.Draw();

	_dxgiSwapChain->Present(0, 0);
}

HRESULT Renderer::GetRenderTargetView(ID3D11Texture2D* texture, ID3D11RenderTargetView** result) {
	auto it = _rtvMap.find(texture);
	if (it != _rtvMap.end()) {
		*result = it->second.Get();
		return S_OK;
	}

	ComPtr<ID3D11RenderTargetView>& r = _rtvMap[texture];
	HRESULT hr = _d3dDevice->CreateRenderTargetView(texture, nullptr, &r);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("CreateRenderTargetView 失败", hr));
		return hr;
	} else {
		*result = r.Get();
		return S_OK;
	}
}

HRESULT Renderer::GetShaderResourceView(ID3D11Texture2D* texture, ID3D11ShaderResourceView** result) {
	auto it = _srvMap.find(texture);
	if (it != _srvMap.end()) {
		*result = it->second.Get();
		return S_OK;
	}

	ComPtr<ID3D11ShaderResourceView>& r = _srvMap[texture];
	HRESULT hr = _d3dDevice->CreateShaderResourceView(texture, nullptr, &r);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("CreateShaderResourceView 失败", hr));
		return hr;
	} else {
		*result = r.Get();
		return S_OK;
	}
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
			D3D_FEATURE_LEVEL_11_1
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

		hr = d3dDevice.As<ID3D11Device3>(&_d3dDevice);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_CRITICAL(logger, MakeComErrorMsg("获取 ID3D11Device5 失败", hr));
			return false;
		}

		hr = d3dDC.As<ID3D11DeviceContext3>(&_d3dDC);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_CRITICAL(logger, MakeComErrorMsg("获取 ID3D11DeviceContext4 失败", hr));
			return false;
		}
	}

	{
		ComPtr<IDXGIFactory2> dxgiFactory;

		hr = _d3dDevice.As<IDXGIDevice4>(&_dxgiDevice);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_CRITICAL(logger, MakeComErrorMsg("获取 IDXGIDevice 失败", hr));
			return false;
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

		DXGI_SWAP_CHAIN_DESC1 sd = {};
		sd.Width = hostSize.cx;
		sd.Height = hostSize.cy;
		sd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = 2;
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		sd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

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

		hr = dxgiSwapChain.As<IDXGISwapChain4>(&_dxgiSwapChain);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_CRITICAL(logger, MakeComErrorMsg("获取 IDXGISwapChain4 失败", hr));
			return false;
		}

		hr = _dxgiSwapChain->SetMaximumFrameLatency(1);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_CRITICAL(logger, MakeComErrorMsg("获取 IDXGISwapChain4 失败", hr));
			return false;
		}

		_frameLatencyWaitableObject = _dxgiSwapChain->GetFrameLatencyWaitableObject();
		
		hr = dxgiFactory->MakeWindowAssociation(App::GetInstance().GetHwndHost(), DXGI_MWA_NO_ALT_ENTER);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("MakeWindowAssociation 失败", hr));
		}
	}

	hr = _dxgiSwapChain->GetBuffer(0, IID_PPV_ARGS(&_backBuffer));
	if (FAILED(hr)) {
		SPDLOG_LOGGER_CRITICAL(logger, MakeComErrorMsg("获取后缓冲区失败", hr));
		return false;
	}

	SPDLOG_LOGGER_INFO(logger, "Renderer 初始化完成");
	return true;
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

bool Renderer::GetSampler(FilterType filterType, ID3D11SamplerState** result) {
	if (filterType == FilterType::LINEAR) {
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
