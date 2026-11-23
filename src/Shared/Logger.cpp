#include "pch.h"
#include "Logger.h"
#include "StrHelper.h"
#include <spdlog/sinks/rotating_file_sink.h>

namespace Magpie {

bool Logger::Initialize(
	spdlog::level::level_enum logLevel,
	std::wstring logFileName,
	int logArchiveAboveSize,
	int logMaxArchiveFiles
) noexcept {
	try {
		_logger = spdlog::rotating_logger_mt(".", std::move(logFileName), logArchiveAboveSize, logMaxArchiveFiles);
		_logger->set_level(logLevel);
		_logger->set_pattern("%Y-%m-%d %H:%M:%S.%e|%l|%s:%#|%!|%v");
		_logger->flush_on(spdlog::level::warn);
#ifdef _DEBUG
		spdlog::flush_every(5s);
#else
		spdlog::flush_every(30s);
#endif
	} catch (const spdlog::spdlog_ex&) {
		return false;
	}

	return true;
}

void Logger::SetLevel(spdlog::level::level_enum logLevel) noexcept {
	assert(_logger);

	_logger->flush();
	_logger->set_level(logLevel);

	static const char* LOG_LEVELS[7] = {
		"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "CRITICAL", "OFF"
	};
	Info(fmt::format("当前日志级别: {}", LOG_LEVELS[logLevel]));
}

void Logger::Flush() noexcept {
	if (_logger) {
		_logger->flush();
	}
}

static std::string MakeWin32ErrorMsg(std::string_view msg) noexcept {
	return fmt::format("{}\n\tLastErrorCode: {}", msg, GetLastError());
}

static std::string MakeNTErrorMsg(std::string_view msg, NTSTATUS status) noexcept {
	return fmt::format("{}\n\tNTSTATUS: {}", msg, status);
}

static std::string MakeComErrorMsg(std::string_view msg, HRESULT hr) noexcept {
	return fmt::format("{}\n\tHRESULT: 0x{:X}", msg, (uint32_t)hr);
}

void Logger::Info(std::string_view msg, const SourceLocation& location) noexcept {
	_Log(spdlog::level::info, msg, location);
}

void Logger::Win32Info(std::string_view msg, const SourceLocation& location) noexcept {
	_Log(spdlog::level::info, MakeWin32ErrorMsg(msg), location);
}

void Logger::NTInfo(std::string_view msg, NTSTATUS status, const SourceLocation& location) noexcept {
	_Log(spdlog::level::info, MakeNTErrorMsg(msg, status), location);
}

void Logger::ComInfo(std::string_view msg, HRESULT hr, const SourceLocation& location) noexcept {
	_Log(spdlog::level::info, MakeComErrorMsg(msg, hr), location);
}

void Logger::Warn(std::string_view msg, const SourceLocation& location) noexcept {
	_Log(spdlog::level::warn, msg, location);
}

void Logger::Win32Warn(std::string_view msg, const SourceLocation& location) noexcept {
	_Log(spdlog::level::warn, MakeWin32ErrorMsg(msg), location);
}

void Logger::NTWarn(std::string_view msg, NTSTATUS status, const SourceLocation& location) noexcept {
	_Log(spdlog::level::warn, MakeNTErrorMsg(msg, status), location);
}

void Logger::ComWarn(std::string_view msg, HRESULT hr, const SourceLocation& location) noexcept {
	_Log(spdlog::level::warn, MakeComErrorMsg(msg, hr), location);
}

void Logger::Error(std::string_view msg, const SourceLocation& location) noexcept {
	_Log(spdlog::level::err, msg, location);
}

void Logger::Win32Error(std::string_view msg, const SourceLocation& location) noexcept {
	_Log(spdlog::level::err, MakeWin32ErrorMsg(msg), location);
}

void Logger::NTError(std::string_view msg, NTSTATUS status, const SourceLocation& location) noexcept {
	_Log(spdlog::level::err, MakeNTErrorMsg(msg, status), location);
}

void Logger::ComError(std::string_view msg, HRESULT hr, const SourceLocation& location) noexcept {
	_Log(spdlog::level::err, MakeComErrorMsg(msg, hr), location);
}

void Logger::Critical(std::string_view msg, const SourceLocation& location) noexcept {
	_Log(spdlog::level::critical, msg, location);
}

void Logger::Win32Critical(std::string_view msg, const SourceLocation& location) noexcept {
	_Log(spdlog::level::critical, MakeWin32ErrorMsg(msg), location);
}

void Logger::NTCritical(std::string_view msg, NTSTATUS status, const SourceLocation& location) noexcept {
	_Log(spdlog::level::critical, MakeNTErrorMsg(msg, status), location);
}

void Logger::ComCritical(std::string_view msg, HRESULT hr, const SourceLocation& location) noexcept {
	_Log(spdlog::level::critical, MakeComErrorMsg(msg, hr), location);
}

void Logger::_Log(spdlog::level::level_enum logLevel, std::string_view msg, const SourceLocation& location) noexcept {
	assert(!msg.empty());

	// 只检查一次是否附加了调试器
	static const bool isDebuggerPresent = IsDebuggerPresent();
	if (isDebuggerPresent && logLevel >= spdlog::level::warn) {
		// 警告或更高等级的日志也记录到调试器
		if (msg.back() == '\n') {
			OutputDebugString(StrHelper::Concat(L"[LOG] ", StrHelper::UTF8ToUTF16(msg)).c_str());
		} else {
			OutputDebugString(StrHelper::Concat(L"[LOG] ", StrHelper::UTF8ToUTF16(msg), L"\n").c_str());
		}
	}

	if (_logger) {
		_logger->log(
			spdlog::source_loc{ location.GetFileName(), (int)location.GetLine(), location.GetFunctionName() },
			logLevel,
			msg
		);
	}
}

}
