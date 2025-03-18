#pragma once
#include <imgui.h>

namespace Magpie {

class DeviceResources;

class ImGuiBackend {
public:
	ImGuiBackend() = default;
	ImGuiBackend(const ImGuiBackend&) = delete;
	ImGuiBackend(ImGuiBackend&&) = delete;

	bool Initialize(DeviceResources* deviceResources) noexcept;

	bool BuildFonts() noexcept;

	void RenderDrawData(const ImDrawData& drawData, POINT viewportOffset) noexcept;

private:
	bool _CreateDeviceObjects() noexcept;

	void _SetupRenderState(const ImDrawData& drawData, POINT viewportOffset) noexcept;

	DeviceResources* _deviceResources = nullptr;

	winrt::com_ptr<ID3D11Buffer> _vertexBuffer;
	int _vertexBufferSize = 5000;

	winrt::com_ptr<ID3D11Buffer> _indexBuffer;
	int _indexBufferSize = 10000;

	winrt::com_ptr<ID3D11VertexShader> _vertexShader;
	winrt::com_ptr<ID3D11InputLayout> _inputLayout;
	winrt::com_ptr<ID3D11Buffer> _vertexConstantBuffer;
	winrt::com_ptr<ID3D11PixelShader> _pixelShader;
	winrt::com_ptr<ID3D11ShaderResourceView> _fontTextureView;
	winrt::com_ptr<ID3D11BlendState> _blendState;
	winrt::com_ptr<ID3D11RasterizerState> _rasterizerState;
};

}
