#include "pch.h"
#include "EffectDrawer.h"
#include "App.h"
#include "Utils.h"
#include <VertexTypes.h>
#include "EffectCompiler.h"

#ifdef _UNICODE
#undef _UNICODE
// Conan 的 muparser 不含 UNICODE 支持
#include <muParser.h>
#define _UNICODE
#else
#include <muParser.h>
#endif


extern std::shared_ptr<spdlog::logger> logger;

// 所有 EffectDrawer 共享一个实例
static mu::Parser exprParser;

void SetExprVars(SIZE inputSize, SIZE outputSize) {
	assert(inputSize.cx > 0 && inputSize.cy > 0);

	static double inputWidth = 0;
	static double inputHeight = 0;
	static double inputPtX = 0;
	static double inputPtY = 0;
	static double outputWidth = 0;
	static double outputHeight = 0;
	static double outputPtX = 0;
	static double outputPtY = 0;
	static double scaleX = 0;
	static double scaleY = 0;

	static bool init = false;

	if (!init) {
		init = true;

		exprParser.DefineVar("INPUT_WIDTH", &inputWidth);
		exprParser.DefineVar("INPUT_HEIGHT", &inputHeight);
		exprParser.DefineVar("INPUT_PT_X", &inputPtX);
		exprParser.DefineVar("INPUT_PT_Y", &inputPtY);
		exprParser.DefineVar("OUTPUT_WIDTH", &outputWidth);
		exprParser.DefineVar("OUTPUT_HEIGHT", &outputHeight);
		exprParser.DefineVar("OUTPUT_PT_X", &outputPtX);
		exprParser.DefineVar("OUTPUT_PT_Y", &outputPtY);
		exprParser.DefineVar("SCALE_X", &scaleX);
		exprParser.DefineVar("SCALE_Y", &scaleY);
	}

	inputWidth = inputSize.cx;
	inputHeight = inputSize.cy;
	inputPtX = 1.0f / inputSize.cx;
	inputPtY = 1.0f / inputSize.cy;
	outputWidth = outputSize.cx;
	outputHeight = outputSize.cy;
	outputPtX = 1.0f / outputSize.cx;
	outputPtY = 1.0f / outputSize.cy;
	scaleX = inputPtX * outputWidth;
	scaleY = inputPtY * outputHeight;
}


bool EffectDrawer::Initialize(const wchar_t* fileName) {
	bool result = false;
	int duration = Utils::Measure([&]() {
		result = !EffectCompiler::Compile(fileName, _effectDesc);
	});

	if (!result) {
		SPDLOG_LOGGER_ERROR(logger, fmt::format("编译 {} 失败", StrUtils::UTF16ToUTF8(fileName)));
		return false;
	} else {
		SPDLOG_LOGGER_INFO(logger, fmt::format("编译 {} 用时 {} 毫秒", StrUtils::UTF16ToUTF8(fileName), duration / 1000.0f));
	}

	Renderer& renderer = App::GetInstance().GetRenderer();
	_d3dDevice = renderer.GetD3DDevice();
	_d3dDC = renderer.GetD3DDC();

	_samplers.resize(_effectDesc.passes.size());
	for (int i = 0; i < _samplers.size(); ++i) {
		if (!renderer.GetSampler(_effectDesc.samplers[i].filterType, &_samplers[i])) {
			SPDLOG_LOGGER_ERROR(logger, fmt::format("创建采样器 {} 失败", _effectDesc.samplers[i].name));
			return false;
		}
	}

	_passes.resize(_effectDesc.passes.size());
	for (int i = 0; i < _passes.size(); ++i) {
		if (!_passes[i].Initialize(this, _effectDesc.passes[i].cso)) {
			SPDLOG_LOGGER_ERROR(logger, fmt::format("Pass{} 初始化失败", i + 1));
			return false;
		}
	}

	// 大小必须为 4 的倍数
	_constants.resize((_effectDesc.constants.size() + _effectDesc.valueConstants.size() + 3) / 4 * 4);

	// 设置常量默认值
	for (int i = 0; i < _effectDesc.constants.size(); ++i) {
		const auto& c = _effectDesc.constants[i];
		if (c.type == EffectConstantType::Float) {
			_constants[i].floatVal = std::get<float>(c.defaultValue);
		} else {
			_constants[i].intVal = std::get<int>(c.defaultValue);
		}
	}

	// 用于快速查找常量名
	for (UINT i = 0; i < _effectDesc.constants.size(); ++i) {
		_constNamesMap.emplace(_effectDesc.constants[i].name, i);
	}
	
	return true;
}

