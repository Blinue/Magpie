#include "pch.h"
#include "CursorDrawer.h"
#include "ByteBuffer.h"
#include "ColorHelper.h"
#include "CommandContext.h"
#include "CursorHelper.h"
#include "D3D12Context.h"
#include "DescriptorHeap.h"
#include "DirectXHelper.h"
#include "Logger.h"
#include "ScalingWindow.h"
#include "shaders/CursorResizerPS.h"
#include "shaders/CursorResizerPS_SM5.h"
#include "shaders/CursorVS.h"
#include "shaders/CursorVS_SM5.h"
#include "shaders/FullscreenVS.h"
#include "shaders/FullscreenVS_SM5.h"
#include "shaders/MaskedCursorPS.h"
#include "shaders/MaskedCursorPS_SM5.h"
#include "shaders/MaskedCursorPS_sRGB.h"
#include "shaders/MaskedCursorPS_sRGB_SM5.h"
#include "shaders/MonochromeCursorPS.h"
#include "shaders/MonochromeCursorPS_SM5.h"
#include "shaders/MonochromeCursorPS_sRGB.h"
#include "shaders/MonochromeCursorPS_sRGB_SM5.h"
#include "shaders/TextureBlitPS.h"
#include "shaders/TextureBlitPS_SM5.h"
#include "Win32Helper.h"
#include <DirectXPackedVector.h>
#include <ShellScalingApi.h>
#include <wil/registry.h>

