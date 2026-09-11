#include "pch.h"
#include "D3D12Context.h"
#include "ScalingWindow.h"
#include "Logger.h"
#include "AppFolderManager.h"
#include "DirectXHelper.h"
#include "StrHelper.h"
#include "DescriptorHeap.h"
#include "Win32Helper.h"

namespace Magpie {

bool D3D12Context::Initialize(
	const GraphicsCardId& graphicsCardId,
	uint32_t maxInFlightFrameCount,
	D3D12_COMMAND_QUEUE_PRIORITY priority,
	D3D12_COMMAND_LIST_TYPE commandListType,
	DescriptorHeap& csuDescriptorHeap,
	DescriptorHeap& rtvDescriptorHeap,
	bool disableFrameFenceTracking
) noexcept {
	_csuDescriptorHeap = &csuDescriptorHeap;
	_rtvDescriptorHeap = &rtvDescriptorHeap;

	HRESULT hr = _CreateDXGIFactory();
	if (FAILED(hr)) {
		Logger::Get().ComError("_CreateDXGIFactory 失败", hr);
		return false;
	}

	if (!_CreateAdapterAndDevice(graphicsCardId)) {
		Logger::Get().Error("_CreateAdapterAndDevice 失败");
		return false;
	}

#ifdef _DEBUG
	// 调试层汇报错误或警告时中断
	if (winrt::com_ptr<ID3D12InfoQueue> infoQueue = _device.try_as<ID3D12InfoQueue>()) {
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
	}
#endif

	_QueryHighestShaderModel();

	// 检查根签名版本
	{
		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = { .HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1 };
		hr = _device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData));
		if (SUCCEEDED(hr)) {
			_rootSignatureVersion = featureData.HighestVersion;
		} else {
			Logger::Get().ComWarn("CheckFeatureSupport 失败", hr);
		}
	}

	// 检查是否是集成显卡
	{
		D3D12_FEATURE_DATA_ARCHITECTURE1 data{};
		hr = _device->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE1, &data, sizeof(data));
		if (SUCCEEDED(hr)) {
			_isUMA = data.UMA;
		} else {
			Logger::Get().ComWarn("CheckFeatureSupport 失败", hr);
		}
	}

	// 检查 D3D12_HEAP_FLAG_CREATE_NOT_ZEROED 支持。是否支持这个功能只和 D3D12 版本有关，
	// 虽然我们随程序部署了 Agility SDK，但旧版 Win10 不支持加载。
	// https://devblogs.microsoft.com/directx/coming-to-directx-12-more-control-over-memory-allocation/
	_isHeapFlagCreateNotZeroedSupported = (bool)_device.try_as<ID3D12Device8>();

	// 检查 Resizable BAR 支持
	{
		D3D12_FEATURE_DATA_D3D12_OPTIONS16 data{};
		hr = _device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS16, &data, sizeof(data));
		if (SUCCEEDED(hr)) {
			_isGPUUploadHeapSupported = data.GPUUploadHeapSupported;
		} else {
			Logger::Get().ComWarn("CheckFeatureSupport 失败", hr);
		}
	}

	// 检查 FP16 支持
	if (!ScalingWindow::Get().Options().IsFP16Disabled()) {
		{
			D3D12_FEATURE_DATA_D3D12_OPTIONS data{};
			hr = _device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &data, sizeof(data));
			if (SUCCEEDED(hr)) {
				_isMinFloat16Supported = data.MinPrecisionSupport & D3D12_SHADER_MIN_PRECISION_SUPPORT_16_BIT;
			} else {
				Logger::Get().ComWarn("CheckFeatureSupport 失败", hr);
			}
		}
		
		// SM 6.2 开始支持原生 16 位标量
		if (_shaderModel >= D3D_SHADER_MODEL_6_2) {
			D3D12_FEATURE_DATA_D3D12_OPTIONS4 data{};
			hr = _device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS4, &data, sizeof(data));
			if (SUCCEEDED(hr)) {
				_isNative16BitSupported = data.Native16BitShaderOpsSupported;
			} else {
				Logger::Get().ComWarn("CheckFeatureSupport 失败", hr);
			}
		}
	}
	
	_LogDeviceInfo();

	if (!_InitializeDeviceResources(
		maxInFlightFrameCount, priority, commandListType, disableFrameFenceTracking)) {
		Logger::Get().Error("_InitializeDeviceResources 失败");
		return false;
	}

	return true;
}

