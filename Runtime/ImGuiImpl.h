#pragma once
#include "pch.h"


class ImGuiImpl {
public:
	ImGuiImpl() = default;
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
	ID3D11RenderTargetView* _rtv = nullptr;
	UINT _handlerId = 0;

	HANDLE _hHookThread = NULL;
	DWORD _hookThreadId = 0;
	std::atomic<float> _wheelData = 0;
};
