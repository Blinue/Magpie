#pragma once
#include "pch.h"


namespace Magpie::Core {

struct DXUtils {
	static bool CompileComputeShader(
		std::string_view hlsl,
		const char* entryPoint,
		ID3DBlob** blob,
		const char* sourceName = nullptr,
		ID3DInclude* include = nullptr,
		const std::vector<std::pair<std::string, std::string>>& macros = {},
		bool warningsAreErrors = false
	);
};

}
