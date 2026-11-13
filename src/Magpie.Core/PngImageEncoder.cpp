#include "pch.h"
#include "PngImageEncoder.h"
#include "Logger.h"
#include "MemStream.h"
#include "resource.h"
#include "Win32Helper.h"
#include <wincodec.h>

namespace Magpie {

ImageEncoderMetadata PngImageEncoder::GetMetadata() noexcept {
	return ImageEncoderMetadata{
		.displayName = "PNG",
		.fileExtension = "png"
	};
}

static HRESULT EmbedBT2020ICC(IWICImagingFactory2* wicFactory, IWICBitmapFrameEncode* frameEncoder) noexcept {
	winrt::com_ptr<IWICColorContext> colorContext;
	HRESULT hr = wicFactory->CreateColorContext(colorContext.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("IWICImagingFactory::CreateColorContext 失败", hr);
		return hr;
	}

	HRSRC hRes = FindResource(NULL, MAKEINTRESOURCEW(IDR_BT2020_ICC), RT_RCDATA);
	if (!hRes) {
		hr = HRESULT_FROM_WIN32(GetLastError());
		Logger::Get().Win32Error("FindResource 失败");
		return hr;
	}

	HGLOBAL hResLoad = LoadResource(NULL, hRes);
	if (!hResLoad) {
		hr = HRESULT_FROM_WIN32(GetLastError());
		Logger::Get().Win32Error("LoadResource 失败");
		return hr;
	}

	hr = colorContext->InitializeFromMemory(
		(const BYTE*)LockResource(hResLoad), SizeofResource(nullptr, hRes));
	if (FAILED(hr)) {
		Logger::Get().ComError("IWICColorContext::InitializeFromMemory 失败", hr);
		return hr;
	}

	IWICColorContext* t = colorContext.get();
	hr = frameEncoder->SetColorContexts(1, &t);
	if (FAILED(hr)) {
		Logger::Get().ComError("IWICBitmapFrameEncode::SetColorContexts 失败", hr);
		return hr;
	}

	return S_OK;
}

static uint32_t MakeUI4(uint8_t v1, uint8_t v2, uint8_t v3, uint8_t v4) noexcept {
	return v1 | (v2 << 8) | (v3 << 16) | (v4 << 24);
}

// PNG 的整数使用 network byte order，高位在前
static uint16_t MakePngUI2(uint16_t value) noexcept {
	return ((value >> 8) & 0xFFu) | ((value & 0xFFu) << 8);
}

static uint32_t MakePngUI4(uint32_t value) noexcept {
	return MakeUI4(uint8_t(value >> 24), uint8_t(value >> 16), uint8_t(value >> 8), uint8_t(value));
}

// 参考自 https://www.w3.org/TR/png-3/#samplecrc。由于要校验的数据很少，
// 无需构建查找表。
static uint32_t CalculateCRC32(const uint8_t* data, uint32_t len) noexcept {
	uint32_t c = 0xFFFFFFFFu;

	for (uint32_t i = 0; i < len; ++i) {
		c ^= data[i];

		for (int j = 0; j < 8; ++j) {
			if (c & 1) {
				c = 0xEDB88320u ^ (c >> 1);
			} else {
				c >>= 1;
			}
		}
	}

	return MakePngUI4(c ^ 0xFFFFFFFFu);
}

// PNG 标准: https://www.w3.org/TR/png-3
static std::array<uint8_t, 16> MakeCICPChunk(winrt::AdvancedColorKind acKind) noexcept {
	std::array<uint8_t, 16> chunk;

	// 数据长度
	*(uint32_t*)&chunk[0] = MakeUI4(0, 0, 0, uint8_t(chunk.size() - 12u));

	// Chunk 类型
	*(uint32_t*)&chunk[4] = MakeUI4('c', 'I', 'C', 'P');

	// Chunk 内容
	*(uint32_t*)&chunk[8] = MakeUI4(
		// Color Primaries: 9 表示 BT.2020
		9,
		// Transfer Function: 15 表示 BT.2020 SDR，16 表示 BT.2020/PQ
		acKind == winrt::AdvancedColorKind::WideColorGamut ? 15 : 16,
		// Matrix Coefficients: 必须为 0
		0,
		// Video Full Range Flag: 1 表示 Full Range
		1
	);

	// CRC32 校验
	*(uint32_t*)&chunk[12] = CalculateCRC32(&chunk[4], (uint32_t)chunk.size() - 8u);

	return chunk;
}

static uint16_t EncodeChroma(float value) noexcept {
	constexpr float CHROMA_DIVISOR = 0.00002f;
	return MakePngUI2((uint16_t)std::lround(value / CHROMA_DIVISOR));
}

static uint32_t EncodeLuminace(float value) noexcept {
	constexpr float LUM_DIVISOR = 0.0001f;
	return MakePngUI4((uint32_t)std::lround(value * SCENE_REFERRED_SDR_WHITE_LEVEL / LUM_DIVISOR));
}

HRESULT PngImageEncoder::Encode(
	const phmap::flat_hash_map<std::string, OptionValue>& /*optionValues*/,
	wil::zstring_view fileName,
	SizeU imageSize,
	std::span<const uint8_t> pixelData,
	uint32_t rowPitch,
	winrt::AdvancedColorKind colorKind
) noexcept {
	std::wstring fileNameUtf16 = StrHelper::UTF8ToUTF16(fileName);

	// 初始化 WIC
	winrt::com_ptr<IWICImagingFactory2> wicFactory =
		winrt::try_create_instance<IWICImagingFactory2>(CLSID_WICImagingFactory);
	if (!wicFactory) {
		Logger::Get().Error("创建 WICImagingFactory2 失败");
		return E_FAIL;
	}

	winrt::com_ptr<IWICBitmapEncoder> bmpEncoder;
	HRESULT hr = wicFactory->CreateEncoder(GUID_ContainerFormatPng, nullptr, bmpEncoder.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("IWICImagingFactory::CreateEncoder 失败", hr);
		return hr;
	}

	// 用于在 WCG 和 HDR 下嵌入 cICP
	winrt::com_ptr<MemStream> memStream;
	{
		winrt::com_ptr<IWICStream> wicStream;
		hr = wicFactory->CreateStream(wicStream.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("IWICImagingFactory::CreateStream 失败", hr);
			return hr;
		}

		if (colorKind == winrt::AdvancedColorKind::StandardDynamicRange) {
			hr = wicStream->InitializeFromFilename(fileNameUtf16.c_str(), GENERIC_WRITE);
			if (FAILED(hr)) {
				Logger::Get().ComError("IWICStream::InitializeFromFilename 失败", hr);
				return hr;
			}
		} else {
			// 导出的 PNG 文件一般是几 MB 到 十几 MB，初始容量设置为 1MB
			memStream = winrt::make_self<MemStream>(1 << 20);

			hr = wicStream->InitializeFromIStream(memStream.get());
			if (FAILED(hr)) {
				Logger::Get().ComError("IWICStream::InitializeFromFilename 失败", hr);
				return hr;
			}
		}

		hr = bmpEncoder->Initialize(wicStream.get(), WICBitmapEncoderNoCache);
		if (FAILED(hr)) {
			Logger::Get().ComError("IWICBitmapEncoder::Initialize 失败", hr);
			return hr;
		}
	}

	winrt::com_ptr<IWICBitmapFrameEncode> frameEncoder;
	hr = bmpEncoder->CreateNewFrame(frameEncoder.put(), nullptr);
	if (FAILED(hr)) {
		Logger::Get().ComError("IWICBitmapEncoder::CreateNewFrame 失败", hr);
		return hr;
	}

	hr = frameEncoder->Initialize(nullptr);
	if (FAILED(hr)) {
		Logger::Get().ComError("IWICBitmapFrameEncode::Initialize 失败", hr);
		return hr;
	}

	hr = frameEncoder->SetSize(imageSize.width, imageSize.height);
	if (FAILED(hr)) {
		Logger::Get().ComError("IWICBitmapFrameEncode::SetSize 失败", hr);
		return hr;
	}

	WICPixelFormatGUID destFormat = GUID_WICPixelFormat48bppRGB;
	hr = frameEncoder->SetPixelFormat(&destFormat);
	if (FAILED(hr)) {
		Logger::Get().ComError("IWICBitmapFrameEncode::SetPixelFormat 失败", hr);
		return hr;
	}
	assert(destFormat == GUID_WICPixelFormat48bppRGB);

	if (colorKind == winrt::AdvancedColorKind::WideColorGamut) {
		// 出于兼容性考虑为 WCG 截图嵌入 icc，这样不支持 cICP 的软件也能正确渲染。但 HDR 截图
		// 不嵌入，因为这不是推荐的做法，而且支持的软件很少，见 https://www.w3.org/TR/png-hdr-pq
		hr = EmbedBT2020ICC(wicFactory.get(), frameEncoder.get());
		if (FAILED(hr)) {
			Logger::Get().ComWarn("EmbedBT2020ICC 失败", hr);
		}
	}

	hr = frameEncoder->WritePixels(
		imageSize.height,
		rowPitch,
		(UINT)pixelData.size(),
		(BYTE*)pixelData.data()
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("IWICBitmapFrameEncode::WritePixels 失败", hr);
		return hr;
	}

	hr = frameEncoder->Commit();
	if (FAILED(hr)) {
		Logger::Get().ComError("IWICBitmapFrameEncode::Commit 失败", hr);
		return hr;
	}

	hr = bmpEncoder->Commit();
	if (FAILED(hr)) {
		Logger::Get().ComError("IWICBitmapEncoder::Commit 失败", hr);
		return hr;
	}

	// 在 WCG 和 HDR 截图中嵌入 cICP
	if (memStream) {
		assert(colorKind != winrt::AdvancedColorKind::StandardDynamicRange);

		const uint8_t* pMem = memStream->GetData();
		const uint32_t memSize = memStream->GetSize();

		// 在 IHDR 后增加数据块。前有 8 字节 PNG 签名和 25 字节 IHDR 数据块 
		constexpr uint32_t CICP_CHUNK_OFFSET = 33;

		if (memSize <= CICP_CHUNK_OFFSET) {
			return E_FAIL;
		}

		CREATEFILE2_EXTENDED_PARAMETERS extendedParams = {
			.dwSize = sizeof(CREATEFILE2_EXTENDED_PARAMETERS),
			.dwFileAttributes = FILE_ATTRIBUTE_NORMAL
		};
		wil::unique_hfile hFile(CreateFile2(
			fileNameUtf16.c_str(), GENERIC_WRITE, 0, CREATE_ALWAYS, &extendedParams));
		if (!hFile) {
			hr = HRESULT_FROM_WIN32(GetLastError());
			Logger::Get().Win32Error("CreateFile2 失败");
			return hr;
		}

		if (!Win32Helper::WriteFile(hFile.get(), std::span(pMem, CICP_CHUNK_OFFSET))) {
			Logger::Get().Error("Win32Helper::WriteFile 失败");
			return E_FAIL;
		}

		// 嵌入 cICP 数据块。WIC 可能添加了 sRGB 和 gAMA 块，cICP 的优先级高于它们
		if (!Win32Helper::WriteFile(hFile.get(), MakeCICPChunk(colorKind))) {
			Logger::Get().Error("Win32Helper::WriteFile 失败");
			return E_FAIL;
		}

		if (!Win32Helper::WriteFile(hFile.get(),
			std::span(pMem + CICP_CHUNK_OFFSET, memSize - CICP_CHUNK_OFFSET))) {
			Logger::Get().Error("Win32Helper::WriteFile 失败");
			return E_FAIL;
		}
	}

	return S_OK;
}

}
