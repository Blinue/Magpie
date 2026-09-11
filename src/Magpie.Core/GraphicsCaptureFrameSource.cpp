#include "pch.h"
#include "GraphicsCaptureFrameSource.h"
#include "DebugInfo.h"
#include "DirectXHelper.h"
#include "DirtyRectsOptimizer.h"
#include "DuplicateFrameChecker.h"
#include "D3D12Context.h"
#include "Logger.h"
#include "ScalingWindow.h"
#include "Win32Helper.h"
#include <dwmapi.h>
#include <Windows.Graphics.Capture.Interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <d3dkmthk.h>

namespace winrt {
using namespace Windows::Graphics;
using namespace Windows::Graphics::Capture;
using namespace Windows::Graphics::DirectX;
using namespace Windows::Graphics::DirectX::Direct3D11;
}

namespace Magpie {

GraphicsCaptureFrameSource::~GraphicsCaptureFrameSource() noexcept {
	if (_captureSession) {
		_StopCapture();
	}

	const HWND hwndSrc =  ScalingWindow::Get().SrcHandle();

	// 还原源窗口圆角
	if (_isRoundCornerDisabled) {
		int value = DWMWCP_DEFAULT;
		HRESULT hr = DwmSetWindowAttribute(
			hwndSrc, DWMWA_WINDOW_CORNER_PREFERENCE, &value, sizeof(value));
		if (FAILED(hr)) {
			Logger::Get().ComError("取消禁用窗口圆角失败", hr);
		} else {
			Logger::Get().Info("已取消禁用窗口圆角");
		}
	}

	// 还原源窗口样式
	if (_isSrcStyleChanged) {
		const DWORD srcExStyle = GetWindowExStyle(hwndSrc);
		SetWindowLongPtr(hwndSrc, GWL_EXSTYLE, srcExStyle & ~WS_EX_APPWINDOW);
	}

	// 还原 Kirikiri 窗口
	if (_taskbarList) {
		_taskbarList->DeleteTab(hwndSrc);
		_taskbarList->AddTab(GetWindowOwner(hwndSrc));

		// 修正任务栏焦点窗口和 Alt+Tab 切换顺序
		if (GetForegroundWindow() == hwndSrc) {
			SetForegroundWindow(GetDesktopWindow());
			SetForegroundWindow(hwndSrc);
		}
	}
}

static winrt::com_ptr<IDXGIAdapter1> FindAdapterOfMonitor(IDXGIFactory7* dxgiFactory, HMONITOR hMon) noexcept {
	winrt::com_ptr<IDXGIAdapter1> adapter;
	winrt::com_ptr<IDXGIOutput> output;
	for (UINT adapterIdx = 0;
		SUCCEEDED(dxgiFactory->EnumAdapters1(adapterIdx, adapter.put()));
		++adapterIdx
	) {
		for (UINT outputIdx = 0;
			SUCCEEDED(adapter->EnumOutputs(outputIdx, output.put()));
			++outputIdx
		) {
			DXGI_OUTPUT_DESC desc;
			if (SUCCEEDED(output->GetDesc(&desc)) && desc.Monitor == hMon) {
				return adapter;
			}
		}
	}

	adapter = nullptr;
	return adapter;
}

static bool CalcWindowCapturedFrameBounds(HWND hWnd, RECT& rect) noexcept {
	// Graphics Capture 的捕获区域没有文档记录，这里的计算是我实验了多种窗口后得出的，
	// 高度依赖实现细节，未来可能会失效。
	// Win10 和 Win11 24H2 开始捕获区域为 extended frame bounds；Win11 24H2 前
	// DwmGetWindowAttribute 对最大化的窗口返回值和 Win10 不同，可能是 OS 的 bug，
	// 应进一步处理。
	HRESULT hr = DwmGetWindowAttribute(hWnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rect, sizeof(rect));
	if (FAILED(hr)) {
		Logger::Get().ComError("DwmGetWindowAttribute 失败", hr);
		return false;
	}

	if (Win32Helper::GetWindowShowCmd(hWnd) != SW_SHOWMAXIMIZED ||
		Win32Helper::GetOSVersion().IsWin10() ||
		Win32Helper::GetOSVersion().Is24H2OrNewer()) {
		return true;
	}

	// 如果窗口禁用了非客户区域绘制则捕获区域为 extended frame bounds
	BOOL hasBorder = TRUE;
	hr = DwmGetWindowAttribute(hWnd, DWMWA_NCRENDERING_ENABLED, &hasBorder, sizeof(hasBorder));
	if (FAILED(hr)) {
		Logger::Get().ComError("DwmGetWindowAttribute 失败", hr);
		return false;
	}

	if (!hasBorder) {
		return true;
	}

	RECT clientRect;
	if (!Win32Helper::GetClientScreenRect(hWnd, clientRect)) {
		Logger::Get().Error("GetClientScreenRect 失败");
		return false;
	}

	// 有些窗口最大化后有部分客户区在屏幕外，如 UWP 和资源管理器，它们的捕获区域
	// 是整个客户区。否则捕获区域不会超出屏幕
	HMONITOR hMon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi{ .cbSize = sizeof(mi) };
	if (!GetMonitorInfo(hMon, &mi)) {
		Logger::Get().Win32Error("GetMonitorInfo 失败");
		return false;
	}

	if (clientRect.top < mi.rcWork.top) {
		rect = clientRect;
	} else {
		Win32Helper::IntersectRect(rect, rect, mi.rcWork);
	}

	return true;
}

static uint32_t CalcCaptureFrameCount() noexcept {
	// maxProducerInFlightFrames(_slots)+1(_latestFrame)+1(_newFrame)+2(备用)
	return ScalingWindow::Get().Options().maxProducerInFlightFrames + 4;
}

bool GraphicsCaptureFrameSource::Initialize(
	D3D12Context& d3d12Context,
	const RECT& srcRect,
	HMONITOR hMonSrc,
	const ColorInfo& colorInfo
) noexcept {
	assert(hMonSrc);

	_d3d12Context = &d3d12Context;
	_isScRGB = colorInfo.kind != winrt::AdvancedColorKind::StandardDynamicRange;

	if (!winrt::GraphicsCaptureSession::IsSupported()) {
		Logger::Get().Error("当前无法使用 Graphics Capture");
		return false;
	}

	// 截至 Win11 25H2，WGC 的脏区域汇报存在滞后的问题，这导致我们无法使用脏区域功能。
	// 考虑下面的场景：
	// 1. 帧 1 到达
	// 2. 帧 2 到达，区域 A 有变化但没有汇报，因此不会复制区域 A
	// 3. 帧 3 到达，汇报区域 A 有变化。对比帧 2 和帧 3 发现区域 A 不变，因此跳过复制
	// 脏区域汇报滞后导致区域 A 的变化被忽略了。
	// 
	// 脏区域处理流程：
	// 1. _Direct3D11CaptureFramePool_FrameArrived 将脏矩形累积到 _latestFrameDirtyRects
	// 2. CheckForNewFrame 将 _latestFrameDirtyRects 移动到 _newFrameDirtyRects
	// 3. CheckForNewFrame 执行重复帧检查，删除 _newFrameDirtyRects 中画面不变的矩形
	// 4. 如果画面变化，Update 将 _newFrameDirtyRects 移动到 curSlot.dirtyRects
	// 5. Update 累积所有 slot 的脏矩形然后更新 curSlot.output
	// 
	// _isDirtyRegionSupported = winrt::ApiInformation::IsPropertyPresent(
	//     winrt::name_of<winrt::GraphicsCaptureSession>(), L"DirtyRegionMode");

	{
		RECT frameBounds;
		if (!CalcWindowCapturedFrameBounds(ScalingWindow::Get().SrcHandle(), frameBounds)) {
			Logger::Get().Error("CalcWindowCapturedFrameBounds 失败");
			return false;
		}

		if (srcRect.left < frameBounds.left || srcRect.top < frameBounds.top) {
			Logger::Get().Error("裁剪边框错误");
			return false;
		}

		// 在源窗口存在 DPI 缩放时有时会有一像素的偏移（取决于窗口在屏幕上的位置）
		// 可能是 DwmGetWindowAttribute 的 bug
		_frameBox = {
			UINT(srcRect.left - frameBounds.left),
			UINT(srcRect.top - frameBounds.top),
			0,
			UINT(srcRect.right - frameBounds.left),
			UINT(srcRect.bottom - frameBounds.top),
			1
		};
	}

	_producerThreadId.store(GetCurrentThreadId(), std::memory_order_relaxed);

	ID3D12Device5* device = d3d12Context.GetDevice();

	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = { .Type = D3D12_COMMAND_LIST_TYPE_COPY };
		HRESULT hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_copyCommandQueue));
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateCommandQueue 失败", hr);
			return false;
		}
	}

	HRESULT hr = device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_COPY,
		D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&_copyCommandList));
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateCommandList1 失败", hr);
		return false;
	}

	const ScalingOptions& options = ScalingWindow::Get().Options();
	_slots.resize(options.maxProducerInFlightFrames);
	_curFrameIdx = options.maxProducerInFlightFrames - 1;

	for (_FrameResourceSlot& slot : _slots) {
		hr = device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&slot.commandAllocator));
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateCommandAllocator 失败", hr);
			return false;
		}
	}

	if (!_CreateCaptureDevice(hMonSrc)) {
		Logger::Get().ComError("_CreateCaptureDevice 失败", hr);
		return false;
	}

	hr = _CreateDisplayDependentResources();
	if (FAILED(hr)) {
		Logger::Get().ComError("_CreateDisplayDependentResources 失败", hr);
		return false;
	}

	const uint32_t captureFrameCount = CalcCaptureFrameCount();
	_captureFrameResourceTable.reserve(captureFrameCount);

	if (options.duplicateFrameDetectionMode != DuplicateFrameDetectionMode::Never) {
		_duplicateFrameChecker = std::make_unique<DuplicateFrameChecker>();
		// 不使用脏区域时可以跳过边界检查，因为捕获帧右下两边没有多余像素
		if (!_duplicateFrameChecker->Initialize(
			_d3d11Device.get(),
			_d3d11DC.get(),
			colorInfo,
			SizeU{ _frameBox.right, _frameBox.bottom },
			captureFrameCount,
			!_isDirtyRegionSupported
		)) {
			Logger::Get().Error("DuplicateFrameChecker::Initialize 失败");
			return false;
		}
	}

	if (!_InitializeCaptureItem()) {
		Logger::Get().Error("_InitializeCaptureItem 失败");
		return false;
	}

	return true;
}

