#pragma once
#include "pch.h"

class A4KSFilter {
public:
	bool Initialize();

	void Draw();

private:
	winrt::com_ptr<ID3D11ComputeShader> _pass1Shader;
	winrt::com_ptr<ID3D11ComputeShader> _pass2Shader;
	winrt::com_ptr<ID3D11ComputeShader> _pass3Shader;
	winrt::com_ptr<ID3D11ComputeShader> _pass4Shader;
	winrt::com_ptr<ID3D11Buffer> _CB;
	ID3D11SamplerState* _sams[2]{};
	ID3D11ShaderResourceView* _srv1 = nullptr;
	ID3D11ShaderResourceView* _srv2 = nullptr;
	ID3D11ShaderResourceView* _srv3 = nullptr;
	ID3D11ShaderResourceView* _srv4 = nullptr;
	ID3D11UnorderedAccessView* _uav1 = nullptr;
	ID3D11UnorderedAccessView* _uav2 = nullptr;
	ID3D11UnorderedAccessView* _uav3 = nullptr;
	ID3D11UnorderedAccessView* _uav4 = nullptr;

	winrt::com_ptr<ID3D11Texture2D> _tex1;
	winrt::com_ptr<ID3D11Texture2D> _tex2;
	winrt::com_ptr<ID3D11Texture2D> _tex3;
};

