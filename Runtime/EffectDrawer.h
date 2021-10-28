#pragma once
#include "pch.h"
#include "EffectDesc.h"
#include <optional>


union Constant32 {
	int intVal;
	float floatVal;
};


class EffectDrawer {
public:
	EffectDrawer() = default;

	EffectDrawer(const EffectDrawer& other);

	EffectDrawer(EffectDrawer&& other) noexcept;

	bool Initialize(const wchar_t* fileName);

	enum class ConstantType {
		Float,
		Int,
		NotFound
	};

	ConstantType GetConstantType(std::string_view name) const;

	bool SetConstant(std::string_view name, float value);

	bool SetConstant(std::string_view name, int value);

	bool CalcOutputSize(SIZE inputSize, SIZE& outputSize) const;

	bool CanSetOutputSize() const;

	void SetOutputSize(SIZE value);

	bool Build(ComPtr<ID3D11Texture2D> input, ComPtr<ID3D11Texture2D> output);

	void Draw();

private:
	class _Pass {
	public:
		bool Initialize(EffectDrawer* parent, size_t index);

		bool Build(std::optional<SIZE> outputSize);

		void Draw();

		void SetParent(EffectDrawer* parent) {
			_parent = parent;
		}
	private:
		EffectDrawer* _parent = nullptr;
		size_t _index = 0;
		
		ComPtr<ID3D11PixelShader> _pixelShader;

		// 后半部分为空，用于解绑
		std::vector<ID3D11ShaderResourceView*> _inputs;
		std::vector<ID3D11RenderTargetView*> _outputs;
		std::vector<ID3D11SamplerState*> _samplers;

		ComPtr<ID3D11Buffer> _vtxBuffer;
		D3D11_VIEWPORT _vp{};
	};

	ComPtr<ID3D11Device> _d3dDevice;
	ComPtr<ID3D11DeviceContext> _d3dDC;

	std::vector<ID3D11SamplerState*> _samplers;
	std::vector<ComPtr<ID3D11Texture2D>> _textures;

	std::unordered_map<std::string_view, UINT> _constNamesMap;
	std::vector<Constant32> _constants;
	ComPtr<ID3D11Buffer> _constantBuffer;

	ComPtr<ID3D11VertexShader> _vertexShader;

	std::optional<SIZE> _outputSize;

	EffectDesc _effectDesc{};
	std::vector<_Pass> _passes;
};