bool GraphicsCaptureFrameSource::Start() noexcept {
	assert(!_captureSession);

	// 尽可能推迟禁用源窗口圆角
	if (!_isRoundCornerDisabled) {
		_DisableRoundCornerInWin11();
	}

	HRESULT hr = _StartCapture();
	if (FAILED(hr)) {
		Logger::Get().ComError("_StartCapture 失败", hr);
		return false;
	}

	return true;
}

HRESULT GraphicsCaptureFrameSource::CheckForNewFrame(bool& isNewFrameAvailable) noexcept {
	{
		auto lk = _latestFrameLock.lock_shared();

		if (_latestFrame) {
			_newFrame = std::move(_latestFrame);
			_latestFrame = nullptr;

			if (_isDirtyRegionSupported) {
				// 如果画面变化接下来会调用 Update，因此 _newFrameDirtyRects 肯定为空
				assert(_newFrameDirtyRects.empty());

				if (_captureFrameResourceTable.empty()) {
					// 第一帧必须更新整个捕获区域
					_newFrameDirtyRects.emplace_back(_frameBox.left, _frameBox.top, _frameBox.right, _frameBox.bottom);
					_latestFrameDirtyRects.clear();
				} else {
					// 交换而不是移动以减少堆分配次数
					std::swap(_newFrameDirtyRects, _latestFrameDirtyRects);
				}
			}
		} else {
			isNewFrameAvailable = (bool)_newFrame;
			return S_OK;
		}
	}
	
	ID3D12Device5* dfDevice = _bridgeDevice ? _bridgeDevice.get() : _d3d12Context->GetDevice();

	winrt::com_ptr<ID3D11Texture2D> d3d11Texture;
	{
		winrt::IDirect3DSurface d3dSurface = _newFrame.Surface();

		using ::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess;
		auto dxgiInterfaceAccess = d3dSurface.try_as<IDirect3DDxgiInterfaceAccess>();
		HRESULT hr = dxgiInterfaceAccess->GetInterface(IID_PPV_ARGS(&d3d11Texture));
		if (FAILED(hr)) {
			Logger::Get().ComError("IDirect3DDxgiInterfaceAccess::GetInterface 失败", hr);
			return hr;
		}

#ifdef _DEBUG
		{
			D3D11_TEXTURE2D_DESC desc;
			d3d11Texture->GetDesc(&desc);
			const DXGI_FORMAT expectedFormat =
				_isScRGB ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_B8G8R8A8_UNORM;
			assert(desc.Format == expectedFormat);
		}
#endif
		
		// 目前 WGC 帧池不会变化，因此可以缓存，需要采取保护措施防止内部实现变化
		auto it = std::find_if(
			_captureFrameResourceTable.begin(),
			_captureFrameResourceTable.end(),
			[&](const std::pair<ID3D11Texture2D*, winrt::com_ptr<ID3D12Resource>>& elem) {
				return elem.first == d3d11Texture.get();
			}
		);
		if (it == _captureFrameResourceTable.end()) {
			// 如果帧池有变化应清空缓存
			if (_captureFrameResourceTable.size() == CalcCaptureFrameCount()) {
				assert(false);
				_captureFrameResourceTable.clear();

				if (_duplicateFrameChecker) {
					_duplicateFrameChecker->OnCaptureStopped();
				}
			}

			auto dxgiResource = d3d11Texture.try_as<IDXGIResource1>();

			wil::unique_handle hSharedResource;
			hr = dxgiResource->CreateSharedHandle(nullptr, GENERIC_ALL, nullptr, hSharedResource.put());
			if (FAILED(hr)) {
				Logger::Get().ComError("IDXGIResource1::CreateSharedHandle 失败", hr);
				return hr;
			}

			winrt::com_ptr<ID3D12Resource> frameResource;
			hr = dfDevice->OpenSharedHandle(hSharedResource.get(), IID_PPV_ARGS(&frameResource));
			if (FAILED(hr)) {
				Logger::Get().ComError("OpenSharedHandle 失败", hr);
				return hr;
			}

			_captureFrameResourceTable.emplace_back(d3d11Texture.get(), std::move(frameResource));
			_newCaptureFrameResourceIdx = (uint32_t)_captureFrameResourceTable.size() - 1;
		} else {
			_newCaptureFrameResourceIdx = uint32_t(it - _captureFrameResourceTable.begin());
		}
	}

	if (!_duplicateFrameChecker) {
		isNewFrameAvailable = true;
		return S_OK;
	}

	if (_isDirtyRegionSupported) {
		DirtyRectsOptimizer::Execute(_newFrameDirtyRects);
	} else {
		// 不支持脏矩形时检查整个捕获区域
		if (_newFrameDirtyRects.empty()) {
			_newFrameDirtyRects.emplace_back(_frameBox.left, _frameBox.top, _frameBox.right, _frameBox.bottom);
		}
	}

	HRESULT hr = _duplicateFrameChecker->CheckFrame(
		d3d11Texture.get(), _newCaptureFrameResourceIdx, _newFrameDirtyRects);
	if (FAILED(hr)) {
		Logger::Get().ComError("DuplicateFrameChecker::CheckFrame 失败", hr);
		return hr;
	}

	// 脏矩形被清空则为重复帧
	isNewFrameAvailable = !_newFrameDirtyRects.empty();
	if (!isNewFrameAvailable) {
		_newFrame = nullptr;
	}

	return S_OK;
}