namespace Magpie {

using namespace DirectX::PackedVector;

// 系统 DPI 在程序的生命周期内不会改变，而且使用 GetIconInfo 获得的位图尺寸
// 和此值有关。
static UINT SYSTEM_DPI;

struct StandardCursorInfo {
	HCURSOR handle;
	const wchar_t* regValueName;
	int resId;
};

static std::array<StandardCursorInfo, 16> STANDARD_CURSORS;

static DWORD GetCursorBaseSize() noexcept {
	DWORD cursorBaseSize = 32;

	HRESULT hr = wil::reg::get_value_nothrow(
		HKEY_CURRENT_USER, L"Control Panel\\Cursors", L"CursorBaseSize", &cursorBaseSize);
	// 键不存在不视为错误
	if (FAILED(hr) && hr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
		Logger::Get().ComError("wil::reg::get_value_dword_nothrow 失败", hr);
	}

	return cursorBaseSize;
}

CursorDrawer::~CursorDrawer() noexcept {
#ifdef _DEBUG
	if (_d3d12Context) {
		auto& csuDescriptorHeap = _d3d12Context->GetDescriptorHeap();
		auto& rtvDescriptorHeap = _d3d12Context->GetDescriptorHeap(true);

		for (const auto& pair : _cursorInfos) {
			pair.second.FreeDescriptors(csuDescriptorHeap, rtvDescriptorHeap);
		}

		for (const _CursorInfo& cursorInfo : _retiredCursorInfos) {
			cursorInfo.FreeDescriptors(csuDescriptorHeap, rtvDescriptorHeap);
		}

		if (_tempOriginTextureSrvOffset != std::numeric_limits<uint32_t>::max()) {
			csuDescriptorHeap.Free(_tempOriginTextureSrvOffset, 1);
		}
		
		for (const _RetiredTempOriginTexture& rt : _retiredTempOriginTextures) {
			if (rt.srvOffset != std::numeric_limits<uint32_t>::max()) {
				csuDescriptorHeap.Free(rt.srvOffset, 1);
			}
		}
	}
#endif
}

bool CursorDrawer::Initialize(
	D3D12Context& d3d12Context,
	const RECT& srcRect,
	const RECT& rendererRect,
	const RECT& destRect,
	const ColorInfo& colorInfo
) noexcept {
	_d3d12Context = &d3d12Context;
	_srcSize.width = uint32_t(srcRect.right - srcRect.left);
	_srcSize.height = uint32_t(srcRect.bottom - srcRect.top);
	_rendererRect = rendererRect;
	_destRect = destRect;
	_colorInfo = colorInfo;

	[[maybe_unused]] static Ignore _ = [] {
		SYSTEM_DPI = GetDpiForSystem();

		// 不包含已废弃的光标, 见 https://learn.microsoft.com/en-us/windows/win32/menurc/about-cursors
		STANDARD_CURSORS[0] = { LoadCursor(NULL, IDC_ARROW), L"Arrow", 0 };
		STANDARD_CURSORS[1] = { LoadCursor(NULL, IDC_IBEAM), L"IBeam", 0 };
		// Wait 和 AppStarting 在“指针大小” 为 1 时是动态光标，否则是静态光标
		STANDARD_CURSORS[2] = { LoadCursor(NULL, IDC_WAIT), L"Wait", 0 };
		STANDARD_CURSORS[3] = { LoadCursor(NULL, IDC_CROSS), L"Crosshair", 0 };
		STANDARD_CURSORS[4] = { LoadCursor(NULL, IDC_UPARROW), L"UpArrow", 0 };
		STANDARD_CURSORS[5] = { LoadCursor(NULL, IDC_SIZENWSE), L"SizeNWSE", 0 };
		STANDARD_CURSORS[6] = { LoadCursor(NULL, IDC_SIZENESW), L"SizeNESW", 0 };
		STANDARD_CURSORS[7] = { LoadCursor(NULL, IDC_SIZEWE), L"SizeWE", 0 };
		STANDARD_CURSORS[8] = { LoadCursor(NULL, IDC_SIZENS), L"SizeNS", 0 };
		STANDARD_CURSORS[9] = { LoadCursor(NULL, IDC_SIZEALL), L"SizeAll", 0 };
		STANDARD_CURSORS[10] = { LoadCursor(NULL, IDC_NO), L"No", 0 };
		STANDARD_CURSORS[11] = { LoadCursor(NULL, IDC_HAND), L"Hand", 0 };
		STANDARD_CURSORS[12] = { LoadCursor(NULL, IDC_APPSTARTING), L"AppStarting", 0 };
		STANDARD_CURSORS[13] = { LoadCursor(NULL, IDC_HELP), L"Help", 0 };
		// Pin 和 Person 只在“指针大小”选项大于 1 时使用注册表中路径，否则使用
		// user32.dll 中的光标资源，ID 分别是 117 和 118。
		STANDARD_CURSORS[14] = { LoadCursor(NULL, IDC_PIN), L"Pin", 117 };
		STANDARD_CURSORS[15] = { LoadCursor(NULL, IDC_PERSON), L"Person", 118 };

		return Ignore();
	}();

	wil::unique_hkey key;
	wil::reg::open_unique_key_nothrow(HKEY_CURRENT_USER, L"Control Panel\\Cursors", key);
	_regWatcher = wil::make_registry_watcher_nothrow(
		std::move(key), false, std::bind_front(&CursorDrawer::_OnCursorsRegChanged, this));

	_cursorBaseSize = GetCursorBaseSize();
	return true;
}

void CursorDrawer::PrepareForDraw(HCURSOR hCursor, POINT cursorPos, bool& needRedraw) noexcept {
	const ScalingWindow& scalingWindow = ScalingWindow::Get();
	const ScalingOptions& options = scalingWindow.Options();

	// 检查自动隐藏光标
	if (options.autoHideCursorDelay.has_value()) {
		using namespace std::chrono;

		// 光标在叠加层上或拖动窗口时禁用自动隐藏。光标处于隐藏状态视为形状不变，考虑形状
		// 变化：箭头->隐藏->箭头，只要位置不变，自动隐藏功能应让光标始终隐藏；反之如果光
		// 标隐藏时移动了或显示时形状变化了应正常显示。
		if (_isCursorVirtualized &&
			!_isMoving &&
			!_isSrcMoving &&
			_curCursorPos == cursorPos &&
			(_lastRawCursorHandle == hCursor || !hCursor)
		) {
			const duration<float> hideDelay(*options.autoHideCursorDelay);
			if (steady_clock::now() - _lastCursorActiveTime > hideDelay) {
				hCursor = NULL;
			}
		} else {
			// 启用自动隐藏时光标形状或位置变化后应记录新的形状、位置和变化时间。位置由
			// _curCursorPos 记录。
			_lastRawCursorHandle = hCursor;
			_lastCursorActiveTime = steady_clock::now();
		}
	}

	// 不要保存指针，_ResolveCursor 后可能失效
	const _CursorInfoKey oldCursorKey = _curCursorInfoKeyValue ?
		_curCursorInfoKeyValue->first : _CursorInfoKey{};
	const uint32_t oldFrameSeqIdx = _curFrameSeqIdx;
	_curCursorInfoKeyValue = nullptr;
	_curFrameSeqIdx = 0;

	if (hCursor && _isCursorVisible) {
		static const HCURSOR hArrowCursor = STANDARD_CURSORS[0].handle;

		// 无法解析的光标换为指针光标
		if (!_unresolvableCursors.empty() && _unresolvableCursors.contains(hCursor)) {
			if (hCursor != hArrowCursor && !_unresolvableCursors.contains(hArrowCursor)) {
				hCursor = hArrowCursor;
			} else {
				hCursor = NULL;
			}
		}

		while (hCursor) {
			_curCursorInfoKeyValue = _ResolveCursor(hCursor, cursorPos);

			if (_curCursorInfoKeyValue) {
				const _CursorInfo& curCursorInfo = _curCursorInfoKeyValue->second;

				if (curCursorInfo.IsAnimated()) {
					auto now = std::chrono::steady_clock::now();

					if (oldCursorKey == _curCursorInfoKeyValue->first) {
						_curFrameSeqIdx = oldFrameSeqIdx;

						// 计算新帧序号
						while (now >= _curFrameSeqEndTime) {
							_curFrameSeqIdx = (_curFrameSeqIdx + 1) %
								(uint32_t)curCursorInfo.frameSequence.size();
							_curFrameSeqEndTime += curCursorInfo.frameSequence[_curFrameSeqIdx].second;
						}
					} else {
						_curFrameSeqEndTime = now + curCursorInfo.frameSequence[0].second;
					}
				}

				const _CursorFrame& curCursorFrame =
					curCursorInfo.frames[curCursorInfo.GetFrameIdx(_curFrameSeqIdx)];
				// 检查光标是否在视口内。hCursor 不为 NULL 说明光标已在缩放窗口内，因此这个检查很少失败
				const RECT cursorRect = {
					cursorPos.x - (LONG)curCursorFrame.hotspot.x,
					cursorPos.y - (LONG)curCursorFrame.hotspot.y,
					cursorRect.left + (LONG)curCursorInfo.size.width,
					cursorRect.top + (LONG)curCursorInfo.size.height
				};
				if (!Win32Helper::IsRectOverlap(cursorRect, _destRect)) {
					_curCursorInfoKeyValue = nullptr;
				}

				break;
			} else {
				_unresolvableCursors.insert(hCursor);

				if (hCursor == hArrowCursor) {
					// 无法解析指针光标
					break;
				} else {
					// 换为指针光标
					hCursor = hArrowCursor;
				}
			}
		}
	}

	// 光标形状或位置变化时总是需要重新绘制；有时即使不变也需要重新绘制，比如色域
	// 或 _cursorBaseSize 变化后。
	_CursorInfoKey newCursorKey = _curCursorInfoKeyValue ?
		_curCursorInfoKeyValue->first : _CursorInfoKey{};
	needRedraw = newCursorKey != oldCursorKey;

	if (_curCursorInfoKeyValue) {
		needRedraw |= _curFrameSeqIdx != oldFrameSeqIdx;
		needRedraw |= cursorPos != _curCursorPos;
	}

	_curCursorPos = cursorPos;
}

HRESULT CursorDrawer::Draw(
	GraphicsContext& graphicsContext,
	uint64_t frameFenceValue,
	uint64_t completedFenceValue,
	uint32_t curFrameSrvOffset,
	ID3D12Resource* backBuffer
) noexcept {
	if (!_curCursorInfoKeyValue) {
		return S_OK;
	}

	_CursorInfo& cursorInfo = _curCursorInfoKeyValue->second;
	const uint32_t cursorFrameIdx = cursorInfo.GetFrameIdx(_curFrameSeqIdx);
	_CursorFrame& cursorFrame = cursorInfo.frames[cursorFrameIdx];

	cursorInfo.lastUseFenceValue = frameFenceValue;

	// 可能会清理 _CursorInfo，但这不会导致 cursorInfo 失效
	_ClearRetiredResources(completedFenceValue);

	if (!cursorFrame.texture) {
		HRESULT hr = _InitializeCursorTexture(
			graphicsContext , cursorInfo, cursorFrameIdx, completedFenceValue);
		if (FAILED(hr)) {
			Logger::Get().ComError("_InitializeCursorTexture 失败", hr);
			return hr;
		}

		cursorFrame.tempResourcesFenceValue = frameFenceValue;

		// 所有帧都初始化后再清理临时资源，因为初始化新帧时可能会复用旧帧的临时资源。注意
		// 不一定按顺序逐个初始化，理论上可能会出现两次渲染间隔过长导致某个中间帧被跳过。
		if (std::all_of(
			cursorInfo.frames.begin(),
			cursorInfo.frames.end(),
			[](const _CursorFrame& frame) { return frame.texture != nullptr; }
		)) {
			_cursorInfosWithTempResources.push_back(_curCursorInfoKeyValue->first);
		}
	}

	const RECT cursorRect = {
		.left = _curCursorPos.x - (LONG)cursorFrame.hotspot.x,
		.top = _curCursorPos.y - (LONG)cursorFrame.hotspot.y,
		.right = cursorRect.left + (LONG)cursorInfo.size.width,
		.bottom = cursorRect.top + (LONG)cursorInfo.size.height
	};

	const bool isSrgb = _colorInfo.kind == winrt::AdvancedColorKind::StandardDynamicRange;
	
	if (cursorFrame.type == _CursorType::Color) {
		winrt::com_ptr<ID3D12PipelineState>& pso = isSrgb ? _colorSrgbPSO : _colorPSO;
		if (!pso) {
			HRESULT hr = _CreateColorPSO(isSrgb, pso);
			if (FAILED(hr)) {
				Logger::Get().ComError("_CreateColorPSO 失败", hr);
				return hr;
			}
		}

		graphicsContext.SetPipelineState(pso.get());
		graphicsContext.SetRootSignature(_colorRootSignature.get());
	} else {
		bool isMonochrome = cursorFrame.type == _CursorType::Monochrome;
		
		winrt::com_ptr<ID3D12PipelineState>& pso = isMonochrome ?
			(isSrgb ? _monochromeSrgbPSO : _monochromePSO) :
			(isSrgb ? _maskedColorSrgbPSO : _maskedColorPSO);

		if (!pso) {
			HRESULT hr = _CreateMaskPSO(isMonochrome, isSrgb, pso);
			if (FAILED(hr)) {
				Logger::Get().ComError("_CreateMaskPSO 失败", hr);
				return hr;
			}
		}

		graphicsContext.SetPipelineState(pso.get());
		graphicsContext.SetRootSignature(_maskRootSignature.get());
	}

	const RECT viewportRect = {
		_destRect.left - _rendererRect.left,
		_destRect.top - _rendererRect.top,
		_destRect.right - _rendererRect.left,
		_destRect.bottom - _rendererRect.top
	};
	const SIZE viewportSize = Win32Helper::GetSizeOfRect(viewportRect);

	{
		// 转换到 NDC
		float constants[] = {
			(cursorRect.left - _destRect.left) * 2 / (float)viewportSize.cx - 1.0f,	// left
			1.0f - (cursorRect.top - _destRect.top) * 2 / (float)viewportSize.cy,	// top
			cursorInfo.size.width * 2 / (float)viewportSize.cx,	// width
			cursorInfo.size.height * 2 / (float)-viewportSize.cy	// height
		};
		graphicsContext.SetRoot32BitConstants(0, (UINT)std::size(constants), constants);
	}

	if (cursorFrame.type == _CursorType::Color) {
		graphicsContext.SetRootDescriptorTable(1, cursorFrame.textureSrvOffset);
	} else {
		DirectXHelper::Constant32 constants[] = {
			{.uintVal = cursorFrame.resSize.width},
			{.uintVal = cursorFrame.resSize.height},
			{.uintVal = cursorInfo.size.width},
			{.uintVal = cursorInfo.size.height},
			{.uintVal = backBuffer ? 0u :uint32_t(cursorRect.left - _destRect.left)},
			{.uintVal = backBuffer ? 0u : uint32_t(cursorRect.top - _destRect.top)},
			// 原始帧需要伽马校正，从渲染目标复制的临时纹理不需要。WCG/HDR 下这个参数有不同的意义
			{.uintVal = isSrgb ? (backBuffer ? 0u : 1u) :
				std::bit_cast<uint32_t>(_colorInfo.sdrWhiteLevel)}
		};
		graphicsContext.SetRoot32BitConstants(1, (UINT)std::size(constants), constants);

		graphicsContext.SetRootDescriptorTable(2, cursorFrame.textureSrvOffset);

		if (backBuffer) {
			// 掩码光标在叠加层上时需要从渲染目标复制光标下区域到临时纹理
			if (_tempOriginTextureSize.width < cursorInfo.size.width ||
				_tempOriginTextureSize.height < cursorInfo.size.height
			) {
				if (_tempOriginTexture) {
					_retiredTempOriginTextures.emplace_back(_RetiredTempOriginTexture{
						.texture = std::move(_tempOriginTexture),
						.fenceValue = frameFenceValue,
						.srvOffset = _tempOriginTextureSrvOffset
					});
				}

				// 对齐到 32 的倍数
				_tempOriginTextureSize.width = (cursorInfo.size.width + 31) & ~31;
				_tempOriginTextureSize.height = (cursorInfo.size.height + 31) & ~31;

				ID3D12Device5* device = _d3d12Context->GetDevice();

				CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

				D3D12_HEAP_FLAGS heapFlags = _d3d12Context->IsHeapFlagCreateNotZeroedSupported() ?
					D3D12_HEAP_FLAG_CREATE_NOT_ZEROED : D3D12_HEAP_FLAG_NONE;

				CD3DX12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
					isSrgb ? DXGI_FORMAT_R8G8B8A8_UNORM : DXGI_FORMAT_R16G16B16A16_FLOAT,
					_tempOriginTextureSize.width,
					_tempOriginTextureSize.height,
					1, 1, 1, 0,
					D3D12_RESOURCE_FLAG_NONE
				);

				HRESULT hr = device->CreateCommittedResource(
					&heapProps,
					heapFlags,
					&texDesc,
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
					nullptr,
					IID_PPV_ARGS(&_tempOriginTexture)
				);
				if (FAILED(hr)) {
					Logger::Get().ComError("CreateCommittedResource 失败", hr);
					return hr;
				}
				
				auto& descriptorHeap = _d3d12Context->GetDescriptorHeap();

				hr = descriptorHeap.Alloc(1, _tempOriginTextureSrvOffset);
				if (FAILED(hr)) {
					Logger::Get().ComError("DescriptorHeap::Alloc 失败", hr);
					return hr;
				}

				CD3DX12_SHADER_RESOURCE_VIEW_DESC srvDesc =
					CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2D(texDesc.Format, 1);
				device->CreateShaderResourceView(_tempOriginTexture.get(), &srvDesc,
					descriptorHeap.GetCpuHandle(_tempOriginTextureSrvOffset));
			}

			graphicsContext.InsertTransitionBarrier(
				_tempOriginTexture.get(),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_COPY_DEST
			);
			graphicsContext.InsertTransitionBarrier(
				backBuffer,
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				D3D12_RESOURCE_STATE_COPY_SOURCE
			);
			
			{
				D3D12_BOX srcBox = {
					UINT(std::max(cursorRect.left - _rendererRect.left, viewportRect.left)),
					UINT(std::max(cursorRect.top - _rendererRect.top, viewportRect.top)),
					0,
					UINT(std::min(cursorRect.right - _rendererRect.left, viewportRect.right)),
					UINT(std::min(cursorRect.bottom - _rendererRect.top, viewportRect.bottom)),
					1
				};
				uint32_t destLeft = uint32_t(std::max(0l,
					viewportRect.left - cursorRect.left + _rendererRect.left));
				uint32_t destTop = uint32_t(std::max(0l,
					viewportRect.top - cursorRect.top + _rendererRect.top));

				assert(destLeft + srcBox.right - srcBox.left <= cursorInfo.size.width);
				assert(destTop + srcBox.bottom - srcBox.top <= cursorInfo.size.height);

				graphicsContext.CopyTextureRegion(
					_tempOriginTexture.get(), destLeft, destTop, backBuffer, &srcBox);
			}
			
			graphicsContext.InsertTransitionBarrier(
				_tempOriginTexture.get(),
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			);
			graphicsContext.InsertTransitionBarrier(
				backBuffer,
				D3D12_RESOURCE_STATE_COPY_SOURCE,
				D3D12_RESOURCE_STATE_RENDER_TARGET
			);

			graphicsContext.SetRootDescriptorTable(3, _tempOriginTextureSrvOffset);
		} else {
			// 直接使用原始帧
			graphicsContext.SetRootDescriptorTable(3, curFrameSrvOffset);
		}
	}

