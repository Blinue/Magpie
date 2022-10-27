#pragma once
#include "pch.h"


namespace winrt::Magpie::UI {

class UpdateService {
public:
	static UpdateService& Get() {
		static UpdateService instance;
		return instance;
	}

	UpdateService(const UpdateService&) = delete;
	UpdateService(UpdateService&&) = delete;

private:
	UpdateService() = default;
};

}
