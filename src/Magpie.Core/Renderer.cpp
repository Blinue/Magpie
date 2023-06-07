#include "pch.h"
#include "Renderer.h"
#include "DeviceResources.h"
#include "ScalingOptions.h"
#include "Logger.h"
#include "Win32Utils.h"

namespace Magpie::Core {

Renderer::Renderer() noexcept {}

Renderer::~Renderer() noexcept {
	if (_backendThread.joinable()) {
		DWORD backendThreadId = GetThreadId(_backendThread.native_handle());
		// 持续尝试直到 _backendThread 创建了消息队列
		while (!PostThreadMessage(backendThreadId, WM_QUIT, 0, 0)) {
			Sleep(1);
		}
		_backendThread.join();
	}
}

bool Renderer::Initialize(HWND /*hwndSrc*/, HWND hwndScaling, const ScalingOptions& options) noexcept {
	_backendThread = std::thread(std::bind(&Renderer::_BackendThreadProc, this, hwndScaling, options));

	_frontendResources = std::make_unique<DeviceResources>();
	if (!_frontendResources->Initialize(hwndScaling, options)) {
		return false;
	}

	return true;
}

void Renderer::Render() noexcept {
	
}

void Renderer::_BackendThreadProc(HWND hwndScaling, const ScalingOptions& options) noexcept {
	winrt::init_apartment(winrt::apartment_type::single_threaded);

	DeviceResources deviceResources;
	if (!deviceResources.Initialize(hwndScaling, options)) {
		return;
	}

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		DispatchMessage(&msg);
	}
}

}