	graphicsContext.RSSetViewportAndScissorRect(viewportRect);
	
	graphicsContext.Draw(4);
	
	return S_OK;
}

void CursorDrawer::OnMoved(const RECT& rendererRect, const RECT& destRect) noexcept {
	_rendererRect = rendererRect;
	_destRect = destRect;
}

void CursorDrawer::OnResized(const RECT& rendererRect, const RECT& destRect) noexcept {
	_rendererRect = rendererRect;
	_destRect = destRect;

	// 光标缩放和源窗口相同则清空已解析的光标，此时已和 GPU 同步
	if (ScalingWindow::Get().Options().cursorScale < FLOAT_EPSILON<float>) {
		_ClearCursorInfos();
	}
}

void CursorDrawer::OnColorInfoChanged(const ColorInfo& colorInfo) noexcept {
	bool wasScRGB = _colorInfo.kind != winrt::AdvancedColorKind::StandardDynamicRange;
	bool isScRGB = colorInfo.kind != winrt::AdvancedColorKind::StandardDynamicRange;
	_colorInfo = colorInfo;

	if (wasScRGB != isScRGB) {
		// 渲染目标纹理格式变了
		_tempOriginTexture = nullptr;
		_tempOriginTextureSize = {};

		if (_tempOriginTextureSrvOffset != std::numeric_limits<uint32_t>::max()) {
			_d3d12Context->GetDescriptorHeap().Free(_tempOriginTextureSrvOffset, 1);
			_tempOriginTextureSrvOffset = std::numeric_limits<uint32_t>::max();
		}
	}

	_ClearCursorInfos();
}

static bool GetCursorSizeFromBmps(HBITMAP hColorBmp, HBITMAP hMaskBmp, SizeU& size) noexcept {
	BITMAP bmp{};
	if (!GetObject(hColorBmp ? hColorBmp : hMaskBmp, sizeof(bmp), &bmp)) {
		Logger::Get().Win32Error("GetObject 失败");
		return false;
	}

	size.width = uint32_t(bmp.bmWidth);
	// 单色光标的掩码位图高度是光标实际高度的两倍
	size.height = uint32_t(hColorBmp ? bmp.bmHeight : bmp.bmHeight / 2);
	return true;
}

static bool GetCursorSizeUnderDpi96(HCURSOR hCursor, SizeU& cursorSize) noexcept {
	ICONINFO iconInfo{};
	{
		DPI_AWARENESS_CONTEXT oldDpiContext =
			SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_UNAWARE);
		auto se = wil::scope_exit([&] {
			SetThreadDpiAwarenessContext(oldDpiContext);
		});

		if (!GetIconInfo(hCursor, &iconInfo)) {
			Logger::Get().Win32Error("GetIconInfo 失败");
			return false;
		}
	}

	wil::unique_hbitmap hColorBmp(iconInfo.hbmColor);
	wil::unique_hbitmap hMaskBmp(iconInfo.hbmMask);

	if (!GetCursorSizeFromBmps(hColorBmp.get(), hMaskBmp.get(), cursorSize)) {
		Logger::Get().Error("GetCursorSizeFromBmps 失败");
		return false;
	}

	return true;
}

