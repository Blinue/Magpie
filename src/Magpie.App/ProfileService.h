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

	bool TestNewProfile(bool isPackaged, std::wstring_view pathOrAumid, std::wstring_view className);

	// copyFrom < 0 表示复制默认配置
	bool AddProfile(bool isPackaged, std::wstring_view pathOrAumid, std::wstring_view className, std::wstring_view name, int copyFrom);

	event_token ProfileAdded(delegate<Profile&> const& handler) {
		return _profileAddedEvent.add(handler);
	}

	WinRTUtils::EventRevoker ProfileAdded(auto_revoke_t, delegate<Profile&> const& handler) {
		event_token token = ProfileAdded(handler);
		return WinRTUtils::EventRevoker([this, token]() {
			ProfileAdded(token);
		});
	}

	void ProfileAdded(event_token const& token) {
		_profileAddedEvent.remove(token);
	}

	void RenameProfile(uint32_t profileIdx, std::wstring_view newName);

	event_token ProfileRenamed(delegate<uint32_t> const& handler) {
		return _profileRenamedEvent.add(handler);
	}

	WinRTUtils::EventRevoker ProfileRenamed(auto_revoke_t, delegate<uint32_t> const& handler) {
		event_token token = ProfileRenamed(handler);
		return WinRTUtils::EventRevoker([this, token]() {
			ProfileRenamed(token);
		});
	}

	void ProfileRenamed(event_token const& token) {
		_profileRenamedEvent.remove(token);
	}

	void RemoveProfile(uint32_t profileIdx);

	event_token ProfileRemoved(delegate<uint32_t> const& handler) {
		return _profileRemovedEvent.add(handler);
	}

	WinRTUtils::EventRevoker ProfileRemoved(auto_revoke_t, delegate<uint32_t> const& handler) {
		event_token token = ProfileRemoved(handler);
		return WinRTUtils::EventRevoker([this, token]() {
			ProfileRemoved(token);
		});
	}

	void ProfileRemoved(event_token const& token) {
		_profileRemovedEvent.remove(token);
	}

	bool MoveProfile(uint32_t profileIdx, bool isMoveUp);

	event_token ProfileMoved(delegate<uint32_t, bool> const& handler) {
		return _profileReorderedEvent.add(handler);
	}

	WinRTUtils::EventRevoker ProfileMoved(auto_revoke_t, delegate<uint32_t, bool> const& handler) {
		event_token token = ProfileMoved(handler);
		return WinRTUtils::EventRevoker([this, token]() {
			ProfileMoved(token);
		});
	}

	void ProfileMoved(event_token const& token) {
		_profileReorderedEvent.remove(token);
	}

	const Profile* GetProfileForWindow(HWND hWnd, bool forAutoScale);

	Profile& DefaultProfile();

	Profile& GetProfile(uint32_t idx);

	uint32_t GetProfileCount();

private:
	ProfileService() = default;

	event<delegate<Profile&>> _profileAddedEvent;
	event<delegate<uint32_t>> _profileRenamedEvent;
	event<delegate<uint32_t>> _profileRemovedEvent;
	event<delegate<uint32_t, bool>> _profileReorderedEvent;
};

}
