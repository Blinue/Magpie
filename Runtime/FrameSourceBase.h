#pragma once
#include "pch.h"


class FrameSourceBase {
public:
	FrameSourceBase() {}

	virtual ~FrameSourceBase();

	// 不可复制，不可移动
	FrameSourceBase(const FrameSourceBase&) = delete;
	FrameSourceBase(FrameSourceBase&&) = delete;

	virtual bool Initialize();

	enum class UpdateState {
		NewFrame,
		NoUpdate,
		Waiting,
		Error
	};

	virtual UpdateState Update() = 0;

	virtual bool IsScreenCapture() = 0;

	// 注意：此函数返回源窗口作为输入部分的位置，但可能和 GetOutput 获取到的纹理尺寸不同
	const RECT& GetSrcFrameRect() const noexcept { return _srcFrameRect; }

	ID3D11Texture2D* GetOutput() {
		return _output.get();
	}

protected:
	virtual bool _HasRoundCornerInWin11() = 0;

	// 获取坐标系 1 到坐标系 2 的映射关系
	// 坐标系 1：屏幕坐标系，即虚拟化后的坐标系。原点为屏幕左上角
	// 坐标系 2：虚拟化前的坐标系，即窗口所见的坐标系，原点为窗口左上角
	// 两坐标系为线性映射，a 和 b 返回该映射的参数
	// 如果窗口本身支持高 DPI，则 a 为 1，否则 a 为 DPI 缩放的倒数
	// 此函数是为了将屏幕上的点映射到窗口坐标系中，并且无视 DPI 虚拟化
	// 坐标系 1 中的 (x1, y1) 映射到 (x1 * a + bx, x2 * a + by)
	static bool _GetMapToOriginDPI(HWND hWnd, double& a, double& bx, double& by);

	static bool _CenterWindowIfNecessary(HWND hWnd, const RECT& rcWork);

	bool _UpdateSrcFrameRect();

	RECT _srcFrameRect{};

	winrt::com_ptr<ID3D11Texture2D> _output;

	bool _roundCornerDisabled = false;
	bool _windowResizingDisabled = false;
};
