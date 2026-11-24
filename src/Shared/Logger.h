#pragma once
#include "Singleton.h"
#if defined(_DEBUG) && defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
#endif
#include <spdlog/spdlog.h>
#if defined(_DEBUG) && defined(__clang__)
#pragma clang diagnostic pop
#endif

namespace Magpie {

// std::source_location 中的函数名包含整个签名过于冗长，我们只需记录函数名，
// 因此创建自己的 SourceLocation。
struct SourceLocation {
	[[nodiscard]] static consteval SourceLocation Current(
		int line = __builtin_LINE(),
		const char* file = __builtin_FILE(),
		const char* function = __builtin_FUNCTION()
	) noexcept {
		return SourceLocation{ line, file, function };
	}

	[[nodiscard]] constexpr SourceLocation() noexcept = default;

	[[nodiscard]] constexpr SourceLocation(
		int line,
		const char* file,
		const char* function
	) noexcept : _line(line), _file(file), _function(function) {}

	[[nodiscard]] constexpr int GetLine() const noexcept {
		return _line;
	}

	[[nodiscard]] constexpr const char* GetFileName() const noexcept {
		return _file;
	}

	constexpr const char* GetFunctionName() const noexcept {
		return _function;
	}

private:
	const int _line = 0;
	const char* _file = nullptr;
	const char* _function = nullptr;
};

class Logger : public Singleton<Logger> {
	friend Singleton<Logger>;

public:
	bool Initialize(spdlog::level::level_enum logLevel, std::wstring logFileName, int logArchiveAboveSize, int logMaxArchiveFiles) noexcept;

	void SetLevel(spdlog::level::level_enum logLevel) noexcept;

	void Flush() noexcept;

	void Info(std::string_view msg, const SourceLocation& location = SourceLocation::Current()) noexcept;
	void Win32Info(std::string_view msg, const SourceLocation& location = SourceLocation::Current()) noexcept;
	void NTInfo(std::string_view msg, NTSTATUS status, const SourceLocation& location = SourceLocation::Current()) noexcept;
	void ComInfo(std::string_view msg, HRESULT hr, const SourceLocation& location = SourceLocation::Current()) noexcept;

	void Warn(std::string_view msg, const SourceLocation& location = SourceLocation::Current()) noexcept;
	void Win32Warn(std::string_view msg, const SourceLocation& location = SourceLocation::Current()) noexcept;
	void NTWarn(std::string_view msg, NTSTATUS status, const SourceLocation& location = SourceLocation::Current()) noexcept;
	void ComWarn(std::string_view msg, HRESULT hr, const SourceLocation& location = SourceLocation::Current()) noexcept;

	void Error(std::string_view msg, const SourceLocation& location = SourceLocation::Current()) noexcept;
	void Win32Error(std::string_view msg, const SourceLocation& location = SourceLocation::Current()) noexcept;
	void NTError(std::string_view msg, NTSTATUS status, const SourceLocation& location = SourceLocation::Current()) noexcept;
	void ComError(std::string_view msg, HRESULT hr, const SourceLocation& location = SourceLocation::Current()) noexcept;

	void Critical(std::string_view msg, const SourceLocation& location = SourceLocation::Current()) noexcept;
	void Win32Critical(std::string_view msg, const SourceLocation& location = SourceLocation::Current()) noexcept;
	void NTCritical(std::string_view msg, NTSTATUS status, const SourceLocation& location = SourceLocation::Current()) noexcept;
	void ComCritical(std::string_view msg, HRESULT hr, const SourceLocation& location = SourceLocation::Current()) noexcept;

private:
	Logger() = default;

	void _Log(spdlog::level::level_enum logLevel, std::string_view msg, const SourceLocation& location) noexcept;

	std::shared_ptr<spdlog::logger> _logger;
};

}
