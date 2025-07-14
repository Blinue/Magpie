#pragma once
#include "App.g.h"
#include "AppSettings.h"
#include "Event.h"
#include <winrt/Windows.Globalization.NumberFormatting.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>

namespace Magpie {
class MainWindow;
}

namespace winrt::Magpie::implementation {

struct RootPage;

class App : public App_base<App, Markup::IXamlMetadataProvider> {
public:
	static App& Get();

	App();
	App(const App&) = delete;
	App(App&&) = delete;

	bool Initialize(const wchar_t* arguments);

	int Run();

	const CoreDispatcher& Dispatcher() const noexcept {
		return _dispatcher;
	}

	void ShowMainWindow() noexcept;

	void Quit();

	void Restart(bool asElevated = false, const wchar_t* arguments = nullptr) noexcept;

	const com_ptr<RootPage>& RootPage() const noexcept;

	const ::Magpie::MainWindow& MainWindow() const noexcept {
		return *_mainWindow;
	}

	::Magpie::MainWindow& MainWindow() noexcept {
		return *_mainWindow;
	}

	bool IsLightTheme() const noexcept {
		return _isLightTheme.load(std::memory_order_relaxed);
	}

	static Windows::Globalization::NumberFormatting::INumberFormatter2 DoubleFormatter();

	::Magpie::MultithreadEvent<bool> ThemeChanged;

private:
	void _Uninitialize();

	bool _CheckSingleInstance() noexcept;

	void _AppSettings_ThemeChanged(::Magpie::AppTheme theme);

	void _UpdateColorValuesChangedRevoker();

	void _UpdateTheme();

	void _ReleaseMutexes() noexcept;

	wil::unique_mutex_nothrow _hSingleInstanceMutex;
	// 以管理员身份运行时持有此锁
	wil::unique_mutex_nothrow _hElevatedMutex;

	Hosting::WindowsXamlManager _windowsXamlManager{ nullptr };

	std::unique_ptr<::Magpie::MainWindow> _mainWindow;

	CoreDispatcher _dispatcher{ nullptr };

	::Magpie::Event<::Magpie::AppTheme>::EventRevoker _themeChangedRevoker;
	Windows::UI::ViewManagement::UISettings _uiSettings;
	Windows::UI::ViewManagement::UISettings::ColorValuesChanged_revoker _colorValuesChangedRevoker;
	std::atomic<bool> _isLightTheme = true;

	::Magpie::Event<bool>::EventRevoker _isShowNotifyIconChangedRevoker;

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

BASIC_FACTORY(App)
