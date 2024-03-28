#pragma once
#include "App.g.h"
#include <winrt/Windows.UI.Xaml.Hosting.h>

namespace winrt::Magpie::App::implementation {

class App : public App_base<App, Markup::IXamlMetadataProvider> {
public:
	App();
	~App();

	void Close();

	void SaveSettings();

	StartUpOptions Initialize(int);

	void Uninitialize();

	bool IsShowNotifyIcon() const noexcept;

	event_token IsShowNotifyIconChanged(EventHandler<bool> const& handler);

	void IsShowNotifyIconChanged(event_token const& token);

	uint64_t HwndMain() const noexcept {
		return (uint64_t)_hwndMain;
	}

	void HwndMain(uint64_t value) noexcept {
		_hwndMain = (HWND)value;
	}

	// 在由外部源引发的回调中可能返回 nullptr
	// 这是因为用户关闭主窗口后 RootPage 不会立刻析构
	Magpie::App::RootPage RootPage() const noexcept {
		return _rootPage.get();
	}
	
	void RootPage(Magpie::App::RootPage const& rootPage) noexcept;

	void Quit() const noexcept;

	void Restart() const noexcept;

private:
	Hosting::WindowsXamlManager _windowsXamlManager{ nullptr };
	weak_ref<Magpie::App::RootPage> _rootPage{ nullptr };
	HWND _hwndMain = NULL;
	bool _isClosed = false;

	////////////////////////////////////////////////////
	// 
	// IXamlMetadataProvider 相关
	// 
	/////////////////////////////////////////////////////
public:
	Markup::IXamlType GetXamlType(Interop::TypeName const& type) {
		return _AppProvider()->GetXamlType(type);
	}

	Markup::IXamlType GetXamlType(hstring const& fullName) {
		return _AppProvider()->GetXamlType(fullName);
	}

	com_array<Markup::XmlnsDefinition> GetXmlnsDefinitions() {
		return _AppProvider()->GetXmlnsDefinitions();
	}

private:
	com_ptr<XamlMetaDataProvider> _AppProvider() {
		if (!_appProvider) {
			_appProvider = make_self<XamlMetaDataProvider>();
		}
		return _appProvider;
	}

	com_ptr<XamlMetaDataProvider> _appProvider;
};

}

namespace winrt::Magpie::App::factory_implementation {

class App : public AppT<App, implementation::App> {
};

}
