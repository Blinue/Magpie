#pragma once
#include "MagpieExtApi.h"
#include "Singleton.h"
#include "SmallVector.h"
#include <parallel_hashmap/phmap.h>

namespace Magpie {

class ExtensionService : public Singleton<ExtensionService> {
	friend Singleton<ExtensionService>;

public:
	void Initialize() noexcept;

private:
	ExtensionService() = default;

	bool _LoadExtension(std::string_view extId) noexcept;

	static HRESULT _HostApiRegisterExtension(const MagpieExtensionDesc* desc);

	// 按 dll 加载顺序排序，这样可以倒序卸载
	SmallVector<wil::unique_hmodule> _extDllHandles;

	struct _ExtensionData {
		uint32_t idx = 0;
		std::string displayName;
		std::string author;
		std::string homePageUrl;
		std::string description;
	};

	// extId -> ExtensionData
	phmap::flat_hash_map<std::string, _ExtensionData> _extDatas;
};

}
