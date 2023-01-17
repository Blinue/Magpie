#pragma once
#include "WinRTUtils.h"

namespace winrt::Magpie::UI {

enum class UpdateStatus {
	Pending,
	Checking,
	NoUpdate,
	ErrorWhileChecking,
	Available,
	Downloading,
	ErrorWhileDownloading,
	Installing
};

enum class UpdateError {
	Network,
	Logical,
	Unknown
};

class UpdateService {
public:
	static UpdateService& Get() noexcept {
		static UpdateService instance;
		return instance;
	}

	UpdateService(const UpdateService&) = delete;
	UpdateService(UpdateService&&) = delete;

	UpdateStatus Status() const noexcept {
		return _status;
	}

	UpdateError Error() const noexcept {
		return _error;
	}

	event_token StatusChanged(delegate<UpdateStatus> const& handler) {
		return _statusChangedEvent.add(handler);
	}

	WinRTUtils::EventRevoker StatusChanged(auto_revoke_t, delegate<UpdateStatus> const& handler) {
		event_token token = StatusChanged(handler);
		return WinRTUtils::EventRevoker([this, token]() {
			StatusChanged(token);
		});
	}

	void StatusChanged(event_token const& token) {
		_statusChangedEvent.remove(token);
	}

	fire_and_forget CheckForUpdatesAsync();

	void DownloadAndInstall();

	void LeavingAboutPage();

	void ClosingMainWindow();

	void Cancel();

	const std::wstring& Tag() const noexcept {
		assert(_status >= UpdateStatus::Available);
		return _tag;
	}

	const std::wstring& BinaryUrl() const noexcept {
		assert(_status >= UpdateStatus::Available);
		return _binaryUrl;
	}

private:
	UpdateService() = default;

	void _Status(UpdateStatus value, UpdateError error = UpdateError::Unknown);

	event<delegate<UpdateStatus>> _statusChangedEvent;

	std::wstring _tag;
	std::wstring _binaryUrl;
	UpdateStatus _status = UpdateStatus::Pending;
	UpdateError _error = UpdateError::Unknown;
};

}
