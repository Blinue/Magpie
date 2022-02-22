#pragma once
#include "pch.h"


class CursorManager {
public:
	CursorManager() = default;
	CursorManager(const CursorManager&) = delete;
	CursorManager(CursorManager&&) = delete;

	~CursorManager();

	bool Initialize();

	void BeginFrame();

	bool HasCursor() const {
		return !!_curCursor;
	}

	const POINT* GetCursorPos() const {
		return _curCursor ? &_curCursorPos : nullptr;
	}

	struct CursorInfo {
		POINT hotSpot{};
		SIZE size{};
	};
	const CursorInfo* GetCursorInfo() const {
		return _curCursor ? _curCursorInfo : nullptr;
	}

	enum class CursorType {
		// 彩色光标，此时纹理中 RGB 通道已预乘 A 通道（premultiplied alpha），A 通道已预先取反
		// 这是为了减少着色器的计算量以及确保（可能进行的）双线性差值的准确性
		// 计算公式：FinalColor = ScreenColor * CursorColor.a + CursorColor.rgb
		Color = 0,
		// 彩色掩码光标，此时 A 通道可能为 0 或 255
		// 为 0 时表示 RGB 通道取代屏幕颜色，为 255 时表示 RGB 通道和屏幕颜色进行异或操作
		MaskedColor,
		// 单色光标，此时 R 通道为 AND 掩码，G 通道为 XOR 掩码，其他通道不使用
		// RG 通道的值只能是 0 或 255
		Monochrome
	};
	bool GetCursorTexture(ID3D11Texture2D** texture, CursorManager::CursorType& cursorType);

private:
	void _StartCapture(POINT cursorPt);

	void _StopCapture(POINT cursorPt);

	void _DynamicClip(POINT cursorPt);

	bool _ResolveCursor(HCURSOR hCursor, bool resolveTexture);

	bool _isUnderCapture = false;
	std::array<bool, 4> _curClips{};

	INT _cursorSpeed = 0;

	// 当前帧的光标，光标不可见则为 NULL
	HCURSOR _curCursor = NULL;
	POINT _curCursorPos{};

	struct _CursorInfo : CursorInfo {
		winrt::com_ptr<ID3D11Texture2D> texture = nullptr;
		CursorType type = CursorType::Color;
	};
	_CursorInfo* _curCursorInfo = nullptr;

	std::unordered_map<HCURSOR, _CursorInfo> _cursorInfos;
};

