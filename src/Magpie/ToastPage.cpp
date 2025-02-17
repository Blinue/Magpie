#include "pch.h"
#include "ToastPage.h"
#if __has_include("ToastPage.g.cpp")
#include "ToastPage.g.cpp"
#endif
#include "Win32Helper.h"
#include "IconHelper.h"
#include "LocalizationService.h"
#include "XamlHelper.h"
#include "App.h"
#include <dwmapi.h>

using namespace ::Magpie;
using namespace winrt::Magpie::implementation;
using namespace winrt;
using namespace Windows::UI::ViewManagement;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Media::Imaging;

namespace winrt::Magpie::implementation {

ToastPage::ToastPage(uint64_t hwndToast) : _hwndToast((HWND)hwndToast) {
	// 异步加载 Logo
	[](ToastPage* that) -> fire_and_forget {
		auto weakThis = that->get_weak();

		SoftwareBitmapSource bitmap;
		co_await bitmap.SetBitmapAsync(IconHelper::ExtractAppSmallIcon());

		if (!weakThis.get()) {
			co_return;
		}

		that->_logo = std::move(bitmap);
		that->RaisePropertyChanged(L"Logo");
	}(this);
}

void ToastPage::InitializeComponent() {
	ToastPageT::InitializeComponent();

	_appThemeChangedRevoker = App::Get().ThemeChanged(auto_revoke, [this](bool) {
		Dispatcher().TryRunAsync(CoreDispatcherPriority::Normal, [this] {
			_UpdateTheme();
		});
	});
	_UpdateTheme();
}

static void UpdateToastPosition(HWND hwndToast, const RECT& frameRect, bool updateZOrder) noexcept {
	// 根据窗口高度调整弹窗位置。
	// 1. 如果高度小于 THRESHOLD1，弹窗位于中心；
	// 2. 如果高度大于 THRESHOLD2，弹窗距离底部边界距离固定；
	// 3. 如果高度在 THRESHOLD1 和 THRESHOLD2 之间，则根据 1、2 的边界线性插值。
	static constexpr double THRESHOLD1 = 120;
	static constexpr double THRESHOLD2 = 720;

	// 高 DPI 下阈值也应相应提高
	const double dpiScaling = GetDpiForWindow(hwndToast) / (double)USER_DEFAULT_SCREEN_DPI;
	const double scaledThreshold1 = THRESHOLD1 * dpiScaling;
	const double scaledThreshold2 = THRESHOLD2 * dpiScaling;

	const int height = frameRect.bottom - frameRect.top;

	// 弹窗中心点距离窗口底部边界的距离
	int marginBottom;
	if (height < scaledThreshold1) {
		marginBottom = height / 2;
	} else if (height < scaledThreshold2) {
		marginBottom = (int)std::lround(scaledThreshold1 / 2 +
			(height - scaledThreshold1) * (THRESHOLD2 / 6 - THRESHOLD1 / 2) / (THRESHOLD2 - THRESHOLD1));
	} else {
		marginBottom = (int)std::lround(scaledThreshold2 / 6);
	}

	SetWindowPos(
		hwndToast,
		NULL,
		(frameRect.left + frameRect.right) / 2,
		frameRect.bottom - marginBottom,
		0,
		0,
		SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOREDRAW | (updateZOrder ? SWP_NOZORDER : 0)
	);
}

fire_and_forget ToastPage::ShowMessageOnWindow(std::wstring title, std::wstring message, HWND hwndTarget, bool showLogo) {
	// !!! HACK !!!
	// 重用 TeachingTip 有一个 bug: 前一个 Toast 正在消失时新的 Toast 不会显示。为了
	// 规避它，我们每次都创建新的 TeachingTip，但要保留旧对象的引用，因为播放动画时销毁
	// 会导致崩溃。oldTeachingTip 的生存期可确保动画播放完毕。
	MUXC::TeachingTip oldTeachingTip = MessageTeachingTip();
	if (oldTeachingTip) {
		UnloadObject(oldTeachingTip);
	} else {
		oldTeachingTip = std::move(_oldTeachingTip);
	}

	CoreDispatcher dispatcher = Dispatcher();
	auto weakThis = get_weak();

	// oldTeachingTip 卸载后弹窗不会立刻隐藏，稍微等待防止弹窗闪烁
	co_await resume_foreground(dispatcher, CoreDispatcherPriority::Low);

	if (!weakThis.get()) {
		co_return;
	}

	RECT frameRect;
	if (!Win32Helper::GetWindowFrameRect(hwndTarget, frameRect)) {
		co_return;
	}

	// 更改所有者关系使弹窗始终在 hwndTarget 上方。如果失败，改为定期将弹窗置顶，如果 hwndTarget
	// 的 IL 更高或是 UWP 窗口就会发生这种情况。
	SetLastError(0);
	const bool isOwned = SetWindowLongPtr(_hwndToast, GWLP_HWNDPARENT, (LONG_PTR)hwndTarget) || GetLastError() == 0;
	if (isOwned) {
		// _hwndToast 的输入已被附加到了 hWnd 上，这是所有者窗口的默认行为，但我们不需要。
		// 见 https://devblogs.microsoft.com/oldnewthing/20130412-00/?p=4683
		AttachThreadInput(
			GetCurrentThreadId(),
			GetWindowThreadProcessId(hwndTarget, nullptr),
			FALSE
		);
	} else {
		SetWindowLongPtr(_hwndToast, GWLP_HWNDPARENT, NULL);
		SetWindowPos(_hwndToast, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
		SetWindowPos(_hwndToast, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
	}

	// 更改所有者后应更新 Z 轴顺序
	UpdateToastPosition(_hwndToast, frameRect, true);

	// 创建新的 TeachingTip
	MUXC::TeachingTip curTeachingTip = FindName(L"MessageTeachingTip").as<MUXC::TeachingTip>();
	// 帮助 XAML 选择合适的字体，直接设置 TeachingTip 的 Language 属性没有作用
	MessageTeachingTipContent().Language(LocalizationService::Get().Language());

	if (title.empty()) {
		TitleTextBlock().Visibility(Visibility::Collapsed);
	} else {
		TitleTextBlock().Text(title);
	}
	MessageTextBlock().Text(message);

	// !!! HACK !!!
	// 移除关闭按钮。必须在模板加载完成后做，TeachingTip 没有 Opening 事件，但可以监听 MessageTextBlock 的
	// LayoutUpdated 事件，它在 TeachingTip 显示前必然会被引发。
	MessageTextBlock().LayoutUpdated([weak(weak_ref(curTeachingTip))](IInspectable const&, IInspectable const&) {
		auto teachingTip = weak.get();
		if (!teachingTip) {
			return;
		}

		IControlProtected protectedAccessor = teachingTip.as<IControlProtected>();

		// 隐藏关闭按钮
		if (DependencyObject closeButton = protectedAccessor.GetTemplateChild(L"AlternateCloseButton")) {
			closeButton.as<FrameworkElement>().Visibility(Visibility::Collapsed);
		}
	});

	// 应用内消息无需显示 logo
	_IsLogoShown(showLogo);

	curTeachingTip.IsOpen(true);

	// 第三个参数用于延长 oldTeachingTip 的生存期，确保关闭动画播放完毕后再析构。
	// TeachingTip 的显示和隐藏动画总计 500ms，显示时长不应少于这个时间。
	// https://github.com/Blinue/microsoft-ui-xaml/blob/75f7666f5907aad29de1cb2e49405cc06d433fba/dev/TeachingTip/TeachingTip.h#L239-L240
	[](CoreDispatcher dispatcher, weak_ref<MUXC::TeachingTip> weakCurTeachingTip, MUXC::TeachingTip) -> fire_and_forget {
		// 显示时长固定 2 秒
		co_await 2s;
		co_await dispatcher;

		MUXC::TeachingTip curTeachingTip = weakCurTeachingTip.get();
		// 如果 curTeachingTip 已被卸载则无需关闭
		if (curTeachingTip && curTeachingTip.IsLoaded()) {
			curTeachingTip.IsOpen(false);
		}
	}(dispatcher, curTeachingTip, oldTeachingTip);

	// 定期更新弹窗位置
	RECT prevframeRect{};
	do {
		co_await resume_background();
		// 等待一帧的时间可以使弹窗的移动更平滑
		DwmFlush();
		co_await dispatcher;

		if (!weakThis.get() || _isClosed) {
			co_return;
		}

		if (!IsWindow((HWND)hwndTarget) || !IsWindow(_hwndToast) || !Win32Helper::GetWindowFrameRect((HWND)hwndTarget, frameRect)) {
			// 附加的窗口已经关闭，toast 也应关闭，_oldTeachingTip 用于延长生命周期避免崩溃
			UnloadObject(curTeachingTip);
			_oldTeachingTip = std::move(curTeachingTip);
			co_return;
		}

		if (!isOwned && GetForegroundWindow() == (HWND)hwndTarget) {
			// 如果 hwndTarget 位于前台，定期将弹窗置顶
			SetWindowPos(_hwndToast, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
			SetWindowPos(_hwndToast, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
		}

		// 窗口没有移动则无需更新
		if (frameRect != prevframeRect) {
			prevframeRect = frameRect;
			UpdateToastPosition(_hwndToast, frameRect, false);
		}
	} while (curTeachingTip.IsLoaded() && curTeachingTip.IsOpen());
}

void ToastPage::_UpdateTheme() {
	const bool isLightTheme = App::Get().IsLightTheme();

	if (IsLoaded() && (ActualTheme() == ElementTheme::Light) == isLightTheme) {
		// 无需切换
		return;
	}

	ElementTheme newTheme = isLightTheme ? ElementTheme::Light : ElementTheme::Dark;
	RequestedTheme(newTheme);

	XamlHelper::UpdateThemeOfXamlPopups(XamlRoot(), newTheme);
	XamlHelper::UpdateThemeOfTooltips(*this, newTheme);
}

void ToastPage::_IsLogoShown(bool value) {
	if (_isLogoShown == value) {
		return;
	}

	_isLogoShown = value;
	RaisePropertyChanged(L"IsLogoShown");
}

}
