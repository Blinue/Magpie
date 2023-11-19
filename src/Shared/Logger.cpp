#include "pch.h"
#include "Logger.h"
#include "StrUtils.h"
#include <spdlog/sinks/rotating_file_sink.h>
#include <fmt/printf.h>


bool Logger::Initialize(spdlog::level::level_enum logLevel, const char* logFileName, int logArchiveAboveSize, int logMaxArchiveFiles) noexcept {
	try {
		_logger = spdlog::rotating_logger_mt(".", logFileName, logArchiveAboveSize, logMaxArchiveFiles);
		_logger->set_level(logLevel);
		_logger->set_pattern("%Y-%m-%d %H:%M:%S.%e|%l|%s:%!|%v");
		_logger->flush_on(spdlog::level::warn);
		spdlog::flush_every(5s);
	} catch (const spdlog::spdlog_ex&) {
		return false;
	}

	return true;
}

bool Logger::Initialize(Logger& logger) noexcept {
	_logger = logger._logger;
	return true;
}

void Logger::SetLevel(spdlog::level::level_enum logLevel) {
	assert(_logger);

	_logger->flush();
	_logger->set_level(logLevel);

	static const char* LOG_LEVELS[7] = {
		"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "CRITICAL", "OFF"
	};
	Info(fmt::format("当前日志级别：{}", LOG_LEVELS[logLevel]));
}

std::string Logger::_MakeWin32ErrorMsg(std::string_view msg) {
	return fmt::format("{}\n\tLastErrorCode：{}", msg, GetLastError());
}

std::string Logger::_MakeComErrorMsg(std::string_view msg, HRESULT hr) {
	return fmt::sprintf("%s\n\tHRESULT：0x%X", msg, hr);
}

void Logger::_Log(spdlog::level::level_enum logLevel, std::string_view msg, const std::source_location& location) {
	assert(!msg.empty());

	if (logLevel >= spdlog::level::warn) {
		// 警告或更高等级的日志也记录到调试器（VS 中的“即时窗口”）
		if (msg.back() == '\n') {
			OutputDebugString(StrUtils::Concat(L"[LOG] ", StrUtils::UTF8ToUTF16(msg)).c_str());
		} else {
			OutputDebugString(StrUtils::Concat(L"[LOG] ", StrUtils::UTF8ToUTF16(msg), L"\n").c_str());
		}
	}

	_logger->log(
		spdlog::source_loc{ location.file_name(), (int)location.line(), location.function_name() },
		logLevel,
		msg
	);
}
