#include "pch.h"
#include "EffectsService.h"
#include <Magpie.Core.h>
#include "StrUtils.h"
#include "Win32Utils.h"
#include "CommonSharedConstants.h"

using namespace Magpie::Runtime;


namespace winrt::Magpie::App {

static void ListEffects(std::vector<std::wstring>& result, std::wstring_view prefix = {}) {
	WIN32_FIND_DATA findData{};
	HANDLE hFind = Win32Utils::SafeHandle(FindFirstFileEx(
		StrUtils::ConcatW(CommonSharedConstants::EFFECTS_DIR, prefix, L"*").c_str(),
		FindExInfoBasic, &findData, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH));
	if (hFind) {
		do {
			std::wstring_view fileName(findData.cFileName);
			if (fileName == L"." || fileName == L"..") {
				continue;
			}

			if (Win32Utils::DirExists(StrUtils::ConcatW(CommonSharedConstants::EFFECTS_DIR, prefix, fileName).c_str())) {
				ListEffects(result, StrUtils::ConcatW(prefix, fileName, L"\\"));
				continue;
			}

			if (!fileName.ends_with(L".hlsl")) {
				continue;
			}

			result.emplace_back(StrUtils::ConcatW(prefix, fileName.substr(0, fileName.size() - 5)));
		} while (FindNextFile(hFind, &findData));

		FindClose(hFind);
	} else {
		Logger::Get().Win32Error("查找缓存文件失败");
	}
}

fire_and_forget EffectsService::StartInitialize() {
	co_await resume_background();

	std::vector<std::wstring> effectNames;
	ListEffects(effectNames);
	uint32_t nEffect = (uint32_t)effectNames.size();

	std::vector<EffectDesc> descs(nEffect);
	Win32Utils::RunParallel([&](uint32_t id) {
		descs[id].name = StrUtils::UTF16ToUTF8(effectNames[id]);
		if (EffectCompiler::Compile(descs[id], EffectCompilerFlags::NoCompile)) {
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
