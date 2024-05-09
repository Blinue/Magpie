#pragma once

namespace winrt::Magpie::App {

struct TouchHelper {
	static bool IsTouchSupportEnabled() noexcept;
	static void IsTouchSupportEnabled(bool value) noexcept;
	static bool TryLaunchTouchHelper(bool& isTouchSupportEnabled) noexcept;
};

}
