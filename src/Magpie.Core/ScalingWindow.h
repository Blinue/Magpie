#pragma once
#include "WindowBase.h"

namespace Magpie::Core {

struct ScalingOptions;
class Renderer;

class ScalingWindow : public WindowBase<ScalingWindow> {
	friend class base_type;

public:
	ScalingWindow() noexcept;
	~ScalingWindow() noexcept;

	bool Create(HINSTANCE hInstance, ScalingOptions&& options) noexcept;

	void Render() noexcept;

protected:
	LRESULT _MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

private:
	std::unique_ptr<Renderer> _renderer;
};

}
