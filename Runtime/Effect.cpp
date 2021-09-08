#include "pch.h"
#include "Effect.h"
#include "App.h"
#include "Utils.h"

using namespace DirectX;

extern std::shared_ptr<spdlog::logger> logger;

struct SimpleVertex {
	XMFLOAT3 Pos;
	XMFLOAT4 TexCoord;
};


bool Effect::InitializeFromString(std::string_view hlsl) {
	Renderer& renderer = App::GetInstance().GetRenderer();
	_Pass& pass = _passes.emplace_back();

	if (!pass.Initialize(this, hlsl)) {
		SPDLOG_LOGGER_ERROR(logger, "Pass1 初始化失败");
		return false;
	}
	PassDesc& desc = _passDescs.emplace_back();
	desc.inputs.push_back(0);
	desc.samplers.push_back(0);
	desc.output = 1;

	_samplers.emplace_back(renderer.GetSampler(Renderer::FilterType::LINEAR));

	return true;
}

bool Effect::InitializeFromFile(const wchar_t* fileName) {
	std::string psText;
	if (!Utils::ReadTextFile(fileName, psText)) {
		SPDLOG_LOGGER_ERROR(logger, fmt::format("读取着色器文件{}失败", Utils::UTF16ToUTF8(fileName)));
		return false;
	}

	return InitializeFromString(psText);
}

bool Effect::Build(ComPtr<ID3D11Texture2D> input, ComPtr<ID3D11Texture2D> output) {
	_textures.emplace_back(input);
	_textures.emplace_back(output);

	D3D11_TEXTURE2D_DESC inputDesc;
	input->GetDesc(&inputDesc);

	for (int i = 0; i < _passes.size(); ++i) {
		PassDesc& desc = _passDescs[i];
		if (!_passes[i].Build(desc.inputs, desc.samplers, desc.constants, output, 
			CalcOutputSize({(long)inputDesc.Width, (long)inputDesc.Height}))) {
			SPDLOG_LOGGER_ERROR(logger, fmt::format("构建 Pass{} 时出错", i + 1));
			return false;
		}
	}

	return true;
}

void Effect::Draw() {
	for (_Pass& pass : _passes) {
		pass.Draw();
	}
}


bool Effect::_Pass::Initialize(Effect* parent, std::string_view pixelShader) {
	Renderer& renderer = App::GetInstance().GetRenderer();
	_parent = parent;
	_d3dDC = renderer.GetD3DDC();

	ComPtr<ID3DBlob> blob = nullptr;
	ComPtr<ID3DBlob> errorMsgs = nullptr;
	HRESULT hr = D3DCompile(pixelShader.data(), pixelShader.size(), nullptr, nullptr, nullptr,
		"PS", "ps_5_0", D3DCOMPILE_ENABLE_STRICTNESS, 0, &blob, &errorMsgs);
	if (FAILED(hr)) {
		if (errorMsgs) {
			SPDLOG_LOGGER_ERROR(logger, fmt::sprintf(
				"编译像素着色器失败：%s\n\tHRESULT：0x%X", (const char*)errorMsgs->GetBufferPointer(), hr));
		}
		return false;
	}
	
	hr = renderer.GetD3DDevice()->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &_psShader);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_CRITICAL(logger, fmt::sprintf("创建像素着色器失败\n\tHRESULT：0x%X", hr));
		return false;
	}

	_vsShader = renderer.GetVSShader();
	_vtxLayout = renderer.GetInputLayout();

	return true;
}

