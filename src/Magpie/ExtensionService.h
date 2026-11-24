#pragma once
#include "ExtensionInfo.h"
#include "MagpieExtApi.h"
#include "Singleton.h"
#include "SmallVector.h"
#include <rapidjson/document.h>
#include <parallel_hashmap/phmap.h>

namespace Magpie {

class ExtensionService : public Singleton<ExtensionService> {
	friend Singleton<ExtensionService>;

public:
	void Initialize() noexcept;

	void Uninitialize() noexcept;

	bool AddExtension(
		std::string_view extId,
		const rapidjson::GenericObject<true, rapidjson::Value>& indexRootObj
	) noexcept;

	const ExtensionInfo* GetExtensionInfo(std::string_view extId) const noexcept;

private:
	ExtensionService() = default;

	void _LoadExtension(
		std::string_view extId,
		const rapidjson::GenericObject<true, rapidjson::Value>& indexRootObj
	) noexcept;

	static HRESULT _HostApiLog(
		MagpieExtContext context,
		MagpieLogSourceLocation sourceLocation,
		MagpieLogLevel level,
		const char* message
	);

	static HRESULT _HotApiGetLanguage(MagpieExtContext context, const char** language) noexcept;

	static HRESULT _HostApiRegisterExtension(MagpieExtContext context, const MagpieExtensionDesc* desc);

	struct _ExtensionData {
		ExtensionInfo info;
		wil::unique_hmodule hDll;
		// 记录 dll 加载顺序，从而倒序卸载
		uint32_t loadNo = 0;
		bool hasError = false;
	};

	using _ExtContextData = std::pair<const std::string, _ExtensionData>;

	// extId -> ExtensionData
	// 不使用 flat_hash_map 有两个原因:
	// 1. 需要确保地址稳定
	// 2. 键值的移动成本较高
	phmap::node_hash_map<std::string, _ExtensionData> _extDatas;

	uint32_t _nextLoadNo = 0;
};

}
