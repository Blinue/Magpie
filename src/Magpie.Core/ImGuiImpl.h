#pragma once

namespace Magpie::Core {

class ImGuiBackend;

class ImGuiImpl {
public:
	ImGuiImpl();
	ImGuiImpl(const ImGuiImpl&) = delete;
	ImGuiImpl(ImGuiImpl&&) = delete;

	~ImGuiImpl();

	bool Initialize();

	void NewFrame();

	void EndFrame();

	void ClearStates();

	// 将提示窗口限制在屏幕内
	static void Tooltip(const char* content, float maxWidth = -1.0f);
private:
	std::unique_ptr<ImGuiBackend> _backend;

	ID3D11RenderTargetView* _rtv = nullptr;
	uint32_t _handlerId = 0;

	HANDLE _hHookThread = NULL;
	DWORD _hookThreadId = 0;
};

}
