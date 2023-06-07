#pragma once
#include "WindowBase.h"
#include "ScalingOptions.h"

namespace Magpie::Core {

class Renderer;

class ScalingWindow : public WindowBase<ScalingWindow> {
	friend class base_type;

public:
	ScalingWindow() noexcept;
	~ScalingWindow() noexcept;

	bool Create(HINSTANCE hInstance, HWND hwndSrc, ScalingOptions&& options) noexcept;

	void Render() noexcept;

protected:
	LRESULT _MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

private:
	int _CheckSrcState() const noexcept;

	bool _CheckForeground(HWND hwndForeground) const noexcept;

	ScalingOptions _options;
	std::unique_ptr<Renderer> _renderer;

	HWND _hwndSrc = NULL;
	RECT _srcWndRect{};
};

}
