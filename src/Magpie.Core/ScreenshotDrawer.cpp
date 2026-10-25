#include "pch.h"
#include "ScreenshotDrawer.h"
#include "CommandContext.h"
#include "DescriptorHeap.h"
#include "DirectXHelper.h"
#include "EffectDrawerBase.h"
#include "GpuResourceManager.h"
#include "LocalizationService.h"
#include "ScalingWindow.h"
#include "ScreenshotFilenameTemplateHelper.h"
#include "shaders/ScreenshotCS.h"
#include "shaders/ScreenshotCS_HDR.h"
#include "shaders/ScreenshotCS_HDR_SM5.h"
#include "shaders/ScreenshotCS_SM5.h"
#include "shaders/ScreenshotCS_WCG.h"
#include "shaders/ScreenshotCS_WCG_SM5.h"
#include "TextureHelper.h"
#include "Win32Helper.h"

namespace Magpie {

ScreenshotDrawer::~ScreenshotDrawer() {
	// 等待截图完成
	MSG msg;
	while (_pendingScreenshotCount != 0) {
		// 必须处理消息队列，否则协程无法执行
		WaitMessage();

		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			DispatchMessage(&msg);
		}
	}

	assert(_screenshotDatas.empty());
}

void ScreenshotDrawer::Initialize(
	D3D12Context& d3d12Context,
	const winrt::DispatcherQueue& dispatcher,
	const ColorInfo& colorInfo
) noexcept {
	_d3d12Context = &d3d12Context;
	_dispatcher = dispatcher;
	_colorInfo = colorInfo;
}

HRESULT ScreenshotDrawer::Draw(
	ComputeContext& computeContext,
	uint32_t textureSrvOffset,
	SizeU textureSize,
	uint64_t frameFenceValue
) noexcept {
	return _DrawImpl(computeContext, textureSrvOffset,
		nullptr, EffectTextureFormat::UNKNOWN, textureSize, frameFenceValue);
}

HRESULT ScreenshotDrawer::Draw(
	ComputeContext& computeContext,
	ID3D12Resource* texture,
	EffectTextureFormat textureFormat,
	SizeU textureSize,
	uint64_t frameFenceValue)
	noexcept {
	return _DrawImpl(computeContext, std::numeric_limits<uint32_t>::max(),
		texture, textureFormat, textureSize, frameFenceValue);
}

static bool IsExportDataValid(
	const EffectDrawerExportData& exportData,
	uint64_t& bufferSize,
	void*& readbackBufferData
) noexcept {
	if (!exportData.readbackBufferAllocation ||
		exportData.textureSize == SizeU{} ||
		exportData.textureSize.width > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION ||
		exportData.textureSize.height > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION ||
		exportData.textureFormat <= EffectTextureFormat::COLOR_SPACE_ADAPTIVE ||
		exportData.textureFormat >= EffectTextureFormat::COUNT) {
		return false;
	}

	uint32_t texelSize = EFFECT_TEXTURE_FORMAT_PROPS[(uint32_t)exportData.textureFormat].texelSize;
	uint32_t rowSize = exportData.textureSize.width * texelSize;

	if (exportData.rowPitch < rowSize) {
		return false;
	}

	ID3D12Resource* buffer = exportData.readbackBufferAllocation->GetResource();
	D3D12_RESOURCE_DESC desc = buffer->GetDesc();

	if (desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
		return false;
	}

	bufferSize = exportData.rowPitch * size_t(exportData.textureSize.height - 1) + rowSize;
	if (desc.Width < bufferSize) {
		return false;
	}

	// CPU 需读取整个缓冲区
	HRESULT hr = buffer->Map(0, nullptr, &readbackBufferData);
	if (FAILED(hr)) {
		Logger::Get().ComError("ID3D12Resource::Map 失败", hr);
		return false;
	}

	return true;
}

