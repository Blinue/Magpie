#pragma once
#include "ImGuiBackend.h"

namespace Magpie {

class DeviceResources;

class ImGuiImpl {
public:
	ImGuiImpl() = default;
	ImGuiImpl(const ImGuiImpl&) = delete;
	ImGuiImpl(ImGuiImpl&&) = delete;

	~ImGuiImpl() noexcept;

	bool Initialize(DeviceResources& deviceResource) noexcept;

	bool BuildFonts() noexcept;

	void NewFrame(float fittsLawAdjustment) noexcept;

	void Draw(POINT drawOffset) noexcept;

	void ClearStates() noexcept;

	void MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

	std::optional<ImVec4> GetWindowRect(const char* id) const noexcept;

	const char* GetHoveredWindowId() const noexcept;

	// 将提示窗口限制在屏幕内
	void Tooltip(const char* content, float maxWidth = -1.0f) noexcept;
private:
	void _UpdateMousePos(float fittsLawAdjustment) noexcept;

	ImGuiBackend _backend;

	uint32_t _handlerId = 0;

	HANDLE _hHookThread = NULL;
	DWORD _hookThreadId = 0;
};

}
