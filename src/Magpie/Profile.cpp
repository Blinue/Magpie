#include "pch.h"
#include "Profile.h"
#include "AppXReader.h"
#include "Logger.h"
#include "StrHelper.h"
#include "Win32Helper.h"
#include <ShlObj.h>

namespace Magpie {

static std::filesystem::path GetSystemScreenshotsDir() noexcept {
	// 如果 Screenshots 文件夹不存在将失败
	wil::unique_cotaskmem_string folder;
	HRESULT hr = SHGetKnownFolderPath(
		FOLDERID_Screenshots, KF_FLAG_DEFAULT, NULL, folder.put());
	if (SUCCEEDED(hr)) {
		return folder.get();
	}

	// 屏幕截图文件夹默认路径是 %USERPROFILE%\Pictures\Screenshots

	hr = SHGetKnownFolderPath(
		FOLDERID_Pictures, KF_FLAG_DEFAULT, NULL, folder.put());
	if (SUCCEEDED(hr)) {
		return StrHelper::Concat(folder.get(), L"\\Screenshots");
	}

	hr = SHGetKnownFolderPath(
		FOLDERID_Profile, KF_FLAG_DEFAULT, NULL, folder.put());
	if (SUCCEEDED(hr)) {
		return StrHelper::Concat(folder.get(), L"\\Pictures\\Screenshots");
	}

	Logger::Get().ComError("SHGetKnownFolderPath 失败", hr);
	return {};
}

std::filesystem::path Profile::GetScreenshotsDir() const noexcept {
	if (screenshotsDir.empty()) {
		// 系统“屏幕截图”文件夹
		return GetSystemScreenshotsDir();
	} else if (screenshotsDir.is_relative()) {
		// 相对路径
		std::wstring workingDir;
		HRESULT hr = wil::GetCurrentDirectoryW(workingDir);
		if (FAILED(hr)) {
			Logger::Get().ComError("wil::GetCurrentDirectoryW 失败", hr);
			return {};
		}

		if (screenshotsDir == L".") {
			return std::filesystem::path(std::move(workingDir));
		} else {
			return (std::filesystem::path(std::move(workingDir)) / screenshotsDir).lexically_normal();
		}
	} else {
		// 绝对路径
		return screenshotsDir;
	}
}

static bool IsSubfolder(const std::wstring& sub, const std::wstring& parent) noexcept {
	if (!sub.starts_with(parent)) {
		return false;
	}

	if (parent.size() == sub.size()) {
		return true;
	}

	return sub[parent.size()] == L'\\';
}

void Profile::SetScreenshotsDir(const std::filesystem::path& value) noexcept {
	assert(!value.empty());

	if (value == GetSystemScreenshotsDir()) {
		// 系统“屏幕截图”文件夹
		screenshotsDir.clear();
		return;
	}

	std::wstring workingDir;
	HRESULT hr = wil::GetCurrentDirectoryW(workingDir);
	if (FAILED(hr)) {
		Logger::Get().ComError("wil::GetCurrentDirectoryW 失败", hr);
		return;
	}

	if (IsSubfolder(value, workingDir)) {
		// 保存位置在工作文件夹内则转换为相对路径
		if (value.native().size() == workingDir.size()) {
			screenshotsDir = L".";
		} else {
			screenshotsDir = StrHelper::Concat(
				L".",
				std::wstring(value.native().begin() + workingDir.size(), value.native().end())
			);
		}
	} else {
		// 绝对路径
		screenshotsDir = value;
	}
}

bool Profile::CanLaunch() const noexcept {
	if (isPackaged) {
		AppXReader appxReader;
		return appxReader.Initialize(pathRule);
	} else {
		return Win32Helper::FileExists(pathRule.c_str());
	}
}

void Profile::Launch() const noexcept {
	assert(CanLaunch());

	if (isPackaged) {
		// 关于启动打包应用的讨论:
		// https://github.com/microsoft/WindowsAppSDK/issues/2856#issuecomment-1224409948
		// 使用 CLSCTX_LOCAL_SERVER 以在独立的进程中启动应用
		// 见 https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nn-shobjidl_core-iapplicationactivationmanager
		winrt::com_ptr<IApplicationActivationManager> aam =
			winrt::try_create_instance<IApplicationActivationManager>(
				CLSID_ApplicationActivationManager, CLSCTX_LOCAL_SERVER);
		if (!aam) {
			Logger::Get().Error("创建 ApplicationActivationManager 失败");
			return;
		}

		// 确保启动为前台窗口
		HRESULT hr = CoAllowSetForegroundWindow(aam.get(), nullptr);
		if (FAILED(hr)) {
			Logger::Get().ComError("创建 CoAllowSetForegroundWindow 失败", hr);
		}

		DWORD procId;
		hr = aam->ActivateApplication(pathRule.c_str(), launchParameters.c_str(), AO_NONE, &procId);
		if (FAILED(hr)) {
			Logger::Get().ComError("IApplicationActivationManager::ActivateApplication 失败", hr);
			return;
		}
	} else {
		const std::wstring& path = !launcherPath.empty() &&
			Win32Helper::FileExists(launcherPath.c_str()) ? launcherPath.native() : pathRule;
		Win32Helper::ShellOpen(path.c_str(), launchParameters.c_str());
	}
}

winrt::fire_and_forget Profile::OpenProgramLocation() const noexcept {
	assert(CanLaunch());

	std::wstring programLocation;
	if (isPackaged) {
		AppXReader appxReader;
		[[maybe_unused]] bool result = appxReader.Initialize(pathRule);
		assert(result);

		programLocation = appxReader.GetExecutablePath();
		if (programLocation.empty()) {
			// 找不到可执行文件则打开应用文件夹
			Win32Helper::ShellOpen(appxReader.GetPackagePath().c_str());
			co_return;
		}
	} else {
		programLocation = pathRule;
	}

	co_await winrt::resume_background();
	Win32Helper::OpenFolderAndSelectFile(programLocation.c_str());
}

}
