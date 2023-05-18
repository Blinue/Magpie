#pragma once

struct ImDrawData;

namespace Magpie::Core {

class ImGuiBackend {
public:
	ImGuiBackend() = default;
	ImGuiBackend(const ImGuiBackend&) = delete;
	ImGuiBackend(ImGuiBackend&&) = delete;

	~ImGuiBackend() noexcept;

	bool Initialize() noexcept;

	void NewFrame() noexcept;
	void RenderDrawData(ImDrawData* draw_data) noexcept;

	// Use if you want to reset your rendering device without losing Dear ImGui state.
	void InvalidateDeviceObjects() noexcept;
	bool CreateDeviceObjects() noexcept;
};

}
