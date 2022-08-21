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
			size_t len = StrUtils::StrLen(findData.cFileName);
			if (len <= 5) {
				continue;
			}

			result.emplace_back(findData.cFileName, len - 5);
		} while (FindNextFile(hFind, &findData));

		FindClose(hFind);
	} else {
		Logger::Get().Win32Error("查找缓存文件失败");
	}

	return result;
}

fire_and_forget EffectsService::StartInitialize() {
	co_await resume_background();

	std::vector<std::wstring> effectNames = ListEffects();
	uint32_t nEffect = (uint32_t)effectNames.size();

	std::vector<EffectDesc> descs(nEffect);
	Win32Utils::RunParallel([&](uint32_t id) {
		descs[id].name = StrUtils::UTF16ToUTF8(effectNames[id]);
		if (!EffectCompiler::Compile(descs[id], EffectCompilerFlags::NoCompile)) {
			descs[id].name.clear();
		}
	}, nEffect);

	for (uint32_t i = 0; i < nEffect; ++i) {
		if (descs[i].name.empty()) {
			continue;
		}

		// 这里修改 _effects 无需同步，因为用户界面尚未显示
		EffectInfo& effect = _effects.emplace_back();
		effect.name = std::move(effectNames[i]);
		effect.params = std::move(descs[i].params);
		effect.hasScale = !descs[i].outSizeExpr.first.empty();
	}

	_initialized = true;
}

void EffectsService::WaitForInitialize() {
	while (!_initialized) {
		Sleep(0);
	}
}

}
