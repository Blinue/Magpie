#include "pch.h"
#include "EffectsService.h"
#include <Runtime.h>
#include "StrUtils.h"
#include "Win32Utils.h"
#include "CommonSharedConstants.h"

using namespace Magpie::Runtime;


namespace winrt::Magpie::App {

static std::vector<std::wstring> ListEffects() {
	std::vector<std::wstring> result;

	WIN32_FIND_DATA findData{};
	HANDLE hFind = Win32Utils::SafeHandle(FindFirstFileEx(
		StrUtils::ConcatW(CommonSharedConstants::EFFECTS_DIR, L"*.hlsl").c_str(),
		FindExInfoBasic, &findData, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH));
	if (hFind) {
		do {
			result.emplace_back(findData.cFileName);
		} while (FindNextFile(hFind, &findData));

		FindClose(hFind);
	} else {
		Logger::Get().Win32Error("查找缓存文件失败");
	}

	return result;
}

void EffectsService::Initialize() {
	std::vector<std::wstring> effectFiles = ListEffects();
	
}

}
