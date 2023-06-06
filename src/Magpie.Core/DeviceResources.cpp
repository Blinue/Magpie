#include "pch.h"
#include "DeviceResources.h"
#include "ScalingOptions.h"

namespace Magpie::Core {

bool DeviceResources::Initialize(HWND /*hwndScaling*/, const ScalingOptions& /*options*/) noexcept {
	return true;
}

bool DeviceResources::IsDebugLayersAvailable() noexcept {
#ifdef _DEBUG
	static bool result = SUCCEEDED(D3D11CreateDevice(
		nullptr,
		D3D_DRIVER_TYPE_NULL,       // There is no need to create a real hardware device.
		nullptr,
		D3D11_CREATE_DEVICE_DEBUG,  // Check for the SDK layers.
		nullptr,                    // Any feature level will do.
		0,
		D3D11_SDK_VERSION,
		nullptr,                    // No need to keep the D3D device reference.
		nullptr,                    // No need to know the feature level.
		nullptr                     // No need to keep the D3D device context reference.
	));
	return result;
#else
	// Relaese 配置不使用调试层
	return false;
#endif
}

}
