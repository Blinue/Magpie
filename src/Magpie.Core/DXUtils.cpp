#include "pch.h"
#include "DXUtils.h"
#include <d3dcompiler.h>
#include "Logger.h"
#include "StrUtils.h"


bool Magpie::Core::DXUtils::CompileComputeShader(
	std::string_view hlsl,
	const char* entryPoint,
	ID3DBlob** blob,
	const char* sourceName,
	ID3DInclude* include,
	const std::vector<std::pair<std::string, std::string>>& macros,
	bool warningsAreErrors
) {
	winrt::com_ptr<ID3DBlob> errorMsgs = nullptr;

	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_ALL_RESOURCES_BOUND;
	if (warningsAreErrors) {
		flags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;
	}

#ifdef _DEBUG
	flags |= D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_DEBUG;
#else
	flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif // _DEBUG

	std::unique_ptr<D3D_SHADER_MACRO[]> mc(new D3D_SHADER_MACRO[macros.size() + 1]);
	for (UINT i = 0; i < macros.size(); ++i) {
		mc[i] = { macros[i].first.c_str(), macros[i].second.c_str() };
	}
	mc[macros.size()] = { nullptr,nullptr };

	HRESULT hr = D3DCompile(hlsl.data(), hlsl.size(), sourceName, mc.get(), include,
		entryPoint, "cs_5_0", flags, 0, blob, errorMsgs.put());
	if (FAILED(hr)) {
		if (errorMsgs) {
			Logger::Get().ComError(StrUtils::Concat("编译计算着色器失败：", (const char*)errorMsgs->GetBufferPointer()), hr);
		}
		return false;
	}

	// 警告消息
	if (errorMsgs) {
		Logger::Get().Warn(StrUtils::Concat("编译计算着色器时产生警告：", (const char*)errorMsgs->GetBufferPointer()));
	}

	return true;
}
