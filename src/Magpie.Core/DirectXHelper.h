#pragma once

namespace Magpie {

struct DirectXHelper {
	static bool CompileComputeShader(
		std::string_view hlsl,
		const char* entryPoint,
		ID3DBlob** blob,
		const char* sourceName = nullptr,
		ID3DInclude* include = nullptr,
		const std::vector<std::pair<std::string, std::string>>& macros = {},
		bool warningsAreErrors = false
	);

	static bool IsDebugLayersAvailable() noexcept;

	static winrt::com_ptr<ID3D11Texture2D> CreateTexture2D(
		ID3D11Device* d3dDevice,
		DXGI_FORMAT format,
		UINT width,
		UINT height,
		UINT bindFlags,
		D3D11_USAGE usage = D3D11_USAGE_DEFAULT,
		UINT miscFlags = 0,
		const D3D11_SUBRESOURCE_DATA* pInitialData = nullptr
	) noexcept;
};

}
