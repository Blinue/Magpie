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

	void _CreateMainWindow();

	void _QuitWithoutMainWindow();

	HINSTANCE _hInst = NULL;
	Win32Utils::ScopedHandle _hSingleInstanceMutex;
	std::optional<MainWindow> _mainWindow;
	winrt::Magpie::App::App _uwpApp{ nullptr };

	// right 存储宽，bottom 存储高
	RECT _mainWndRect{};
	bool _isMainWndMaximized = false;
};

}
