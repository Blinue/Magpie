#pragma once
#include "pch.h"


namespace winrt::Magpie::UI {

enum class UpdateResult {
	NoUpdate,
	Available,
	NetworkError
};

class UpdateService {
public:
	static UpdateService& Get() {
		static UpdateService instance;
		return instance;
	}

	UpdateService(const UpdateService&) = delete;
	UpdateService(UpdateService&&) = delete;

	IAsyncAction CheckForUpdateAsync();

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