void D3D12Context::CopyDevice(const D3D12Context& other) {
	_csuDescriptorHeap = other._csuDescriptorHeap;
	_rtvDescriptorHeap = other._rtvDescriptorHeap;
	_device = other._device;
	_shaderModel = other._shaderModel;
	_rootSignatureVersion = other._rootSignatureVersion;
	_isUMA = other._isUMA;
	_isHeapFlagCreateNotZeroedSupported = other._isHeapFlagCreateNotZeroedSupported;
	_isGPUUploadHeapSupported = other._isGPUUploadHeapSupported;
	_isMinFloat16Supported = other._isMinFloat16Supported;
	_isNative16BitSupported = other._isNative16BitSupported;
}

bool D3D12Context::InitializeAfterCopyDevice(
	uint32_t maxInFlightFrameCount,
	D3D12_COMMAND_QUEUE_PRIORITY priority,
	D3D12_COMMAND_LIST_TYPE commandListType,
	bool disableFrameFenceTracking
) noexcept {
	HRESULT hr = _CreateDXGIFactory();
	if (FAILED(hr)) {
		Logger::Get().ComError("_CreateDXGIFactory 失败", hr);
		return false;
	}

	if (!_CreateAdapterFromDevice()) {
		Logger::Get().ComError("_CreateDXGIFactory 失败", hr);
		return false;
	}
	
	if (!_InitializeDeviceResources(maxInFlightFrameCount, priority, commandListType, disableFrameFenceTracking)) {
		Logger::Get().Error("_InitializeDeviceResources 失败");
		return false;
	}

	return true;
}

IDXGIFactory7* D3D12Context::GetDXGIFactoryForEnumingAdapters() noexcept {
	if (!_dxgiFactory->IsCurrent()) {
		HRESULT hr = _CreateDXGIFactory();
		if (FAILED(hr)) {
			Logger::Get().ComError("_CreateDXGIFactory 失败", hr);
			return nullptr;
		}
	}

	return _dxgiFactory.get();
}

HRESULT D3D12Context::Signal(uint64_t& fenceValue) noexcept {
	fenceValue = ++_curFenceValue;
	return _commandQueue->Signal(_fence.get(), _curFenceValue);
}

HRESULT D3D12Context::WaitForFenceValue(uint64_t fenceValue) noexcept {
	if (_fence->GetCompletedValue() >= fenceValue) {
		return S_OK;
	} else {
		return _fence->SetEventOnCompletion(fenceValue, nullptr);
	}
}

HRESULT D3D12Context::WaitForGpu() noexcept {
	HRESULT hr = _commandQueue->Signal(_fence.get(), ++_curFenceValue);
	if (FAILED(hr)) {
		Logger::Get().ComError("ID3D12CommandQueue::Signal 失败", hr);
		return hr;
	}

	return WaitForFenceValue(_curFenceValue);
}

HRESULT D3D12Context::WaitForCommandQueue(ID3D12CommandQueue* commandQueue) noexcept {
	HRESULT hr = commandQueue->Signal(_fence.get(), ++_curFenceValue);
	if (FAILED(hr)) {
		Logger::Get().ComError("ID3D12CommandQueue::Signal 失败", hr);
		return hr;
	}

	hr = _commandQueue->Wait(_fence.get(), _curFenceValue);
	if (FAILED(hr)) {
		Logger::Get().ComError("ID3D12CommandQueue::Wait 失败", hr);
		return hr;
	}

	return S_OK;
}

