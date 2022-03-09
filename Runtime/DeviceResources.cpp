#include "pch.h"
#include "DeviceResources.h"
#include "App.h"
#include "StrUtils.h"
#include "Logger.h"


static inline void LogAdapter(const DXGI_ADAPTER_DESC1& adapterDesc) {
	Logger::Get().Info(fmt::format("当前图形适配器：\n\tVendorId：{:#x}\n\tDeviceId：{:#x}\n\t描述：{}",
		adapterDesc.VendorId, adapterDesc.DeviceId, StrUtils::UTF16ToUTF8(adapterDesc.Description)));
}

static winrt::com_ptr<IDXGIAdapter3> ObtainGraphicsAdapter(IDXGIFactory4* dxgiFactory, int adapterIdx) {
	winrt::com_ptr<IDXGIAdapter1> adapter;

	if (adapterIdx >= 0) {
		HRESULT hr = dxgiFactory->EnumAdapters1(adapterIdx, adapter.put());
		if (SUCCEEDED(hr)) {
			DXGI_ADAPTER_DESC1 desc;
			HRESULT hr = adapter->GetDesc1(&desc);
			if (FAILED(hr)) {
				return nullptr;
			}

			LogAdapter(desc);
			return adapter.try_as<IDXGIAdapter3>();
		}
	}

	// 枚举查找第一个支持 D3D11 的图形适配器
	for (UINT adapterIndex = 0;
		SUCCEEDED(dxgiFactory->EnumAdapters1(adapterIndex, adapter.put()));
		++adapterIndex
	) {
		DXGI_ADAPTER_DESC1 desc;
		HRESULT hr = adapter->GetDesc1(&desc);
		if (FAILED(hr)) {
			continue;
		}

		if (desc.Flags == DXGI_ADAPTER_FLAG_SOFTWARE) {
			continue;
		}

		D3D_FEATURE_LEVEL featureLevels[] = {
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0
		};
		UINT nFeatureLevels = ARRAYSIZE(featureLevels);

		hr = D3D11CreateDevice(
			adapter.get(),
			D3D_DRIVER_TYPE_UNKNOWN,
			nullptr,
			0,
			featureLevels,
			nFeatureLevels,
			D3D11_SDK_VERSION,
			nullptr,
			nullptr,
			nullptr
		);
		if (SUCCEEDED(hr)) {
			LogAdapter(desc);
			return adapter.try_as<IDXGIAdapter3>();
		}
	}

	// 回落到 Basic Render Driver Adapter（WARP）
	// https://docs.microsoft.com/en-us/windows/win32/direct3darticles/directx-warp
	HRESULT hr = dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&adapter));
	if (FAILED(hr)) {
		Logger::Get().ComError("创建 WARP 设备失败", hr);
		return nullptr;
	}

	return adapter.try_as<IDXGIAdapter3>();
}

