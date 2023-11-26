#pragma once
#include <winrt/Magpie.App.h>
#include "Win32Utils.h"
#include "MainWindow.h"

namespace Magpie {

class XamlApp {
public:
	static XamlApp& Get() noexcept {
		static XamlApp instance;
		return instance;
	}

	XamlApp(const XamlApp&) = delete;
	XamlApp(XamlApp&&) = delete;

	bool Initialize(HINSTANCE hInstance, const wchar_t* arguments);

	int Run();

	void ShowMainWindow() noexcept;

	void Quit();

	void Restart(bool asElevated = false, const wchar_t* arguments = nullptr) noexcept;

	void SaveSettings();

private:
	XamlApp();
	~XamlApp();

	bool _CheckSingleInstance();

	void _InitializeLogger();

	bool _CreateMainWindow();

	void _QuitWithoutMainWindow();

	void _MainWindow_Destoryed();

	void _ReleaseMutexes() noexcept;

	HINSTANCE _hInst = NULL;
	Win32Utils::ScopedHandle _hSingleInstanceMutex;
	// 以管理员身份运行时持有此 Mutex
	Win32Utils::ScopedHandle _hElevatedMutex;

	winrt::Magpie::App::App _uwpApp{ nullptr };

	MainWindow _mainWindow;
	winrt::Point _mainWindowCenter{};
	winrt::Size _mainWindowSizeInDips{};
	bool _isMainWndMaximized = false;
};

}
