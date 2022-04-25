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

private:
	ID3D11RenderTargetView* _rtv = nullptr;
	UINT _handlerId = 0;

	HANDLE _hHookThread = NULL;
	DWORD _hookThreadId = 0;
	std::atomic<float> _wheelData = 0;
};
