#include "pch.h"
#include "Renderer.h"
#include "App.h"
#include <DirectXColors.h>

using namespace DirectX;

extern std::shared_ptr<spdlog::logger> logger;

const char vertexShader[] = R"(
	struct VS_OUTPUT {
		float4 Position : SV_POSITION; // vertex position
		float4 TexCoord : TEXCOORD0;   // vertex texture coords
	};

	VS_OUTPUT VS(float4 pos : POSITION, float4 texCoord : TEXCOORD) {
		VS_OUTPUT output = { pos, texCoord };
		return output;
	}
)";


bool Renderer::Initialize() {
	if (!_InitD3D()) {
		return false;
	}

	return true;
}

bool Renderer::InitializeEffects() {
	// 编译顶点着色器
	ComPtr<ID3DBlob> errorMsgs = nullptr;
	ComPtr<ID3DBlob> blob = nullptr;
	HRESULT hr = D3DCompile(vertexShader, sizeof(vertexShader), nullptr, nullptr, nullptr,
		"VS", "vs_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &blob, &errorMsgs);
	if (FAILED(hr)) {
		if (errorMsgs) {
			SPDLOG_LOGGER_CRITICAL(logger, fmt::sprintf(
				"编译顶点着色器失败：%s\n\tHRESULT：0x%X", (const char*)errorMsgs->GetBufferPointer(), hr));
		}
		return false;
	}

	hr = _d3dDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &_vsShader);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_CRITICAL(logger, fmt::sprintf("创建顶点着色器失败\n\tHRESULT：0x%X", hr));
		return false;
	}

	// 创建输入布局
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(XMFLOAT3), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);
	hr = _d3dDevice->CreateInputLayout(layout, numElements, blob->GetBufferPointer(),
		blob->GetBufferSize(), &_inputLayout);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_CRITICAL(logger, fmt::sprintf("创建输入布局失败\n\tHRESULT：0x%X", hr));
		return false;
	}

	Effect& effect = _effects.emplace_back();
	if (!effect.InitializeLanczos()) {
		SPDLOG_LOGGER_CRITICAL(logger, "初始化 Effect 失败");
		return false;
	}

	ComPtr<ID3D11Texture2D> input = App::GetInstance().GetFrameSource().GetOutput();
	D3D11_TEXTURE2D_DESC inputDesc;
	input->GetDesc(&inputDesc);
	SIZE hostSize = App::GetInstance().GetHostWndSize();

	// 等比缩放到最大
	float fillScale = std::min(float(hostSize.cx) / inputDesc.Width, float(hostSize.cy) / inputDesc.Height);
	effect.SetScale(fillScale, fillScale);

	if (!effect.Build(input, _backBuffer)) {
		SPDLOG_LOGGER_CRITICAL(logger, "构建 Effect 失败");
		return false;
	}

	return true;
}

void Renderer::Render() {
	if (!App::GetInstance().GetFrameSource().Update()) {
		return;
	}

	for (Effect& effect : _effects) {
		effect.Draw();
	}

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
		SPDLOG_LOGGER_ERROR(logger, fmt::sprintf("CreateRenderTargetView 失败\n\tHRESULT：0x%X", hr));
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
		SPDLOG_LOGGER_ERROR(logger, fmt::sprintf("CreateShaderResourceView 失败\n\tHRESULT：0x%X", hr));
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
			SPDLOG_LOGGER_CRITICAL(logger, fmt::sprintf("D3D11CreateDevice 失败\n\tHRESULT：0x%X", hr));
			return false;
		}
		SPDLOG_LOGGER_INFO(logger, fmt::format("已创建 D3D Device\n\t功能级别：{}", featureLevel));

		hr = d3dDevice.As<ID3D11Device5>(&_d3dDevice);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_CRITICAL(logger, fmt::sprintf("获取 ID3D11Device5 失败\n\tHRESULT：0x%X", hr));
			return false;
		}

		hr = d3dDC.As<ID3D11DeviceContext4>(&_d3dDC);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_CRITICAL(logger, fmt::sprintf("获取 ID3D11DeviceContext4 失败\n\tHRESULT：0x%X", hr));
			return false;
		}
	}

	{
		ComPtr<IDXGIFactory2> dxgiFactory;

		hr = _d3dDevice.As<IDXGIDevice4>(&_dxgiDevice);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_CRITICAL(logger, fmt::sprintf("获取 IDXGIDevice 失败\n\tHRESULT：0x%X", hr));
			return false;
		}

		ComPtr<IDXGIAdapter> dxgiAdapter;
		hr = _dxgiDevice->GetAdapter(&dxgiAdapter);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_CRITICAL(logger, fmt::sprintf("获取 IDXGIAdapter 失败\n\tHRESULT：0x%X", hr));
			return false;
		}

		hr = dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));
		if (FAILED(hr)) {
			SPDLOG_LOGGER_CRITICAL(logger, fmt::sprintf("获取 IDXGIFactory2 失败\n\tHRESULT：0x%X", hr));
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

		hr = dxgiFactory->CreateSwapChainForHwnd(
			_d3dDevice.Get(),
			App::GetInstance().GetHwndHost(),
			&sd,
			nullptr,
			nullptr,
			&_dxgiSwapChain
		);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_CRITICAL(logger, fmt::sprintf("创建交换链失败\n\tHRESULT：{0x%X", hr));
			return false;
		}

		hr = dxgiFactory->MakeWindowAssociation(App::GetInstance().GetHwndHost(), DXGI_MWA_NO_ALT_ENTER);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, fmt::sprintf("MakeWindowAssociation 失败\n\tHRESULT：0x%X", hr));
		}
	}

	hr = _dxgiSwapChain->GetBuffer(0, IID_PPV_ARGS(&_backBuffer));
	if (FAILED(hr)) {
		SPDLOG_LOGGER_CRITICAL(logger, fmt::sprintf("获取后缓冲区失败\n\tHRESULT：0x%X", hr));
		return false;
	}

	// 所有效果都使用三角形带拓扑
	_d3dDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	SPDLOG_LOGGER_INFO(logger, "Renderer 初始化完成");
	return true;
}

ComPtr<ID3D11SamplerState> Renderer::GetSampler(FilterType filterType) {
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
				SPDLOG_LOGGER_ERROR(logger, fmt::sprintf("创建 ID3D11SamplerState 出错\n\tHRESULT：0x%X", hr));
				return nullptr;
			} else {
				SPDLOG_LOGGER_INFO(logger, "已创建 ID3D11SamplerState\n\tFilter：D3D11_FILTER_MIN_MAG_MIP_LINEAR");
			}
		}

		return _linearSampler;
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
				SPDLOG_LOGGER_ERROR(logger, fmt::sprintf("创建 ID3D11SamplerState 出错\n\tHRESULT：0x%X", hr));
				return nullptr;
			} else {
				SPDLOG_LOGGER_INFO(logger, "已创建 ID3D11SamplerState\n\tFilter：D3D11_FILTER_MIN_MAG_MIP_POINT");
			}
		}

		return _pointSampler;
	}
}
