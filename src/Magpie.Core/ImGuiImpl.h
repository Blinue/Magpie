#pragma once
#include "ImGuiBackend.h"

namespace Magpie::Core {

class DeviceResources;

class ImGuiImpl {
public:
	ImGuiImpl() = default;
	ImGuiImpl(const ImGuiImpl&) = delete;
	ImGuiImpl(ImGuiImpl&&) = delete;

	~ImGuiImpl() noexcept;

	bool Initialize(DeviceResources* deviceResource) noexcept;

	bool BuildFonts() noexcept;

	void NewFrame() noexcept;

	void Draw() noexcept;

	void ClearStates() noexcept;

	void MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

	// 将提示窗口限制在屏幕内
	static void Tooltip(const char* content, float maxWidth = -1.0f) noexcept;
private:
	void _UpdateMousePos() noexcept;

	ImGuiBackend _backend;

	uint32_t _handlerId = 0;

	HANDLE _hHookThread = NULL;
	DWORD _hookThreadId = 0;
};

}