HRESULT GraphicsCaptureFrameSource::Update(uint32_t& outputIdx) noexcept {
	if (!_newFrame) {
		// 没有新帧
		outputIdx = _curFrameIdx;
		return S_OK;
	}

	if (_duplicateFrameChecker) {
		_duplicateFrameChecker->OnFrameAdopted(_newCaptureFrameResourceIdx);
	}

	_curFrameIdx = (_curFrameIdx + 1) % (uint32_t)_slots.size();
	_FrameResourceSlot& curSlot = _slots[_curFrameIdx];

	curSlot.captureFrame = std::move(_newFrame);
	_newFrame = nullptr;
	curSlot.captureFrameResourceIdx = _newCaptureFrameResourceIdx;

	if (_isDirtyRegionSupported) {
		assert(!_newFrameDirtyRects.empty());
		// 交换而不是移动以减少堆分配次数
		std::swap(curSlot.dirtyRects, _newFrameDirtyRects);
		_newFrameDirtyRects.clear();
	}
	
	ID3D12Resource* curFrameResource =
		_captureFrameResourceTable[_newCaptureFrameResourceIdx].second.get();

	// curSlot.output 到 curFrameResource 的脏矩形
	SmallVector<RectU> allDirtyRects;
	if (_isDirtyRegionSupported) {
		for (const _FrameResourceSlot& slot : _slots) {
			allDirtyRects.append(slot.dirtyRects);
		}
		DirtyRectsOptimizer::Execute(allDirtyRects);

#ifdef _DEBUG
		// 所有脏矩形应在 _frameBox 内
		for (const RectU& dirtyRect : allDirtyRects) {
			assert(dirtyRect.left >= _frameBox.left && dirtyRect.top >= _frameBox.top &&
				dirtyRect.right <= _frameBox.right && dirtyRect.bottom <= _frameBox.bottom);
		}
#endif
	}
	
#ifdef MP_DEBUG_INFO
	{
		auto lk = DEBUG_INFO.lock.lock_exclusive();

		if (DEBUG_INFO.dtmFrameNumer == 0) {
			DEBUG_INFO.dtmDwmQPC = curSlot.captureFrame.SystemRelativeTime().count();
			DEBUG_INFO.dtmFrameNumer = DEBUG_INFO.producerFrameNumber;
		}

		if (DEBUG_INFO.ctpCapturedFrame && DEBUG_INFO.ctpFrameNumer == 0) {
			if (winrt::get_abi(curSlot.captureFrame) == DEBUG_INFO.ctpCapturedFrame) {
				DEBUG_INFO.ctpFrameNumer = DEBUG_INFO.producerFrameNumber;
			} else {
				// 追踪的捕获帧被错过，需要重新测量
				DEBUG_INFO.ctpCapturedFrame = nullptr;
			}
		}
	}
#endif

	// 同适配器数据路径: frameResource -> output
	// 跨适配器数据路径: frameResource -> bridgeResource|sharedResource -> output

	HRESULT hr = curSlot.commandAllocator->Reset();
	if (FAILED(hr)) {
		return hr;
	}

	hr = _copyCommandList->Reset(curSlot.commandAllocator.get(), nullptr);
	if (FAILED(hr)) {
		return hr;
	}

	if (_bridgeDevice) {
		_FrameCrossAdapterResourceSlot& curCASlot = _crossAdapterSlots[_curFrameIdx];

		hr = curCASlot.commandAllocator->Reset();
		if (FAILED(hr)) {
			return hr;
		}

		hr = _bridgeCopyCommandList->Reset(curCASlot.commandAllocator.get(), nullptr);
		if (FAILED(hr)) {
			return hr;
		}

		{
			CD3DX12_TEXTURE_COPY_LOCATION src(curFrameResource, 0);
			CD3DX12_TEXTURE_COPY_LOCATION dest(curCASlot.bridgeResource.get(), 0);

			if (_isDirtyRegionSupported) {
				for (const RectU& dirtyRect : allDirtyRects) {
					D3D12_BOX box = {
						.left = dirtyRect.left,
						.top = dirtyRect.top,
						.right = dirtyRect.right,
						.bottom = dirtyRect.bottom,
						.back = 1
					};
					_bridgeCopyCommandList->CopyTextureRegion(
						&dest, dirtyRect.left - _frameBox.left, dirtyRect.top - _frameBox.top, 0, &src, &box);
				}
			} else {
				_bridgeCopyCommandList->CopyTextureRegion(&dest, 0, 0, 0, &src, &_frameBox);
			}
		}

		hr = _bridgeCopyCommandList->Close();
		if (FAILED(hr)) {
			Logger::Get().ComError("ID3D12GraphicsCommandList::Close 失败", hr);
			return hr;
		}

		{
			ID3D12CommandList* t = _bridgeCopyCommandList.get();
			_bridgeCopyCommandQueue->ExecuteCommandLists(1, &t);
		}

		hr = _bridgeCopyCommandQueue->Signal(_bridgeFence.get(), ++_curCrossAdapterFenceValue);
		if (FAILED(hr)) {
			Logger::Get().ComError("ID3D12CommandQueue::Signal 失败", hr);
			return hr;
		}

		hr = _copyCommandQueue->Wait(_sharedFence.get(), _curCrossAdapterFenceValue);
		if (FAILED(hr)) {
			Logger::Get().ComError("ID3D12CommandQueue::Wait 失败", hr);
			return hr;
		}

		// 需要复制整个纹理时使用 CopyResource
		auto canCopyResource = [&] {
			if (!_isDirtyRegionSupported) {
				return true;
			}

			if (allDirtyRects.size() > 1) {
				return false;
			}

			const RectU& dirtyRect = allDirtyRects[0];
			return dirtyRect.left == _frameBox.left && dirtyRect.top == _frameBox.top &&
				dirtyRect.right == _frameBox.right && dirtyRect.bottom == _frameBox.bottom;
		};

		if (canCopyResource()) {
			_copyCommandList->CopyResource(curSlot.output.get(), curCASlot.sharedResource.get());
		} else {
			CD3DX12_TEXTURE_COPY_LOCATION src(curCASlot.sharedResource.get(), 0);
			CD3DX12_TEXTURE_COPY_LOCATION dest(curSlot.output.get(), 0);

			for (const RectU& dirtyRect : allDirtyRects) {
				D3D12_BOX box = {
					.left = dirtyRect.left - _frameBox.left,
					.top = dirtyRect.top - _frameBox.top,
					.right = dirtyRect.right - _frameBox.left,
					.bottom = dirtyRect.bottom - _frameBox.top,
					.back = 1
				};
				// AMD 集显有时会少复制最后一行，将复制队列改为计算队列或用 CopyResource 可以规避，似乎是驱动 bug
				_copyCommandList->CopyTextureRegion(&dest, box.left, box.top, 0, &src, &box);
			}
		}
	} else {
		CD3DX12_TEXTURE_COPY_LOCATION src(curFrameResource, 0);
		CD3DX12_TEXTURE_COPY_LOCATION dest(curSlot.output.get(), 0);

		if (_isDirtyRegionSupported) {
			for (const RectU& dirtyRect : allDirtyRects) {
				D3D12_BOX box = {
					.left = dirtyRect.left,
					.top = dirtyRect.top,
					.right = dirtyRect.right,
					.bottom = dirtyRect.bottom,
					.back = 1
				};
				_copyCommandList->CopyTextureRegion(
					&dest, dirtyRect.left - _frameBox.left, dirtyRect.top - _frameBox.top, 0, &src, &box);
			}
		} else {
			_copyCommandList->CopyTextureRegion(&dest, 0, 0, 0, &src, &_frameBox);
		}
	}
	
	hr = _copyCommandList->Close();
	if (FAILED(hr)) {
		Logger::Get().ComError("ID3D12GraphicsCommandList::Close 失败", hr);
		return hr;
	}

	{
		ID3D12CommandList* t = _copyCommandList.get();
		_copyCommandQueue->ExecuteCommandLists(1, &t);
	}

	hr = _d3d12Context->WaitForCommandQueue(_copyCommandQueue.get());
	if (FAILED(hr)) {
		Logger::Get().ComError("D3D12Context::WaitForCommandQueue 失败", hr);
		return hr;
	}

	outputIdx = _curFrameIdx;
	return S_OK;
}

