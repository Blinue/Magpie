#include "pch.h"
#include "GraphicsCaptureFrameSource.h"
#include "StrUtils.h"
#include "Utils.h"
#include "DeviceResources.h"
#include "Logger.h"
#include <Windows.Graphics.DirectX.Direct3D11.interop.h>
#include "Win32Utils.h"
#include "DirectXHelper.h"
#include "ScalingOptions.h"

namespace winrt {
using namespace Windows::Graphics;
using namespace Windows::Graphics::Capture;
using namespace Windows::Graphics::DirectX;
using namespace Windows::Graphics::DirectX::Direct3D11;
}

namespace Magpie::Core {

bool GraphicsCaptureFrameSource::Initialize(HWND hwndSrc, HWND hwndScaling, const ScalingOptions& options, ID3D11Device5* d3dDevice) noexcept {
	if (!FrameSourceBase::Initialize(hwndSrc, hwndScaling, options, d3dDevice)) {
		Logger::Get().Error("初始化 FrameSourceBase 失败");
		return false;
	}

	HRESULT hr;

	winrt::com_ptr<IGraphicsCaptureItemInterop> interop;
	try {
		if (!winrt::GraphicsCaptureSession::IsSupported()) {
			Logger::Get().Error("当前不支持 WinRT 捕获");
			return false;
		}

		winrt::com_ptr<IDXGIDevice> dxgiDevice;
		_d3dDevice->QueryInterface<IDXGIDevice>(dxgiDevice.put());

		hr = CreateDirect3D11DeviceFromDXGIDevice(
			dxgiDevice.get(),
			reinterpret_cast<::IInspectable**>(winrt::put_abi(_wrappedD3DDevice))
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("创建 IDirect3DDevice 失败", hr);
			return false;
		}

		// 从窗口句柄获取 GraphicsCaptureItem
		interop = winrt::get_activation_factory<winrt::GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
		if (!interop) {
			Logger::Get().Error("获取 IGraphicsCaptureItemInterop 失败");
			return false;
		}
	} catch (const winrt::hresult_error& e) {
		Logger::Get().Error(StrUtils::Concat("初始化 WinRT 失败：", StrUtils::UTF16ToUTF8(e.message())));
		return false;
	}

	if (!_CaptureWindow(interop.get(), options)) {
		Logger::Get().Info("窗口捕获失败，回落到屏幕捕获");

		if (_CaptureMonitor(interop.get(), options, hwndScaling)) {
			_isScreenCapture = true;
		} else {
			Logger::Get().Error("屏幕捕获失败");
			return false;
		}
	}

	_output = DirectXHelper::CreateTexture2D(
		d3dDevice,
		DXGI_FORMAT_B8G8R8A8_UNORM,
		_frameBox.right - _frameBox.left,
		_frameBox.bottom - _frameBox.top,
		D3D11_BIND_SHADER_RESOURCE
	);
	if (!_output) {
		Logger::Get().Error("创建纹理失败");
		return false;
	}

	if (!StartCapture()) {
		Logger::Get().Error("_StartCapture 失败");
		return false;
	}

	Logger::Get().Info("GraphicsCaptureFrameSource 初始化完成");
	return true;
}

FrameSourceBase::UpdateState GraphicsCaptureFrameSource::Update() noexcept {
	if (!_captureSession) {
		return UpdateState::Waiting;
	}

	winrt::Direct3D11CaptureFrame frame = _captureFramePool.TryGetNextFrame();
	if (!frame) {
		// 因为已通过 FrameArrived 注册回调，所以每当有新帧时会有新消息到达
		return UpdateState::Waiting;
	}

	// 从帧获取 IDXGISurface
	winrt::IDirect3DSurface d3dSurface = frame.Surface();

	winrt::com_ptr<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess> dxgiInterfaceAccess(
		d3dSurface.as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>()
	);

	winrt::com_ptr<ID3D11Texture2D> withFrame;
	HRESULT hr = dxgiInterfaceAccess->GetInterface(IID_PPV_ARGS(&withFrame));
	if (FAILED(hr)) {
		Logger::Get().ComError("从获取 IDirect3DSurface 获取 ID3D11Texture2D 失败", hr);
		return UpdateState::Error;
	}

	winrt::com_ptr<ID3D11DeviceContext> d3dDC;
	_d3dDevice->GetImmediateContext(d3dDC.put());

	d3dDC->CopySubresourceRegion(_output.get(), 0, 0, 0, 0, withFrame.get(), 0, &_frameBox);

	frame.Close();
	return UpdateState::NewFrame;
}

bool GraphicsCaptureFrameSource::_CaptureWindow(IGraphicsCaptureItemInterop* interop, const ScalingOptions& options) {
	// DwmGetWindowAttribute 和 Graphics.Capture 无法应用于子窗口

	// 包含边框的窗口尺寸
	RECT srcFrameBounds{};
	HRESULT hr = DwmGetWindowAttribute(_hwndSrc,
		DWMWA_EXTENDED_FRAME_BOUNDS, &srcFrameBounds, sizeof(srcFrameBounds));
	if (FAILED(hr)) {
		Logger::Get().ComError("DwmGetWindowAttribute 失败", hr);
		return false;
	}

	RECT srcFrameRect = _GetSrcFrameRect(options.cropping, options.IsCaptureTitleBar());
	if (srcFrameRect == RECT{}) {
		Logger::Get().Error("_GetSrcFrameRect 失败");
		return false;
	}

	// 在源窗口存在 DPI 缩放时有时会有一像素的偏移（取决于窗口在屏幕上的位置）
	// 可能是 DwmGetWindowAttribute 的 bug
	_frameBox = {
		UINT(srcFrameRect.left - srcFrameBounds.left),
		UINT(srcFrameRect.top - srcFrameBounds.top),
		0,
		UINT(srcFrameRect.right - srcFrameBounds.left),
		UINT(srcFrameRect.bottom - srcFrameBounds.top),
		1
	};

	if (_TryCreateGraphicsCaptureItem(interop, _hwndSrc)) {
		return true;
	}

	// 尝试设置源窗口样式，因为 WGC 只能捕获位于 Alt+Tab 列表中的窗口
	LONG_PTR srcExStyle = GetWindowLongPtr(_hwndSrc, GWL_EXSTYLE);
	if ((srcExStyle & WS_EX_APPWINDOW) == 0) {
		// 添加 WS_EX_APPWINDOW 样式，确保源窗口可被 Alt+Tab 选中
		if (SetWindowLongPtr(_hwndSrc, GWL_EXSTYLE, srcExStyle | WS_EX_APPWINDOW)) {
			Logger::Get().Info("已改变源窗口样式");
			_originalSrcExStyle = srcExStyle;

			if (_TryCreateGraphicsCaptureItem(interop, _hwndSrc)) {
				_RemoveOwnerFromAltTabList(_hwndSrc);
				return true;
			}
		} else {
			Logger::Get().Win32Error("SetWindowLongPtr 失败");
		}
	}

	// 如果窗口使用 ITaskbarList 隐藏了任务栏图标也不会出现在 Alt+Tab 列表。这种情况很罕见
	_taskbarList = winrt::try_create_instance<ITaskbarList>(CLSID_TaskbarList);
	if (_taskbarList && SUCCEEDED(_taskbarList->HrInit())) {
		hr = _taskbarList->AddTab(_hwndSrc);
		if (SUCCEEDED(hr)) {
			Logger::Get().Info("已添加任务栏图标");

			if (_TryCreateGraphicsCaptureItem(interop, _hwndSrc)) {
				_RemoveOwnerFromAltTabList(_hwndSrc);
				return true;
			}
		} else {
			_taskbarList = nullptr;
			Logger::Get().Error("ITaskbarList::AddTab 失败");
		}
	} else {
		_taskbarList = nullptr;
		Logger::Get().Error("创建 ITaskbarList 失败");
	}

	// 上面的尝试失败了则还原更改
	if (_taskbarList) {
		_taskbarList->DeleteTab(_hwndSrc);
		_taskbarList = nullptr;
	}
	if (_originalSrcExStyle) {
		// 首先还原所有者窗口的样式以压制任务栏的动画
		if (_originalOwnerExStyle) {
			SetWindowLongPtr(GetWindowOwner(_hwndSrc), GWL_EXSTYLE, _originalOwnerExStyle);
			_originalOwnerExStyle = 0;
		}

		SetWindowLongPtr(_hwndSrc, GWL_EXSTYLE, _originalSrcExStyle);
		_originalSrcExStyle = 0;
	}

	return false;
}

bool GraphicsCaptureFrameSource::_TryCreateGraphicsCaptureItem(IGraphicsCaptureItemInterop* interop, HWND hwndSrc) noexcept {
	try {
		HRESULT hr = interop->CreateForWindow(
			hwndSrc,
			winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(),
			winrt::put_abi(_captureItem)
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("创建 GraphicsCaptureItem 失败", hr);
			return false;
		}
	} catch (const winrt::hresult_error& e) {
		Logger::Get().Info(StrUtils::Concat("源窗口无法使用窗口捕获：", StrUtils::UTF16ToUTF8(e.message())));
		return false;
	}

	return true;
}

// 部分使用 Kirikiri 引擎的游戏有着这样的架构：游戏窗口并非根窗口，它被一个尺寸为 0 的窗口
// 所有。此时 Alt+Tab 列表中的窗口和任务栏图标实际上是所有者窗口，这会导致 WGC 捕获游戏窗
// 口时失败。_CaptureWindow 在初次捕获失败后会将 WS_EX_APPWINDOW 样式添加到游戏窗口，这
// 可以工作，但也导致所有者窗口和游戏窗口同时出现在 Alt+Tab 列表中，引起用户的困惑。
// 
// 此函数检测这种情况并改变所有者窗口的样式将它从 Alt+Tab 列表中移除。
void GraphicsCaptureFrameSource::_RemoveOwnerFromAltTabList(HWND hwndSrc) noexcept {
	HWND hwndOwner = GetWindowOwner(hwndSrc);
	if (!hwndOwner) {
		return;
	}

	RECT ownerRect{};
	if (!GetWindowRect(hwndOwner, &ownerRect)) {
		Logger::Get().Win32Error("GetWindowRect 失败");
		return;
	}

	// 检查所有者窗口尺寸
	if (ownerRect.right != ownerRect.left || ownerRect.bottom != ownerRect.top) {
		return;
	}

	LONG_PTR ownerExStyle = GetWindowLongPtr(hwndOwner, GWL_EXSTYLE);
	if (ownerExStyle == 0) {
		Logger::Get().Win32Error("GetWindowLongPtr 失败");
		return;
	}

	if (!SetWindowLongPtr(hwndOwner, GWL_EXSTYLE, ownerExStyle | WS_EX_TOOLWINDOW)) {
		Logger::Get().Win32Error("SetWindowLongPtr 失败");
		return;
	}

	_originalOwnerExStyle = ownerExStyle;
}

bool GraphicsCaptureFrameSource::_CaptureMonitor(IGraphicsCaptureItemInterop* interop, const ScalingOptions& options, HWND hwndScaling) {
	// Win10 无法隐藏黄色边框，因此只在 Win11 中回落到屏幕捕获
	if (!Win32Utils::GetOSVersion().IsWin11()) {
		Logger::Get().Error("无法使用屏幕捕获");
		return false;
	}

	// 使全屏窗口无法被捕获到
	// WDA_EXCLUDEFROMCAPTURE 只在 Win10 20H1 及更新版本中可用
	if (!SetWindowDisplayAffinity(hwndScaling, WDA_EXCLUDEFROMCAPTURE)) {
		Logger::Get().Win32Error("SetWindowDisplayAffinity 失败");
		return false;
	}

	HMONITOR hMonitor = MonitorFromWindow(_hwndSrc, MONITOR_DEFAULTTONEAREST);
	if (!hMonitor) {
		Logger::Get().Win32Error("MonitorFromWindow 失败");
		return false;
	}

	MONITORINFO mi{};
	mi.cbSize = sizeof(mi);
	if (!GetMonitorInfo(hMonitor, &mi)) {
		Logger::Get().Win32Error("GetMonitorInfo 失败");
		return false;
	}

	// 放在屏幕左上角而不是中间可以提高帧率，这里是为了和 DesktopDuplication 保持一致
	if (!_CenterWindowIfNecessary(_hwndSrc, mi.rcWork)) {
		Logger::Get().Error("居中源窗口失败");
		return false;
	}

	RECT srcFrameRect = _GetSrcFrameRect(options.cropping, options.IsCaptureTitleBar());
	if (srcFrameRect == RECT{}) {
		Logger::Get().Error("_GetSrcFrameRect 失败");
		return false;
	}

	_frameBox = {
		UINT(srcFrameRect.left - mi.rcMonitor.left),
		UINT(srcFrameRect.top - mi.rcMonitor.top),
		0,
		UINT(srcFrameRect.right - mi.rcMonitor.left),
		UINT(srcFrameRect.bottom - mi.rcMonitor.top),
		1
	};

	try {
		HRESULT hr = interop->CreateForMonitor(
			hMonitor,
			winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(),
			winrt::put_abi(_captureItem)
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("创建 GraphicsCaptureItem 失败", hr);
			return false;
		}
	} catch (const winrt::hresult_error& e) {
		Logger::Get().Info(StrUtils::Concat("捕获屏幕失败：", StrUtils::UTF16ToUTF8(e.message())));
		return false;
	}

	return true;
}

bool GraphicsCaptureFrameSource::StartCapture() {
	if (_captureSession) {
		return true;
	}

	try {
		// 创建帧缓冲池
		// 帧的尺寸和 _captureItem.Size() 不同
		_captureFramePool = winrt::Direct3D11CaptureFramePool::Create(
			_wrappedD3DDevice,
			winrt::DirectXPixelFormat::B8G8R8A8UIntNormalized,
			1,	// 帧的缓存数量
			{ (int)_frameBox.right, (int)_frameBox.bottom } // 帧的尺寸为包含源窗口的最小尺寸
		);

		// 注册回调是为了确保每当有新的帧时会向当前线程发送消息
		// 回调中什么也不做
		_captureFramePool.FrameArrived([](const auto&, const auto&) {});

		_captureSession = _captureFramePool.CreateCaptureSession(_captureItem);

		// 不捕获光标
		if (winrt::ApiInformation::IsPropertyPresent(
			winrt::name_of<winrt::GraphicsCaptureSession>(),
			L"IsCursorCaptureEnabled"
			)) {
				// 从 v2004 开始提供
			_captureSession.IsCursorCaptureEnabled(false);
		}

		// 不显示黄色边框
		if (winrt::ApiInformation::IsPropertyPresent(
			winrt::name_of<winrt::GraphicsCaptureSession>(),
			L"IsBorderRequired"
			)) {
				// 从 Win10 v2104 开始提供
				// Win32 应用中无需请求权限
			_captureSession.IsBorderRequired(false);
		}

		_captureSession.StartCapture();
	} catch (const winrt::hresult_error& e) {
		Logger::Get().Info(StrUtils::Concat("Graphics Capture 失败：", StrUtils::UTF16ToUTF8(e.message())));
		return false;
	}

	return true;
}

void GraphicsCaptureFrameSource::StopCapture() {
	if (_captureSession) {
		_captureSession.Close();
		_captureSession = nullptr;
	}
	if (_captureFramePool) {
		_captureFramePool.Close();
		_captureFramePool = nullptr;
	}
}

GraphicsCaptureFrameSource::~GraphicsCaptureFrameSource() {
	StopCapture();

	if (_taskbarList) {
		_taskbarList->DeleteTab(_hwndSrc);
	}

	// 还原源窗口样式
	if (_originalSrcExStyle) {
		// 首先还原所有者窗口的样式以压制任务栏的动画
		if (_originalOwnerExStyle) {
			SetWindowLongPtr(GetWindowOwner(_hwndSrc), GWL_EXSTYLE, _originalOwnerExStyle);
		}

		SetWindowLongPtr(_hwndSrc, GWL_EXSTYLE, _originalSrcExStyle);
	}
}

}
