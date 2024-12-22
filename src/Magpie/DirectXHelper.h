#pragma once
#include "Profile.h"

namespace Magpie {

struct DirectXHelper {
	static GraphicsCardId GetGraphicsCardIdFromIdx(int idx) noexcept;
};

}