HRESULT GraphicsCaptureFrameSource::OnColorInfoChanged(const ColorInfo& colorInfo) noexcept {
	const bool wasScRGB = _isScRGB;
	_isScRGB = colorInfo.kind != winrt::AdvancedColorKind::StandardDynamicRange;

	// 一旦色域变化我们需要立刻获得新帧，因此重启捕获而不是使用 Recreate
	_StopCapture();

	HRESULT hr = _StartCapture();
	if (FAILED(hr)) {
		Logger::Get().ComError("_StartCapture 失败", hr);
		return hr;
	}

	if (_isScRGB != wasScRGB) {
		hr = _CreateDisplayDependentResources();
		if (FAILED(hr)) {
			Logger::Get().ComError("_CreateDisplayDependentResources 失败", hr);
			return hr;
		}
	}
	
	return S_OK;
}

// 显示光标时需要重启捕获，否则光标可能不会立刻显示
HRESULT GraphicsCaptureFrameSource::OnCursorVisibilityChanged(bool isVisible, bool onDestory) noexcept {
	if (!isVisible) {
		return S_OK;
	}

	HRESULT hr = _d3d12Context->WaitForGpu();
	if (FAILED(hr)) {
		Logger::Get().ComError("D3D12Context::WaitForGpu 失败", hr);
		return hr;
	}

	_StopCapture();

	if (onDestory) {
		// FIXME: 这里尝试修复拖动窗口时光标不显示的问题，但有些环境下不起作用
		SystemParametersInfo(SPI_SETCURSORS, 0, nullptr, 0);
	} else {
		hr = _StartCapture();
		if (FAILED(hr)) {
			Logger::Get().ComError("_StartCapture 失败", hr);
			return hr;
		}
	}

	return S_OK;
}

