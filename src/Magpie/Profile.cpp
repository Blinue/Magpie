#include "pch.h"
#include "Profile.h"
#include "Logger.h"
#include "StrHelper.h"
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

}
