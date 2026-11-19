#pragma once
#include "ExtensionInfo.h"
#include "MagpieExtApi.h"
#include "Singleton.h"
#include "SmallVector.h"
#include <parallel_hashmap/phmap.h>

namespace Magpie {

class ExtensionService : public Singleton<ExtensionService> {
	friend Singleton<ExtensionService>;

public:
	void Initialize() noexcept;

	void Uninitialize() noexcept;

	bool AddExtension(std::string_view extId, std::string_view updateUrl) noexcept;

	std::string GenerateImportedExtId() noexcept;

private:
	ExtensionService() = default;

	bool _LoadExtension(std::string_view extId) noexcept;

	static HRESULT _HostApiRegisterExtension(const MagpieExtensionDesc* desc);

	struct _ExtensionData {
		ExtensionInfo info;
		wil::unique_hmodule hDll;
		// 记录 dll 加载顺序，从而倒序卸载
		uint32_t loadNo = 0;
	};

	// extId -> ExtensionData
	phmap::flat_hash_map<std::string, _ExtensionData> _extDatas;

	uint32_t _nextLoadNo = 0;
};

}