std::pair<const CursorDrawer::_CursorInfoKey, CursorDrawer::_CursorInfo>* CursorDrawer::_ResolveCursor(
	HCURSOR hCursor,
	POINT cursorPos
) noexcept {
	assert(hCursor);

	// 检索光标所在屏幕的 DPI
	const HMONITOR hCurMon = MonitorFromPoint(
		{ cursorPos.x + _destRect.left, cursorPos.y + _destRect.top }, MONITOR_DEFAULTTOPRIMARY);
	UINT monitorDpi = USER_DEFAULT_SCREEN_DPI;
	HRESULT hr = GetDpiForMonitor(hCurMon, MDT_EFFECTIVE_DPI, &monitorDpi, &monitorDpi);
	if (FAILED(hr)) {
		Logger::Get().ComError("GetDpiForMonitor 失败", hr);
	}
	
	auto it = _cursorInfos.find(_CursorInfoKey{ hCursor, monitorDpi });
	if (it != _cursorInfos.end()) {
		return &*it;
	}

	// 检查此光标是否不随 DPI 缩放
	it = _cursorInfos.find(_CursorInfoKey{ hCursor, 0 });
	if (it != _cursorInfos.end()) {
		return &*it;
	}

	_CursorInfo cursorInfo{};
	bool isCursorDpiAware = true;

	ICONINFOEX cursorIconInfoEx = { .cbSize = sizeof(cursorIconInfoEx) };
	if (!GetIconInfoEx(hCursor, &cursorIconInfoEx)) {
		Logger::Get().Win32Error("GetIconInfoEx 失败");
		return nullptr;
	}

	wil::unique_hbitmap hResColorBmp(cursorIconInfoEx.hbmColor);
	wil::unique_hbitmap hResMaskBmp(cursorIconInfoEx.hbmMask);

	SizeU cursorBmpSize;
	if (!GetCursorSizeFromBmps(hResColorBmp.get(), hResMaskBmp.get(), cursorBmpSize)) {
		Logger::Get().Error("GetCursorSizeFromBmps 失败");
		return nullptr;
	}

	// 将线程 DPI 感知设为 unaware 后 GetIconInfo 可以获得 100% DPI 缩放下的光标位图。
	// 我们借助这个特性检查光标是否随 DPI 缩放，不过只在程序启动时系统 DPI 缩放不是
	// 100% 时有效。
	if (SYSTEM_DPI == 96) {
		// 如果不能确定光标是否随 DPI 缩放则假设为真，绝大多数时候是对的
		cursorInfo.size = _CalcCursorSize(cursorBmpSize, SYSTEM_DPI, monitorDpi, true);
	} else {
		SizeU cursorSizeDpi96;
		if (!GetCursorSizeUnderDpi96(hCursor, cursorSizeDpi96)) {
			Logger::Get().Error("GetCursorSizeUnderDpi96 失败");
			return nullptr;
		}
		
		// 不同 DPI 下光标位图尺寸不变说明光标不跟随 DPI 缩放
		isCursorDpiAware = cursorBmpSize != cursorSizeDpi96;
		cursorInfo.size = _CalcCursorSize(cursorSizeDpi96, 96, monitorDpi, isCursorDpiAware);
	}

	SmallVector<wil::unique_hcursor, 1> cursorFrameRes;
	PointU resHotspot = { cursorIconInfoEx.xHotspot, cursorIconInfoEx.yHotspot };

	// 尝试从光标原始来源加载指定尺寸的资源，如果失败旧只能使用 GetIconInfo 得到的位图，
	// DPI 虚拟化机制可能导致图像质量严重下降。
	_TryResolveCursorFramesFromSource(hCursor, cursorIconInfoEx,
		cursorInfo.size.width, cursorFrameRes, cursorInfo.frameSequence);

	const auto initCursorFrameFromRes = [&](_CursorFrame& curCursorFrame, HCURSOR hResCursor) {
		ICONINFO iconInfo{};
		if (!GetIconInfo(hResCursor, &iconInfo)) {
			Logger::Get().Win32Error("GetIconInfo 失败");
			return false;
		}

		wil::unique_hbitmap hResColorBmp(iconInfo.hbmColor);
		wil::unique_hbitmap hResMaskBmp(iconInfo.hbmMask);

		if (!GetCursorSizeFromBmps(hResColorBmp.get(), hResMaskBmp.get(), curCursorFrame.resSize)) {
			Logger::Get().Error("GetCursorSizeFromBmps 失败");
			return false;
		}

		curCursorFrame.hotspot.x = (uint32_t)lround(
			iconInfo.xHotspot * cursorInfo.size.width / (double)curCursorFrame.resSize.width);
		curCursorFrame.hotspot.y = (uint32_t)lround(
			iconInfo.yHotspot * cursorInfo.size.height / (double)curCursorFrame.resSize.height);

		if (!_ResolveCursorFramePixels(
			curCursorFrame, hResColorBmp.get(), hResMaskBmp.get())) {
			Logger::Get().Error("_ResolveCursorFramePixels 失败");
			return false;
		}

		return true;
	};
	
	if (!cursorFrameRes.empty()) {
		cursorInfo.frames.resize(cursorFrameRes.size());
		for (uint32_t i = 0; i < cursorFrameRes.size(); ++i) {
			if (!initCursorFrameFromRes(cursorInfo.frames[i], cursorFrameRes[i].get())) {
				Logger::Get().Error("initCursorFrameFromRes 失败");
				return nullptr;
			}
		}
	} else {
		// 尝试使用未记录的 API 提取动画光标的每一帧
		SmallVector<HCURSOR, 1> rawCursorFrameRes;
		CursorHelper::TryResolveAnimatedCursor(hCursor, rawCursorFrameRes, cursorInfo.frameSequence);

		if (!rawCursorFrameRes.empty()) {
			cursorInfo.frames.resize(rawCursorFrameRes.size());
			for (uint32_t i = 0; i < rawCursorFrameRes.size(); ++i) {
				if (!initCursorFrameFromRes(cursorInfo.frames[i], rawCursorFrameRes[i])) {
					Logger::Get().Error("initCursorFrameFromRes 失败");
					return nullptr;
				}
			}
		} else {
			// 如果前面的解析都失败就直接从 hCursor 提取位图
			cursorInfo.frames.resize(1);

			_CursorFrame& cursorFrame = cursorInfo.frames[0];
			cursorFrame.resSize = cursorBmpSize;
			cursorFrame.hotspot.x = (uint32_t)lround(
				resHotspot.x * cursorInfo.size.width / (double)cursorFrame.resSize.width);
			cursorFrame.hotspot.y = (uint32_t)lround(
				resHotspot.y * cursorInfo.size.height / (double)cursorFrame.resSize.height);

			if (!_ResolveCursorFramePixels(cursorFrame, hResColorBmp.get(), hResMaskBmp.get())) {
				Logger::Get().Error("_ResolveCursorFramePixels 失败");
				return nullptr;
			}
		}
	}

	return &*_cursorInfos.emplace(_CursorInfoKey{ hCursor, isCursorDpiAware ? monitorDpi : 0 },
		std::move(cursorInfo)).first;
}

SizeU CursorDrawer::_CalcCursorSize(
	SizeU cursorBmpSize,
	uint32_t cursorDpi,
	uint32_t monitorDpi,
	bool isCursorDpiAware
) const noexcept {
	float scale = ScalingWindow::Get().Options().cursorScale;
	if (scale < FLOAT_EPSILON<float>) {
		// 光标缩放和源窗口相同
		SIZE destSize = Win32Helper::GetSizeOfRect(_destRect);
		scale = (((float)destSize.cx / _srcSize.width) +
			((float)destSize.cy / _srcSize.height)) / 2;
	}

	if (isCursorDpiAware) {
		scale *= (GetSystemMetricsForDpi(SM_CXCURSOR, monitorDpi) * _cursorBaseSize) /
			(GetSystemMetricsForDpi(SM_CXCURSOR, cursorDpi) * 32.0f);
	}
	
	return { (uint32_t)std::lround(cursorBmpSize.width * scale),
		(uint32_t)std::lround(cursorBmpSize.height * scale) };
}

void CursorDrawer::_TryResolveCursorFramesFromSource(
	HCURSOR hCursor,
	const ICONINFOEX& iconInfoEx,
	uint32_t preferedWidth,
	SmallVectorImpl<wil::unique_hcursor>& frames,
	SmallVectorImpl<std::pair<uint32_t, std::chrono::nanoseconds>>& frameSequence
) const noexcept {
	assert(frames.empty());

	auto it = std::find_if(
		STANDARD_CURSORS.begin(),
		STANDARD_CURSORS.end(),
		[&](const StandardCursorInfo& info) {
			return info.handle == hCursor;
		}
	);
	if (it == STANDARD_CURSORS.end()) {
		// 自定义光标尝试从 ICONINFOEX 中读取光标资源 ID
		if (StrHelper::StrLen(iconInfoEx.szModName) > 0) {
			wil::unique_hmodule hModule(
				LoadLibraryEx(iconInfoEx.szModName, NULL, LOAD_LIBRARY_AS_DATAFILE));
			if (hModule) {
				LPCWSTR resName = iconInfoEx.wResID != 0 ?
					MAKEINTRESOURCE(iconInfoEx.wResID) : iconInfoEx.szResName;
				wil::unique_hcursor hResCursor =
					CursorHelper::ExtractCursorFromModule(hModule.get(), resName, preferedWidth);
				if (hResCursor) {
					frames.push_back(std::move(hResCursor));
				}
			} else {
				Logger::Get().Win32Error("LoadLibraryEx 失败");
			}
		}
	} else {
		// 标准光标尝试从注册表读取光标文件路径
		if (_cursorBaseSize == 32 && it->resId != 0) {
			HMODULE hUser32 = GetModuleHandle(L"user32.dll");
			wil::unique_hcursor hResCursor = CursorHelper::ExtractCursorFromModule(
				hUser32, MAKEINTRESOURCE(it->resId), preferedWidth);
			if (hResCursor) {
				frames.push_back(std::move(hResCursor));
			} else {
				Logger::Get().Error("CursorHelper::ExtractCursorFromModule 失败");
			}
		} else {
			wil::unique_bstr regValue;
			HRESULT hr = wil::reg::get_value_nothrow(
				HKEY_CURRENT_USER, L"Control Panel\\Cursors", it->regValueName, &regValue);
			// 失败不视为错误
			if (FAILED(hr)) {
				return;
			}

			// 可能为空
			if (SysStringLen(regValue.get()) == 0) {
				return;
			}

			// 路径中可能包含环境变量字符串
			std::wstring filePath;
			hr = wil::ExpandEnvironmentStringsW(regValue.get(), filePath);
			if (FAILED(hr)) {
				Logger::Get().ComError("wil::ExpandEnvironmentStringsW 失败", hr);
				return;
			}

			if (!CursorHelper::ExtractCursorFramesFromFile(
				filePath.c_str(), preferedWidth, frames, frameSequence)) {
				Logger::Get().Error("CursorHelper::ExtractCursorFramesFromFile 失败");
			}
		}
	}
}

