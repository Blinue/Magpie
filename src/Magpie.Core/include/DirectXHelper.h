#pragma once
#include <dxgi1_6.h>
#include <d3d11_4.h>

namespace Magpie {

struct DirectXHelper {
	static bool IsWARP(const DXGI_ADAPTER_DESC1& desc) noexcept {
		// 不要检查 DXGI_ADAPTER_FLAG_SOFTWARE 标志，如果系统没有安装任何显卡，WARP 没有这个标志。
		// 这两个值来自 https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/d3d10-graphics-programming-guide-dxgi#new-info-about-enumerating-adapters-for-windows-8
		return desc.VendorId == 0x1414 && desc.DeviceId == 0x8c;
	}

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
