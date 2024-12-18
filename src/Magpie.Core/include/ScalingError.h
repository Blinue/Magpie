#pragma once

enum class ScalingError {
	NoError,

	/////////////////////////////////////
	// 
	// 先决条件错误
	// 
	/////////////////////////////////////

	// 未配置缩放模式或者缩放模式不合法
	InvalidScalingMode,
	// 启用触控支持失败
	TouchSupport,
	// 通用的不支持缩放错误
	InvalidSourceWindow,
	// 不支持缩放系统窗口，这个错误无需显示消息
	SystemWindow,
	// 因窗口已最大化或全屏而无法缩放，可通过更改设置强制缩放
	Maximized,
	// 因窗口的 IL 更高而无法缩放
	LowIntegrityLevel,

	/////////////////////////////////////
	//
	// 初始化和缩放时错误
	//
	/////////////////////////////////////

	// 通用的缩放失败错误
	ScalingFailedGeneral,
	// FrameSource 初始化失败
	CaptureFailed,
	// ID3D11Device5::CreateFence 失败
	CreateFenceFailed
};
