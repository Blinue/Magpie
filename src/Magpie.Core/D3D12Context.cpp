#include "pch.h"
#include "D3D12Context.h"
#include "AppFolderManager.h"
#include "DescriptorHeap.h"
#include "DirectXHelper.h"
#include "GpuResourceManager.h"
#include "Logger.h"
#include "ScalingWindow.h"
#include "StrHelper.h"

namespace Magpie {

bool D3D12Context::Initialize(
	const GraphicsCardId& graphicsCardId,
	uint32_t maxInFlightFrameCount,
	D3D12_COMMAND_QUEUE_PRIORITY priority,
	D3D12_COMMAND_LIST_TYPE commandListType,
	DescriptorHeap& csuDescriptorHeap,
	DescriptorHeap& rtvDescriptorHeap,
	GpuResourceManager& gpuResourceManager,
	bool disableFrameFenceTracking
) noexcept {
	_csuDescriptorHeap = &csuDescriptorHeap;
	_rtvDescriptorHeap = &rtvDescriptorHeap;
	_gpuResourceManager = &gpuResourceManager;
	
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
		hr = CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData));
		if (SUCCEEDED(hr)) {
			_rootSignatureVersion = featureData.HighestVersion;
		} else {
			Logger::Get().ComWarn("CheckFeatureSupport 失败", hr);
		}
	}

	if (!ScalingWindow::Get().Options().IsFP16Disabled()) {
		// SM 6.2 开始支持原生 16 位标量
		if (_shaderModel >= D3D_SHADER_MODEL_6_2) {
			D3D12_FEATURE_DATA_D3D12_OPTIONS4 data{};
			hr = CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS4, &data, sizeof(data));
			if (SUCCEEDED(hr)) {
				_isNative16BitSupported = data.Native16BitShaderOpsSupported;
			} else {
				Logger::Get().ComWarn("CheckFeatureSupport 失败", hr);
			}
		}
	}

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
	_gpuResourceManager = other._gpuResourceManager;
	_device = other._device;
	_device16 = other._device16;
	_shaderModel = other._shaderModel;
	_rootSignatureVersion = other._rootSignatureVersion;
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

bool D3D12Context::IsUMA() const noexcept {
	return _gpuResourceManager->GetAllocator()->IsUMA();
}

bool D3D12Context::IsGPUUploadHeapSupported() const noexcept {
	return _gpuResourceManager->GetAllocator()->IsGPUUploadHeapSupported();
}

bool D3D12Context::IsMinFloat16Supported() const noexcept {
	if (ScalingWindow::Get().Options().IsFP16Disabled()) {
		return false;
	}

	auto allocator = _gpuResourceManager->GetAllocator();
	return allocator->GetD3D12Options().MinPrecisionSupport & D3D12_SHADER_MIN_PRECISION_SUPPORT_16_BIT;
}

HRESULT D3D12Context::CheckFeatureSupport(
	D3D12_FEATURE feature,
	void* pFeatureSupportData,
	UINT featureSupportDataSize
) const noexcept {
	HRESULT hr;

	// ID3D12Device::CheckFeatureSupport 已废弃，调试层会产生警告
	if (_device16) {
		hr = _device16->CheckFeatureSupport1(
			feature, pFeatureSupportData, featureSupportDataSize, D3D12_SDK_VERSION);
		if (FAILED(hr)) {
			Logger::Get().ComError("CheckFeatureSupport1 失败", hr);
		}
	} else {
		hr = _device->CheckFeatureSupport(
			feature, pFeatureSupportData, featureSupportDataSize);
		if (FAILED(hr)) {
			Logger::Get().ComError("CheckFeatureSupport 失败", hr);
		}
	}

	return hr;
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

	_device16 = _device.try_as<ID3D12Device16>();
	if (!_device16) {
		Logger::Get().Info("获取 ID3D12Device16 失败");
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
		D3D_SHADER_MODEL_6_10,
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

	// HighestShaderModel 从低到高排序
	HighestShaderModel versionLimit = ScalingWindow::Get().Options().highestShaderModel;
	uint32_t startIdx = versionLimit == HighestShaderModel::NotLimited ? 0 :
		versionCount - (uint32_t)versionLimit;

	for (uint32_t i = startIdx; i < versionCount; ++i) {
		D3D12_FEATURE_DATA_SHADER_MODEL data = { .HighestShaderModel = allModelVersions[i]};
		HRESULT hr = CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &data, sizeof(data));
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

}
