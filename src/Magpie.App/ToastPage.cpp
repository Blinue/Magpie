#include "pch.h"
#include "ToastPage.h"
#if __has_include("ToastPage.g.cpp")
#include "ToastPage.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml::Controls;

namespace winrt::Magpie::App::implementation {

fire_and_forget ToastPage::ShowMessage(const hstring& message) {
	// !!! HACK !!!
	// 重用 TeachingTip 有一个 bug: 前一个 Toast 正在消失时新的 Toast 不会显示。为了
	// 规避它，我们每次都创建新的 TeachingTip，但要保留旧对象的引用，因为播放动画时销毁
	// 会导致崩溃。oldToastTeachingTip 的生存期可确保动画播放完毕。
	MUXC::TeachingTip oldTeachingTip = MessageTeachingTip();
	if (oldTeachingTip) {
		UnloadObject(oldTeachingTip);
	}

	weak_ref<MUXC::TeachingTip> weakTeachingTip;
	{
		// 创建新的 TeachingTip
		MUXC::TeachingTip newTeachingTip = FindName(L"MessageTeachingTip").as<MUXC::TeachingTip>();
		MessageTextBlock().Text(message);
		newTeachingTip.IsOpen(true);

		// !!! HACK !!!
		// 移除关闭按钮。必须在模板加载完成后做，TeachingTip 没有 Opening 事件，但可以监听 MessageTextBlock 的
		// LayoutUpdated 事件，它在 TeachingTip 显示前必然会被引发。
		MessageTextBlock().LayoutUpdated([weak(weak_ref(newTeachingTip))](IInspectable const&, IInspectable const&) {
			auto teachingTip = weak.get();
			if (!teachingTip) {
				return;
			}

			IControlProtected protectedAccessor = teachingTip.as<IControlProtected>();

			// 隐藏关闭按钮
			if (DependencyObject closeButton = protectedAccessor.GetTemplateChild(L"AlternateCloseButton")) {
				closeButton.as<FrameworkElement>().Visibility(Visibility::Collapsed);
			}

			// 减小 Flyout 尺寸
			if (DependencyObject container = protectedAccessor.GetTemplateChild(L"TailOcclusionGrid")) {
				container.as<FrameworkElement>().MinWidth(0.0);
			}
		});

		weakTeachingTip = newTeachingTip;
	}

	auto weakThis = get_weak();
	CoreDispatcher dispatcher = Dispatcher();
	// 显示时长固定 2 秒
	co_await 2s;
	co_await dispatcher;

	if (weakThis.get()) {
		MUXC::TeachingTip curTeachingTip = MessageTeachingTip();
		if (curTeachingTip == weakTeachingTip.get()) {
			// 如果已经显示新的 Toast 则无需关闭，因为 newTeachingTip 已被卸载（但仍在生存期内）
			curTeachingTip.IsOpen(false);
		}
	}
}

}
