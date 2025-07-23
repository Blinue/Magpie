#include "pch.h"
#include "ScreenshotHelper.h"
#include "Logger.h"
#include "StrHelper.h"
#include <charconv>

namespace Magpie {

static bool ExtractNumber(std::wstring_view fileName, std::string& numStr, uint32_t& curNum) noexcept {
	const size_t dotPos = fileName.find_last_of(L'.');
	if (dotPos == std::wstring_view::npos || dotPos < 8) {
		return false;
	}

	// "Magpie_" 共 7 个字符
	size_t len = dotPos - 7;
	numStr.resize(len);

	for (size_t i = 0; i < len; ++i) {
		wchar_t c = fileName[i + 7];
		if (c >= L'0' && c <= L'9') {
			numStr[i] = (char)c;
		} else {
			return false;
		}
	}

	return std::from_chars(numStr.c_str(), numStr.c_str() + numStr.size(), curNum).ec == std::errc{};
}

uint32_t ScreenshotHelper::FindUnusedScreenshotNum(const std::filesystem::path& screenshotsDir) noexcept {
	WIN32_FIND_DATA findData{};
	const std::wstring pattern = StrHelper::Concat(screenshotsDir.native(), L"\\Magpie_*");
	wil::unique_hfind hFind(FindFirstFileEx(
		pattern.c_str(), FindExInfoBasic, &findData, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH));
	if (!hFind) {
		Logger::Get().Win32Error("FindFirstFileEx 失败");
		return 0;
	}

	// 新截图应在所有现有截图之后，因此查找最大序号。如果最大序号是 UINT_MAX 则回落
	// 到查找最小的可用序号，不过这种数据除了特意构造不可能出现
	uint32_t result = 0;

	std::string numStr;
	std::vector<uint32_t> nums;
	bool shouldFallback = false;
	do {
		uint32_t curNum;
		if (!ExtractNumber(findData.cFileName, numStr, curNum) || curNum == 0) {
			continue;
		}

		nums.push_back(curNum);

		if (shouldFallback) {
			continue;
		}
		
		if (curNum == std::numeric_limits<uint32_t>::max()) {
			// 回落到查找最小的可用序号
			shouldFallback = true;
			continue;
		}

		result = std::max(result, curNum + 1);
	} while (FindNextFile(hFind.get(), &findData));

	if (!shouldFallback) {
		return result;
	}

	// 查找最小的可用序号
	std::sort(nums.begin(), nums.end());
	assert(nums.back() == std::numeric_limits<uint32_t>::max());

	if (nums[0] != 1) {
		return 1;
	}

	const size_t size = nums.size();
	for (size_t i = 1; i < size; ++i) {
		result = nums[i - 1] + 1;
		if (nums[i] > result) {
			return result;
		}
	}

	// 不可能执行到这里
	return 0;
}

}
