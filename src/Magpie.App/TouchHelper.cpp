#include "pch.h"
#include "TouchHelper.h"
#include "Logger.h"
#include "StrUtils.h"
#include "Win32Utils.h"
#include "CommonSharedConstants.h"

namespace winrt::Magpie::App {

static std::wstring GetTouchHelperPath() noexcept {
	wil::unique_cotaskmem_string system32Dir;
	HRESULT hr = SHGetKnownFolderPath(
		FOLDERID_System, KF_FLAG_DEFAULT, NULL, system32Dir.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("SHGetKnownFolderPath 失败", hr);
		return {};
	}

	return StrUtils::Concat(system32Dir.get(),
		L"\\Magpie\\", CommonSharedConstants::TOUCH_HELPER_EXE_NAME);
}

bool TouchHelper::IsTouchSupportEnabled() noexcept {
	// 不检查版本号
	return Win32Utils::FileExists(GetTouchHelperPath().c_str());
}

void TouchHelper::IsTouchSupportEnabled(bool value) noexcept {
	SHELLEXECUTEINFO execInfo{
		.cbSize = sizeof(execInfo),
		.fMask = SEE_MASK_NOASYNC | SEE_MASK_NOCLOSEPROCESS,
		.lpVerb = L"runas",
		.lpFile = Win32Utils::GetExePath().c_str(),
		.lpParameters = value ? L" -r" : L" -ur"
	};

	if (ShellExecuteEx(&execInfo)) {
		wil::unique_process_handle hProcess(execInfo.hProcess);
		if (hProcess) {
			wil::handle_wait(hProcess.get());
		}
	} else if (GetLastError() != ERROR_CANCELLED) {
		Logger::Get().Win32Error("ShellExecuteEx 失败");
	}
}

static bool CheckAndFixTouchHelper(std::wstring& path) noexcept {
	// 检查版本号
	path += L".ver";

	std::vector<uint8_t> versionData;

	const auto checkVersion = [&]() {
		if (!Win32Utils::ReadFile(path.c_str(), versionData)) {
			Logger::Get().Error("读取版本号失败");
		}

		return versionData.size() == 4 &&
			*(uint32_t*)versionData.data() == CommonSharedConstants::TOUCH_HELPER_VERSION;
	};

	if (!checkVersion()) {
		// 版本号不匹配，尝试修复，这会请求管理员权限
		TouchHelper::IsTouchSupportEnabled(true);

		if (!checkVersion()) {
			// 修复失败
			return false;
		}
	}

	path.erase(path.size() - 4);
	return true;
}

void TouchHelper::TryLaunchTouchHelper() noexcept {
	std::wstring path = GetTouchHelperPath();
	if (!Win32Utils::FileExists(path.c_str())) {
		// 未启用触控支持
		return;
	}

	wil::unique_mutex_nothrow hSingleInstanceMutex;

	bool alreadyExists = false;
	if (hSingleInstanceMutex.try_create(
		CommonSharedConstants::TOUCH_HELPER_SINGLE_INSTANCE_MUTEX_NAME,
		CREATE_MUTEX_INITIAL_OWNER,
		MUTEX_ALL_ACCESS,
		nullptr,
		&alreadyExists
	) && !alreadyExists) {
		hSingleInstanceMutex.ReleaseMutex();

		// TouchHelper 未在运行则启动它

		// 检查版本是否匹配并尝试修复
		if (!CheckAndFixTouchHelper(path)) {
			// 修复失败
			return;
		}

		Win32Utils::ShellOpen(path.c_str());
	}
}

}