bool EffectDrawer::SetConstant(std::string_view name, float value) {
	auto it = _constNamesMap.find(name);
	if (it == _constNamesMap.end()) {
		return false;
	}
	UINT index = it->second;

	const auto& desc = _effectDesc.constants[index];
	if (desc.type != EffectConstantType::Float) {
		return false;
	}

	if (_constants[index].floatVal == value) {
		return true;
	}

	// 检查是否是合法的值
	if (desc.minValue.index() == 1) {
		if (value < std::get<float>(desc.minValue)) {
			return false;
		}
	}

	if (desc.maxValue.index() == 1) {
		if (value > std::get<float>(desc.maxValue)) {
			return false;
		}
	}

	_constants[index].floatVal = value;

	return true;
}

bool EffectDrawer::SetConstant(std::string_view name, int value) {
	auto it = _constNamesMap.find(name);
	if (it == _constNamesMap.end()) {
		return false;
	}
	UINT index = it->second;

	const auto& desc = _effectDesc.constants[index];
	if (desc.type != EffectConstantType::Int) {
		return false;
	}

	if (_constants[index].intVal == value) {
		return true;
	}

	// 检查是否是合法的值
	if (desc.minValue.index() == 2) {
		if (value < std::get<int>(desc.minValue)) {
			return false;
		}
	}

	if (desc.maxValue.index() == 2) {
		if (value > std::get<int>(desc.maxValue)) {
			return false;
		}
	}

	_constants[index].intVal = value;

	return true;
}

bool EffectDrawer::CalcOutputSize(SIZE inputSize, SIZE& outputSize) const {
	if (CanSetOutputSize()) {
		outputSize = _outputSize.has_value() ? _outputSize.value() : inputSize;
		return true;
	} else {
		// Effect 已指定输出尺寸
		SetExprVars(inputSize, {});

		try {
			exprParser.SetExpr(_effectDesc.outSizeExpr.first);
			outputSize.cx = std::lround(exprParser.Eval());
			exprParser.SetExpr(_effectDesc.outSizeExpr.second);
			outputSize.cy = std::lround(exprParser.Eval());
		} catch (...) {
			return false;
		}

		return true;
	}
}

bool EffectDrawer::CanSetOutputSize() const {
	return _effectDesc.outSizeExpr.first.empty();
}

void EffectDrawer::SetOutputSize(SIZE value) {
	_outputSize = value;
}

bool EffectDrawer::Build(ComPtr<ID3D11Texture2D> input, ComPtr<ID3D11Texture2D> output) {
	D3D11_TEXTURE2D_DESC inputDesc;
	input->GetDesc(&inputDesc);
	SIZE inputSize = { (long)inputDesc.Width, (long)inputDesc.Height };

	SIZE outputSize;
	if (!CalcOutputSize({ (long)inputDesc.Width, (long)inputDesc.Height }, outputSize)) {
		return false;
	}
	
	SetExprVars(inputSize, outputSize);

	// 创建中间纹理
	_textures.resize(_effectDesc.textures.size() + 1);
	_textures[0] = input;
	for (size_t i = 1, end = _effectDesc.textures.size() - 1; i < end; ++i) {
		SIZE texSize{};
		try {
			exprParser.SetExpr(_effectDesc.textures[i].sizeExpr.first);
			texSize.cx = std::lround(exprParser.Eval());
			exprParser.SetExpr(_effectDesc.textures[i].sizeExpr.second);
			texSize.cy = std::lround(exprParser.Eval());
		} catch (...) {
			return false;
		}

		if (texSize.cx <= 0 || texSize.cy <= 0) {
			return false;
		}

		D3D11_TEXTURE2D_DESC desc{};
		desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		desc.Width = texSize.cx;
		desc.Height = texSize.cy;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		HRESULT hr = App::GetInstance().GetRenderer().GetD3DDevice()->CreateTexture2D(
			&desc, nullptr, _textures[i].ReleaseAndGetAddressOf());
		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("创建 Texture2D 失败", hr));
			return false;
		}
	}

	_textures.back() = output;

	for (size_t i = 0; i < _effectDesc.valueConstants.size(); ++i) {
		const auto& d = _effectDesc.valueConstants[i];

		double value;
		try {
			exprParser.SetExpr(d.valueExpr);
			value = exprParser.Eval();
		} catch (...) {
			return false;
		}
		
		if (_effectDesc.valueConstants[i].type == EffectConstantType::Float) {
			_constants[i + _effectDesc.constants.size()].floatVal = (float)value;
		} else {
			_constants[i + _effectDesc.constants.size()].intVal = (int)std::lround(value);
		}
	}

	// 创建常量缓冲区
	D3D11_BUFFER_DESC bd{};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = 4 * (UINT)_constants.size();
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	D3D11_SUBRESOURCE_DATA initData{};
	initData.pSysMem = _constants.data();
	
	HRESULT hr = _d3dDevice->CreateBuffer(&bd, &initData, &_constantBuffer);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("CreateBuffer 失败", hr));
		return false;
	}

	for (size_t i = 0; i < _passes.size(); ++i) {
		EffectPassDesc& desc = _effectDesc.passes[i];

		// 为空时表示输出到 OUTPUT
		if (desc.outputs.empty()) {
			desc.outputs.push_back(UINT(_effectDesc.textures.size()));
		}

		if (!_passes[i].Build(desc.inputs, desc.outputs[0], outputSize)
		) {
			SPDLOG_LOGGER_ERROR(logger, fmt::format("构建 Pass{} 时出错", i + 1));
			return false;
		}
	}

	return true;
}

