﻿#include "pch.h"
#include "ToastPage.h"
#if __has_include("ToastPage.g.cpp")
#include "ToastPage.g.cpp"
#endif
#include "Win32Utils.h"
#include "IconHelper.h"

using namespace winrt;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Media::Imaging;

namespace winrt::Magpie::App::implementation {

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

static void UpdateToastPosition(HWND hwndToast, const RECT& frameRect) noexcept {
	static constexpr int THRESHOLD1 = 120;
	static constexpr int THRESHOLD2 = 720;

	const int dpi = (int)GetDpiForWindow(hwndToast);

	const int height = frameRect.bottom - frameRect.top;
	int dist;
	if (height < THRESHOLD1 * USER_DEFAULT_SCREEN_DPI / dpi) {
		dist = height / 2;
	} else if (height < THRESHOLD2 * USER_DEFAULT_SCREEN_DPI / dpi) {
		dist = (height - THRESHOLD1 * USER_DEFAULT_SCREEN_DPI / dpi) * (THRESHOLD2 / 6 - THRESHOLD1 / 2) / (THRESHOLD2 - THRESHOLD1) + THRESHOLD1 / 2 * USER_DEFAULT_SCREEN_DPI / dpi;
	} else {
		dist = THRESHOLD2 / 6 * USER_DEFAULT_SCREEN_DPI / dpi;
	}

	SetWindowPos(
		hwndToast,
		NULL,
		(frameRect.left + frameRect.right) / 2,
		frameRect.bottom - dist,
		0,
		0,
		SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOREDRAW
	);
}

fire_and_forget ToastPage::ShowMessageOnWindow(hstring message, uint64_t hwndTarget) {
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

	// 备份要使用的变量，后面避免使用 this
	CoreDispatcher dispatcher = Dispatcher();
	auto weakThis = get_weak();

	// oldTeachingTip 卸载后弹窗不会立刻隐藏，稍微等待防止弹窗闪烁
	co_await resume_foreground(dispatcher, CoreDispatcherPriority::Low);

	if (!weakThis.get()) {
		co_return;
	}

	RECT frameRect;
	if (!Win32Utils::GetWindowFrameRect((HWND)hwndTarget, frameRect)) {
		co_return;
	}

	// 更改所有者关系使弹窗始终在 hwndTarget 上方
	SetWindowLongPtr(_hwndToast, GWLP_HWNDPARENT, (LONG_PTR)hwndTarget);
	// hwndToast 的输入已被附加到了 hWnd 上，这是所有者窗口的默认行为，但我们不需要。
	// 见 https://devblogs.microsoft.com/oldnewthing/20130412-00/?p=4683
	AttachThreadInput(
		GetCurrentThreadId(),
		GetWindowThreadProcessId((HWND)hwndTarget, nullptr),
		FALSE
	);
	
	UpdateToastPosition(_hwndToast, frameRect);

	// 创建新的 TeachingTip
	MUXC::TeachingTip curTeachingTip = FindName(L"MessageTeachingTip").as<MUXC::TeachingTip>();
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

	// 定期更新弹窗位置，这里应避免使用 this
	RECT prevframeRect{};
	const HWND hwndToast = _hwndToast;
	do {
		co_await resume_background();
		// 等待一帧的时间可以使弹窗的移动更平滑
		DwmFlush();
		co_await dispatcher;

		if (!IsWindow((HWND)hwndTarget) || !IsWindow(hwndToast)) {
			// 附加的窗口已经关闭，toast 也应关闭，_oldTeachingTip 用于延长生命周期避免崩溃
			UnloadObject(curTeachingTip);
			_oldTeachingTip = std::move(curTeachingTip);
			break;
		}

		if (!Win32Utils::GetWindowFrameRect((HWND)hwndTarget, frameRect)) {
			break;
		}

		// 窗口没有移动则无需更新
		if (frameRect == prevframeRect) {
			continue;
		}
		prevframeRect = frameRect;

		UpdateToastPosition(hwndToast, frameRect);
	} while (curTeachingTip.IsLoaded() && curTeachingTip.IsOpen());
}

}