static void FillColorCursorRGB(
	HALF* textureData,
	uint8_t* pixels,
	const ColorInfo& colorInfo,
	float alpha
) noexcept {
	// 预乘 Alpha 通道并归一化
	float factor = alpha / 255.0f;

	if (colorInfo.kind == winrt::AdvancedColorKind::StandardDynamicRange) {
		*textureData++ = XMConvertFloatToHalf(pixels[2] * factor);
		*textureData++ = XMConvertFloatToHalf(pixels[1] * factor);
		*textureData = XMConvertFloatToHalf(pixels[0] * factor);
	} else {
		if (colorInfo.kind == winrt::AdvancedColorKind::HighDynamicRange) {
			factor *= colorInfo.sdrWhiteLevel;
		}

		*textureData++ = XMConvertFloatToHalf(ColorHelper::SrgbToLinear(pixels[2]) * factor);
		*textureData++ = XMConvertFloatToHalf(ColorHelper::SrgbToLinear(pixels[1]) * factor);
		*textureData = XMConvertFloatToHalf(ColorHelper::SrgbToLinear(pixels[0]) * factor);
	}
}

bool CursorDrawer::_ResolveCursorFramePixels(
	_CursorFrame& cursorFrame,
	HBITMAP hColorBmp,
	HBITMAP hMaskBmp
) const noexcept {
	const SizeU bmpSize = {
		cursorFrame.resSize.width,
		hColorBmp ? cursorFrame.resSize.height : cursorFrame.resSize.height * 2
	};

	// 单色光标也转换成 32 位以方便处理
	BITMAPINFO bi = {
		.bmiHeader = {
			.biSize = sizeof(BITMAPINFOHEADER),
			.biWidth = (LONG)bmpSize.width,
			.biHeight = -(LONG)bmpSize.height,
			.biPlanes = 1,
			.biBitCount = 32,
			.biCompression = BI_RGB,
			.biSizeImage = DWORD(bmpSize.width * bmpSize.height * 4)
		}
	};

	ByteBuffer pixels(bi.bmiHeader.biSizeImage);
	wil::unique_hdc_window hdcScreen(wil::window_dc(GetDC(NULL)));
	if (GetDIBits(hdcScreen.get(), hColorBmp ? hColorBmp : hMaskBmp, 0, bmpSize.height,
		pixels.Data(), &bi, DIB_RGB_COLORS) != (int)bmpSize.height
	) {
		Logger::Get().Win32Error("GetDIBits 失败");
		return false;
	}

	if (hColorBmp) {
		// 彩色掩码光标和彩色光标的区别在于前者的透明通道全为 0
		bool hasAlpha = false;
		for (uint32_t i = 3; i < bi.bmiHeader.biSizeImage; i += 4) {
			if (pixels[i] != 0) {
				hasAlpha = true;
				break;
			}
		}

		if (hasAlpha) {
			// 彩色光标
			cursorFrame.type = _CursorType::Color;
			cursorFrame.resTextureData.Resize(bi.bmiHeader.biSizeImage * 2);

			HALF* textureData = (HALF*)cursorFrame.resTextureData.Data();

			for (uint32_t i = 0; i < bi.bmiHeader.biSizeImage; i += 4) {
				float alpha = pixels[i + 3] / 255.0f;
				FillColorCursorRGB(&textureData[i], &pixels[i], _colorInfo, alpha);
				textureData[i + 3] = XMConvertFloatToHalf(1.0f - alpha);
			}
		} else {
			// 彩色掩码光标。如果不需要应用 XOR 则转换成彩色光标
			ByteBuffer maskPixels(bi.bmiHeader.biSizeImage);

			if (GetDIBits(hdcScreen.get(), hMaskBmp, 0, bmpSize.height,
				maskPixels.Data(), &bi, DIB_RGB_COLORS) != (int)bmpSize.height
			) {
				Logger::Get().Win32Error("GetDIBits 失败");
				return false;
			}

			// 计算此彩色掩码光标是否可以转换为彩色光标
			bool canConvertToColor = true;
			for (uint32_t i = 0; i < bi.bmiHeader.biSizeImage; i += 4) {
				if (maskPixels[i] != 0 &&
					(pixels[i] != 0 || pixels[i + 1] != 0 || pixels[i + 2] != 0)
				) {
					// 掩码不为 0 则不能转换为彩色光标
					canConvertToColor = false;
					break;
				}
			}

			if (canConvertToColor) {
				// 转换为彩色光标以获得更好的插值效果和渲染性能
				cursorFrame.type = _CursorType::Color;
				cursorFrame.resTextureData.Resize(bi.bmiHeader.biSizeImage * 2);

				HALF* textureData = (HALF*)cursorFrame.resTextureData.Data();

				for (uint32_t i = 0; i < bi.bmiHeader.biSizeImage; i += 4) {
					if (maskPixels[i] == 0) {
						// Alpha 通道为 0，无需设置
						FillColorCursorRGB(&textureData[i], &pixels[i], _colorInfo, 1.0f);
					} else {
						// 透明像素
						textureData[i + 3] = XMConvertFloatToHalf(1.0f);
					}
				}
			} else {
				cursorFrame.type = _CursorType::MaskedColor;

				// 将 XOR 掩码复制到透明通道中
				for (uint32_t i = 0; i < bi.bmiHeader.biSizeImage; i += 4) {
					std::swap(pixels[i], pixels[i + 2]);
					pixels[i + 3] = maskPixels[i];
				}

				cursorFrame.resTextureData = std::move(pixels);
			}
		}
	} else {
		// 单色光标。如果不需要应用反色则转换成彩色光标
		const uint32_t halfSize = bi.bmiHeader.biSizeImage / 2;

		// 计算此单色光标是否可以转换为彩色光标
		bool canConvertToColor = true;
		for (uint32_t i = 0; i < halfSize; i += 4) {
			// 上半部分是 AND 掩码，下半部分是 XOR 掩码
			if (pixels[i] != 0 && pixels[i + halfSize] != 0) {
				canConvertToColor = false;
				break;
			}
		}
		
		if (canConvertToColor) {
			// 转换为彩色光标以获得更好的插值效果和渲染性能
			cursorFrame.type = _CursorType::Color;
			cursorFrame.resTextureData.Resize(bi.bmiHeader.biSizeImage * 2);

			HALF* textureData = (HALF*)cursorFrame.resTextureData.Data();

			for (uint32_t i = 0; i < halfSize; i += 4) {
				// 上半部分是 AND 掩码，下半部分是 XOR 掩码
				// https://learn.microsoft.com/en-us/windows-hardware/drivers/display/drawing-monochrome-pointers
				if (pixels[i] == 0) {
					// 黑色全为 0，无需设置
					if (pixels[i + halfSize] != 0) {
						// 白色。非 HDR 时 sdrWhiteLevel 始终为 1
						std::fill_n(&textureData[i], 3, XMConvertFloatToHalf(_colorInfo.sdrWhiteLevel));
					}
				} else {
					// 透明
					textureData[i + 3] = XMConvertFloatToHalf(1.0f);
				}
			}
		} else {
			cursorFrame.type = _CursorType::Monochrome;

			// 位图数据：上半部分是 AND 掩码，下半部分是 XOR 掩码
			// 纹理数据：高四位是 AND 掩码，低四位是 XOR 掩码
			uint8_t* upperPtr = &pixels[0];
			uint8_t* lowerPtr = &pixels[halfSize];
			uint8_t* targetPtr = &pixels[0];
			for (uint32_t i = 0; i < halfSize; i += 4) {
				*targetPtr++ = (*upperPtr & 0xf0) | (*lowerPtr & 0xf);

				upperPtr += 4;
				lowerPtr += 4;
			}

			cursorFrame.resTextureData = std::move(pixels);
		}
	}

	return true;
}

