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

	bool Initialize(const wchar_t* fileName, bool lastEffect = false);

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

	bool Build(ID3D11Texture2D* input, ID3D11Texture2D* output);

	void Draw(bool noUpdate = false);

	bool HasDynamicConstants() const {
		return !_dynamicConstants.empty();
	}

	static bool UpdateExprDynamicVars();
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
		
		winrt::com_ptr<ID3D11ComputeShader> _computeShader;

		// 后半部分为空，用于解绑
		std::vector<ID3D11ShaderResourceView*> _inputs;
		std::vector<ID3D11RenderTargetView*> _outputs;

		winrt::com_ptr<ID3D11Buffer> _vtxBuffer;
		D3D11_VIEWPORT _vp{};
	};

	std::vector<ID3D11SamplerState*> _samplers;
	std::vector<winrt::com_ptr<ID3D11Texture2D>> _textures;

	std::unordered_map<std::string_view, UINT> _constNamesMap;
	std::vector<Constant32> _constants;
	std::vector<Constant32> _dynamicConstants;
	winrt::com_ptr<ID3D11Buffer> _constantBuffer;
	winrt::com_ptr<ID3D11Buffer> _dynamicConstantBuffer;

	winrt::com_ptr<ID3D11VertexShader> _vertexShader;

	std::optional<SIZE> _outputSize;

	EffectDesc _effectDesc{};
	std::vector<_Pass> _passes;
};
