#pragma once
#include "Singleton.h"

namespace Magpie {

class ExtensionService : public Singleton<ExtensionService> {
	friend Singleton<ExtensionService>;

public:
	bool Initialize() noexcept;

private:
	ExtensionService() = default;
};

}
