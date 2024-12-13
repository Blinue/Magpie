#pragma once
#include <spdlog/spdlog.h>

namespace Magpie::Core {

// std::source_location 中的函数名包含整个签名过于冗长，我们只需记录函数名，
// 因此创建自己的 SourceLocation
struct SourceLocation {
	[[nodiscard]] static consteval SourceLocation Current(
		std::uint_least32_t line = __builtin_LINE(),
		const char* file = __builtin_FILE(),
		const char* function = __builtin_FUNCTION()
	) noexcept {
		return SourceLocation{ line, file, function };
	}

	[[nodiscard]] constexpr SourceLocation() noexcept = default;

	[[nodiscard]] constexpr SourceLocation(
		const std::uint_least32_t line,
		const char* file,
		const char* function
	) noexcept : _line(line), _file(file), _function(function) {}

	[[nodiscard]] constexpr std::uint_least32_t Line() const noexcept {
		return _line;
	}

	[[nodiscard]] constexpr const char* FileName() const noexcept {
		return _file;
	}

	constexpr const char* FunctionName() const noexcept {
		return _function;
	}

private:
	const std::uint_least32_t _line = 0;
	const char* _file = nullptr;
	const char* _function = nullptr;
};

class Logger {
public:
	static Logger& Get() noexcept {
		static Logger instance;
		return instance;
	}

	bool Initialize(spdlog::level::level_enum logLevel, const char* logFileName, int logArchiveAboveSize, int logMaxArchiveFiles) noexcept;

	void SetLevel(spdlog::level::level_enum logLevel) noexcept;

	void Flush() noexcept {
		_logger->flush();
	}

	void Info(std::string_view msg, const SourceLocation& location = SourceLocation::Current()) noexcept {
		_Log(spdlog::level::info, msg, location);
	}

	void Win32Info(std::string_view msg, const SourceLocation& location = SourceLocation::Current()) noexcept {
		_Log(spdlog::level::info, _MakeWin32ErrorMsg(msg), location);
	}

	void NTInfo(std::string_view msg, NTSTATUS status, const SourceLocation& location = SourceLocation::Current()) noexcept {
		_Log(spdlog::level::info, _MakeNTErrorMsg(msg, status), location);
	}

	void ComInfo(std::string_view msg, HRESULT hr, const SourceLocation& location = SourceLocation::Current()) noexcept {
		_Log(spdlog::level::info, _MakeComErrorMsg(msg, hr), location);
	}

	void Warn(std::string_view msg, const SourceLocation& location = SourceLocation::Current()) noexcept {
		_Log(spdlog::level::warn, msg, location);
	}

	void Win32Warn(std::string_view msg, const SourceLocation& location = SourceLocation::Current()) noexcept {
		_Log(spdlog::level::warn, _MakeWin32ErrorMsg(msg), location);
	}

	void NTWarn(std::string_view msg, NTSTATUS status, const SourceLocation& location = SourceLocation::Current()) noexcept {
		_Log(spdlog::level::warn, _MakeNTErrorMsg(msg, status), location);
	}

	void ComWarn(std::string_view msg, HRESULT hr, const SourceLocation& location = SourceLocation::Current()) noexcept {
		_Log(spdlog::level::warn, _MakeComErrorMsg(msg, hr), location);
	}

	void Error(std::string_view msg, const SourceLocation& location = SourceLocation::Current()) noexcept {
		_Log(spdlog::level::err, msg, location);
	}

	void Win32Error(std::string_view msg, const SourceLocation& location = SourceLocation::Current()) noexcept {
		_Log(spdlog::level::err, _MakeWin32ErrorMsg(msg), location);
	}

	void NTError(std::string_view msg, NTSTATUS status, const SourceLocation& location = SourceLocation::Current()) noexcept {
		_Log(spdlog::level::err, _MakeNTErrorMsg(msg, status), location);
	}

	void ComError(std::string_view msg, HRESULT hr, const SourceLocation& location = SourceLocation::Current()) noexcept {
		_Log(spdlog::level::err, _MakeComErrorMsg(msg, hr), location);
	}

	void Critical(std::string_view msg, const SourceLocation& location = SourceLocation::Current()) noexcept {
		_Log(spdlog::level::critical, msg, location);
	}

	void Win32Critical(std::string_view msg, const SourceLocation& location = SourceLocation::Current()) noexcept {
		_Log(spdlog::level::critical, _MakeWin32ErrorMsg(msg), location);
	}

	void NTCritical(std::string_view msg, NTSTATUS status, const SourceLocation& location = SourceLocation::Current()) noexcept {
		_Log(spdlog::level::critical, _MakeNTErrorMsg(msg, status), location);
	}

	void ComCritical(std::string_view msg, HRESULT hr, const SourceLocation& location = SourceLocation::Current()) noexcept {
		_Log(spdlog::level::critical, _MakeComErrorMsg(msg, hr), location);
	}

private:
	static std::string _MakeWin32ErrorMsg(std::string_view msg) noexcept;

	static std::string _MakeNTErrorMsg(std::string_view msg, NTSTATUS status) noexcept;

	static std::string _MakeComErrorMsg(std::string_view msg, HRESULT hr) noexcept;

	void _Log(spdlog::level::level_enum logLevel, std::string_view msg, const SourceLocation& location) noexcept;

	std::shared_ptr<spdlog::logger> _logger;
};

}