HRESULT CursorDrawer::_InitializeCursorTexture(
	GraphicsContext& graphicsContext,
	_CursorInfo& cursorInfo,
	uint32_t cursorFrameIdx,
	uint64_t completedFenceValue
) noexcept {
	ID3D12Device5* device = _d3d12Context->GetDevice();
	_CursorFrame& curCursorFrame = cursorInfo.frames[cursorFrameIdx];

	// 尝试复用旧帧的临时资源。基本上动态光标的所有帧都是相同类型和尺寸，因此能复用的概率很高
	for (_CursorFrame& frame : cursorInfo.frames) {
		if (frame.uploadBuffer &&
			frame.tempResourcesFenceValue <= completedFenceValue &&
			frame.resSize == curCursorFrame.resSize &&
			frame.type == curCursorFrame.type
		) {
			curCursorFrame.uploadBuffer = std::move(frame.uploadBuffer);
			curCursorFrame.resTexture = std::move(frame.resTexture);
			std::swap(curCursorFrame.resTextureSrvOffset, frame.resTextureSrvOffset);
			break;
		}
	}

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);

	D3D12_HEAP_FLAGS heapFlags = _d3d12Context->IsHeapFlagCreateNotZeroedSupported() ?
		D3D12_HEAP_FLAG_CREATE_NOT_ZEROED : D3D12_HEAP_FLAG_NONE;

	CD3DX12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		curCursorFrame.type == _CursorType::Color ? DXGI_FORMAT_R16G16B16A16_FLOAT :
		(curCursorFrame.type == _CursorType::Monochrome ? DXGI_FORMAT_R8_UINT : DXGI_FORMAT_R8G8B8A8_UNORM),
		curCursorFrame.resSize.width, curCursorFrame.resSize.height, 1, 1);

	uint32_t textureRowSize = curCursorFrame.resSize.width *
		(curCursorFrame.type == _CursorType::Color ? 8 :
			(curCursorFrame.type == _CursorType::Monochrome ? 1 : 4));
	uint32_t textureRowPitch = DirectXHelper::Align(textureRowSize, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
	uint32_t bufferSize = textureRowPitch * texDesc.Height;

	if (!curCursorFrame.uploadBuffer) {
		CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

		HRESULT hr = device->CreateCommittedResource(
			&heapProps,
			heapFlags,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&curCursorFrame.uploadBuffer)
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateCommittedResource 失败", hr);
			return hr;
		}
	}

	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

	if (curCursorFrame.resTexture) {
		// 如果可以复用 resTexture 说明执行了缩放，resTexture 目前处于
		// PIXEL_SHADER_RESOURCE 状态。
		graphicsContext.InsertTransitionBarrier(
			curCursorFrame.resTexture.get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_COPY_DEST
		);
	} else {
		HRESULT hr = device->CreateCommittedResource(
			&heapProps,
			heapFlags,
			&texDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&curCursorFrame.resTexture)
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateCommittedResource 失败", hr);
			return hr;
		}
	}

	CD3DX12_RANGE emptyRange{};
	void* pData;
	HRESULT hr = curCursorFrame.uploadBuffer->Map(0, &emptyRange, &pData);
	if (FAILED(hr)) {
		Logger::Get().ComError("ID3D12Resource::Map 失败", hr);
		return hr;
	}

	if (textureRowSize == textureRowPitch) {
		std::memcpy(pData, curCursorFrame.resTextureData.Data(), bufferSize);
	} else {
		for (uint32_t i = 0; i < curCursorFrame.resSize.height; ++i) {
			std::memcpy(
				(uint8_t*)pData + textureRowPitch * i,
				&curCursorFrame.resTextureData[textureRowSize * i],
				textureRowSize
			);
		}
	}

	curCursorFrame.uploadBuffer->Unmap(0, nullptr);

	curCursorFrame.resTextureData.Clear();

	graphicsContext.CopyTextureRegion(
		CD3DX12_TEXTURE_COPY_LOCATION(curCursorFrame.resTexture.get()),
		0,
		0,
		CD3DX12_TEXTURE_COPY_LOCATION(
			curCursorFrame.uploadBuffer.get(),
			D3D12_PLACED_SUBRESOURCE_FOOTPRINT{
				.Footprint = {
					.Format = texDesc.Format,
					.Width = (UINT)texDesc.Width,
					.Height = texDesc.Height,
					.Depth = 1,
					.RowPitch = textureRowPitch
				}
			}
		)
	);

	graphicsContext.InsertTransitionBarrier(
		curCursorFrame.resTexture.get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	);

	auto& descriptorHeap = _d3d12Context->GetDescriptorHeap();

	// * 单色光标和彩色掩码光标始终使用最近邻采样
	// * 不需要缩放时执行最近邻采样
	// * 需要缩小时始终使用 CatmullRom
	// * 需要放大时是否使用 CatmullRom 取决于 cursorInterpolationMode
	if (curCursorFrame.type == _CursorType::Color &&
		(cursorInfo.size.width < curCursorFrame.resSize.width ||
		(cursorInfo.size.width > curCursorFrame.resSize.width &&
			ScalingWindow::Get().Options().cursorInterpolationMode == CursorInterpolationMode::Bicubic))
	) {
		// SDR 色域下由于预乘 alpha 无法在线性空间插值
		texDesc.Width = cursorInfo.size.width;
		texDesc.Height = cursorInfo.size.height;
		texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		hr = device->CreateCommittedResource(
			&heapProps,
			heapFlags,
			&texDesc,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			nullptr,
			IID_PPV_ARGS(&curCursorFrame.texture)
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateCommittedResource 失败", hr);
			return hr;
		}

		// 以 D3D12_HEAP_FLAG_CREATE_NOT_ZEROED 创建的资源作为渲染目标需先
		// Discard/Clear/Copy。
		if (heapFlags & D3D12_HEAP_FLAG_CREATE_NOT_ZEROED) {
			graphicsContext.DiscardResource(curCursorFrame.texture.get());
		}
		
		auto& rtvDescriptorHeap = _d3d12Context->GetDescriptorHeap(true);

		if (curCursorFrame.resTextureSrvOffset == std::numeric_limits<uint32_t>::max()) {
			hr = descriptorHeap.Alloc(1, curCursorFrame.resTextureSrvOffset);
			if (FAILED(hr)) {
				Logger::Get().ComError("DescriptorHeap::Alloc 失败", hr);
				return hr;
			}
		}

		hr = rtvDescriptorHeap.Alloc(1, curCursorFrame.textureRtvOffset);
		if (FAILED(hr)) {
			Logger::Get().ComError("DescriptorHeap::Alloc 失败", hr);
			return hr;
		}

		{
			CD3DX12_SHADER_RESOURCE_VIEW_DESC srvDesc =
				CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2D(texDesc.Format, 1);
			device->CreateShaderResourceView(
				curCursorFrame.resTexture.get(),
				&srvDesc,
				descriptorHeap.GetCpuHandle(curCursorFrame.resTextureSrvOffset)
			);
		}
		
		{
			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {
				.Format = texDesc.Format,
				.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D
			};
			device->CreateRenderTargetView(
				curCursorFrame.texture.get(),
				&rtvDesc,
				rtvDescriptorHeap.GetCpuHandle(curCursorFrame.textureRtvOffset)
			);
		}

		if (!_cursorResizerPSO) {
			hr = _CreateCursorResizerPSO();
			if (FAILED(hr)) {
				Logger::Get().ComError("_CreateCursorResizerPSO 失败", hr);
				return hr;
			}
		}
		
		graphicsContext.SetPipelineState(_cursorResizerPSO.get());
		graphicsContext.SetRootSignature(_cursorResizerRootSignature.get());

		{
			DirectXHelper::Constant32 constants[] = {
				{.uintVal = curCursorFrame.resSize.width},
				{.uintVal = curCursorFrame.resSize.height},
				{.floatVal = 1.0f / curCursorFrame.resSize.width},
				{.floatVal = 1.0f / curCursorFrame.resSize.height}
			};
			graphicsContext.SetRoot32BitConstants(0, (UINT)std::size(constants), constants);
		}

		graphicsContext.SetRootDescriptorTable(1, curCursorFrame.resTextureSrvOffset);

		graphicsContext.RSSetViewportAndScissorRect(CD3DX12_RECT(
			0, 0, (LONG)cursorInfo.size.width, (LONG)cursorInfo.size.height));

		uint32_t oldRtvOffset = graphicsContext.OMGetRenderTarget();
		graphicsContext.OMSetRenderTarget(curCursorFrame.textureRtvOffset);
		
		graphicsContext.Draw(3);

		// 还原渲染目标
		graphicsContext.OMSetRenderTarget(oldRtvOffset);

		graphicsContext.InsertTransitionBarrier(
			curCursorFrame.texture.get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		);
	} else {
		curCursorFrame.texture = std::move(curCursorFrame.resTexture);
	}

	hr = descriptorHeap.Alloc(1, curCursorFrame.textureSrvOffset);
	if (FAILED(hr)) {
		Logger::Get().ComError("DescriptorHeap::Alloc 失败", hr);
		return hr;
	}

	CD3DX12_SHADER_RESOURCE_VIEW_DESC srvDesc =
		CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2D(texDesc.Format, 1);
	device->CreateShaderResourceView(curCursorFrame.texture.get(), &srvDesc,
		descriptorHeap.GetCpuHandle(curCursorFrame.textureSrvOffset));

	return S_OK;
}

void CursorDrawer::_ClearCursorInfos() noexcept {
	_lastRawCursorHandle = NULL;
	_curCursorInfoKeyValue = nullptr;

	auto& csuDescriptorHeap = _d3d12Context->GetDescriptorHeap();
	auto& rtvDescriptorHeap = _d3d12Context->GetDescriptorHeap(true);
	for (const auto& pair : _cursorInfos) {
		pair.second.FreeDescriptors(csuDescriptorHeap, rtvDescriptorHeap);
	}

	_cursorInfos.clear();
	_cursorInfosWithTempResources.clear();
}

