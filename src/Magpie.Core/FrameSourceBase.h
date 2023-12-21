#pragma once

namespace Magpie::Core {

class DeviceResources;

class FrameSourceBase {
public:
	FrameSourceBase() noexcept;

	virtual ~FrameSourceBase() noexcept;

	// 不可复制，不可移动
	FrameSourceBase(const FrameSourceBase&) = delete;
	FrameSourceBase(FrameSourceBase&&) = delete;

	bool Initialize(DeviceResources& deviceResources) noexcept;

	enum class UpdateState {
		NewFrame,
		NoChange,
		Waiting,
		Error
	};

	UpdateState Update() noexcept;

	ID3D11Texture2D* GetOutput() noexcept {
		return _output.get();
	}

	// 注意：返回源窗口作为输入部分的位置，但可能和 GetOutput 获取到的纹理尺寸不同，
	// 因为源窗口可能存在 DPI 缩放，而某些捕获方法无视 DPI 缩放
	const RECT& SrcRect() const noexcept { return _srcRect; }

	std::pair<uint32_t, uint32_t> GetStatisticsForDynamicDetection() const noexcept;

	virtual const char* Name() const noexcept = 0;

	virtual bool IsScreenCapture() const noexcept = 0;

	virtual void OnCursorVisibilityChanged(bool /*isVisible*/) noexcept {};

protected:
	virtual bool _Initialize() noexcept = 0;

	virtual UpdateState _Update() noexcept = 0;

	virtual bool _HasRoundCornerInWin11() noexcept = 0;

	virtual bool _CanCaptureTitleBar() noexcept = 0;

	bool _CalcSrcRect() noexcept;

	// 获取坐标系 1 到坐标系 2 的映射关系
	// 坐标系 1：屏幕坐标系，即虚拟化后的坐标系。原点为屏幕左上角
	// 坐标系 2：虚拟化前的坐标系，即源窗口所见的坐标系，原点为窗口左上角
	// 两坐标系为线性映射，a 和 b 返回该映射的参数
	// 如果窗口本身支持高 DPI，则 a 为 1，否则 a 为 DPI 缩放的倒数
	// 此函数是为了将屏幕上的点映射到窗口坐标系中，并且无视 DPI 虚拟化
	// 坐标系 1 中的 (x1, y1) 映射到 (x1 * a + bx, x2 * a + by)
	static bool _GetMapToOriginDPI(HWND hWnd, double& a, double& bx, double& by) noexcept;

	static bool _CenterWindowIfNecessary(HWND hWnd, const RECT& rcWork) noexcept;

	RECT _srcRect{};

	DeviceResources* _deviceResources = nullptr;
	winrt::com_ptr<ID3D11Texture2D> _output;

	winrt::com_ptr<ID3D11Buffer> _resultBuffer;
	winrt::com_ptr<ID3D11Buffer> _readBackBuffer;
	winrt::com_ptr<ID3D11ComputeShader> _dupFrameCS;
	std::pair<uint32_t, uint32_t> _dispatchCount;

	bool _roundCornerDisabled = false;
	bool _windowResizingDisabled = false;

private:
	bool _IsDuplicateFrame();

	// 用于检查重复帧
	winrt::com_ptr<ID3D11Texture2D> _prevFrame;
	uint16_t _nextSkipCount;
	uint16_t _framesLeft;
	// (预测错误帧数, 总计跳过帧数)
	std::atomic<std::pair<uint32_t, uint32_t>> _statistics;
	bool _isCheckingForDuplicateFrame = true;
};

}