bool GraphicsCaptureFrameSource::_CreateCaptureDevice(HMONITOR hMonSrc) noexcept {
	// 查找源窗口所在屏幕连接的适配器
	winrt::com_ptr<IDXGIAdapter1> srcMonAdapter =
		FindAdapterOfMonitor(_d3d12Context->GetDXGIFactoryForEnumingAdapters(), hMonSrc);
	if (srcMonAdapter) {
		DXGI_ADAPTER_DESC desc;
		HRESULT hr = srcMonAdapter->GetDesc(&desc);
		if (SUCCEEDED(hr)) {
			if (desc.AdapterLuid != _d3d12Context->GetDevice()->GetAdapterLuid()) {
				// 跨适配器捕获
				if (!_CreateBridgeDeviceResources(srcMonAdapter.get())) {
					// 失败则使用渲染设备捕获，交给 WGC 中转
					srcMonAdapter = nullptr;

					// 清理跨适配器资源
					_bridgeDevice = nullptr;
					_bridgeCopyCommandQueue = nullptr;
					_bridgeCopyCommandList = nullptr;
					_sharedFence = nullptr;
					_bridgeFence = nullptr;
					_crossAdapterSlots.clear();
				}
			}
		} else {
			Logger::Get().ComError("IDXGIAdapter1::GetDesc 失败", hr);
		}
	}

	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};
	const UINT nFeatureLevels = ARRAYSIZE(featureLevels);

	UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	winrt::com_ptr<ID3D11Device> d3dDevice;
	winrt::com_ptr<ID3D11DeviceContext> d3dDC;
	D3D_FEATURE_LEVEL featureLevel;
	HRESULT hr = D3D11CreateDevice(
		srcMonAdapter ? srcMonAdapter.get() : _d3d12Context->GetDXGIAdapter(),
		D3D_DRIVER_TYPE_UNKNOWN,
		nullptr,
		createDeviceFlags,
		featureLevels,
		nFeatureLevels,
		D3D11_SDK_VERSION,
		d3dDevice.put(),
		&featureLevel,
		d3dDC.put()
	);

	if (FAILED(hr)) {
		Logger::Get().ComError("D3D11CreateDevice 失败", hr);
		return false;
	}

	std::string_view fl;
	switch (featureLevel) {
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
	Logger::Get().Info(fmt::format("已创建 D3D11 设备\n\t功能级别: {}", fl));

	_d3d11Device = d3dDevice.try_as<ID3D11Device5>();
	if (!_d3d11Device) {
		Logger::Get().Error("获取 ID3D11Device5 失败");
		return false;
	}

	_d3d11DC = d3dDC.try_as<ID3D11DeviceContext4>();
	if (!_d3d11DC) {
		Logger::Get().Error("获取 ID3D11DeviceContext4 失败");
		return false;
	}

#ifdef _DEBUG
	// 调试层汇报错误或警告时中断
	if (winrt::com_ptr<ID3D11InfoQueue> infoQueue = d3dDevice.try_as<ID3D11InfoQueue>()) {
		infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
		infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, TRUE);
	}
#endif
	
	winrt::com_ptr<IDXGIDevice> dxgiDevice = d3dDevice.try_as<IDXGIDevice>();
	if (!dxgiDevice) {
		Logger::Get().Error("获取 IDXGIDevice 失败");
		return false;
	}

	// 设置优先级为 Soft Realtime，我们希望捕获和检查重复帧尽可能快。如果使用常规优先级，当前台窗口
	// 执行繁重 GPU 任务时，检查重复帧有时耗时数毫秒，这是不可接受的。很遗憾 D3D12 没有等价接口，这
	// 是使用 D3D11 检查重复帧的主要原因。
	hr = dxgiDevice->SetGPUThreadPriority(D3DKMT_SETCONTEXTSCHEDULINGPRIORITY_ABSOLUTE | 29);
	if (FAILED(hr)) {
		Logger::Get().ComError("IDXGIDevice::SetGPUThreadPriority 失败", hr);
	}

	return true;
}

// 通过 D3D12 跨适配器共享机制共享捕获图像
bool GraphicsCaptureFrameSource::_CreateBridgeDeviceResources(IDXGIAdapter1* dxgiAdapter) noexcept {
	HRESULT hr = D3D12CreateDevice(dxgiAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&_bridgeDevice));
	if (FAILED(hr)) {
		Logger::Get().ComError("D3D12CreateDevice 失败", hr);
		return false;
	}

	Logger::Get().Info("已创建 D3D12 设备");

