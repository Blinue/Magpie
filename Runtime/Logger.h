#pragma once
#include "pch.h"
#include <source_location>


class Logger {
public:
	static Logger& Get() {
		static Logger instance;
		return instance;
	}

	bool Initialize(UINT logLevel, const char* logFileName, int logArchiveAboveSize, int logMaxArchiveFiles);

	void SetLevel(spdlog::level::level_enum logLevel);

	void Flush() {
		_logger->flush();
	}

	void Info(std::string_view msg, const std::source_location& location = std::source_location::current()) {
		_logger->log(
			spdlog::source_loc{ location.file_name(), (int)location.line(), location.function_name()},
			spdlog::level::info,
			msg
		);
	}

	void Win32Info(std::string_view msg, const std::source_location& location = std::source_location::current()) {
		_logger->log(
			spdlog::source_loc{ location.file_name(), (int)location.line(), location.function_name() },
			spdlog::level::info,
			_MakeWin32ErrorMsg(msg)
		);
	}

	void ComInfo(std::string_view msg, HRESULT hr, const std::source_location& location = std::source_location::current()) {
		_logger->log(
			spdlog::source_loc{ location.file_name(), (int)location.line(), location.function_name() },
			spdlog::level::info,
			_MakeComErrorMsg(msg, hr)
		);
	}

	void Warn(std::string_view msg, const std::source_location& location = std::source_location::current()) {
		_logger->log(
			spdlog::source_loc{ location.file_name(), (int)location.line(), location.function_name() },
			spdlog::level::warn,
			msg
		);
	}

	void Win32Warn(std::string_view msg, const std::source_location& location = std::source_location::current()) {
		_logger->log(
			spdlog::source_loc{ location.file_name(), (int)location.line(), location.function_name() },
			spdlog::level::warn,
			_MakeWin32ErrorMsg(msg)
		);
	}

	void ComWarn(std::string_view msg, HRESULT hr, const std::source_location& location = std::source_location::current()) {
		_logger->log(
			spdlog::source_loc{ location.file_name(), (int)location.line(), location.function_name() },
			spdlog::level::warn,
			_MakeComErrorMsg(msg, hr)
		);
	}

	void Error(std::string_view msg, const std::source_location& location = std::source_location::current()) {
		_logger->log(
			spdlog::source_loc{ location.file_name(), (int)location.line(), location.function_name() },
			spdlog::level::err,
			msg
		);
	}

	void Win32Error(std::string_view msg, const std::source_location& location = std::source_location::current()) {
		_logger->log(
			spdlog::source_loc{ location.file_name(), (int)location.line(), location.function_name() },
			spdlog::level::err,
			_MakeWin32ErrorMsg(msg)
		);
	}

	void ComError(std::string_view msg, HRESULT hr, const std::source_location& location = std::source_location::current()) {
		_logger->log(
			spdlog::source_loc{ location.file_name(), (int)location.line(),
			location.function_name() },
			spdlog::level::err,
			_MakeComErrorMsg(msg, hr)
		);
	}

	void Critical(std::string_view msg, const std::source_location& location = std::source_location::current()) {
		_logger->log(
			spdlog::source_loc{ location.file_name(), (int)location.line(), location.function_name() },
			spdlog::level::critical,
			msg
		);
	}

	void Win32Critical(std::string_view msg, const std::source_location& location = std::source_location::current()) {
		_logger->log(
			spdlog::source_loc{ location.file_name(), (int)location.line(), location.function_name() },
			spdlog::level::critical,
			_MakeWin32ErrorMsg(msg)
		);
	}

	void ComCritical(std::string_view msg, HRESULT hr, const std::source_location& location = std::source_location::current()) {
		_logger->log(
			spdlog::source_loc{ location.file_name(), (int)location.line(), location.function_name() },
			spdlog::level::critical,
			_MakeComErrorMsg(msg, hr)
		);
	}

private:
	static std::string _MakeWin32ErrorMsg(std::string_view msg) {
		return fmt::format("{}\n\tLastErrorCode：{}", msg, GetLastError());
	}

	static std::string _MakeComErrorMsg(std::string_view msg, HRESULT hr) {
		return fmt::sprintf("%s\n\tHRESULT：0x%X", msg, hr);
	}

	std::shared_ptr<spdlog::logger> _logger;
};
