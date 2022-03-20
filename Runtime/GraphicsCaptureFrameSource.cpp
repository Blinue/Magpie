#include "pch.h"
#include "GraphicsCaptureFrameSource.h"
#include "App.h"
#include "StrUtils.h"
#include <Windows.Graphics.DirectX.Direct3D11.interop.h>
#include <winrt/Windows.Foundation.Metadata.h>
#include "Utils.h"
#include "DeviceResources.h"
#include "Logger.h"


namespace winrt {
using namespace Windows::Foundation::Metadata;
}


bool GraphicsCaptureFrameSource::Initialize() {
	if (!FrameSourceBase::Initialize()) {
		Logger::Get().Error("初始化 FrameSourceBase 失败");
		return false;
	}

	App::Get().SetErrorMsg(ErrorMessages::FAILED_TO_CAPTURE);

	// 只在 Win10 1903 及更新版本中可用
	const RTL_OSVERSIONINFOW& version = Utils::GetOSVersion();
	if (Utils::CompareVersion(version.dwMajorVersion, version.dwMinorVersion, version.dwBuildNumber, 10, 0, 18362) < 0) {
		Logger::Get().Error("当前操作系统无法使用 Graphics Capture");
		return false;
	}

	HRESULT hr;
	
	winrt::com_ptr<IGraphicsCaptureItemInterop> interop;
	try {
		if (!winrt::ApiInformation::IsTypePresent(L"Windows.Graphics.Capture.GraphicsCaptureSession")) {
			Logger::Get().Error("不存在 GraphicsCaptureSession API");
			return false;
		}
		if (!winrt::GraphicsCaptureSession::IsSupported()) {
			Logger::Get().Error("当前不支持 WinRT 捕获");
			return false;
		}

		hr = CreateDirect3D11DeviceFromDXGIDevice(
			App::Get().GetDeviceResources().GetDXGIDevice(),
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

	// 有两层回落：
	// 1. 首先使用常规的窗口捕获
	// 2. 如果失败，尝试设置源窗口样式，因为 WGC 只能捕获位于 Alt+Tab 列表中的窗口
	// 3. 如果再次失败，改为使用屏幕捕获
	if (!_CaptureFromWindow(interop.get())) {
		Logger::Get().Info("窗口捕获失败，尝试设置源窗口样式");

		if (!_CaptureFromStyledWindow(interop.get())) {
			Logger::Get().Info("窗口捕获失败，尝试使用屏幕捕获");

			if (!_CaptureFromMonitor(interop.get())) {
				Logger::Get().Error("屏幕捕获失败");
				return false;
			} else {
				_isScreenCapture = true;
			}
		}
	}

	try {
		// 创建帧缓冲池
		// 帧的尺寸和 _captureItem.Size() 不同
		_captureFramePool = winrt::Direct3D11CaptureFramePool::CreateFreeThreaded(
			_wrappedD3DDevice,
			winrt::DirectXPixelFormat::B8G8R8A8UIntNormalized,
			1,	// 帧的缓存数量
			{ (int)_frameBox.right, (int)_frameBox.bottom } // 帧的尺寸为包含源窗口的最小尺寸
		);

		_frameArrived = _captureFramePool.FrameArrived(
			winrt::auto_revoke,
			{ this, &GraphicsCaptureFrameSource::_OnFrameArrived }
		);

		_captureSession = _captureFramePool.CreateCaptureSession(_captureItem);

		// 不捕获光标
		if (winrt::ApiInformation::IsPropertyPresent(
			L"Windows.Graphics.Capture.GraphicsCaptureSession",
			L"IsCursorCaptureEnabled"
		)) {
			// 从 v2004 开始提供
			_captureSession.IsCursorCaptureEnabled(false);
		} else {
			Logger::Get().Info("当前系统无 IsCursorCaptureEnabled API");
		}

		// 不显示黄色边框
		if (winrt::ApiInformation::IsPropertyPresent(
			L"Windows.Graphics.Capture.GraphicsCaptureSession",
			L"IsBorderRequired"
		)) {
			// 从 Win10 v2104 开始提供
			// 先请求权限
			auto status = winrt::GraphicsCaptureAccess::RequestAccessAsync(winrt::GraphicsCaptureAccessKind::Borderless).get();
			if (status == decltype(status)::Allowed) {
				_captureSession.IsBorderRequired(false);
			} else {
				Logger::Get().Info("IsCursorCaptureEnabled 失败");
			}
		} else {
			Logger::Get().Info("当前系统无 IsBorderRequired API");
		}

		// 开始捕获
		_captureSession.StartCapture();
	} catch (const winrt::hresult_error& e) {
		Logger::Get().Info(StrUtils::Concat("Graphics Capture 失败：", StrUtils::UTF16ToUTF8(e.message())));
		return false;
	}

	_output = App::Get().GetDeviceResources().CreateTexture2D(
		DXGI_FORMAT_B8G8R8A8_UNORM,
		_frameBox.right - _frameBox.left,
		_frameBox.bottom - _frameBox.top,
		D3D11_BIND_SHADER_RESOURCE
	);
	if (!_output) {
		Logger::Get().Error("创建纹理失败");
		return false;
	}

	InitializeConditionVariable(&_cv);

	App::Get().SetErrorMsg(ErrorMessages::GENERIC);
	Logger::Get().Info("GraphicsCaptureFrameSource 初始化完成");
	return true;
}

FrameSourceBase::UpdateState GraphicsCaptureFrameSource::Update() {
	// 每次睡眠 1 毫秒等待新帧到达，防止 CPU 占用过高
	BOOL update = FALSE;

	{
		std::scoped_lock lk(_cs);
		if (!_newFrameArrived) {
			SleepConditionVariableCS(&_cv, _cs.get(), 1);
		}

		if (_newFrameArrived) {
			_newFrameArrived = false;
			update = TRUE;
		}
	}

	if (update) {
		winrt::Direct3D11CaptureFrame frame = _captureFramePool.TryGetNextFrame();
		if (!frame) {
			// 缓冲池没有帧，不应发生此情况
			assert(false);

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

		App::Get().GetDeviceResources().GetD3DDC()
			->CopySubresourceRegion(_output.get(), 0, 0, 0, 0, withFrame.get(), 0, &_frameBox);

		return UpdateState::NewFrame;
	} else {
		return UpdateState::Waiting;
	}
}

bool GraphicsCaptureFrameSource::_CaptureFromWindow(IGraphicsCaptureItemInterop* interop) {
	// DwmGetWindowAttribute 和 Graphics.Capture 无法应用于子窗口
	HWND hwndSrc = App::Get().GetHwndSrc();

	// 包含边框的窗口尺寸
	RECT srcRect{};
	if (!Utils::GetWindowFrameRect(hwndSrc, srcRect)) {
		Logger::Get().Error("GetWindowFrameRect 失败");
		return false;
	}

	if (!_UpdateSrcFrameRect()) {
		Logger::Get().Error("UpdateSrcFrameRect 失败");
		return false;
	}

	// 在源窗口存在 DPI 缩放时有时会有一像素的偏移（取决于窗口在屏幕上的位置）
	// 可能是 DwmGetWindowAttribute 的 bug
	_frameBox = {
		UINT(_srcFrameRect.left - srcRect.left),
		UINT(_srcFrameRect.top - srcRect.top),
		0,
		UINT(_srcFrameRect.right - srcRect.left),
		UINT(_srcFrameRect.bottom - srcRect.top),
		1
	};

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

bool GraphicsCaptureFrameSource::_CaptureFromStyledWindow(IGraphicsCaptureItemInterop* interop) {
	HWND hwndSrc = App::Get().GetHwndSrc();

	_srcWndStyle = GetWindowLongPtr(hwndSrc, GWL_EXSTYLE);
	if (_srcWndStyle == 0) {
		Logger::Get().Win32Error("GetWindowLongPtr 失败");
		return false;
	}

	// 删除 WS_EX_TOOLWINDOW，添加 WS_EX_APPWINDOW 样式，确保源窗口可被 Alt+Tab 选中
	LONG_PTR newStyle = (_srcWndStyle & !WS_EX_TOOLWINDOW) | WS_EX_APPWINDOW;

	if (_srcWndStyle == newStyle) {
		// 如果源窗口已经可被 Alt+Tab 选中，则回落到屏幕捕获
		Logger::Get().Info("源窗口无需改变样式");
		return false;
	}

	if (!SetWindowLongPtr(hwndSrc, GWL_EXSTYLE, newStyle)) {
		Logger::Get().Win32Error("SetWindowLongPtr 失败");
		return false;
	}

	if (!_CaptureFromWindow(interop)) {
		Logger::Get().Error("改变样式后捕获窗口失败");
		// 还原源窗口样式
		SetWindowLongPtr(hwndSrc, GWL_EXSTYLE, _srcWndStyle);
		_srcWndStyle = 0;
		return false;
	}

	return true;
}

bool GraphicsCaptureFrameSource::_CaptureFromMonitor(IGraphicsCaptureItemInterop* interop) {
	// WDA_EXCLUDEFROMCAPTURE 只在 Win10 v2004 及更新版本中可用
	const RTL_OSVERSIONINFOW& version = Utils::GetOSVersion();
	if (Utils::CompareVersion(version.dwMajorVersion, version.dwMinorVersion, version.dwBuildNumber, 10, 0, 19041) < 0) {
		Logger::Get().Error("当前操作系统无法使用全屏捕获");
		return false;
	}

	// 使全屏窗口无法被捕获到
	if (!SetWindowDisplayAffinity(App::Get().GetHwndHost(), WDA_EXCLUDEFROMCAPTURE)) {
		Logger::Get().Win32Error("SetWindowDisplayAffinity 失败");
		return false;
	}

	HWND hwndSrc = App::Get().GetHwndSrc();
	HMONITOR hMonitor = MonitorFromWindow(hwndSrc, MONITOR_DEFAULTTONEAREST);
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
	if (!_CenterWindowIfNecessary(hwndSrc, mi.rcWork)) {
		Logger::Get().Error("居中源窗口失败");
		return false;
	}

	if (!_UpdateSrcFrameRect()) {
		Logger::Get().Error("UpdateSrcFrameRect 失败");
		return false;
	}

	_frameBox = {
		UINT(_srcFrameRect.left - mi.rcMonitor.left),
		UINT(_srcFrameRect.top - mi.rcMonitor.top),
		0,
		UINT(_srcFrameRect.right - mi.rcMonitor.left),
		UINT(_srcFrameRect.bottom - mi.rcMonitor.top),
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

void GraphicsCaptureFrameSource::_OnFrameArrived(winrt::Direct3D11CaptureFramePool const&, winrt::IInspectable const&) {
	// 更改标志，如果主线程正在等待，唤醒主线程
	{
		std::scoped_lock lk(_cs);
		_newFrameArrived = true;
	}

	WakeConditionVariable(&_cv);
}

GraphicsCaptureFrameSource::~GraphicsCaptureFrameSource() {
	if (_frameArrived) {
		_frameArrived.revoke();
	}
	if (_captureSession) {
		_captureSession.Close();
	}
	if (_captureFramePool) {
		_captureFramePool.Close();
	}

	// 还原源窗口样式
	if (_srcWndStyle) {
		SetWindowLongPtr(App::Get().GetHwndSrc(), GWL_EXSTYLE, _srcWndStyle);
	}
}
