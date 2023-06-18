#pragma once
#include <parallel_hashmap/phmap.h>
#include "ScalingOptions.h"

namespace Magpie::Core {

class DeviceResources;

class CursorDrawer {
public:
	CursorDrawer() noexcept = default;
	CursorDrawer(const CursorDrawer&) = delete;
	CursorDrawer(CursorDrawer&&) = delete;

	bool Initialize(
		DeviceResources& deviceResources,
		ID3D11Texture2D* backBuffer,
		const RECT& viewportRect,
		const ScalingOptions& options
	) noexcept;

	void Draw(HCURSOR hCursor, POINT cursorPos) noexcept;

private:
	enum class _CursorType {
		// 彩色光标，此时纹理中 RGB 通道已预乘 A 通道（premultiplied alpha），A 通道已预先取反
		// 这是为了减少着色器的计算量以及确保（可能进行的）双线性差值的准确性
		// 计算公式：FinalColor = ScreenColor * CursorColor.a + CursorColor
		Color = 0,
		// 彩色掩码光标，此时 A 通道可能为 0 或 255
		// 为 0 时表示 RGB 通道取代屏幕颜色，为 255 时表示 RGB 通道和屏幕颜色进行异或操作
		MaskedColor,
		// 单色光标，此时 R 通道为 AND 掩码，G 通道为 XOR 掩码，其他通道不使用
		// RG 通道的值只能是 0 或 255
		Monochrome
	};

	struct _CursorInfo {
		POINT hotSpot{};
		SIZE size{};
		winrt::com_ptr<ID3D11Texture2D> texture = nullptr;
		_CursorType type = _CursorType::Color;
	};

	const _CursorInfo* _ResolveCursor(HCURSOR hCursor) noexcept;

	bool _SetSimplePS(ID3D11Texture2D* cursorTexture) noexcept;

	bool _SetPremultipliedAlphaBlend(bool enable) noexcept;

	DeviceResources* _deviceResources = nullptr;
	ID3D11Texture2D* _backBuffer = nullptr;

	RECT _viewportRect{};

	float _cursorScaling = 1.0f;
	CursorInterpolationMode _interpolationMode = CursorInterpolationMode::NearestNeighbor;

	phmap::flat_hash_map<HCURSOR, _CursorInfo> _cursorInfos;

	winrt::com_ptr<ID3D11VertexShader> _simpleVS;
	winrt::com_ptr<ID3D11InputLayout> _simpleIL;
	winrt::com_ptr<ID3D11Buffer> _vtxBuffer;
	winrt::com_ptr<ID3D11PixelShader> _simplePS;
	winrt::com_ptr<ID3D11BlendState> premultipliedAlphaBlendBlendState;
};

}