void ScreenshotDrawer::ExportInternalTexture(
	EffectDrawerExportData&& exportData,
	uint64_t frameFenceValue,
	uint32_t effectIdx
) noexcept {
	uint64_t bufferSize;
	void* readbackBufferData;
	if (IsExportDataValid(exportData, bufferSize, readbackBufferData)) {
		_screenshotDatas.push_back(_ScreenshotData{
			.fenceValue = frameFenceValue,
			.readbackBufferAllocation = std::move(exportData.readbackBufferAllocation),
			.readbackBufferData = readbackBufferData,
			.textureSize = exportData.textureSize,
			.textureFormat = exportData.textureFormat,
			.rowPitch = exportData.rowPitch,
			.bufferSize = bufferSize,
			.colorInfo = _colorInfo
		});
	} else {
		Logger::Get().Error(fmt::format("EffectDrawer[{}] 导出内部纹理失败", effectIdx));

		if (exportData.readbackBufferAllocation) {
			_d3d12Context->GetGpuResourceManager().RetireResource(
				std::move(exportData.readbackBufferAllocation), false);
		}

		// readbackBufferData 为空表示截图失败
		_screenshotDatas.push_back(_ScreenshotData{});
	}
}

void ScreenshotDrawer::OnColorInfoChanged(const ColorInfo& colorInfo) noexcept {
	const winrt::AdvancedColorKind oldKind = _colorInfo.kind;

	_colorInfo = colorInfo;

	if (colorInfo.kind != oldKind) {
		_screenshotPSO = nullptr;
	}
}

static void AppendSuffixToScreenshotFilename(std::wstring& screenshotFileName) noexcept {
	const ScalingOptions& options = ScalingWindow::Get().Options();

	uint32_t suffixNum = 1;

	WIN32_FIND_DATA findData{};
	wil::unique_hfind hFind(FindFirstFileEx(
		StrHelper::Concat(options.screenshotsDir.native(), L"\\", screenshotFileName, L"*").c_str(),
		FindExInfoBasic, &findData, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH));
	if (hFind) {
		do {
			std::wstring_view fileName(findData.cFileName);

			// 不考虑扩展名
			if (size_t dotPos = fileName.find_last_of(L'.'); dotPos != std::wstring_view::npos) {
				fileName.remove_suffix(fileName.size() - dotPos);
			}

			// 文件名重复时格式为 "{baseFileName} ({suffixNum})"

			// 原始文件名视为 1
			if (fileName.size() == screenshotFileName.size()) {
				assert(fileName == screenshotFileName);
				suffixNum = std::max(suffixNum, 2u);
				continue;
			}

			// 后缀至少 4 个字符
			if (fileName.size() < screenshotFileName.size() + 4) {
				continue;
			}

			fileName.remove_prefix(screenshotFileName.size());

			if (!fileName.starts_with(L" (") || !fileName.ends_with(L')')) {
				continue;
			}

			uint32_t curSuffixNum;
			std::string curSuffixNumStr = StrHelper::UTF16ToUTF8(
				std::wstring_view(fileName.data() + 2, fileName.size() - 3));
			const char* end = curSuffixNumStr.data() + curSuffixNumStr.size();

			std::from_chars_result result = std::from_chars(
				curSuffixNumStr.data(), end, curSuffixNum);
			if (result.ec != std::errc{} || result.ptr != end || curSuffixNum < 2) {
				continue;
			}

			suffixNum = std::max(suffixNum, curSuffixNum + 1);
		} while (FindNextFile(hFind.get(), &findData));
	}

	if (suffixNum > 1) {
		screenshotFileName += L" (";
		screenshotFileName += StrHelper::ToWString(suffixNum);
		screenshotFileName += L')';
	}
}

enum class TakeScreenshotResult {
	Success,
	InvalidDirectory,
	InvalidFilenameTemplate,
	InternalError
};

