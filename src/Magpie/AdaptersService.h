#pragma once

namespace Magpie {

class AdaptersService {
public:
	static AdaptersService& Get() noexcept {
		static AdaptersService instance;
		return instance;
	}

	AdaptersService(const AdaptersService&) = delete;
	AdaptersService(AdaptersService&&) = delete;

	bool Initialize() noexcept;

	void Uninitialize() noexcept;

private:
	AdaptersService() = default;
};

}
