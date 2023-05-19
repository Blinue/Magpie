#pragma once

struct ImDrawData;

namespace Magpie::Core {

class ImGuiBackend {
public:
	ImGuiBackend() = default;
	ImGuiBackend(const ImGuiBackend&) = delete;
	ImGuiBackend(ImGuiBackend&&) = delete;

	bool Initialize() noexcept;

	void NewFrame() noexcept;
	void RenderDrawData(ImDrawData* drawData) noexcept;
	
private:
	bool _CreateDeviceObjects() noexcept;

	void _SetupRenderState(ImDrawData* drawData, ID3D11DeviceContext* ctx) noexcept;
	bool _CreateFontsTexture() noexcept;

	winrt::com_ptr<ID3D11Buffer> _vertexBuffer;
	int _vertexBufferSize = 5000;

	winrt::com_ptr<ID3D11Buffer> _indexBuffer;
	int _indexBufferSize = 10000;

	winrt::com_ptr<ID3D11VertexShader> _vertexShader;
	winrt::com_ptr<ID3D11InputLayout> _inputLayout;
	winrt::com_ptr<ID3D11Buffer> _vertexConstantBuffer;
	winrt::com_ptr<ID3D11PixelShader> _pixelShader;
	winrt::com_ptr<ID3D11SamplerState> _fontSampler;
	winrt::com_ptr<ID3D11ShaderResourceView> _fontTextureView;
	winrt::com_ptr<ID3D11BlendState> _blendState;
	winrt::com_ptr<ID3D11RasterizerState> _rasterizerState;
};

}