#ifdef _DEBUG
	// 调试层汇报错误或警告时中断
	if (winrt::com_ptr<ID3D12InfoQueue> infoQueue = _bridgeDevice.try_as<ID3D12InfoQueue>()) {
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
	}
#endif

	// 不应使用集成显卡捕获，集成显卡没有高速的专用显存，捕获延迟很高
	{
		D3D12_FEATURE_DATA_ARCHITECTURE1 value{};
		hr = _bridgeDevice->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE1, &value, sizeof(value));
		if (FAILED(hr)) {
			Logger::Get().ComWarn("CheckFeatureSupport 失败", hr);
		}

		if (value.UMA) {
			Logger::Get().Info("不使用集成显卡捕获");
			return false;
		}
	}

	ID3D12Device5* device = _d3d12Context->GetDevice();
	const uint32_t frameCount = ScalingWindow::Get().Options().maxProducerInFlightFrames;

	_crossAdapterSlots.resize(frameCount);

	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = { .Type = D3D12_COMMAND_LIST_TYPE_COPY };
		hr = _bridgeDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_bridgeCopyCommandQueue));
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateCommandQueue 失败", hr);
			return false;
		}
	}

	hr = _bridgeDevice->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_COPY,
		D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&_bridgeCopyCommandList));
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateCommandList1 失败", hr);
		return false;
	}

	for (_FrameCrossAdapterResourceSlot& slot : _crossAdapterSlots) {
		hr = _bridgeDevice->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&slot.commandAllocator));
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateCommandAllocator 失败", hr);
			return false;
		}
	}

	// 创建跨适配器栅栏，遵循“写入者创建”的原则
	hr = _bridgeDevice->CreateFence(
		0,
		D3D12_FENCE_FLAG_SHARED | D3D12_FENCE_FLAG_SHARED_CROSS_ADAPTER,
		IID_PPV_ARGS(&_bridgeFence)
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateFence 失败", hr);
		return false;
	}

	wil::unique_handle hSharedFence;
	hr = _bridgeDevice->CreateSharedHandle(
		_bridgeFence.get(), nullptr, GENERIC_ALL, nullptr, hSharedFence.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateSharedHandle 失败", hr);
		return false;
	}

	hr = device->OpenSharedHandle(hSharedFence.get(), IID_PPV_ARGS(&_sharedFence));
	if (FAILED(hr)) {
		Logger::Get().ComError("OpenSharedHandle 失败", hr);
		return false;
	}

	return true;
}

HRESULT GraphicsCaptureFrameSource::_CreateDisplayDependentResources() noexcept {
	ID3D12Device5* device = _d3d12Context->GetDevice();

	// 创建每帧输出纹理
	{
		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

		D3D12_HEAP_FLAGS heapFlags = _d3d12Context->IsHeapFlagCreateNotZeroedSupported() ?
			D3D12_HEAP_FLAG_CREATE_NOT_ZEROED : D3D12_HEAP_FLAG_NONE;

		CD3DX12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
			_isScRGB ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_B8G8R8A8_TYPELESS,
			UINT64(_frameBox.right - _frameBox.left),
			_frameBox.bottom - _frameBox.top,
			1, 1, 1, 0,
			D3D12_RESOURCE_FLAG_NONE
		);

		for (_FrameResourceSlot& slot : _slots) {
			HRESULT hr = device->CreateCommittedResource(&heapProps, heapFlags,
				&texDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&slot.output));
			if (FAILED(hr)) {
				Logger::Get().ComError("CreateCommittedResource 失败", hr);
				return hr;
			}
		}
	}

	if (!_bridgeDevice) {
		return S_OK;
	}

	// 跨适配器捕获时创建跨适配器共享纹理

	CD3DX12_RESOURCE_DESC textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		_isScRGB ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_B8G8R8A8_UNORM,
		UINT64(_frameBox.right - _frameBox.left),
		_frameBox.bottom - _frameBox.top,
		1, 1, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER,
		D3D12_TEXTURE_LAYOUT_ROW_MAJOR
	);

	D3D12_RESOURCE_ALLOCATION_INFO textureInfo =
		_bridgeDevice->GetResourceAllocationInfo(0, 1, &textureDesc);

	const uint32_t frameCount = ScalingWindow::Get().Options().maxProducerInFlightFrames;

	// 创建跨适配器共享堆。应遵循“写入者创建”的原则，否则可能无法正确同步，Intel 集显作为
	// 捕获设备时存在这个问题。
	{
		const bool isCreateNotZeroedSupported = (bool)_bridgeDevice.try_as<ID3D12Device8>();
		CD3DX12_HEAP_DESC heapDesc(
			textureInfo.SizeInBytes * frameCount,
			D3D12_HEAP_TYPE_DEFAULT,
			0,
			D3D12_HEAP_FLAG_SHARED | D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER |
			(isCreateNotZeroedSupported ? D3D12_HEAP_FLAG_CREATE_NOT_ZEROED : D3D12_HEAP_FLAG_NONE)
		);
		HRESULT hr = _bridgeDevice->CreateHeap(&heapDesc, IID_PPV_ARGS(&_bridgeHeap));
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateHeap 失败", hr);
			return hr;
		}

		wil::unique_handle hSharedHeap;
		hr = _bridgeDevice->CreateSharedHandle(
			_bridgeHeap.get(), nullptr, GENERIC_ALL, nullptr, hSharedHeap.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateSharedHandle 失败", hr);
			return hr;
		}

		hr = device->OpenSharedHandle(hSharedHeap.get(), IID_PPV_ARGS(&_sharedHeap));
		if (FAILED(hr)) {
			Logger::Get().ComError("OpenSharedHandle 失败", hr);
			return hr;
		}
	}

	for (uint32_t i = 0; i < frameCount; ++i) {
		_FrameCrossAdapterResourceSlot& curSlot = _crossAdapterSlots[i];

		HRESULT hr = _bridgeDevice->CreatePlacedResource(
			_bridgeHeap.get(),
			textureInfo.SizeInBytes * i,
			&textureDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&curSlot.bridgeResource)
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("CreatePlacedResource 失败", hr);
			return hr;
		}

		hr = device->CreatePlacedResource(
			_sharedHeap.get(),
			textureInfo.SizeInBytes * i,
			&textureDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&curSlot.sharedResource)
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("CreatePlacedResource 失败", hr);
			return hr;
		}
	}

	return S_OK;
}