bool DeviceResources::Initialize() {
#ifdef _DEBUG
	UINT flag = DXGI_CREATE_FACTORY_DEBUG;
#else
	UINT flag = 0;
#endif // _DEBUG

	HRESULT hr = CreateDXGIFactory2(flag, IID_PPV_ARGS(_dxgiFactory.put()));
	if (FAILED(hr)) {
		return false;
	}

	// 检查可变帧率支持
	BOOL supportTearing = FALSE;

	hr = _dxgiFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &supportTearing, sizeof(supportTearing));
	if (FAILED(hr)) {
		Logger::Get().ComWarn("CheckFeatureSupport 失败", hr);
	}
	_supportTearing = !!supportTearing;

	Logger::Get().Info(fmt::format("可变刷新率支持：{}", supportTearing ? "是" : "否"));

	if (App::Get().IsDisableVSync() && !supportTearing) {
		Logger::Get().Error("当前显示器不支持可变刷新率");
		App::Get().SetErrorMsg(ErrorMessages::VSYNC_OFF_NOT_SUPPORTED);
		return false;
	}

	UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
	if (IsDebugLayersAvailable()) {
		// 在 DEBUG 配置启用调试层
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	}

	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};
	UINT nFeatureLevels = ARRAYSIZE(featureLevels);

	_graphicsAdapter = ObtainGraphicsAdapter(_dxgiFactory.get(), App::Get().GetAdapterIdx());
	if (!_graphicsAdapter) {
		Logger::Get().Error("找不到可用 Adapter");
		return false;
	}

	winrt::com_ptr<ID3D11Device> d3dDevice;
	winrt::com_ptr<ID3D11DeviceContext> d3dDC;
	hr = D3D11CreateDevice(
		_graphicsAdapter.get(),
		D3D_DRIVER_TYPE_UNKNOWN,
		nullptr,
		createDeviceFlags,
		featureLevels,
		nFeatureLevels,
		D3D11_SDK_VERSION,
		d3dDevice.put(),
		&_featureLevel,
		d3dDC.put()
	);

	if (FAILED(hr)) {
		Logger::Get().ComError("D3D11CreateDevice 失败", hr);
		return false;
	}

	std::string_view fl;
	switch (_featureLevel) {
	case D3D_FEATURE_LEVEL_11_1:
		fl = "11.1";
		break;
	case D3D_FEATURE_LEVEL_11_0:
		fl = "11.0";
		break;
	default:
		fl = "未知";
		break;
	}
	Logger::Get().Info(fmt::format("已创建 D3D Device\n\t功能级别：{}", fl));

	_d3dDevice = d3dDevice.try_as<ID3D11Device3>();
	if (!_d3dDevice) {
		Logger::Get().Error("获取 ID3D11Device1 失败");
		return false;
	}

	_d3dDC = d3dDC.try_as<ID3D11DeviceContext3>();
	if (!_d3dDC) {
		Logger::Get().Error("获取 ID3D11DeviceContext1 失败");
		return false;
	}

	_dxgiDevice = _d3dDevice.try_as<IDXGIDevice4>();
	if (!_dxgiDevice) {
		Logger::Get().Error("获取 IDXGIDevice 失败");
		return false;
	}

	if (!_CreateSwapChain()) {
		Logger::Get().Error("_CreateSwapChain 失败");
		return false;
	}

	return true;
}

bool DeviceResources::IsDebugLayersAvailable() {
#ifdef _DEBUG
	static std::optional<bool> result = std::nullopt;

	if (!result.has_value()) {
		HRESULT hr = D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_NULL,       // There is no need to create a real hardware device.
			nullptr,
			D3D11_CREATE_DEVICE_DEBUG,  // Check for the SDK layers.
			nullptr,                    // Any feature level will do.
			0,
			D3D11_SDK_VERSION,
			nullptr,                    // No need to keep the D3D device reference.
			nullptr,                    // No need to know the feature level.
			nullptr                     // No need to keep the D3D device context reference.
		);

		result = SUCCEEDED(hr);
	}

	return result.value_or(false);
#else
	// Relaese 配置不使用调试层
	return false;
#endif
}

winrt::com_ptr<ID3D11Texture2D> DeviceResources::CreateTexture2D(
	DXGI_FORMAT format,
	UINT width,
	UINT height,
	UINT bindFlags,
	D3D11_USAGE usage,
	UINT miscFlags,
	const D3D11_SUBRESOURCE_DATA* pInitialData
) {
	D3D11_TEXTURE2D_DESC desc{};
	desc.Format = format;
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.BindFlags = bindFlags;
	desc.Usage = usage;
	desc.MiscFlags = miscFlags;

	winrt::com_ptr<ID3D11Texture2D> result;
	HRESULT hr = _d3dDevice->CreateTexture2D(&desc, pInitialData, result.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateTexture2D 失败", hr);
		return nullptr;
	}

	return result;
}

void DeviceResources::BeginFrame() {
	WaitForSingleObjectEx(_frameLatencyWaitableObject.get(), 1000, TRUE);
	_d3dDC->ClearState();
}

void DeviceResources::EndFrame() {
	if (App::Get().IsDisableVSync()) {
		_swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
	} else {
		_swapChain->Present(1, 0);
	}
}

