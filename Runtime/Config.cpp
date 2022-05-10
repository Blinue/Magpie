#include "pch.h"
#include "Config.h"
#include "Logger.h"


enum class FlagMasks : UINT {
	NoCursor = 0x1,
	AdjustCursorSpeed = 0x2,
	SaveEffectSources = 0x4,
	SimulateExclusiveFullscreen = 0x8,
	DisableLowLatency = 0x10,
	BreakpointMode = 0x20,
	DisableWindowResizing = 0x40,
	DisableDirectFlip = 0x80,
	Is3DMode = 0x100,
	CropTitleBarOfUWP = 0x200,
	DisableEffectCache = 0x400,
	DisableVSync = 0x800,
	WarningsAreErrors = 0x1000,
	ShowFPS = 0x2000
};


bool Config::Initialize(float cursorZoomFactor, UINT cursorInterpolationMode, int adapterIdx, UINT multiMonitorUsage, const RECT& cropBorders, UINT flags) {
	_cursorZoomFactor = cursorZoomFactor;
	_cursorInterpolationMode = cursorInterpolationMode;
	_adapterIdx = adapterIdx;
	_multiMonitorUsage = multiMonitorUsage;
	_cropBorders = cropBorders;

	_isNoCursor = flags & (UINT)FlagMasks::NoCursor;
	_isAdjustCursorSpeed = flags & (UINT)FlagMasks::AdjustCursorSpeed;
	_isSaveEffectSources = flags & (UINT)FlagMasks::SaveEffectSources;
	_isSimulateExclusiveFullscreen = flags & (UINT)FlagMasks::SimulateExclusiveFullscreen;
	_isDisableLowLatency = flags & (UINT)FlagMasks::DisableLowLatency;
	_isBreakpointMode = flags & (UINT)FlagMasks::BreakpointMode;
	_isDisableWindowResizing = flags & (UINT)FlagMasks::DisableWindowResizing;
	_isDisableDirectFlip = flags & (UINT)FlagMasks::DisableDirectFlip;
	_is3DMode = flags & (UINT)FlagMasks::Is3DMode;
	_isCropTitleBarOfUWP = flags & (UINT)FlagMasks::CropTitleBarOfUWP;
	_isDisableEffectCache = flags & (UINT)FlagMasks::DisableEffectCache;
	_isDisableVSync = flags & (UINT)FlagMasks::DisableVSync;
	_isTreatWarningsAsErrors = flags & (UINT)FlagMasks::WarningsAreErrors;
	_isShowFPS = flags & (UINT)FlagMasks::ShowFPS;

	Logger::Get().Info(fmt::format("运行时配置：\n\tadjustCursorSpeed：{}\n\tdisableLowLatency：{}\n\tbreakpointMode：{}\n\tdisableWindowResizing：{}\n\tdisableDirectFlip：{}\n\t3DMode：{}\n\tadapterIdx：{}\n\tcropTitleBarOfUWP：{}\n\tmultiMonitorUsage: {}\n\tnoCursor: {}\n\tdisableEffectCache: {}\n\tsimulateExclusiveFullscreen: {}\n\tcursorInterpolationMode: {}\n\tcropLeft: {}\n\tcropTop: {}\n\tcropRight: {}\n\tcropBottom: {}\n\tshowFPS: {}", IsAdjustCursorSpeed(), IsDisableLowLatency(), IsBreakpointMode(), IsDisableWindowResizing(), IsDisableDirectFlip(), Is3DMode(), GetAdapterIdx(), IsCropTitleBarOfUWP(), GetMultiMonitorUsage(), IsNoCursor(), IsDisableEffectCache(), IsSimulateExclusiveFullscreen(), GetCursorInterpolationMode(), cropBorders.left, cropBorders.top, cropBorders.right, cropBorders.bottom, IsShowFPS()));

	return true;
}

void Config::SetShowFPS(bool value) noexcept {
	if (value == _isShowFPS) {
		return;
	}

	_isShowFPS = value;

	for (const auto& cb : _showFPSCbs) {
		_queuedCbs.push_front(cb);
	}
}

void Config::OnBeginFrame() {
	// 最多处理 3 次，以避免陷入回调循环
	for (int i = 0; i < 3 && !_queuedCbs.empty(); ++i) {
		std::deque<std::function<void()>> cbs = std::move(_queuedCbs);
		_queuedCbs.clear();

		for (auto& cb : cbs) {
			cb();
		}
	}
}