// 部分使用 Kirikiri 引擎的游戏有着这样的架构: 游戏窗口并非顶级窗口，而是被一个零尺寸
// 的窗口所有。此时 Alt+Tab 列表中的窗口和任务栏图标实际上是所有者窗口，这会导致 WGC
// 捕获失败，需要特殊处理。
static bool IsKirikiriWindow(HWND hwndSrc) noexcept {
	// Win11 24H2 的某次更新修复了这个问题，保险起见从 25H2 开始不再使用 trick
	if (Win32Helper::GetOSVersion().Is25H2OrNewer()) {
		return false;
	}

	const HWND hwndOwner = GetWindowOwner(hwndSrc);
	if (!hwndOwner) {
		return false;
	}

	RECT ownerRect;
	if (!GetWindowRect(hwndOwner, &ownerRect)) {
		Logger::Get().Win32Error("GetWindowRect 失败");
		return false;
	}

	// 所有者窗口尺寸为零，而且是顶级窗口
	return ownerRect.left == ownerRect.right && ownerRect.top == ownerRect.bottom &&
		!GetWindowOwner(hwndOwner);
}

bool GraphicsCaptureFrameSource::_InitializeCaptureItem() noexcept {
	winrt::com_ptr<IDXGIDevice> dxgiDevice;
	HRESULT hr = _d3d11Device->QueryInterface<IDXGIDevice>(dxgiDevice.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("获取 IDXGIDevice 失败", hr);
		return false;
	}

	hr = CreateDirect3D11DeviceFromDXGIDevice(
		dxgiDevice.get(), (IInspectable**)winrt::put_abi(_wrappedDevice));
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateDirect3D11DeviceFromDXGIDevice 失败", hr);
		return false;
	}

	winrt::com_ptr<IGraphicsCaptureItemInterop> interop =
		winrt::try_get_activation_factory<winrt::GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
	if (!interop) {
		Logger::Get().Error("获取 IGraphicsCaptureItemInterop 失败");
		return false;
	}

	const HWND hwndSrc = ScalingWindow::Get().SrcHandle();

	const DWORD srcExStyle = GetWindowExStyle(hwndSrc);
	// WS_EX_APPWINDOW 样式使窗口始终在 Alt+Tab 列表中显示
	if (srcExStyle & WS_EX_APPWINDOW) {
		hr = interop->CreateForWindow(
			hwndSrc, winrt::guid_of<winrt::GraphicsCaptureItem>(), winrt::put_abi(_captureItem));
		if (FAILED(hr)) {
			Logger::Get().ComError("IGraphicsCaptureItemInterop::CreateForWindow 失败", hr);
			return false;
		}

		return true;
	}

	// 第一次尝试捕获。Kirikiri 窗口必定失败，无需尝试
	const bool isSrcKirikiri = IsKirikiriWindow(hwndSrc);
	if (isSrcKirikiri) {
		Logger::Get().Info("源窗口有零尺寸的所有者窗口");
	} else {
		hr = interop->CreateForWindow(
			hwndSrc, winrt::guid_of<winrt::GraphicsCaptureItem>(), winrt::put_abi(_captureItem));
		if (SUCCEEDED(hr)) {
			return true;
		} else {
			Logger::Get().ComError("IGraphicsCaptureItemInterop::CreateForWindow 失败", hr);
		}
	}

	// 添加 WS_EX_APPWINDOW 样式
	if (!SetWindowLongPtr(hwndSrc, GWL_EXSTYLE, srcExStyle | WS_EX_APPWINDOW)) {
		Logger::Get().Win32Error("SetWindowLongPtr 失败");
		return false;
	}

	Logger::Get().Info("已改变源窗口样式");
	_isSrcStyleChanged = true;

	// Kirikiri 窗口改变样式后所有者窗口和游戏窗口将同时出现在 Alt+Tab 列表和任务栏中。
	// 虽然所有窗口都会如此，但 Kirikiri 的特殊之处在于两个窗口的图标和标题相同，为了不
	// 引起困惑应隐藏所有者窗口的图标。
	if (isSrcKirikiri) {
		_taskbarList = winrt::try_create_instance<ITaskbarList>(CLSID_TaskbarList);
		if (_taskbarList) {
			hr = _taskbarList->HrInit();
			if (SUCCEEDED(hr)) {
				// 修正任务栏图标
				_taskbarList->DeleteTab(GetWindowOwner(hwndSrc));
				_taskbarList->AddTab(hwndSrc);

				// 修正 Alt+Tab 切换顺序
				if (GetForegroundWindow() == hwndSrc) {
					SetForegroundWindow(GetDesktopWindow());
					SetForegroundWindow(hwndSrc);
				}
			} else {
				Logger::Get().ComError("ITaskbarList::HrInit 失败", hr);
				_taskbarList = nullptr;
			}
		} else {
			Logger::Get().Error("创建 ITaskbarList 失败");
		}
	}

	// 再次尝试捕获
	hr = interop->CreateForWindow(
		hwndSrc, winrt::guid_of<winrt::GraphicsCaptureItem>(), winrt::put_abi(_captureItem));
	if (FAILED(hr)) {
		Logger::Get().ComError("IGraphicsCaptureItemInterop::CreateForWindow 失败", hr);

		if (_isSrcStyleChanged) {
			// 恢复源窗口样式
			SetWindowLongPtr(hwndSrc, GWL_EXSTYLE, srcExStyle);
			_isSrcStyleChanged = false;
		}

		return false;
	}

	return true;
}