HRESULT D3D12Context::BeginFrame(uint32_t& curFrameIndex, ID3D12PipelineState* initialState) noexcept {
	if (!_frameFenceValues.empty()) {
		HRESULT hr = WaitForFenceValue(_frameFenceValues[_curFrameIndex]);
		if (FAILED(hr)) {
			Logger::Get().ComError("WaitForFenceValue 失败", hr);
			return hr;
		}
	}

	HRESULT hr = _commandAllocators[_curFrameIndex]->Reset();
	if (FAILED(hr)) {
		Logger::Get().ComError("ID3D12CommandAllocator::Reset 失败", hr);
		return hr;
	}

	hr = _commandList->Reset(_commandAllocators[_curFrameIndex].get(), initialState);
	if (FAILED(hr)) {
		Logger::Get().ComError("ID3D12GraphicsCommandList::Reset 失败", hr);
		return hr;
	}

	curFrameIndex = _curFrameIndex;
	return S_OK;
}

HRESULT D3D12Context::EndFrame() noexcept {
	if (!_frameFenceValues.empty()) {
		HRESULT hr = Signal(_frameFenceValues[_curFrameIndex]);
		if (FAILED(hr)) {
			Logger::Get().ComError("Signal 失败", hr);
			return hr;
		}
	}

	_curFrameIndex = (_curFrameIndex + 1) % (uint32_t)_commandAllocators.size();
	return S_OK;
}

HRESULT D3D12Context::_CreateDXGIFactory() noexcept {
	UINT flags = 0;
#ifdef _DEBUG
	flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
	HRESULT hr = CreateDXGIFactory2(flags, IID_PPV_ARGS(&_dxgiFactory));
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateDXGIFactory2 失败", hr);
	}
	return hr;
}

bool D3D12Context::_InitializeDeviceResources(
	uint32_t maxInFlightFrameCount,
	D3D12_COMMAND_QUEUE_PRIORITY priority,
	D3D12_COMMAND_LIST_TYPE commandListType,
	bool disableFrameFenceTracking
) noexcept {
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = {
			.Type = commandListType,
			.Priority = priority,
			.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE
		};
		HRESULT hr = _device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_commandQueue));
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateCommandQueue 失败", hr);
			return false;
		}
	}

	HRESULT hr = _device->CreateCommandList1(0, commandListType,
		D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&_commandList));
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateCommandList1 失败", hr);
		return false;
	}

	_commandAllocators.resize(maxInFlightFrameCount);
	for (winrt::com_ptr<ID3D12CommandAllocator>& commandAllocator : _commandAllocators) {
		hr = _device->CreateCommandAllocator(commandListType, IID_PPV_ARGS(&commandAllocator));
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateCommandAllocator 失败", hr);
			return false;
		}
	}

	hr = _device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateFence 失败", hr);
		return false;
	}

	// 如果已在外部同步则无需追踪每帧的栅栏值
	if (!disableFrameFenceTracking) {
		_frameFenceValues.resize(maxInFlightFrameCount);
	}

	return true;
}

// 和 D3D12SDKLayers.dll 不同，OS 加载 d3d10warp.dll 时不遵循 D3D12SDKPath。
// 这个函数确保加载匹配的 d3d10warp.dll。
static void FixD3D10WarpDll(IDXGIAdapter1* warpAdapter) noexcept {
	assert(!GetModuleHandle(L"d3d10warp.dll"));

	HMODULE hD3D12Core = GetModuleHandle(L"D3D12Core.dll");
	if (!hD3D12Core) {
		// 如果 D3D12Core.dll 尚未加载则加载它
		D3D12CreateDevice(warpAdapter, D3D_FEATURE_LEVEL_11_0, winrt::guid_of<ID3D12Device>(), nullptr);

		hD3D12Core = GetModuleHandle(L"D3D12Core.dll");
		if (!hD3D12Core) {
			// 可能 OS 不支持 Agility SDK
			return;
		}
	}

	// 检查是否加载了随程序部署的 D3D12Core.dll
	std::wstring d3d12CorePath;
	wil::GetModuleFileNameW(hD3D12Core, d3d12CorePath);
	if (d3d12CorePath.starts_with(AppFolderManager::Get().GetExeDir().native())) {
		// 加载随程序部署的 d3d10warp.dll
		std::filesystem::path warpDllPath =
			AppFolderManager::Get().GetD3D12Dir() / L"d3d10warp.dll";
		LoadLibrary(warpDllPath.c_str());
	}
}

