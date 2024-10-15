#pragma once
#include <winrt/Windows.UI.Xaml.Hosting.h>

namespace winrt::Magpie::App {

struct XamlHostingHelper {
	class ManagerWrapper {
	public:
		ManagerWrapper();
		~ManagerWrapper();

		ManagerWrapper(const ManagerWrapper&) = delete;
		ManagerWrapper(ManagerWrapper&&) = default;

	private:
		Hosting::WindowsXamlManager _windowsXamlManager{ nullptr };
	};
};

}
