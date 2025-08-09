#include "pch.h"
#include "GraphicsCaptureFrameSource.h"
#include "DeviceResources.h"
#include "DirectXHelper.h"
#include "Logger.h"
#include "ScalingWindow.h"
#include "StrHelper.h"
#include "Win32Helper.h"
#include <dwmapi.h>
#include <Windows.Graphics.DirectX.Direct3D11.interop.h>

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
		d3dSurface.try_as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>()
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

// 部分使用 Kirikiri 引擎的游戏有着这样的架构: 游戏窗口并非顶级窗口，而是被一个零尺寸
// 的窗口所有。此时 Alt+Tab 列表中的窗口和任务栏图标实际上是所有者窗口，这会导致 WGC
// 捕获失败。我们特殊处理这类窗口。
static bool IsKirikiriWindow(HWND hwndSrc) noexcept {
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

	const DWORD srcExStyle = GetWindowExStyle(hwndSrc);
	// WS_EX_APPWINDOW 样式使窗口始终在 Alt+Tab 列表中显示
	if (srcExStyle & WS_EX_APPWINDOW) {
		return _TryCreateGraphicsCaptureItem(interop);
	}

	const bool isSrcKirikiri = IsKirikiriWindow(hwndSrc);
	if (isSrcKirikiri) {
		Logger::Get().Info("源窗口有零尺寸的所有者窗口");
	} else {
		// 第一次尝试捕获。Kirikiri 窗口必定失败，无需尝试
		if (_TryCreateGraphicsCaptureItem(interop)) {
			return true;
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
			HRESULT hr = _taskbarList->HrInit();
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
			}
		} else {
			Logger::Get().Error("创建 ITaskbarList 失败");
		}
	}

	// 再次尝试捕获
	if (_TryCreateGraphicsCaptureItem(interop)) {
		return true;
	} else {
		if (_isSrcStyleChanged) {
			// 恢复源窗口样式
			SetWindowLongPtr(hwndSrc, GWL_EXSTYLE, srcExStyle);
		}
		return false;
	}
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

	// 还原源窗口样式
	if (_isSrcStyleChanged) {
		const DWORD srcExStyle = GetWindowExStyle(hwndSrc);
		SetWindowLongPtr(hwndSrc, GWL_EXSTYLE, srcExStyle & ~WS_EX_APPWINDOW);
	}

	// 还原 Kirikiri 窗口
	if (_taskbarList) {
		_taskbarList->DeleteTab(hwndSrc);
		_taskbarList->AddTab(GetWindowOwner(hwndSrc));

		if (GetForegroundWindow() == hwndSrc) {
			SetForegroundWindow(GetDesktopWindow());
			SetForegroundWindow(hwndSrc);
		}
	}
}

}
