#include "pch.h"
#include "EffectsService.h"
#include <Magpie.Core.h>
#include "StrUtils.h"
#include "Win32Utils.h"
#include "CommonSharedConstants.h"
#include "Logger.h"
#include <d3dcompiler.h>	// ID3DBlob
#include <Magpie.Core.h>

using namespace Magpie::Core;

namespace winrt::Magpie::App {

EffectInfo::EffectInfo() {}

EffectInfo::~EffectInfo() {}

static void ListEffects(std::vector<std::wstring>& result, std::wstring_view prefix = {}) {
	result.reserve(80);

	WIN32_FIND_DATA findData{};
	HANDLE hFind = Win32Utils::SafeHandle(FindFirstFileEx(
		StrUtils::Concat(CommonSharedConstants::EFFECTS_DIR, prefix, L"*").c_str(),
		FindExInfoBasic, &findData, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH));
	if (hFind) {
		do {
			std::wstring_view fileName(findData.cFileName);
			if (fileName == L"." || fileName == L"..") {
				continue;
			}

			if (Win32Utils::DirExists(StrUtils::Concat(CommonSharedConstants::EFFECTS_DIR, prefix, fileName).c_str())) {
				ListEffects(result, StrUtils::Concat(prefix, fileName, L"\\"));
				continue;
			}

			if (!fileName.ends_with(L".hlsl")) {
				continue;
			}

			result.emplace_back(StrUtils::Concat(prefix, fileName.substr(0, fileName.size() - 5)));
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

	_effectsMap.reserve(nEffect);
	for (uint32_t i = 0; i < nEffect; ++i) {
		EffectDesc& effectDesc = descs[i];
		if (effectDesc.name.empty()) {
			continue;
		}

		// 这里修改 _effects 无需同步，因为用户界面尚未显示
		EffectInfo& effect = _effects.emplace_back();
		effect.name = std::move(effectNames[i]);

		if (effectDesc.sortName.empty()) {
			effect.sortName = effect.name;
		} else {
			size_t pos = effect.name.find_last_of(L'\\');
			if (pos == std::wstring::npos) {
				effect.sortName = StrUtils::UTF8ToUTF16(effectDesc.sortName);
			} else {
				effect.sortName = StrUtils::Concat(
					std::wstring_view(effect.name.c_str(), pos + 1),
					StrUtils::UTF8ToUTF16(effectDesc.sortName)
				);
			}
		}
		
		effect.params = std::move(effectDesc.params);
		if (effectDesc.outSizeExpr.first.empty()) {
			effect.flags |= EffectInfoFlags::CanScale;
		}
		if (effectDesc.flags & EffectFlags::GenericDownscaler) {
			effect.flags |= EffectInfoFlags::GenericDownscaler;
		}

		_effectsMap.emplace(effect.name, (uint32_t)_effects.size() - 1);
	}

	_initialized.store(true, std::memory_order_release);
}

void EffectsService::WaitForInitialize() {
	while (!_initialized.load(std::memory_order_acquire)) {
		Sleep(0);
	}
}

}