HRESULT CursorDrawer::_CreateColorPSO(
	bool isSrgb,
	winrt::com_ptr<ID3D12PipelineState>& result
) noexcept {
	ID3D12Device5* device = _d3d12Context->GetDevice();

	if (!_colorRootSignature) {
		winrt::com_ptr<ID3DBlob> signature;
		{
			CD3DX12_DESCRIPTOR_RANGE1 srvRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0,
				D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);
			D3D12_ROOT_PARAMETER1 rootParams[] = {
				{
					.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
					.Constants = {
						.Num32BitValues = 4
					},
					.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX
				},
				{
					.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
					.DescriptorTable = {
						.NumDescriptorRanges = 1,
						.pDescriptorRanges = &srvRange
					},
					.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL
				}
			};
			D3D12_STATIC_SAMPLER_DESC samplerDesc = {
				.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT,
				.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
				.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
				.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
				.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
				.ShaderRegister = 0
			};
			CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc(
				(UINT)std::size(rootParams), rootParams, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_NONE);

			HRESULT hr = D3DX12SerializeVersionedRootSignature(
				&rootSignatureDesc, _d3d12Context->GetRootSignatureVersion(), signature.put(), nullptr);
			if (FAILED(hr)) {
				Logger::Get().ComError("D3DX12SerializeVersionedRootSignature 失败", hr);
				return hr;
			}
		}

		HRESULT hr = device->CreateRootSignature(
			0,
			signature->GetBufferPointer(),
			signature->GetBufferSize(),
			IID_PPV_ARGS(&_colorRootSignature)
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateRootSignature 失败", hr);
			return hr;
		}
	}

	bool isSM6Supported = _d3d12Context->GetShaderModel() >= D3D_SHADER_MODEL_6_0;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {
		.pRootSignature = _colorRootSignature.get(),
		.VS = DirectXHelper::SelectShader(isSM6Supported, CursorVS, CursorVS_SM5),
		.PS = DirectXHelper::SelectShader(isSM6Supported, TextureBlitPS, TextureBlitPS_SM5),
		.BlendState = {
			.RenderTarget = {{
				// FinalColor = CursorColor.rgb + ScreenColor * CursorColor.a
				.BlendEnable = TRUE,
				.SrcBlend = D3D12_BLEND_ONE,
				.DestBlend = D3D12_BLEND_SRC_ALPHA,
				.BlendOp = D3D12_BLEND_OP_ADD,
				.SrcBlendAlpha = D3D12_BLEND_ONE,
				.DestBlendAlpha = D3D12_BLEND_ZERO,
				.BlendOpAlpha = D3D12_BLEND_OP_ADD,
				.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL
			}}
		},
		.SampleMask = UINT_MAX,
		.RasterizerState = {
			.FillMode = D3D12_FILL_MODE_SOLID,
			.CullMode = D3D12_CULL_MODE_NONE
		},
		.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
		.NumRenderTargets = 1,
		.RTVFormats = { isSrgb ? DXGI_FORMAT_R8G8B8A8_UNORM : DXGI_FORMAT_R16G16B16A16_FLOAT },
		.SampleDesc = { .Count = 1 }
	};
	HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&result));
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateGraphicsPipelineState 失败", hr);
		return hr;
	}

	return S_OK;
}

HRESULT CursorDrawer::_CreateMaskPSO(
	bool isMonochrome,
	bool isSrgb,
	winrt::com_ptr<ID3D12PipelineState>& result
) noexcept {
	if (!_maskRootSignature) {
		winrt::com_ptr<ID3DBlob> signature;
		{
			CD3DX12_DESCRIPTOR_RANGE1 srvRange1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0,
				D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);
			CD3DX12_DESCRIPTOR_RANGE1 srvRange2(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0,
				D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);
			D3D12_ROOT_PARAMETER1 rootParams[] = {
				{
					.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
					.Constants = {
						.ShaderRegister = 0,
						.Num32BitValues = 4
					},
					.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX
				},
				{
					.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
					.Constants = {
						.ShaderRegister = 1,
						.Num32BitValues = 7
					},
					.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL
				},
				{
					.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
					.DescriptorTable = {
						.NumDescriptorRanges = 1,
						.pDescriptorRanges = &srvRange1
					},
					.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL
				},
				{
					.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
					.DescriptorTable = {
						.NumDescriptorRanges = 1,
						.pDescriptorRanges = &srvRange2
					},
					.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL
				}
			};
			CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc(
				(UINT)std::size(rootParams), rootParams, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

			HRESULT hr = D3DX12SerializeVersionedRootSignature(
				&rootSignatureDesc,
				_d3d12Context->GetRootSignatureVersion(),
				signature.put(),
				nullptr
			);
			if (FAILED(hr)) {
				Logger::Get().ComError("D3DX12SerializeVersionedRootSignature 失败", hr);
				return hr;
			}
		}

		HRESULT hr = _d3d12Context->GetDevice()->CreateRootSignature(
			0,
			signature->GetBufferPointer(),
			signature->GetBufferSize(),
			IID_PPV_ARGS(&_maskRootSignature)
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateRootSignature 失败", hr);
			return hr;
		}
	}

	bool isSM6Supported = _d3d12Context->GetShaderModel() >= D3D_SHADER_MODEL_6_0;

	D3D12_SHADER_BYTECODE psByteCode;
	if (isMonochrome) {
		if (isSrgb) {
			psByteCode = DirectXHelper::SelectShader(
				isSM6Supported, MonochromeCursorPS_sRGB, MonochromeCursorPS_sRGB_SM5);
		} else {
			psByteCode = DirectXHelper::SelectShader(
				isSM6Supported, MonochromeCursorPS, MonochromeCursorPS_SM5);
		}
	} else {
		if (isSrgb) {
			psByteCode = DirectXHelper::SelectShader(
				isSM6Supported, MaskedCursorPS_sRGB, MaskedCursorPS_sRGB_SM5);
		} else {
			psByteCode = DirectXHelper::SelectShader(
				isSM6Supported, MaskedCursorPS, MaskedCursorPS_SM5);
		}
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {
		.pRootSignature = _maskRootSignature.get(),
		.VS = DirectXHelper::SelectShader(isSM6Supported, CursorVS, CursorVS_SM5),
		.PS = psByteCode,
		.BlendState = {
			.RenderTarget = {{ .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL }}
		},
		.SampleMask = UINT_MAX,
		.RasterizerState = {
			.FillMode = D3D12_FILL_MODE_SOLID,
			.CullMode = D3D12_CULL_MODE_NONE
		},
		.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
		.NumRenderTargets = 1,
		.RTVFormats = { isSrgb ? DXGI_FORMAT_R8G8B8A8_UNORM : DXGI_FORMAT_R16G16B16A16_FLOAT },
		.SampleDesc = { .Count = 1 }
	};
	HRESULT hr = _d3d12Context->GetDevice()->CreateGraphicsPipelineState(
		&psoDesc, IID_PPV_ARGS(&result));
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateGraphicsPipelineState 失败", hr);
		return hr;
	}

	return S_OK;
}

HRESULT CursorDrawer::_CreateCursorResizerPSO() noexcept {
	ID3D12Device5* device = _d3d12Context->GetDevice();

	winrt::com_ptr<ID3DBlob> signature;
	{
		CD3DX12_DESCRIPTOR_RANGE1 srvRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0,
			D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);
		D3D12_ROOT_PARAMETER1 rootParams[] = {
			{
				.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
				.Constants = {
					.Num32BitValues = 4
				},
				.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL
			},
			{
				.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
				.DescriptorTable = {
					.NumDescriptorRanges = 1,
					.pDescriptorRanges = &srvRange
				},
				.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL
			}
		};
		D3D12_STATIC_SAMPLER_DESC samplerDesc = {
			.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
			.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
			.ShaderRegister = 0
		};
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc(
			(UINT)std::size(rootParams), rootParams, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_NONE);

		HRESULT hr = D3DX12SerializeVersionedRootSignature(
			&rootSignatureDesc,
			_d3d12Context->GetRootSignatureVersion(),
			signature.put(),
			nullptr
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("D3DX12SerializeVersionedRootSignature 失败", hr);
			return hr;
		}
	}

	HRESULT hr = device->CreateRootSignature(
		0,
		signature->GetBufferPointer(),
		signature->GetBufferSize(),
		IID_PPV_ARGS(&_cursorResizerRootSignature)
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateRootSignature 失败", hr);
		return hr;
	}

	bool isSM6Supported = _d3d12Context->GetShaderModel() >= D3D_SHADER_MODEL_6_0;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {
		.pRootSignature = _cursorResizerRootSignature.get(),
		.VS = DirectXHelper::SelectShader(isSM6Supported, FullscreenVS, FullscreenVS_SM5),
		.PS = DirectXHelper::SelectShader(isSM6Supported, CursorResizerPS, CursorResizerPS_SM5),
		.BlendState = {
			.RenderTarget = {{ .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL }}
		},
		.SampleMask = UINT_MAX,
		.RasterizerState = {
			.FillMode = D3D12_FILL_MODE_SOLID,
			.CullMode = D3D12_CULL_MODE_NONE
		},
		.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
		.NumRenderTargets = 1,
		.RTVFormats = { DXGI_FORMAT_R16G16B16A16_FLOAT },
		.SampleDesc = { .Count = 1 }
	};
	hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&_cursorResizerPSO));
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateGraphicsPipelineState 失败", hr);
		return hr;
	}

	return S_OK;
}

