#include "pch.h"
#include "EffectsService.h"
#include "StrHelper.h"
#include "Win32Helper.h"
#include "CommonSharedConstants.h"
#include "Logger.h"
#include <d3dcompiler.h>	// ID3DBlob
#include "EffectCompiler.h"
#include "EffectDesc.h"

using namespace Magpie::Core;

namespace winrt::Magpie {

EffectInfo::EffectInfo() {}

EffectInfo::~EffectInfo() {}

static void ListEffects(std::vector<std::wstring>& result, std::wstring_view prefix = {}) {
	result.reserve(80);

	WIN32_FIND_DATA findData{};
	wil::unique_hfind hFind(FindFirstFileEx(
		StrHelper::Concat(CommonSharedConstants::EFFECTS_DIR, prefix, L"*").c_str(),
		FindExInfoBasic, &findData, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH));
	if (hFind) {
		do {
			std::wstring_view fileName(findData.cFileName);
			if (fileName == L"." || fileName == L"..") {
				continue;
			}

			if (Win32Helper::DirExists(StrHelper::Concat(CommonSharedConstants::EFFECTS_DIR, prefix, fileName).c_str())) {
				ListEffects(result, StrHelper::Concat(prefix, fileName, L"\\"));
				continue;
			}

			if (!fileName.ends_with(L".hlsl")) {
				continue;
			}

			result.emplace_back(StrHelper::Concat(prefix, fileName.substr(0, fileName.size() - 5)));
		} while (FindNextFile(hFind.get(), &findData));
	} else {
		Logger::Get().Win32Error("查找缓存文件失败");
	}
}

fire_and_forget EffectsService::StartInitialize() {
	co_await resume_background();

	std::vector<std::wstring> effectNames;
	ListEffects(effectNames);

	const uint32_t nEffect = (uint32_t)effectNames.size();
	_effectsMap.reserve(nEffect);
	_effects.reserve(nEffect);

	// 用于同步 _effectsMap 和 _effects 的初始化
	wil::srwlock srwLock;

	// 并行解析效果
	Win32Helper::RunParallel([&](uint32_t id) {
		EffectDesc effectDesc;

		effectDesc.name = StrHelper::UTF16ToUTF8(effectNames[id]);
		if (EffectCompiler::Compile(effectDesc, EffectCompilerFlags::NoCompile)) {
			return;
		}

		EffectInfo effect;
		effect.name = std::move(effectNames[id]);

		if (effectDesc.sortName.empty()) {
			effect.sortName = effect.name;
		} else {
			size_t pos = effect.name.find_last_of(L'\\');
			if (pos == std::wstring::npos) {
				effect.sortName = StrHelper::UTF8ToUTF16(effectDesc.sortName);
			} else {
				effect.sortName = StrHelper::Concat(
					std::wstring_view(effect.name.c_str(), pos + 1),
					StrHelper::UTF8ToUTF16(effectDesc.sortName)
				);
			}
		}

		effect.params = std::move(effectDesc.params);
		if (effectDesc.GetOutputSizeExpr().first.empty()) {
			effect.flags |= EffectInfoFlags::CanScale;
		}

		auto lock = srwLock.lock_exclusive();
		_effectsMap.emplace(effect.name, (uint32_t)_effects.size());
		_effects.emplace_back(std::move(effect));
	}, nEffect);

	_initialized.store(true, std::memory_order_release);
	_initialized.notify_one();
}

void EffectsService::WaitForInitialize() {
	_initialized.wait(false, std::memory_order_acquire);
}

}