bool D3D12Context::_CreateAdapterAndDevice(const GraphicsCardId& graphicsCardId) noexcept {
	winrt::com_ptr<IDXGIAdapter1> adapter;

	if (!ScalingWindow::Get().Options().UseWarp()) {
		// 记录不支持 D3D12 的显卡索引，防止重复尝试
		int failedIdx = -1;

		if (graphicsCardId.idx >= 0) {
			assert(graphicsCardId.vendorId != 0 && graphicsCardId.deviceId != 0);

			// 先使用索引
			HRESULT hr = _dxgiFactory->EnumAdapters1(graphicsCardId.idx, adapter.put());
			if (SUCCEEDED(hr)) {
				DXGI_ADAPTER_DESC1 desc;
				hr = adapter->GetDesc1(&desc);
				if (SUCCEEDED(hr)) {
					if (desc.VendorId == graphicsCardId.vendorId && desc.DeviceId == graphicsCardId.deviceId) {
						if (_TryCreateD3DDevice(adapter, desc)) {
							return true;
						}

						failedIdx = graphicsCardId.idx;
						Logger::Get().Warn("用户指定的显示卡不支持 D3D12");
					} else {
						Logger::Get().Warn("显卡配置已变化");
					}
				}
			}

			// 如果已确认该显卡不支持 D3D12，不再重复尝试
			if (failedIdx == -1) {
				// 枚举查找 vendorId 和 deviceId 匹配的显卡
				for (UINT adapterIdx = 0;
					SUCCEEDED(_dxgiFactory->EnumAdapters1(adapterIdx, adapter.put()));
					++adapterIdx
				) {
					if ((int)adapterIdx == graphicsCardId.idx) {
						// 已经检查了 graphicsCardId.idx
						continue;
					}

					DXGI_ADAPTER_DESC1 desc;
					hr = adapter->GetDesc1(&desc);
					if (FAILED(hr)) {
						continue;
					}

					if (desc.VendorId == graphicsCardId.vendorId && desc.DeviceId == graphicsCardId.deviceId) {
						if (_TryCreateD3DDevice(adapter, desc)) {
							return true;
						}

						failedIdx = (int)adapterIdx;
						Logger::Get().Warn("用户指定的显示卡不支持 D3D12");
						break;
					}
				}
			}
		}

		// 枚举查找第一个支持 D3D12 的显卡
		for (UINT adapterIdx = 0;
			SUCCEEDED(_dxgiFactory->EnumAdapters1(adapterIdx, adapter.put()));
			++adapterIdx
		) {
			if ((int)adapterIdx == failedIdx) {
				// 无需再次尝试
				continue;
			}

			DXGI_ADAPTER_DESC1 desc;
			HRESULT hr = adapter->GetDesc1(&desc);
			if (FAILED(hr) || DirectXHelper::IsWARP(desc)) {
				continue;
			}

			if (_TryCreateD3DDevice(adapter, desc)) {
				return true;
			}
		}
	}

	// 作为最后手段，回落到 CPU 渲染 (WARP)
	// https://docs.microsoft.com/en-us/windows/win32/direct3darticles/directx-warp
	HRESULT hr = _dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&adapter));
	if (FAILED(hr)) {
		Logger::Get().ComError("EnumWarpAdapter 失败", hr);
		return false;
	}

	[[maybe_unused]] static Ignore _ = [](IDXGIAdapter1* warpAdapter) {
		FixD3D10WarpDll(warpAdapter);
		return Ignore();
	}(adapter.get());
	
	DXGI_ADAPTER_DESC1 desc;
	hr = adapter->GetDesc1(&desc);
	if (FAILED(hr) || !_TryCreateD3DDevice(adapter, desc)) {
		Logger::Get().Error("创建 WARP 设备失败");
		return false;
	}

	return true;
}