void CursorDrawer::_ClearRetiredResources(uint64_t completedFenceValue) noexcept {
	if (!_cursorInfosWithTempResources.empty()) {
		auto& csuDescriptorHeap = _d3d12Context->GetDescriptorHeap();
		auto& rtvDescriptorHeap = _d3d12Context->GetDescriptorHeap(true);

		// 有用的 fence 值是所有帧中最高的 tempResourcesFenceValue，它们按升序排列
		auto it = _cursorInfosWithTempResources.begin();
		for (; it != _cursorInfosWithTempResources.end(); ++it) {
			auto it1 = _cursorInfos.find(*it);
			if (it1 == _cursorInfos.end()) {
				// 不会出现这种情况，万一出现可以安全删除
				assert(false);
			} else {
				SmallVectorImpl<_CursorFrame>& frames = it1->second.frames;

				// 所有帧都已初始化才会进入 _cursorInfosWithTempResources
				assert(std::all_of(frames.begin(), frames.end(), [](const _CursorFrame& frame) {
					return frame.texture != nullptr;
				}));

				if (std::any_of(frames.begin(), frames.end(), [&](const _CursorFrame& frame) {
					return frame.tempResourcesFenceValue > completedFenceValue;
				})) {
					break;
				}

				for (_CursorFrame& frame : frames) {
					frame.uploadBuffer = nullptr;
					frame.resTexture = nullptr;

					if (frame.textureRtvOffset != std::numeric_limits<uint32_t>::max()) {
						rtvDescriptorHeap.Free(frame.textureRtvOffset, 1);
						frame.textureRtvOffset = std::numeric_limits<uint32_t>::max();
					}
					if (frame.resTextureSrvOffset != std::numeric_limits<uint32_t>::max()) {
						csuDescriptorHeap.Free(frame.resTextureSrvOffset, 1);
						frame.resTextureSrvOffset = std::numeric_limits<uint32_t>::max();
					}
				}
			}
		}

		if (it != _cursorInfosWithTempResources.begin()) {
			size_t removeCount = it - _cursorInfosWithTempResources.begin();
			_cursorInfosWithTempResources.erase(_cursorInfosWithTempResources.begin(), it);
			Logger::Get().Info(fmt::format("已清理 {} 个 _cursorInfosWithTempResources 成员，剩余 {}",
				removeCount, _cursorInfosWithTempResources.size()).c_str());
		}
	}

	// _cursorInfos 成员太多时清理使用频率较低的
	static constexpr uint32_t MAX_CURSOR_INFO_COUNT = 63;
	if (_cursorInfos.size() > MAX_CURSOR_INFO_COUNT) {
		std::vector<const std::pair<const _CursorInfoKey, _CursorInfo>*> cursorInfos;
		for (const auto& pair : _cursorInfos) {
			cursorInfos.push_back(&pair);
		}

		// 以最近使用的顺序排序
		std::sort(cursorInfos.begin(), cursorInfos.end(), [](auto l, auto r) {
			return l->second.lastUseFenceValue < r->second.lastUseFenceValue;
		});

		auto& csuDescriptorHeap = _d3d12Context->GetDescriptorHeap();
		auto& rtvDescriptorHeap = _d3d12Context->GetDescriptorHeap(true);
		// 最多删除一半成员
		uint32_t removeCount = 0;
		for (uint32_t i = 0; i < (MAX_CURSOR_INFO_COUNT + 1) / 2; ++i) {
			const _CursorInfo& curCursorInfo = cursorInfos[i]->second;

			if (curCursorInfo.lastUseFenceValue <= completedFenceValue) {
				curCursorInfo.FreeDescriptors(csuDescriptorHeap, rtvDescriptorHeap);
				_cursorInfos.erase(cursorInfos[i]->first);
				++removeCount;
			} else {
				break;
			}
		}
		Logger::Get().Info(fmt::format("已清理 {} 个 _cursorInfos 成员，剩余 {}",
			removeCount, _cursorInfos.size()).c_str());
	}

	if (!_retiredCursorInfos.empty()) {
		auto& csuDescriptorHeap = _d3d12Context->GetDescriptorHeap();
		auto& rtvDescriptorHeap = _d3d12Context->GetDescriptorHeap(true);

		// fenceValue 按升序排列
		auto it = _retiredCursorInfos.begin();
		for (; it != _retiredCursorInfos.end(); ++it) {
			if (it->lastUseFenceValue <= completedFenceValue) {
				it->FreeDescriptors(csuDescriptorHeap, rtvDescriptorHeap);
			}
		}

		if (it != _retiredCursorInfos.begin()) {
			size_t removeCount = it - _retiredCursorInfos.begin();
			_retiredCursorInfos.erase(_retiredCursorInfos.begin(), it);
			Logger::Get().Info(fmt::format("已清理 {} 个 _retiredCursorInfos 成员，剩余 {}",
				removeCount, _retiredCursorInfos.size()).c_str());
		}
	}

	if (!_retiredTempOriginTextures.empty()) {
		auto& csuDescriptorHeap = _d3d12Context->GetDescriptorHeap();

		// fenceValue 按升序排列
		auto it = _retiredTempOriginTextures.begin();
		for (; it != _retiredTempOriginTextures.end(); ++it) {
			if (it->fenceValue > completedFenceValue) {
				break;
			}

			if (it->srvOffset != std::numeric_limits<uint32_t>::max()) {
				csuDescriptorHeap.Free(it->srvOffset, 1);
			}
		}

		if (it != _retiredTempOriginTextures.begin()) {
			size_t removeCount = it - _retiredTempOriginTextures.begin();
			_retiredTempOriginTextures.erase(_retiredTempOriginTextures.begin(), it);
			Logger::Get().Info(fmt::format("已清理 {} 个 _retiredTempOriginTextures 成员，剩余 {}",
				removeCount, _retiredTempOriginTextures.size()).c_str());
		}
	}
}

void CursorDrawer::_OnCursorsRegChanged(wil::RegistryChangeKind) noexcept {
	uint32_t oldRunId = ScalingWindow::RunId();

	// 系统可能正在生成新光标，保险起见等待一段时间。当前在线程池中，可直接 Sleep
	Sleep(500);

	// HKEY_CURRENT_USER\Control Panel\Cursors 中任何键有改变都触发重新解析，
	// 这可以覆盖所有鼠标指针相关设置的更改。
	ScalingWindow::Dispatcher().TryEnqueue(
		[this, cursorBaseSize(GetCursorBaseSize()), oldRunId]() {
		if (ScalingWindow::RunId() != oldRunId || !ScalingWindow::Get().Handle()) {
			return;
		}

		_cursorBaseSize = cursorBaseSize;

		// _cursorBaseSize 改变后现有 _CursorInfo 失效
		for (auto& pair : _cursorInfos) {
			_retiredCursorInfos.push_back(std::move(pair.second));

			// 所有权已经转移，避免重复释放
			for (_CursorFrame& frame : pair.second.frames) {
				frame.textureSrvOffset = std::numeric_limits<uint32_t>::max();
				frame.textureRtvOffset = std::numeric_limits<uint32_t>::max();
				frame.resTextureSrvOffset = std::numeric_limits<uint32_t>::max();
			}
		}

		// 将 fenceValue 按升序排列
		std::sort(
			_retiredCursorInfos.begin(),
			_retiredCursorInfos.end(),
			[](const _CursorInfo& l, const _CursorInfo& r) {
				return l.lastUseFenceValue < r.lastUseFenceValue;
			}
		);

		_ClearCursorInfos();
	});
}

void CursorDrawer::_CursorInfo::FreeDescriptors(
	DescriptorHeap& csuDescriptorHeap,
	DescriptorHeap& rtvDescriptorHeap
) const noexcept {
	for (const _CursorFrame& frame : frames) {
		if (frame.textureSrvOffset != std::numeric_limits<uint32_t>::max()) {
			csuDescriptorHeap.Free(frame.textureSrvOffset, 1);
		}
		if (frame.textureRtvOffset != std::numeric_limits<uint32_t>::max()) {
			rtvDescriptorHeap.Free(frame.textureRtvOffset, 1);
		}
		if (frame.resTextureSrvOffset != std::numeric_limits<uint32_t>::max()) {
			csuDescriptorHeap.Free(frame.resTextureSrvOffset, 1);
		}
	}
}

}
