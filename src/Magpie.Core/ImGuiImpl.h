#pragma once
#include "ImGuiBackend.h"
#include <parallel_hashmap/phmap.h>

namespace Magpie {

class DeviceResources;
struct OverlayWindowOption;

class ImGuiImpl {
public:
	ImGuiImpl() = default;
	ImGuiImpl(const ImGuiImpl&) = delete;
	ImGuiImpl(ImGuiImpl&&) = delete;

	~ImGuiImpl() noexcept;

	bool Initialize(DeviceResources& deviceResource) noexcept;

	bool BuildFonts() noexcept;

	void NewFrame(
		phmap::flat_hash_map<std::string, OverlayWindowOption>& windowOptions,
		float fittsLawAdjustment,
		float dpiScale
	) noexcept;

	void Draw(POINT drawOffset) noexcept;

	void ClearStates() noexcept;

	void MessageHandler(UINT msg, WPARAM wParam, LPARAM lParam) noexcept;

	std::optional<ImVec4> GetWindowRect(const char* id) const noexcept;

	const char* GetHoveredWindowId() const noexcept;

	// 将提示窗口限制在屏幕内
	void Tooltip(
		const char* content,
		float dpiScale,
		const char* description = nullptr,
		float maxWidth = -1.0f
	) noexcept;
private:
	void _UpdateMousePos(float fittsLawAdjustment) noexcept;

	ImGuiBackend _backend;

	phmap::flat_hash_map<std::string, ImVec4> _windowRects;

	uint32_t _handlerId = 0;

	HANDLE _hHookThread = NULL;
	DWORD _hookThreadId = 0;
};

}
