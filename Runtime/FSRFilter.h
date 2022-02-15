#pragma once
#include "pch.h"


class FSRFilter {
public:
	bool Initialize();

	void Draw();

private:
	winrt::com_ptr<ID3D11ComputeShader> _easuShader;
	winrt::com_ptr<ID3D11ComputeShader> _rcasShader;
	winrt::com_ptr<ID3D11Buffer> _easuCB;
	winrt::com_ptr<ID3D11Buffer> _rcasCB;
	ID3D11SamplerState* _sam = nullptr;
	ID3D11ShaderResourceView* _srv1 = nullptr;
	ID3D11ShaderResourceView* _srv2 = nullptr;
	ID3D11UnorderedAccessView* _uav1 = nullptr;
	ID3D11UnorderedAccessView* _uav2 = nullptr;

	winrt::com_ptr<ID3D11Texture2D> _tex;
	SIZE _outputSize{};
};