void GraphicsCaptureFrameSource::_Direct3D11CaptureFramePool_FrameArrived(
	const winrt::Direct3D11CaptureFramePool& pool,
	const winrt::IInspectable&
) {
	winrt::Direct3D11CaptureFrame frame{ nullptr };
	SmallVector<RectU> dirtyRects;

	// 取最新帧
	while (true) {
		winrt::Direct3D11CaptureFrame nextFrame = pool.TryGetNextFrame();
		if (!nextFrame) {
			break;
		}

		frame = std::move(nextFrame);

		if (_isDirtyRegionSupported) {
			for (const winrt::RectInt32& dirtyRect : frame.DirtyRegions()) {
				RECT clipped = {
					std::max(dirtyRect.X, (int)_frameBox.left),
					std::max(dirtyRect.Y, (int)_frameBox.top),
					std::min(dirtyRect.X + dirtyRect.Width, (int)_frameBox.right),
					std::min(dirtyRect.Y + dirtyRect.Height, (int)_frameBox.bottom),
				};
				if (clipped.right <= clipped.left || clipped.bottom <= clipped.top) {
					continue;
				}

				dirtyRects.emplace_back((uint32_t)clipped.left, (uint32_t)clipped.top,
					(uint32_t)clipped.right, (uint32_t)clipped.bottom);
			}
		}
	}

	if (!frame || (_isDirtyRegionSupported && dirtyRects.empty())) {
		return;
	}

	{
		auto lk = _latestFrameLock.lock_exclusive();

		_latestFrame = std::move(frame);

		if (_isDirtyRegionSupported) {
			// 累积脏矩形
			if (_latestFrameDirtyRects.empty()) {
				_latestFrameDirtyRects = std::move(dirtyRects);
			} else {
				_latestFrameDirtyRects.append(dirtyRects);
			}
		}
		
#ifdef MP_DEBUG_INFO
		{
			auto debugLock = DEBUG_INFO.lock.lock_exclusive();

			if (!DEBUG_INFO.ctpCapturedFrame) {
				LARGE_INTEGER counter;
				QueryPerformanceCounter(&counter);
				DEBUG_INFO.ctpCaptureQPC = counter.QuadPart;

				DEBUG_INFO.ctpCapturedFrame = winrt::get_abi(_latestFrame);
			}
		}
#endif
	}

	// 唤起生产者线程
	PostThreadMessage(_producerThreadId.load(std::memory_order_relaxed), WM_NULL, 0, 0);
}

void GraphicsCaptureFrameSource::_DisableRoundCornerInWin11() noexcept {
	if (Win32Helper::GetOSVersion().IsWin10()) {
		return;
	}

	const HWND hwndSrc = ScalingWindow::Get().SrcHandle();

	int value = DWMWCP_DONOTROUND;
	HRESULT hr = DwmSetWindowAttribute(
		hwndSrc, DWMWA_WINDOW_CORNER_PREFERENCE, &value, sizeof(value));
	if (FAILED(hr)) {
		Logger::Get().ComError("禁用窗口圆角失败", hr);
		return;
	}

	_isRoundCornerDisabled = true;
}

HRESULT GraphicsCaptureFrameSource::_StartCapture() noexcept {
	assert(!_captureFramePool && !_captureSession);

	try {
		// 创建帧缓冲池。帧的尺寸为包含捕获区域的最小尺寸，既能降低显存压力，又使得检查重复帧时无需边界检查
		_captureFramePool = winrt::Direct3D11CaptureFramePool::CreateFreeThreaded(
			_wrappedDevice,
			_isScRGB ? winrt::DirectXPixelFormat::R16G16B16A16Float : winrt::DirectXPixelFormat::B8G8R8A8UIntNormalized,
			CalcCaptureFrameCount(),
			{ (int)_frameBox.right, (int)_frameBox.bottom }
		);

		_captureFramePool.FrameArrived(
			{ this, &GraphicsCaptureFrameSource::_Direct3D11CaptureFramePool_FrameArrived });

		_captureSession = _captureFramePool.CreateCaptureSession(_captureItem);

		// 禁止捕获光标。从 Win10 v2004 开始支持
		if (winrt::ApiInformation::IsPropertyPresent(
			winrt::name_of<winrt::GraphicsCaptureSession>(),
			L"IsCursorCaptureEnabled"
		)) {
			_captureSession.IsCursorCaptureEnabled(false);
		}

		// 不显示黄色边框，Win32 应用中无需请求权限。从 Win11 开始支持
		if (winrt::ApiInformation::IsPropertyPresent(
			winrt::name_of<winrt::GraphicsCaptureSession>(),
			L"IsBorderRequired"
		)) {
			_captureSession.IsBorderRequired(false);
		}

		// Win11 24H2 中必须设置 MinUpdateInterval 才能使捕获帧率超过 60FPS
		if (winrt::ApiInformation::IsPropertyPresent(
			winrt::name_of<winrt::GraphicsCaptureSession>(),
			L"MinUpdateInterval"
		)) {
			_captureSession.MinUpdateInterval(1ms);
		}

		_captureSession.StartCapture();
	} catch (const winrt::hresult_error& e) {
		Logger::Get().ComInfo(StrHelper::Concat("启动捕获失败: ", StrHelper::UTF16ToUTF8(e.message())), e.code());
		return e.code();
	}

	return S_OK;
}

// 调用前应等待 GPU 
void GraphicsCaptureFrameSource::_StopCapture() noexcept {
	assert(_captureFramePool && _captureSession);

	_captureSession.Close();
	_captureSession = nullptr;

	_captureFramePool.Close();
	_captureFramePool = nullptr;

	// 捕获已结束，可以安全操作 _latestFrame
	{
		auto lk = _latestFrameLock.lock_exclusive();
		_latestFrame = nullptr;
		_latestFrameDirtyRects.clear();
	}

	_captureFrameResourceTable.clear();

	for (_FrameResourceSlot& slot : _slots) {
		// output 将继续使用，直到重启捕获
		slot.captureFrame = nullptr;
	}

	if (_duplicateFrameChecker) {
		_duplicateFrameChecker->OnCaptureStopped();
	}
}

}
