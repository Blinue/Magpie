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

	const std::wstring& Tag() const noexcept {
		return _tag;
	}

	const std::wstring& BinaryUrl() const noexcept {
		return _binaryUrl;
	}

private:
	UpdateService() = default;

	std::wstring _tag;
	std::wstring _binaryUrl;
	UpdateResult _result = UpdateResult::NoUpdate;
};

}
