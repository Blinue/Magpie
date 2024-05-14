#pragma once
#include "WinRTUtils.h"

namespace winrt::Magpie::App {

struct Profile;

class ProfileService {
public:
	static ProfileService& Get() noexcept {
		static ProfileService instance;
		return instance;
	}

	ProfileService(const ProfileService&) = delete;
	ProfileService(ProfileService&&) = delete;

	bool TestNewProfile(bool isPackaged, std::wstring_view pathOrAumid, std::wstring_view className) noexcept;

	// copyFrom < 0 表示复制默认配置
	bool AddProfile(bool isPackaged, std::wstring_view pathOrAumid, std::wstring_view className, std::wstring_view name, int copyFrom);

	void RenameProfile(uint32_t profileIdx, std::wstring_view newName);

	void RemoveProfile(uint32_t profileIdx);

	bool MoveProfile(uint32_t profileIdx, bool isMoveUp);

	const Profile* GetProfileForWindow(HWND hWnd, bool forAutoScale) noexcept;

	Profile& DefaultProfile() noexcept;

	Profile& GetProfile(uint32_t idx) noexcept;

	uint32_t GetProfileCount() noexcept;

	WinRTUtils::Event<delegate<Profile&>> ProfileAdded;
	WinRTUtils::Event<delegate<uint32_t>> ProfileRenamed;
	WinRTUtils::Event<delegate<uint32_t>> ProfileRemoved;
	WinRTUtils::Event<delegate<uint32_t, bool>> ProfileMoved;

private:
	ProfileService() = default;
};

}
