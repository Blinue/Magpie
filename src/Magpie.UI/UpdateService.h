#pragma once

namespace winrt::Magpie::UI {

enum class UpdateResult {
	NoUpdate,
	Available,
	NetworkError,
	UnknownError
};

class UpdateService {
public:
	static UpdateService& Get() noexcept {
		static UpdateService instance;
		return instance;
	}

	UpdateService(const UpdateService&) = delete;
	UpdateService(UpdateService&&) = delete;

	IAsyncAction CheckForUpdatesAsync();

	UpdateResult GetResult() const noexcept {
		return _result;
	}

private:
	UpdateService() = default;

	std::wstring _tag;
	std::wstring _binaryUrl;
	UpdateResult _result = UpdateResult::NoUpdate;
};

}