bool DeviceResources::_CreateSwapChain() {
	const RECT& hostWndRect = App::Get().GetHostWndRect();

	DXGI_SWAP_CHAIN_DESC1 sd = {};
	sd.Width = hostWndRect.right - hostWndRect.left;
	sd.Height = hostWndRect.bottom - hostWndRect.top;
	sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Scaling = DXGI_SCALING_NONE;
	sd.BufferUsage = DXGI_USAGE_UNORDERED_ACCESS;
	sd.BufferCount = (App::Get().IsDisableLowLatency() || App::Get().IsDisableVSync()) ? 3 : 2;
	// 使用 DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL 而不是 DXGI_SWAP_EFFECT_FLIP_DISCARD
	// 不渲染四周（可能存在的）黑边，因此必须保证交换链缓冲区不被改变
	// 否则将不得不在每帧渲染前清空后缓冲区，这个操作在一些显卡上比较耗时
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	// 只要显卡支持始终启用 DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING
	sd.Flags = (_supportTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0)
		| DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

	winrt::com_ptr<IDXGISwapChain1> dxgiSwapChain = nullptr;
	HRESULT hr = _dxgiFactory->CreateSwapChainForHwnd(
		_d3dDevice.get(),
		App::Get().GetHwndHost(),
		&sd,
		nullptr,
		nullptr,
		dxgiSwapChain.put()
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("创建交换链失败", hr);
		return false;
	}

	_swapChain = dxgiSwapChain.try_as<IDXGISwapChain4>();
	if (!_swapChain) {
		Logger::Get().Error("获取 IDXGISwapChain2 失败");
		return false;
	}

	// 关闭低延迟模式或关闭垂直同步时将最大延迟设为 2 以使 CPU 和 GPU 并行执行
	_swapChain->SetMaximumFrameLatency(
		App::Get().IsDisableLowLatency() || App::Get().IsDisableVSync() ? 2 : 1);

	_frameLatencyWaitableObject.reset(_swapChain->GetFrameLatencyWaitableObject());
	if (!_frameLatencyWaitableObject) {
		Logger::Get().Error("GetFrameLatencyWaitableObject 失败");
		return false;
	}

	hr = _dxgiFactory->MakeWindowAssociation(App::Get().GetHwndHost(), DXGI_MWA_NO_ALT_ENTER);
	if (FAILED(hr)) {
		Logger::Get().ComError("MakeWindowAssociation 失败", hr);
	}

	// 检查 Multiplane Overlay 和 Hardware Composition 支持
	BOOL supportMPO = FALSE;
	BOOL supportHardwareComposition = FALSE;
	winrt::com_ptr<IDXGIOutput> output;
	hr = _swapChain->GetContainingOutput(output.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("获取 IDXGIOutput 失败", hr);
	} else {
		winrt::com_ptr<IDXGIOutput2> output2 = output.try_as<IDXGIOutput2>();
		if (!output2) {
			Logger::Get().Info("获取 IDXGIOutput2 失败");
		} else {
			supportMPO = output2->SupportsOverlays();
		}

		winrt::com_ptr<IDXGIOutput6> output6 = output.try_as<IDXGIOutput6>();
		if (!output6) {
			Logger::Get().Info("获取 IDXGIOutput6 失败");
		} else {
			UINT flags;
			hr = output6->CheckHardwareCompositionSupport(&flags);
			if (FAILED(hr)) {
				Logger::Get().ComError("CheckHardwareCompositionSupport 失败", hr);
			} else {
				supportHardwareComposition = flags & DXGI_HARDWARE_COMPOSITION_SUPPORT_FLAG_WINDOWED;
			}
		}
	}

	Logger::Get().Info(StrUtils::Concat("Hardware Composition 支持：", supportHardwareComposition ? "是" : "否"));
	Logger::Get().Info(StrUtils::Concat("Multiplane Overlay 支持：", supportMPO ? "是" : "否"));

	hr = _swapChain->GetBuffer(0, IID_PPV_ARGS(_backBuffer.put()));
	if (FAILED(hr)) {
		Logger::Get().ComError("获取后缓冲区失败", hr);
		return false;
	}

	return true;
}