void EffectDrawer::Draw() {
	ID3D11Buffer* t = _constantBuffer.Get();
	_d3dDC->PSSetConstantBuffers(0, 1, &t);
	_d3dDC->PSSetSamplers(0, (UINT)_samplers.size(), _samplers.data());

	for (_Pass& pass : _passes) {
		pass.Draw();
	}
}


bool EffectDrawer::_Pass::Initialize(EffectDrawer* parent, ComPtr<ID3DBlob> shaderBlob) {
	Renderer& renderer = App::GetInstance().GetRenderer();
	_parent = parent;
	_d3dDC = renderer.GetD3DDC();

	HRESULT hr = renderer.GetD3DDevice()->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &_pixelShader);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, MakeComErrorMsg("创建像素着色器失败", hr));
		return false;
	}

	return true;
}

bool EffectDrawer::_Pass::Build(const std::vector<UINT>& inputs, UINT output, std::optional<SIZE> outputSize) {
	Renderer& renderer = App::GetInstance().GetRenderer();

	_inputs.resize(inputs.size());
	for (int i = 0; i < _inputs.size(); ++i) {
		if (!renderer.GetShaderResourceView(_parent->_textures[inputs[i]].Get(), &_inputs[i])) {
			SPDLOG_LOGGER_ERROR(logger,"获取 ShaderResourceView 失败");
			return false;
		}
	}

	ComPtr<ID3D11Texture2D> outputTex = _parent->_textures[output];
	if (!App::GetInstance().GetRenderer().GetRenderTargetView(outputTex.Get(), &_outputRtv)) {
		SPDLOG_LOGGER_ERROR(logger, "获取 RenderTargetView 失败");
		return false;
	}

	D3D11_TEXTURE2D_DESC desc;
	outputTex->GetDesc(&desc);
	SIZE outputTextureSize = { (LONG)desc.Width, (LONG)desc.Height };

	_vp.Width = (float)outputTextureSize.cx;
	_vp.Height = (float)outputTextureSize.cy;
	_vp.MinDepth = 0.0f;
	_vp.MaxDepth = 1.0f;

	// 创建顶点缓冲区
	float outputLeft, outputTop, outputRight, outputBottom;
	if (outputSize.has_value() && (outputTextureSize.cx != outputSize->cx || outputTextureSize.cy != outputSize->cy)) {
		outputLeft = std::floorf(((float)outputTextureSize.cx - outputSize->cx) / 2) * 2 / outputTextureSize.cx - 1;
		outputTop = 1 - std::floorf(((float)outputTextureSize.cy - outputSize->cy) / 2) * 2 / outputTextureSize.cy;
		outputRight = outputLeft + 2 * outputSize->cx / (float)outputTextureSize.cx;
		outputBottom = outputTop - 2 * outputSize->cy / (float)outputTextureSize.cy;

		VertexPositionTexture vertices[] = {
			{ XMFLOAT3(outputLeft, outputTop, 0.5f), XMFLOAT2(0.0f, 0.0f) },
			{ XMFLOAT3(outputRight, outputTop, 0.5f), XMFLOAT2(1.0f, 0.0f) },
			{ XMFLOAT3(outputLeft, outputBottom, 0.5f), XMFLOAT2(0.0f, 1.0f) },
			{ XMFLOAT3(outputRight, outputBottom, 0.5f), XMFLOAT2(1.0f, 1.0f) }
		};
		D3D11_BUFFER_DESC bd = {};
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(vertices);
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA InitData = {};
		InitData.pSysMem = vertices;
		HRESULT hr = renderer.GetD3DDevice()->CreateBuffer(&bd, &InitData, &_vtxBuffer);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, fmt::sprintf("创建顶点缓冲区失败\n\tHRESULT：0x%X", hr));
			return false;
		}
	}

	return true;
}

void EffectDrawer::_Pass::Draw() {
	_d3dDC->PSSetShaderResources(0, 0, nullptr);
	_d3dDC->OMSetRenderTargets(1, &_outputRtv, nullptr);
	_d3dDC->RSSetViewports(1, &_vp);

	_d3dDC->PSSetShader(_pixelShader.Get(), nullptr, 0);
	_d3dDC->PSSetShaderResources(0, (UINT)_inputs.size(), _inputs.data());

	if (_vtxBuffer) {
		App::GetInstance().GetRenderer().SetSimpleVS(_vtxBuffer.Get());
		_d3dDC->Draw(4, 0);
	} else {
		App::GetInstance().GetRenderer().SetFillVS();
		_d3dDC->Draw(3, 0);
	}
}