static TakeScreenshotResult ExportScreenshot(
	HWND hwndSrc,
	std::wstring_view imgFormat,
	SizeU textureSize,
	EffectTextureFormat textureFormat,
	const std::span<uint8_t>& pixelData,
	uint32_t rowPitch,
	const ColorInfo& colorInfo,
	std::wstring& screenshotFileName
) noexcept {
	// 确保截图保存目录存在
	const ScalingOptions& options = ScalingWindow::Get().Options();

	if (options.screenshotsDir.empty()) {
		return TakeScreenshotResult::InvalidDirectory;
	}

	if (!Win32Helper::CreateDir(options.screenshotsDir.c_str(), true)) {
		Logger::Get().Error("CreateDir 失败");
		return TakeScreenshotResult::InvalidDirectory;
	}

	// 解析文件名模板
	{
		std::string fileNameUtf8;
		if (!ScreenshotFilenameTemplateHelper::Apply(
			options.screenshotFilenameTemplate, hwndSrc, fileNameUtf8)) {
			Logger::Get().Error("ScreenshotFilenameTemplateHelper::Apply 失败");
			return TakeScreenshotResult::InvalidFilenameTemplate;
		}

		screenshotFileName = StrHelper::UTF8ToUTF16(fileNameUtf8);
	}

	std::filesystem::path screenshotPath;

	// 加锁并提前创建空文件以保证后缀的唯一性
	static wil::srwlock lock;
	{
		auto lk = lock.lock_exclusive();

		AppendSuffixToScreenshotFilename(screenshotFileName);

		screenshotFileName += L'.';
		screenshotFileName += imgFormat;

		screenshotPath = options.screenshotsDir / screenshotFileName;

		CREATEFILE2_EXTENDED_PARAMETERS extendedParams = {
			.dwSize = sizeof(CREATEFILE2_EXTENDED_PARAMETERS),
			.dwFileAttributes = FILE_ATTRIBUTE_NORMAL
		};

		wil::unique_hfile hFile(CreateFile2(
			screenshotPath.c_str(), GENERIC_WRITE, 0, CREATE_ALWAYS, &extendedParams));
		if (!hFile) {
			Logger::Get().Win32Error("CreateFile2 失败");
			return TakeScreenshotResult::InternalError;
		}
	}

	HRESULT hr = TextureHelper::Save(screenshotPath.c_str(), textureSize,
		textureFormat, pixelData, rowPitch, colorInfo);
	if (FAILED(hr)) {
		Logger::Get().ComError("TextureHelper::SaveTexture 失败", hr);
		return TakeScreenshotResult::InternalError;
	}

	return TakeScreenshotResult::Success;
}

void ScreenshotDrawer::OnProducerFrameCompleted(uint64_t fenceValue) noexcept {
	for (int i = (int)_screenshotDatas.size() - 1; i >= 0; --i) {
		_ScreenshotData& curData = _screenshotDatas[i];

		if (curData.fenceValue > fenceValue) {
			continue;
		}

		[&curData, this](winrt::DispatcherQueue dispatcher) -> winrt::fire_and_forget {
			++_pendingScreenshotCount;

			// 确保即使协程出了问题也能恢复 _pendingScreenshotCount
			auto se = wil::scope_exit([&] {
				--_pendingScreenshotCount;
			});

			TakeScreenshotResult result;
			std::wstring screenshotFileName;
			if (curData.readbackBufferData) {
				// 导出时要求 pixelData 的最后一行有填充位
				std::vector<uint8_t> pixelData(size_t(curData.rowPitch) * curData.textureSize.height);
				std::memcpy(pixelData.data(), curData.readbackBufferData, curData.bufferSize);

				HWND hwndSrc = ScalingWindow::Get().SrcHandle();
				// 中间纹理导出为 dds，否则导出为 png
				std::wstring_view imgFormat = curData.tempTextureAllocation ? L"png" : L"dds";
				SizeU textureSize = curData.textureSize;
				EffectTextureFormat textureFormat = curData.textureFormat;
				uint32_t rowPitch = curData.rowPitch;
				ColorInfo colorInfo = curData.colorInfo;

				// 转到后台导出。从现在开始不要访问 curData
				co_await winrt::resume_background();

				result = ExportScreenshot(hwndSrc, imgFormat, textureSize,
					textureFormat, pixelData, rowPitch, colorInfo, screenshotFileName);
				if (result != TakeScreenshotResult::Success) {
					Logger::Get().Error("ExportScreenshot 失败");
				}

				co_await dispatcher;
			} else {
				// readbackBufferData 为空表示截图失败
				result = TakeScreenshotResult::InternalError;
			}

			LocalizationService& ls = LocalizationService::Get();

			if (result == TakeScreenshotResult::Success) {
				winrt::hstring successMsg = ls.GetLocalizedString(L"Message_ScreenshotSaved");
				ScalingWindow::Get().ShowToast(
					fmt::format(fmt::runtime(std::wstring_view(successMsg)), screenshotFileName));
			} else {
				const wchar_t* errorMsgs[] = {
					L"Message_ScreenshotFailed_InvalidDirectory",
					L"Message_ScreenshotFailed_InvalidFilenameTemplate",
					L"Message_ScreenshotFailed_InternalError"
				};
				ScalingWindow::Get().ShowToast(
					ls.GetLocalizedString(L"Message_ScreenshotFailed_Title"),
					ls.GetLocalizedString(errorMsgs[(size_t)result - 1])
				);
			}
		}(_dispatcher);

		// 释放当前 _ScreenshotData
		if (curData.descriptorBaseOffset != std::numeric_limits<uint32_t>::max()) {
			auto& descriptorHeap = _d3d12Context->GetDescriptorHeap();
			descriptorHeap.Free(curData.descriptorBaseOffset, curData.hasSRV ? 2 : 1);
		}

		_screenshotDatas.erase(_screenshotDatas.begin() + i);
	}
}