bool D3D12Context::_TryCreateD3DDevice(
	const winrt::com_ptr<IDXGIAdapter1>& adapter,
	const DXGI_ADAPTER_DESC1& adapterDesc
) noexcept {
	HRESULT hr = D3D12CreateDevice(adapter.get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&_device));
	if (FAILED(hr)) {
		Logger::Get().ComError("D3D12CreateDevice 失败", hr);
		return false;
	}

	_dxgiAdapter = adapter.try_as<IDXGIAdapter4>();
	if (!_dxgiAdapter) {
		Logger::Get().Error("获取 IDXGIAdapter4 失败");
		return false;
	}

	Logger::Get().Info(fmt::format("图形适配器\n\tVendorId: {:#x}\n\tDeviceId: {:#x}\n\tDescription: {}",
		adapterDesc.VendorId, adapterDesc.DeviceId, StrHelper::UTF16ToUTF8(adapterDesc.Description)));

	return true;
}

bool D3D12Context::_CreateAdapterFromDevice() noexcept {
	const LUID adapterLuid = _device->GetAdapterLuid();

	winrt::com_ptr<IDXGIAdapter1> adapter;
	for (UINT adapterIdx = 0;
		SUCCEEDED(_dxgiFactory->EnumAdapters1(adapterIdx, adapter.put()));
		++adapterIdx
	) {
		DXGI_ADAPTER_DESC1 desc;
		HRESULT hr = adapter->GetDesc1(&desc);
		if (FAILED(hr)) {
			continue;
		}

		if (desc.AdapterLuid != adapterLuid) {
			continue;
		}

		_dxgiAdapter = adapter.try_as<IDXGIAdapter4>();
		if (_dxgiAdapter) {
			return true;
		} else {
			Logger::Get().Error("获取 IDXGIAdapter4 失败");
			return false;
		}
	}

	return false;
}

void D3D12Context::_QueryHighestShaderModel() noexcept {
	// 如果运行时不知道 HighestShaderModel，CheckFeatureSupport 将返回 E_INVALIDARG
	// （这只会发生在不支持 Agility SDK 的旧版本 Win10 上）。官方推荐从新到旧依次检查每
	// 个版本。
	constexpr std::array allModelVersions = {
		D3D_SHADER_MODEL_6_9,
		D3D_SHADER_MODEL_6_8,
		D3D_SHADER_MODEL_6_7,
		D3D_SHADER_MODEL_6_6,
		D3D_SHADER_MODEL_6_5,
		D3D_SHADER_MODEL_6_4,
		D3D_SHADER_MODEL_6_3,
		D3D_SHADER_MODEL_6_2,
		D3D_SHADER_MODEL_6_1,
		D3D_SHADER_MODEL_6_0,
		D3D_SHADER_MODEL_5_1
	};
	constexpr uint32_t versionCount = (uint32_t)std::size(allModelVersions);

	HighestShaderModel versionLimit = ScalingWindow::Get().Options().highestShaderModel;
	uint32_t startIdx = versionLimit == HighestShaderModel::NotLimited ? 0 : (uint32_t)versionLimit - 1;

	for (uint32_t i = startIdx; i < versionCount; ++i) {
		D3D12_FEATURE_DATA_SHADER_MODEL data = { .HighestShaderModel = allModelVersions[i]};
		HRESULT hr = _device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &data, sizeof(data));
		if (hr == E_INVALIDARG) {
			continue;
		}

		if (SUCCEEDED(hr)) {
			_shaderModel = data.HighestShaderModel;
		} else {
			Logger::Get().ComWarn("CheckFeatureSupport 失败", hr);
		}

		return;
	}
}