bool Effect::_Pass::Build(
	const std::vector<int>& inputs,
	const std::vector<int>& samplers,
	const std::vector<int>& constants,
	ComPtr<ID3D11Texture2D> output,
	std::optional<SIZE> outputSize
) {
	Renderer& renderer = App::GetInstance().GetRenderer();
	HRESULT hr;

	_inputs.resize(inputs.size());
	for (int i = 0; i < _inputs.size(); ++i) {
		hr = renderer.GetShaderResourceView(_parent->_textures[inputs[i]].Get(), &_inputs[i]);
		if (FAILED(hr)) {
			SPDLOG_LOGGER_ERROR(logger, fmt::sprintf("获取 ShaderResourceView 失败\n\tHRESULT：0x%X", hr));
			return false;
		}
	}

	_samplers.resize(samplers.size());
	for (int i = 0; i < _samplers.size(); ++i) {
		_samplers[i] = _parent->_samplers[samplers[i]].Get();
	}

	_constants = constants;

	hr = App::GetInstance().GetRenderer().GetRenderTargetView(output.Get(), &_outputRtv);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_CRITICAL(logger, fmt::sprintf("获取 RenderTargetView 失败\n\tHRESULT：0x%X", hr));
		return false;
	}

	D3D11_TEXTURE2D_DESC desc;
	output->GetDesc(&desc);
	SIZE outputTextureSize = { (LONG)desc.Width, (LONG)desc.Height };

	_vp.Width = (float)outputTextureSize.cx;
	_vp.Height = (float)outputTextureSize.cy;
	_vp.MinDepth = 0.0f;
	_vp.MaxDepth = 1.0f;

	// 创建顶点缓冲区
	float outputLeft, outputTop, outputRight, outputBottom;
	if (!outputSize.has_value() || (outputTextureSize.cx == outputSize->cx && outputTextureSize.cy == outputSize->cy)) {
		outputLeft = outputBottom = -1;
		outputRight = outputTop = 1;
	} else {
		outputLeft = std::floorf(((float)outputTextureSize.cx - outputSize->cx) / 2) * 2 / outputTextureSize.cx - 1;
		outputTop = 1 - std::ceilf(((float)outputTextureSize.cy - outputSize->cy) / 2) * 2 / outputTextureSize.cy;
		outputRight = outputLeft + 2 * outputSize->cx / (float)outputTextureSize.cx;
		outputBottom = outputTop - 2 * outputSize->cy / (float)outputTextureSize.cy;
	}

	float pixelWidth = 1.0f / outputSize->cx;
	float pixelHeight = 1.0f / outputSize->cy;
	SimpleVertex vertices[] = {
		{ XMFLOAT3(outputLeft, outputTop, 0.5f), XMFLOAT4(0.0f, 0.0f, pixelWidth, pixelHeight) },
		{ XMFLOAT3(outputRight, outputTop, 0.5f), XMFLOAT4(1.0f, 0.0f, pixelWidth, pixelHeight) },
		{ XMFLOAT3(outputLeft, outputBottom, 0.5f), XMFLOAT4(0.0f, 1.0f, pixelWidth, pixelHeight) },
		{ XMFLOAT3(outputRight, outputBottom, 0.5f), XMFLOAT4(1.0f, 1.0f, pixelWidth, pixelHeight) }
	};
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(vertices);
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA InitData = {};
	InitData.pSysMem = vertices;
	hr = renderer.GetD3DDevice()->CreateBuffer(&bd, &InitData, &_vtxBuffer);
	if (FAILED(hr)) {
		SPDLOG_LOGGER_ERROR(logger, fmt::sprintf("创建顶点缓冲区失败\n\tHRESULT：0x%X", hr));
		return false;
	}

	return true;
}

void Effect::_Pass::Draw() {
	_d3dDC->OMSetRenderTargets(1, &_outputRtv, nullptr);

	// 如果和上一个 Pass 相同不再重复设置参数
	static _Pass* lastPass = nullptr;
	if (lastPass == this) {
		_d3dDC->Draw(4, 0);
		return;
	}
	lastPass = this;

	_d3dDC->RSSetViewports(1, &_vp);

	_d3dDC->IASetInputLayout(_vtxLayout.Get());

	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	auto t = _vtxBuffer.Get();
	_d3dDC->IASetVertexBuffers(0, 1, &t, &stride, &offset);

	_d3dDC->VSSetShader(_vsShader.Get(), nullptr, 0);

	_d3dDC->PSSetShader(_psShader.Get(), nullptr, 0);
	if (!_samplers.empty()) {
		_d3dDC->PSSetSamplers(0, (UINT)_samplers.size(), _samplers.data());
	}
	if (!_inputs.empty()) {
		_d3dDC->PSSetShaderResources(0, (UINT)_inputs.size(), _inputs.data());
	}

	_d3dDC->Draw(4, 0);
}