HRESULT ScreenshotDrawer::_InitializeScreenshotRootSignature() noexcept {
	winrt::com_ptr<ID3DBlob> signature;

	CD3DX12_DESCRIPTOR_RANGE1 srvRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0,
		D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);
	CD3DX12_DESCRIPTOR_RANGE1 uavRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0,
		D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);

	D3D12_ROOT_PARAMETER1 rootParams[] = {
		{
			.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
			.DescriptorTable = {
				.NumDescriptorRanges = 1,
				.pDescriptorRanges = &srvRange
			}
		},
		{
			.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
			.DescriptorTable = {
				.NumDescriptorRanges = 1,
				.pDescriptorRanges = &uavRange
			}
		}
	};

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc(
		(UINT)std::size(rootParams), rootParams, 0, nullptr);

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

	hr = _d3d12Context->GetDevice()->CreateRootSignature(
		0,
		signature->GetBufferPointer(),
		signature->GetBufferSize(),
		IID_PPV_ARGS(&_screenshotRootSignature)
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateRootSignature 失败", hr);
		return hr;
	}

	return S_OK;
}

HRESULT ScreenshotDrawer::_DrawImpl(
	ComputeContext& computeContext,
	uint32_t textureSrvOffset,
	ID3D12Resource* texture,
	EffectTextureFormat textureFormat,
	SizeU textureSize,
	uint64_t frameFenceValue
) noexcept {
	ID3D12Device5* device = _d3d12Context->GetDevice();
	auto& gpuResourceManager = _d3d12Context->GetGpuResourceManager();
	auto& descriptorHeap = _d3d12Context->GetDescriptorHeap();

	_ScreenshotData& curData = _screenshotDatas.emplace_back();

	curData.fenceValue = frameFenceValue;
	curData.textureSize = textureSize;
	curData.textureFormat = EffectTextureFormat::R16G16B16A16_UNORM;
	curData.colorInfo = _colorInfo;
	curData.hasSRV = textureSrvOffset == std::numeric_limits<uint32_t>::max();

	const EffectTextureFormatProps& originTextureFormatProps =
		EFFECT_TEXTURE_FORMAT_PROPS[(uint32_t)textureFormat];
	const EffectTextureFormatProps& screenshotTextureFormatProps =
		EFFECT_TEXTURE_FORMAT_PROPS[(uint32_t)curData.textureFormat];

	uint32_t rowSize;
	DirectXHelper::ComputeBufferLayout(textureSize,
		screenshotTextureFormatProps.texelSize, rowSize, curData.rowPitch, curData.bufferSize);

	HRESULT hr = gpuResourceManager.CreateBuffer(
		D3D12_HEAP_TYPE_READBACK,
		curData.bufferSize,
		D3D12_RESOURCE_STATE_COPY_DEST,
		curData.readbackBufferAllocation.put()
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("GpuResourceManager::CreateBuffer 失败", hr);
		return hr;
	}

	// CPU 需读取整个缓冲区
	hr = curData.readbackBufferAllocation->GetResource()->Map(0, nullptr, &curData.readbackBufferData);
	if (FAILED(hr)) {
		Logger::Get().ComError("ID3D12Resource::Map 失败", hr);
		return hr;
	}

	// 把要导出的纹理转换为用于导出截图的格式。
	// SDR: 需做伽马校正
	// WCG: 需转换为 BT.2020 SDR
	// HDR: 需转换为 BT.2020/PQ

	hr = gpuResourceManager.CreateTexture(
		D3D12_HEAP_TYPE_DEFAULT,
		screenshotTextureFormatProps.dxgiFormat,
		textureSize,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		curData.tempTextureAllocation.put()
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("GpuResourceManager::CreateBuffer 失败", hr);
		return hr;
	}

	hr = descriptorHeap.Alloc(curData.hasSRV ? 2 : 1, curData.descriptorBaseOffset);
	if (FAILED(hr)) {
		Logger::Get().ComError("DescriptorHeap::Alloc 失败", hr);
		return hr;
	}

	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(descriptorHeap.GetCpuHandle(curData.descriptorBaseOffset));

		if (curData.hasSRV) {
			CD3DX12_SHADER_RESOURCE_VIEW_DESC srvDesc =
				CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2D(originTextureFormatProps.dxgiFormat, 1);

			device->CreateShaderResourceView(texture, &srvDesc, cpuHandle);
			cpuHandle.Offset(descriptorHeap.GetDescriptorSize());
		}

		CD3DX12_UNORDERED_ACCESS_VIEW_DESC uavDesc =
			CD3DX12_UNORDERED_ACCESS_VIEW_DESC::Tex2D(screenshotTextureFormatProps.dxgiFormat);
		device->CreateUnorderedAccessView(
			curData.tempTextureAllocation->GetResource(), nullptr, &uavDesc, cpuHandle);
	}

	if (!_screenshotRootSignature) {
		hr = _InitializeScreenshotRootSignature();
		if (FAILED(hr)) {
			Logger::Get().ComError("_InitializeScreenshotRootSignature 失败", hr);
			return hr;
		}
	}
	computeContext.SetRootSignature(_screenshotRootSignature.get());

	if (!_screenshotPSO) {
		D3D12_SHADER_BYTECODE psByteCode;
		bool isSM6Supported = _d3d12Context->GetShaderModel() >= D3D_SHADER_MODEL_6_0;
		if (curData.colorInfo.kind == winrt::AdvancedColorKind::StandardDynamicRange) {
			psByteCode = DirectXHelper::SelectShader(
				isSM6Supported, ScreenshotCS, ScreenshotCS_SM5);
		} else if (curData.colorInfo.kind == winrt::AdvancedColorKind::WideColorGamut) {
			psByteCode = DirectXHelper::SelectShader(
				isSM6Supported, ScreenshotCS_WCG, ScreenshotCS_WCG_SM5);
		} else {
			psByteCode = DirectXHelper::SelectShader(
				isSM6Supported, ScreenshotCS_HDR, ScreenshotCS_HDR_SM5);
		}

		D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {
			.pRootSignature = _screenshotRootSignature.get(),
			.CS = psByteCode
		};
		hr = _d3d12Context->GetDevice()->CreateComputePipelineState(
			&psoDesc, IID_PPV_ARGS(&_screenshotPSO));
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateComputePipelineState 失败", hr);
			return hr;
		}
	}

	computeContext.SetPipelineState(_screenshotPSO.get());

	if (curData.hasSRV) {
		computeContext.SetRootDescriptorTable(0, curData.descriptorBaseOffset);
		computeContext.SetRootDescriptorTable(1, curData.descriptorBaseOffset + 1);
	} else {
		computeContext.SetRootDescriptorTable(0, textureSrvOffset);
		computeContext.SetRootDescriptorTable(1, curData.descriptorBaseOffset);
	}

	constexpr uint32_t BLOCK_SIZE = 16;
	computeContext.Dispatch(
		(textureSize.width + BLOCK_SIZE - 1) / BLOCK_SIZE,
		(textureSize.height + BLOCK_SIZE - 1) / BLOCK_SIZE
	);

	computeContext.InsertTransitionBarrier(
		curData.tempTextureAllocation->GetResource(),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_COPY_SOURCE
	);

	computeContext.CopyTextureRegion(
		CD3DX12_TEXTURE_COPY_LOCATION(
			curData.readbackBufferAllocation->GetResource(),
			D3D12_PLACED_SUBRESOURCE_FOOTPRINT{
				.Footprint = {
					.Format = screenshotTextureFormatProps.dxgiFormat,
					.Width = curData.textureSize.width,
					.Height = curData.textureSize.height,
					.Depth = 1,
					.RowPitch = curData.rowPitch
				}
			}
		),
		0,
		0,
		CD3DX12_TEXTURE_COPY_LOCATION(curData.tempTextureAllocation->GetResource())
	);

	return S_OK;
}

}