void D3D12Context::_LogDeviceInfo() noexcept {
	std::string_view featureLevel;
	{
		D3D_FEATURE_LEVEL featureLevels[] = {
			D3D_FEATURE_LEVEL_12_2,
			D3D_FEATURE_LEVEL_12_1,
			D3D_FEATURE_LEVEL_12_0,
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0
		};
		D3D12_FEATURE_DATA_FEATURE_LEVELS featureData = {
			.NumFeatureLevels = (UINT)std::size(featureLevels),
			.pFeatureLevelsRequested = featureLevels
		};
		HRESULT hr = _device->CheckFeatureSupport(
			D3D12_FEATURE_FEATURE_LEVELS, &featureData, sizeof(featureData));
		if (SUCCEEDED(hr)) {
			switch (featureData.MaxSupportedFeatureLevel) {
			case D3D_FEATURE_LEVEL_12_2:
				featureLevel = "12.2";
				break;
			case D3D_FEATURE_LEVEL_12_1:
				featureLevel = "12.1";
				break;
			case D3D_FEATURE_LEVEL_12_0:
				featureLevel = "12.0";
				break;
			case D3D_FEATURE_LEVEL_11_1:
				featureLevel = "11.1";
				break;
			case D3D_FEATURE_LEVEL_11_0:
				featureLevel = "11.0";
				break;
			default:
				featureLevel = "未知";
				break;
			}
		} else {
			Logger::Get().ComWarn("CheckFeatureSupport 失败", hr);
			featureLevel = "未知";
		}
	}

	std::string_view shaderModel;
	switch (_shaderModel) {
	case D3D_SHADER_MODEL_6_9:
		shaderModel = "6.9";
		break;
	case D3D_SHADER_MODEL_6_8:
		shaderModel = "6.8";
		break;
	case D3D_SHADER_MODEL_6_7:
		shaderModel = "6.7";
		break;
	case D3D_SHADER_MODEL_6_6:
		shaderModel = "6.6";
		break;
	case D3D_SHADER_MODEL_6_5:
		shaderModel = "6.5";
		break;
	case D3D_SHADER_MODEL_6_4:
		shaderModel = "6.4";
		break;
	case D3D_SHADER_MODEL_6_3:
		shaderModel = "6.3";
		break;
	case D3D_SHADER_MODEL_6_2:
		shaderModel = "6.2";
		break;
	case D3D_SHADER_MODEL_6_1:
		shaderModel = "6.1";
		break;
	case D3D_SHADER_MODEL_6_0:
		shaderModel = "6.0";
		break;
	default:
		shaderModel = "5.1";
		break;
	}

	constexpr const char* boolStrs[] = { "否","是" };

	Logger::Get().Info(fmt::format(R"(已创建 D3D12 设备
	功能级别: {}
	shader model 版本: {}
	根签名版本: {}
	集成显卡: {}
	D3D12_HEAP_FLAG_CREATE_NOT_ZEROED 支持: {}
	Resizable BAR 支持: {}
	min16float 支持: {}
	原生 16 位标量支持: {})",
		featureLevel,
		shaderModel,
		_rootSignatureVersion == D3D_ROOT_SIGNATURE_VERSION_1_1 ? "1.1" : "1.0",
		boolStrs[_isUMA],
		boolStrs[_isHeapFlagCreateNotZeroedSupported],
		boolStrs[_isGPUUploadHeapSupported],
		boolStrs[_isMinFloat16Supported],
		boolStrs[_isNative16BitSupported]
	));
}

}
