#include "pch.h"
#include "Renderer.h"
#include "DeviceResources.h"
#include "ScalingOptions.h"

namespace Magpie::Core {

Renderer::Renderer() noexcept {}

Renderer::~Renderer() noexcept {}

bool Renderer::Initialize(HWND hwndScaling, const ScalingOptions& options) noexcept {
    _deviceResources = std::make_unique<DeviceResources>();
    if (!_deviceResources->Initialize(hwndScaling, options)) {
        return false;
    }

    return true;
}

void Renderer::Render() noexcept {
    WaitMessage();
}

}