bool DeviceResources::GetShaderResourceView(ID3D11Texture2D* texture, ID3D11ShaderResourceView** result) {
	auto it = _srvMap.find(texture);
	if (it != _srvMap.end()) {
		*result = it->second.get();
		return true;
	}

	winrt::com_ptr<ID3D11ShaderResourceView>& r = _srvMap[texture];
	HRESULT hr = _d3dDevice->CreateShaderResourceView(texture, nullptr, r.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateShaderResourceView 失败", hr);
		return false;
	} else {
		*result = r.get();
		return true;
	}
}

bool DeviceResources::GetUnorderedAccessView(ID3D11Texture2D* texture, ID3D11UnorderedAccessView** result) {
	auto it = _uavMap.find(texture);
	if (it != _uavMap.end()) {
		*result = it->second.get();
		return true;
	}

	winrt::com_ptr<ID3D11UnorderedAccessView>& r = _uavMap[texture];

	D3D11_UNORDERED_ACCESS_VIEW_DESC desc{};
	desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MipSlice = 0;

	HRESULT hr = _d3dDevice->CreateUnorderedAccessView(texture, &desc, r.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateUnorderedAccessView 失败", hr);
		return false;
	} else {
		*result = r.get();
		return true;
	}
}

bool DeviceResources::CompileShader(std::string_view hlsl, const char* entryPoint, ID3DBlob** blob, const char* sourceName, ID3DInclude* include) {
	winrt::com_ptr<ID3DBlob> errorMsgs = nullptr;

	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_ALL_RESOURCES_BOUND;
	if (App::Get().IsTreatWarningsAsErrors()) {
		flags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;
	}

#ifdef _DEBUG
	flags |= D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_DEBUG;
#else
	flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif // _DEBUG


	HRESULT hr = D3DCompile(hlsl.data(), hlsl.size(), sourceName, nullptr, include,
		entryPoint, "cs_5_0", flags, 0, blob, errorMsgs.put());
	if (FAILED(hr)) {
		if (errorMsgs) {
			Logger::Get().ComError(StrUtils::Concat("编译计算着色器失败：", (const char*)errorMsgs->GetBufferPointer()), hr);
		}
		return false;
	} else {
		// 警告消息
		if (errorMsgs) {
			Logger::Get().Warn(StrUtils::Concat("编译计算着色器时产生警告：", (const char*)errorMsgs->GetBufferPointer()));
		}
	}

	return true;
}

bool DeviceResources::GetRenderTargetView(ID3D11Texture2D* texture, ID3D11RenderTargetView** result) {
	auto it = _rtvMap.find(texture);
	if (it != _rtvMap.end()) {
		*result = it->second.get();
		return true;
	}

	winrt::com_ptr<ID3D11RenderTargetView>& r = _rtvMap[texture];
	HRESULT hr = _d3dDevice->CreateRenderTargetView(texture, nullptr, r.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateRenderTargetView 失败", hr);
		return false;
	} else {
		*result = r.get();
		return true;
	}
}

bool DeviceResources::GetSampler(D3D11_FILTER filterMode, D3D11_TEXTURE_ADDRESS_MODE addressMode, ID3D11SamplerState** result) {
	auto key = std::make_pair(filterMode, addressMode);
	auto it = _samMap.find(key);
	if (it != _samMap.end()) {
		*result = it->second.get();
		return true;
	}

	winrt::com_ptr<ID3D11SamplerState> sam;

	D3D11_SAMPLER_DESC desc{};
	desc.Filter = filterMode;
	desc.AddressU = addressMode;
	desc.AddressV = addressMode;
	desc.AddressW = addressMode;
	desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	desc.MinLOD = 0;
	desc.MaxLOD = 0;
	HRESULT hr = _d3dDevice->CreateSamplerState(&desc, sam.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("创建 ID3D11SamplerState 出错", hr);
		return false;
	}

	*result = sam.get();
	_samMap.emplace(key, std::move(sam));
	return true;
}
