#pragma once
#include <functional>
#include <string>

namespace Magpie {

// Magpie.Core 向应用层报告 ONNX 状态的钩子。
//
// The app layer installs Callback; Magpie.Core calls Report from the scaling
// thread. Kept in include/ because OnnxEffectDrawer.h is internal to
// Magpie.Core and not visible to src/Magpie.
//
// Magpie.Core is a static library linked into Magpie.exe, so a plain
// std::function is enough - no cross-module event plumbing is needed.
struct OnnxStatus {
	// 在缩放线程上调用，实现必须自行处理线程同步
	// Invoked on the scaling thread; the implementation must marshal.
	static inline std::function<void(std::wstring, std::wstring)> Callback;

	// 必须在 third_party\onnxruntime.dll 被固定之后调用
	// Must be called after third_party\onnxruntime.dll has been pinned, and
	// before any Ort API use. ORT_API_MANUAL_INIT disables the header's own
	// pre-main initialization, which would otherwise bind System32's copy.
#ifdef MAGPIE_ONNX_ENABLED
	static void InitOrtApi() noexcept;
#else
	static void InitOrtApi() noexcept {}
#endif

	// onnxruntime.dll 是否已加载并绑定；未加载时必须跳过所有 ONNX 代码
	// Whether onnxruntime.dll loaded and the API table was bound. When false,
	// every ONNX path must be skipped - the DLL is delay-loaded, so touching
	// the API would raise inside the loader.
	static inline bool IsOrtAvailable = false;

	static void Report(std::wstring title, std::wstring text) noexcept {
		if (Callback) {
			Callback(std::move(title), std::move(text));
		}
	}
};

}
