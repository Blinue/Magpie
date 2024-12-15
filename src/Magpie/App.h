#pragma once
#include "App.g.h"
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include "MainWindow.h"

namespace winrt::Magpie::implementation {

class App : public App_base<App, Markup::IXamlMetadataProvider> {
public:
	static App& Get();

	App();
	App(const App&) = delete;
	App(App&&) = delete;

	bool Initialize(HINSTANCE hInstance, const wchar_t* arguments);

	int Run();

	void ShowMainWindow() noexcept;

	void Quit();

	void Restart(bool asElevated = false, const wchar_t* arguments = nullptr) noexcept;

	void SaveSettings();

	// 在由外部源引发的回调中可能返回 nullptr
	// 这是因为用户关闭主窗口后 RootPage 不会立刻析构
	const Magpie::RootPage& RootPage() const noexcept;

	void Restart() const noexcept;

	const ::Magpie::MainWindow& MainWindow() const noexcept {
		return _mainWindow;
	}

	::Magpie::MainWindow& MainWindow() noexcept {
		return _mainWindow;
	}

private:
	bool _CheckSingleInstance() noexcept;

	void _QuitWithoutMainWindow();

	void _MainWindow_Destoryed();

	void _ReleaseMutexes() noexcept;

	HINSTANCE _hInst = NULL;
	wil::unique_mutex_nothrow _hSingleInstanceMutex;
	// 以管理员身份运行时持有此锁
	wil::unique_mutex_nothrow _hElevatedMutex;

	Hosting::WindowsXamlManager _windowsXamlManager{ nullptr };

	::Magpie::MainWindow _mainWindow;
	Point _mainWindowCenter{};
	Size _mainWindowSizeInDips{};
	bool _isMainWndMaximized = false;

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

namespace winrt::Magpie::factory_implementation {

class App : public AppT<App, implementation::App> {
};

}
