#pragma once
#include "WindowBase.h"

namespace Magpie::Core {

class ScalingWindow : public WindowBase<ScalingWindow> {
	friend class base_type;

public:
	bool Create(HINSTANCE hInstance) noexcept;

	void Render() noexcept;
};

}
