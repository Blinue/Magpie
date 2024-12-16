#pragma once
#include "Event.h"

namespace Magpie {

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

class UpdateService {
public:
	static UpdateService& Get() noexcept {
		static UpdateService instance;
		return instance;
	}

	UpdateService(const UpdateService&) = delete;
	UpdateService(UpdateService&&) = delete;

	void Initialize() noexcept;

	UpdateStatus Status() const noexcept {
		return _status;
	}

	winrt::fire_and_forget CheckForUpdatesAsync(bool isAutoUpdate);

	winrt::fire_and_forget DownloadAndInstall();

	double DownloadProgress() const noexcept {
		assert(_status == UpdateStatus::Downloading);
		return _downloadProgress;
	}

	void EnteringAboutPage();

	void Cancel();

	const std::wstring& Tag() const noexcept {
		assert(_status >= UpdateStatus::Available);
		return _tag;
	}

	const std::wstring& BinaryUrl() const noexcept {
		assert(_status >= UpdateStatus::Available);
		return _binaryUrl;
	}

	bool IsShowOnHomePage() const noexcept {
		return _showOnHomePage;
	}

	void IsShowOnHomePage(bool value) noexcept {
		if (_showOnHomePage == value) {
			return;
		}

		_showOnHomePage = value;
		IsShowOnHomePageChanged.Invoke(value);
	}

	Core::Event<UpdateStatus> StatusChanged;
	Core::Event<double> DownloadProgressChanged;
	Core::Event<bool> IsShowOnHomePageChanged;

private:
	UpdateService() = default;

	void _Status(UpdateStatus value);

	winrt::fire_and_forget _Timer_Tick(winrt::Threading::ThreadPoolTimer const& timer);

	void _StartTimer();
	void _StopTimer();

	void _MainWindow_Closed();

	// DispatcherTimer 在不显示主窗口时可能停滞，因此使用 ThreadPoolTimer
	winrt::Threading::ThreadPoolTimer _timer{ nullptr };
	winrt::CoreDispatcher _dispatcher{ nullptr };

	std::wstring _tag;
	std::wstring _binaryUrl;
	std::wstring _binaryHash;
	UpdateStatus _status = UpdateStatus::Pending;
	double _downloadProgress = 0;
	bool _downloadCancelled = false;
	bool _showOnHomePage = false;
};

}
