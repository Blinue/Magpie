#include "pch.h"
#include "GraphicsCaptureFrameSource.h"
#include "StrHelper.h"
#include "DeviceResources.h"
#include "Logger.h"
#include <Windows.Graphics.DirectX.Direct3D11.interop.h>
#include "Win32Helper.h"
#include "DirectXHelper.h"
#include "ScalingOptions.h"
#include "ScalingWindow.h"
#include <dwmapi.h>

namespace winrt {
using namespace Windows::Graphics;
using namespace Windows::Graphics::Capture;
using namespace Windows::Graphics::DirectX;
using namespace Windows::Graphics::DirectX::Direct3D11;
}

namespace Magpie {

bool GraphicsCaptureFrameSource::_Initialize() noexcept {
	ID3D11Device5* d3dDevice = _deviceResources->GetD3DDevice();

	HRESULT hr;

	winrt::com_ptr<IGraphicsCaptureItemInterop> interop;
	try {
		if (!winrt::GraphicsCaptureSession::IsSupported()) {
			Logger::Get().Error("当前不支持 WinRT 捕获");
			return false;
		}

		winrt::com_ptr<IDXGIDevice> dxgiDevice;
		d3dDevice->QueryInterface<IDXGIDevice>(dxgiDevice.put());

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
		Logger::Get().Error(StrHelper::Concat("初始化 WinRT 失败: ", StrHelper::UTF16ToUTF8(e.message())));
		return false;
	}

	if (!_CaptureWindow(interop.get())) {
		Logger::Get().Error("窗口捕获失败");
		return false;
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

	Logger::Get().Info("GraphicsCaptureFrameSource 初始化完成");
	return true;
}

bool GraphicsCaptureFrameSource::Start() noexcept {
	_DisableRoundCornerInWin11();
	return _StartCapture();
}

FrameSourceState GraphicsCaptureFrameSource::_Update() noexcept {
	if (!_captureSession) {
		return FrameSourceState::Waiting;
	}

	winrt::Direct3D11CaptureFrame frame = _captureFramePool.TryGetNextFrame();
	if (!frame) {
		// 因为已通过 FrameArrived 注册回调，所以每当有新帧时会有新消息到达
		return FrameSourceState::Waiting;
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
		return FrameSourceState::Error;
	}

	_deviceResources->GetD3DDC()->CopySubresourceRegion(_output.get(), 0, 0, 0, 0, withFrame.get(), 0, &_frameBox);

	return FrameSourceState::NewFrame;
}

void GraphicsCaptureFrameSource::OnCursorVisibilityChanged(bool isVisible, bool onDestory) noexcept {
	// 显示光标时必须重启捕获
	if (isVisible) {
		_StopCapture();
		
		if (onDestory) {
			// FIXME: 这里尝试修复拖动窗口时光标不显示的问题，但有些环境下不起作用
			SystemParametersInfo(SPI_SETCURSORS, 0, nullptr, 0);
		} else {
			_StartCapture();
		}
	}
}

static bool CalcWindowCapturedFrameBounds(HWND hWnd, RECT& rect) noexcept {
	// Graphics Capture 的捕获区域没有文档记录，这里的计算是我实验了多种窗口后得出的，
	// 高度依赖实现细节，未来可能会失效。
	// Win10 和 Win11 24H2 开始捕获区域为 extended frame bounds；Win11 24H2 前
	// DwmGetWindowAttribute 对最大化的窗口返回值和 Win10 不同，可能是 OS 的 bug，
	// 应进一步处理。
	const auto& srcTracker = ScalingWindow::Get().SrcTracker();
	rect = srcTracker.WindowFrameRect();
	
	if (!srcTracker.IsZoomed() ||
		Win32Helper::GetOSVersion().IsWin10() ||
		Win32Helper::GetOSVersion().Is24H2OrNewer())
	{
		return true;
	}

	// 如果窗口禁用了非客户区域绘制则捕获区域为 extended frame bounds
	BOOL hasBorder = TRUE;
	HRESULT hr = DwmGetWindowAttribute(hWnd, DWMWA_NCRENDERING_ENABLED, &hasBorder, sizeof(hasBorder));
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

bool GraphicsCaptureFrameSource::_CaptureWindow(IGraphicsCaptureItemInterop* interop) noexcept {
	const SrcTracker& srcTracker = ScalingWindow::Get().SrcTracker();
	const HWND hwndSrc = srcTracker.Handle();
	const RECT& srcRect = srcTracker.SrcRect();

	RECT frameBounds;
	if (!CalcWindowCapturedFrameBounds(hwndSrc, frameBounds)) {
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

	if (_TryCreateGraphicsCaptureItem(interop)) {
		return true;
	}

	// 尝试设置源窗口样式，因为 WGC 只能捕获位于 Alt+Tab 列表中的窗口
	LONG_PTR srcExStyle = GetWindowLongPtr(hwndSrc, GWL_EXSTYLE);
	if ((srcExStyle & WS_EX_APPWINDOW) == 0) {
		// 添加 WS_EX_APPWINDOW 样式，确保源窗口可被 Alt+Tab 选中
		if (SetWindowLongPtr(hwndSrc, GWL_EXSTYLE, srcExStyle | WS_EX_APPWINDOW)) {
			Logger::Get().Info("已改变源窗口样式");
			_originalSrcExStyle = srcExStyle;

			if (_TryCreateGraphicsCaptureItem(interop)) {
				_RemoveOwnerFromAltTabList(hwndSrc);
				return true;
			}
		} else {
			Logger::Get().Win32Error("SetWindowLongPtr 失败");
		}
	}

	// 如果窗口使用 ITaskbarList 隐藏了任务栏图标也不会出现在 Alt+Tab 列表。这种情况很罕见
	_taskbarList = winrt::try_create_instance<ITaskbarList>(CLSID_TaskbarList);
	if (_taskbarList && SUCCEEDED(_taskbarList->HrInit())) {
		HRESULT hr = _taskbarList->AddTab(hwndSrc);
		if (SUCCEEDED(hr)) {
			Logger::Get().Info("已添加任务栏图标");

			if (_TryCreateGraphicsCaptureItem(interop)) {
				_RemoveOwnerFromAltTabList(hwndSrc);
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
		_taskbarList->DeleteTab(hwndSrc);
		_taskbarList = nullptr;
	}
	if (_originalSrcExStyle) {
		// 首先还原所有者窗口的样式以压制任务栏的动画
		if (_originalOwnerExStyle) {
			SetWindowLongPtr(GetWindowOwner(hwndSrc), GWL_EXSTYLE, _originalOwnerExStyle);
			_originalOwnerExStyle = 0;
		}

		SetWindowLongPtr(hwndSrc, GWL_EXSTYLE, _originalSrcExStyle);
		_originalSrcExStyle = 0;
	}

	return false;
}

bool GraphicsCaptureFrameSource::_TryCreateGraphicsCaptureItem(IGraphicsCaptureItemInterop* interop) noexcept {
	try {
		HRESULT hr = interop->CreateForWindow(
			ScalingWindow::Get().SrcTracker().Handle(),
			winrt::guid_of<winrt::GraphicsCaptureItem>(),
			winrt::put_abi(_captureItem)
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("创建 GraphicsCaptureItem 失败", hr);
			return false;
		}
	} catch (const winrt::hresult_error& e) {
		Logger::Get().Info(StrHelper::Concat("源窗口无法使用窗口捕获: ", StrHelper::UTF16ToUTF8(e.message())));
		return false;
	}

	return true;
}

// 部分使用 Kirikiri 引擎的游戏有着这样的架构: 游戏窗口并非根窗口，它被一个尺寸为 0 的窗口
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

bool GraphicsCaptureFrameSource::_StartCapture() noexcept {
	if (_captureSession) {
		return true;
	}

	try {
		// 创建帧缓冲池。帧的尺寸和 _captureItem.Size() 不同
		_captureFramePool = winrt::Direct3D11CaptureFramePool::Create(
			_wrappedD3DDevice,
			winrt::DirectXPixelFormat::B8G8R8A8UIntNormalized,
			1,	// 帧的缓存数量
			{ (int)_frameBox.right, (int)_frameBox.bottom } // 帧的尺寸为包含源窗口的最小尺寸
		);

		// 注册回调是为了确保每当有新的帧时会向当前线程发送消息，回调中什么也不做
		_captureFramePool.FrameArrived([](const auto&, const auto&) {});

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
		Logger::Get().Info(StrHelper::Concat("Graphics Capture 失败: ", StrHelper::UTF16ToUTF8(e.message())));
		return false;
	}

	return true;
}

void GraphicsCaptureFrameSource::_StopCapture() noexcept {
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
	_StopCapture();

	const HWND hwndSrc = ScalingWindow::Get().SrcTracker().Handle();

	if (_taskbarList) {
		_taskbarList->DeleteTab(hwndSrc);
	}

	// 还原源窗口样式
	if (_originalSrcExStyle) {
		// 首先还原所有者窗口的样式以压制任务栏的动画
		if (_originalOwnerExStyle) {
			SetWindowLongPtr(GetWindowOwner(hwndSrc), GWL_EXSTYLE, _originalOwnerExStyle);
		}

		SetWindowLongPtr(hwndSrc, GWL_EXSTYLE, _originalSrcExStyle);
	}
}

}
