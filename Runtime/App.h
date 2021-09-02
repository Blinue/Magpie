#pragma once
#include "pch.h"


class App {
public:
	static App* GetInstance() {
		static App* instance = new App();
		return instance;
	}

	bool Initialize(std::shared_ptr<spdlog::logger> logger);

	std::shared_ptr<spdlog::logger> GetLogger() const {
		return _logger;
	}

private:
	App() {}

	std::shared_ptr<spdlog::logger> _logger = nullptr;
};
