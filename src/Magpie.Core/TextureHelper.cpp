#include "pch.h"
#include "D3D12Context.h"
#include "DDS.h"
#include "Logger.h"
#include "TextureHelper.h"
#include "WICImageLoader.h"
#include "DirectXHelper.h"
#include "resource.h"
#include "MemStream.h"
#include "Win32Helper.h"

///////////////////////////////////////////////////////////////////////////////
// 
// 读取和保存 DDS 文件的代码来自 https://github.com/microsoft/DirectXTex
// 
///////////////////////////////////////////////////////////////////////////////

namespace Magpie {

static HRESULT LoadTextureDataFromFile(
	_In_z_ const wchar_t* fileName,
	std::unique_ptr<uint8_t[]>& ddsData,
	const DDS_HEADER** header,
	const uint8_t** bitData,
	size_t* bitSize
) noexcept {
	if (!header || !bitData || !bitSize) {
		return E_POINTER;
	}

	*bitSize = 0;

	// open the file
	wil::unique_hfile hFile(CreateFile2(
		fileName,
		GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING,
		nullptr
	));

	if (!hFile) {
		return HRESULT_FROM_WIN32(GetLastError());
	}

	// Get the file size
	FILE_STANDARD_INFO fileInfo{};
	if (!GetFileInformationByHandleEx(hFile.get(), FileStandardInfo, &fileInfo, sizeof(fileInfo))) {
		return HRESULT_FROM_WIN32(GetLastError());
	}

	// File is too big for 32-bit allocation, so reject read
	if (fileInfo.EndOfFile.HighPart > 0) {
		return E_FAIL;
	}

	// Need at least enough data to fill the header and magic number to be a valid DDS
	if (fileInfo.EndOfFile.LowPart < DDS_MIN_HEADER_SIZE) {
		return E_FAIL;
	}

	// create enough space for the file data
	ddsData.reset(new (std::nothrow) uint8_t[fileInfo.EndOfFile.LowPart]);
	if (!ddsData) {
		return E_OUTOFMEMORY;
	}

	// read the data in
	DWORD bytesRead = 0;
	if (!ReadFile(hFile.get(),
		ddsData.get(),
		fileInfo.EndOfFile.LowPart,
		&bytesRead,
		nullptr
	)) {
		ddsData.reset();
		return HRESULT_FROM_WIN32(GetLastError());
	}

	if (bytesRead < fileInfo.EndOfFile.LowPart) {
		ddsData.reset();
		return E_FAIL;
	}

	size_t len = fileInfo.EndOfFile.LowPart;

	// DDS files always start with the same magic number ("DDS ")
	const auto dwMagicNumber = *reinterpret_cast<const uint32_t*>(ddsData.get());
	if (dwMagicNumber != DDS_MAGIC) {
		ddsData.reset();
		return E_FAIL;
	}

	auto hdr = reinterpret_cast<const DDS_HEADER*>(ddsData.get() + sizeof(uint32_t));

	// Verify header to validate DDS file
	if (hdr->size != sizeof(DDS_HEADER) ||
		hdr->ddspf.size != sizeof(DDS_PIXELFORMAT)) {
		ddsData.reset();
		return E_FAIL;
	}

	// Check for DX10 extension
	bool bDXT10Header = false;
	if ((hdr->ddspf.flags & DDS_FOURCC) &&
		(MAKEFOURCC('D', 'X', '1', '0') == hdr->ddspf.fourCC)) {
		// Must be long enough for both headers and magic value
		if (len < DDS_DX10_HEADER_SIZE) {
			ddsData.reset();
			return E_FAIL;
		}

		bDXT10Header = true;
	}

	// setup the pointers in the process request
	*header = hdr;
	auto offset = DDS_MIN_HEADER_SIZE
		+ (bDXT10Header ? sizeof(DDS_HEADER_DXT10) : 0u);
	*bitData = ddsData.get() + offset;
	*bitSize = len - offset;

	return S_OK;
}

static uint32_t BytesPerPixel(_In_ DXGI_FORMAT fmt) noexcept {
	switch (fmt) {
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
		return 16;

	case DXGI_FORMAT_R16G16B16A16_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R16G16B16A16_SNORM:
	case DXGI_FORMAT_R32G32_FLOAT:
		return 8;

	case DXGI_FORMAT_R10G10B10A2_UNORM:
	case DXGI_FORMAT_R11G11B10_FLOAT:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_SNORM:
	case DXGI_FORMAT_R16G16_FLOAT:
	case DXGI_FORMAT_R16G16_UNORM:
	case DXGI_FORMAT_R16G16_SNORM:
	case DXGI_FORMAT_R32_FLOAT:
		return 4;

	case DXGI_FORMAT_R8G8_UNORM:
	case DXGI_FORMAT_R8G8_SNORM:
	case DXGI_FORMAT_R16_FLOAT:
	case DXGI_FORMAT_R16_UNORM:
	case DXGI_FORMAT_R16_SNORM:
		return 2;
		
	default:
		assert(fmt == DXGI_FORMAT_R8_UNORM || fmt == DXGI_FORMAT_R8_SNORM);
		return 1;
	}
}

#define ISBITMASK( r,g,b,a ) ( ddpf.RBitMask == r && ddpf.GBitMask == g && ddpf.BBitMask == b && ddpf.ABitMask == a )

static DXGI_FORMAT GetDXGIFormat(const DDS_PIXELFORMAT& ddpf) noexcept {
	if (ddpf.flags & DDS_RGB) {
		// Note that sRGB formats are written using the "DX10" extended header

		switch (ddpf.RGBBitCount) {
		case 32:
			if (ISBITMASK(0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000)) {
				return DXGI_FORMAT_R8G8B8A8_UNORM;
			}

			// No DXGI format maps to ISBITMASK(0x000000ff,0x0000ff00,0x00ff0000,0) aka D3DFMT_X8B8G8R8

			// Note that many common DDS reader/writers (including D3DX) swap the
			// the RED/BLUE masks for 10:10:10:2 formats. We assume
			// below that the 'backwards' header mask is being used since it is most
			// likely written by D3DX. The more robust solution is to use the 'DX10'
			// header extension and specify the DXGI_FORMAT_R10G10B10A2_UNORM format directly

			// For 'correct' writers, this should be 0x000003ff,0x000ffc00,0x3ff00000 for RGB data
			if (ISBITMASK(0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000)) {
				return DXGI_FORMAT_R10G10B10A2_UNORM;
			}

			// No DXGI format maps to ISBITMASK(0x000003ff,0x000ffc00,0x3ff00000,0xc0000000) aka D3DFMT_A2R10G10B10

			if (ISBITMASK(0x0000ffff, 0xffff0000, 0, 0)) {
				return DXGI_FORMAT_R16G16_UNORM;
			}

			if (ISBITMASK(0xffffffff, 0, 0, 0)) {
				// Only 32-bit color channel format in D3D9 was R32F
				return DXGI_FORMAT_R32_FLOAT; // D3DX writes this out as a FourCC of 114
			}
			break;

		case 24:
			// No 24bpp DXGI formats aka D3DFMT_R8G8B8
			break;

		case 16:
			// No DXGI format maps to ISBITMASK(0x7c00,0x03e0,0x001f,0) aka D3DFMT_X1R5G5B5

			// NVTT versions 1.x wrote this as RGB instead of LUMINANCE
			if (ISBITMASK(0x00ff, 0, 0, 0xff00)) {
				return DXGI_FORMAT_R8G8_UNORM;
			}
			if (ISBITMASK(0xffff, 0, 0, 0)) {
				return DXGI_FORMAT_R16_UNORM;
			}

			// No DXGI format maps to ISBITMASK(0x0f00,0x00f0,0x000f,0) aka D3DFMT_X4R4G4B4

			// No 3:3:2:8 or paletted DXGI formats aka D3DFMT_A8R3G3B2, D3DFMT_A8P8, etc.
			break;

		case 8:
			// NVTT versions 1.x wrote this as RGB instead of LUMINANCE
			if (ISBITMASK(0xff, 0, 0, 0)) {
				return DXGI_FORMAT_R8_UNORM;
			}

			// No 3:3:2 or paletted DXGI formats aka D3DFMT_R3G3B2, D3DFMT_P8
			break;

		default:
			return DXGI_FORMAT_UNKNOWN;
		}
	} else if (ddpf.flags & DDS_LUMINANCE) {
		switch (ddpf.RGBBitCount) {
		case 16:
			if (ISBITMASK(0xffff, 0, 0, 0)) {
				return DXGI_FORMAT_R16_UNORM; // D3DX10/11 writes this out as DX10 extension
			}
			if (ISBITMASK(0x00ff, 0, 0, 0xff00)) {
				return DXGI_FORMAT_R8G8_UNORM; // D3DX10/11 writes this out as DX10 extension
			}
			break;

		case 8:
			if (ISBITMASK(0xff, 0, 0, 0)) {
				return DXGI_FORMAT_R8_UNORM; // D3DX10/11 writes this out as DX10 extension
			}

			// No DXGI format maps to ISBITMASK(0x0f,0,0,0xf0) aka D3DFMT_A4L4

			if (ISBITMASK(0x00ff, 0, 0, 0xff00)) {
				return DXGI_FORMAT_R8G8_UNORM; // Some DDS writers assume the bitcount should be 8 instead of 16
			}
			break;

		default:
			return DXGI_FORMAT_UNKNOWN;
		}
	} else if (ddpf.flags & DDS_BUMPDUDV) {
		switch (ddpf.RGBBitCount) {
		case 32:
			if (ISBITMASK(0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000)) {
				return DXGI_FORMAT_R8G8B8A8_SNORM; // D3DX10/11 writes this out as DX10 extension
			}
			if (ISBITMASK(0x0000ffff, 0xffff0000, 0, 0)) {
				return DXGI_FORMAT_R16G16_SNORM; // D3DX10/11 writes this out as DX10 extension
			}

			// No DXGI format maps to ISBITMASK(0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000) aka D3DFMT_A2W10V10U10
			break;

		case 16:
			if (ISBITMASK(0x00ff, 0xff00, 0, 0)) {
				return DXGI_FORMAT_R8G8_SNORM; // D3DX10/11 writes this out as DX10 extension
			}
			break;

		default:
			return DXGI_FORMAT_UNKNOWN;
		}

		// No DXGI format maps to DDPF_BUMPLUMINANCE aka D3DFMT_L6V5U5, D3DFMT_X8L8V8U8
	} else if (ddpf.flags & DDS_FOURCC) {
		// Check for D3DFORMAT enums being set here
		switch (ddpf.fourCC) {
		case 36: // D3DFMT_A16B16G16R16
			return DXGI_FORMAT_R16G16B16A16_UNORM;

		case 110: // D3DFMT_Q16W16V16U16
			return DXGI_FORMAT_R16G16B16A16_SNORM;

		case 111: // D3DFMT_R16F
			return DXGI_FORMAT_R16_FLOAT;

		case 112: // D3DFMT_G16R16F
			return DXGI_FORMAT_R16G16_FLOAT;

		case 113: // D3DFMT_A16B16G16R16F
			return DXGI_FORMAT_R16G16B16A16_FLOAT;

		case 114: // D3DFMT_R32F
			return DXGI_FORMAT_R32_FLOAT;

		case 115: // D3DFMT_G32R32F
			return DXGI_FORMAT_R32G32_FLOAT;

		case 116: // D3DFMT_A32B32G32R32F
			return DXGI_FORMAT_R32G32B32A32_FLOAT;

			// No DXGI format maps to D3DFMT_CxV8U8

		default:
			return DXGI_FORMAT_UNKNOWN;
		}
	}

	return DXGI_FORMAT_UNKNOWN;
}

#undef ISBITMASK

static bool CheckDDSHeader(DXGI_FORMAT format, _In_ const DDS_HEADER* header) noexcept {
	// Bound sizes (for security purposes we don't trust DDS file metadata larger than the Direct3D hardware requirements)
	if (header->mipMapCount > D3D12_REQ_MIP_LEVELS) {
		return false;
	}

	if ((header->ddspf.flags & DDS_FOURCC) &&
		(MAKEFOURCC('D', 'X', '1', '0') == header->ddspf.fourCC)) {
		auto d3d10ext = (const DDS_HEADER_DXT10*)((const char*)header + sizeof(DDS_HEADER));

		if (d3d10ext->arraySize != 1) {
			return false;
		}

		if (d3d10ext->dxgiFormat != format) {
			return false;
		}

		if (d3d10ext->resourceDimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D) {
			return false;
		}

		if (d3d10ext->miscFlag & DDS_RESOURCE_MISC_TEXTURECUBE) {
			return false;
		}
	} else {
		if (format != GetDXGIFormat(header->ddspf)) {
			return false;
		}

		if (header->flags & DDS_HEADER_FLAGS_VOLUME) {
			return false;
		} else if (header->caps2 & DDS_CUBEMAP) {
			return false;
		}
	}

	if (header->width > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION ||
		header->height > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION) {
		return false;
	}

	return true;
}

static winrt::com_ptr<ID3D12Resource> LoadTextureFromDDS(
	const wchar_t* fileName,
	DXGI_FORMAT format,
	const D3D12Context& d3d12Context,
	SizeU& textureSize
) noexcept {
	const DDS_HEADER* header = nullptr;
	const uint8_t* bitData = nullptr;
	size_t bitSize = 0;

	std::unique_ptr<uint8_t[]> ddsData;
	HRESULT hr = LoadTextureDataFromFile(
		fileName,
		ddsData,
		&header,
		&bitData,
		&bitSize
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("LoadTextureDataFromFile 失败", hr);
		return nullptr;
	}

	if (!CheckDDSHeader(format, header)) {
		Logger::Get().Error("CheckDDSHeader 失败");
		return nullptr;
	}

	textureSize.width = header->width;
	textureSize.height = header->height;

	uint32_t pixelByteCount = BytesPerPixel(format);
	uint32_t rowPitch = textureSize.width * pixelByteCount;
	size_t numBytes = (size_t)rowPitch * textureSize.height;

	if (numBytes > bitSize) {
		Logger::Get().Error("文件格式错误");
		return nullptr;
	}

	winrt::com_ptr<ID3D12Resource> result;

	ID3D12Device5* device = d3d12Context.GetDevice();

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);

	D3D12_HEAP_FLAGS heapFlags = d3d12Context.IsHeapFlagCreateNotZeroedSupported() ?
		D3D12_HEAP_FLAG_CREATE_NOT_ZEROED : D3D12_HEAP_FLAG_NONE;

	uint32_t textureRowPitch = DirectXHelper::Align(rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
	uint32_t bufferSize = textureRowPitch * textureSize.height;

	CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	hr = device->CreateCommittedResource(
		&heapProps,
		heapFlags,
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&result)
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateCommittedResource 失败", hr);
		return nullptr;
	}

	CD3DX12_RANGE emptyRange{};
	void* pData;
	hr = result->Map(0, &emptyRange, &pData);
	if (FAILED(hr)) {
		Logger::Get().ComError("ID3D12Resource::Map 失败", hr);
		return nullptr;
	}

	if (rowPitch == textureRowPitch) {
		std::memcpy(pData, bitData, numBytes);
	} else {
		for (uint32_t i = 0; i < textureSize.height; ++i) {
			std::memcpy(
				(uint8_t*)pData + textureRowPitch * i,
				bitData + rowPitch * i,
				rowPitch
			);
		}
	}

	result->Unmap(0, nullptr);

	return std::move(result);
}

static winrt::com_ptr<ID3D12Resource> LoadTextureFromImage(
	const wchar_t* fileName,
	const D3D12Context& d3d12Context,
	DXGI_FORMAT& format,
	SizeU& textureSize
) noexcept {
	WICPixelFormatGUID targetWicFormat;
	switch (format) {
	case DXGI_FORMAT_R8G8B8A8_UNORM:
		targetWicFormat = GUID_WICPixelFormat32bppRGBA;
		break;
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
		targetWicFormat = GUID_WICPixelFormat64bppRGBAHalf;
		break;
	default:
		assert(format == DXGI_FORMAT_R32G32B32A32_FLOAT);
		targetWicFormat = GUID_WICPixelFormat128bppRGBAFloat;
		break;
	}

	bool isSRGB = false;
	winrt::com_ptr<IWICBitmapSource> wicBitmap =
		WICImageLoader::LoadFromFile(fileName, targetWicFormat, isSRGB);
	if (!wicBitmap) {
		Logger::Get().Error("WICImageLoader::LoadFromFile 失败");
		return nullptr;
	}

	// 如果图片是 sRGB 则修改 format。对于其他格式 WIC 会自动执行伽马校正。
	if (isSRGB) {
		format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	}

	// 检查 D3D 纹理尺寸限制
	HRESULT hr = wicBitmap->GetSize(&textureSize.width, &textureSize.height);
	if (FAILED(hr)) {
		Logger::Get().ComError("GetSize 失败", hr);
		return nullptr;
	}

	if (textureSize.width > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION ||
		textureSize.height > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION)
	{
		Logger::Get().Error("图像尺寸超出限制");
		return nullptr;
	}

	winrt::com_ptr<ID3D12Resource> result;

	ID3D12Device5* device = d3d12Context.GetDevice();

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);

	D3D12_HEAP_FLAGS heapFlags = d3d12Context.IsHeapFlagCreateNotZeroedSupported() ?
		D3D12_HEAP_FLAG_CREATE_NOT_ZEROED : D3D12_HEAP_FLAG_NONE;

	CD3DX12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
		format, textureSize.width, textureSize.height, 1, 1);

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT textureLayout;
	UINT64 bufferSize;
	device->GetCopyableFootprints(&texDesc, 0, 1, 0,
		&textureLayout, nullptr, nullptr, &bufferSize);

	// GetCopyableFootprints 计算出的字节数可能不包含最后一行的填充位，而 WIC 需要
	bufferSize = std::max(bufferSize, UINT64(textureLayout.Footprint.RowPitch * textureSize.height));

	CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	hr = device->CreateCommittedResource(
		&heapProps,
		heapFlags,
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&result)
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateCommittedResource 失败", hr);
		return nullptr;
	}

	CD3DX12_RANGE emptyRange{};
	void* pData;
	hr = result->Map(0, &emptyRange, &pData);
	if (FAILED(hr)) {
		Logger::Get().ComError("ID3D12Resource::Map 失败", hr);
		return nullptr;
	}

	// 这个调用是否会意外读取 pData 的内容？保险起见可以使用临时缓冲区。
	hr = wicBitmap->CopyPixels(nullptr, textureLayout.Footprint.RowPitch, (UINT)bufferSize, (BYTE*)pData);
	if (FAILED(hr)) {
		Logger::Get().ComError("CopyPixels 失败", hr);
		return nullptr;
	}

	result->Unmap(0, nullptr);

	return std::move(result);
}

//-------------------------------------------------------------------------------------
// Computes the image row pitch in bytes, and the slice ptich (size in bytes of the image)
// based on DXGI format, width, and height
//-------------------------------------------------------------------------------------
static void ComputePitch(DXGI_FORMAT fmt, size_t width, size_t height, size_t& rowPitch, size_t& slicePitch) noexcept {
	// Default byte alignment
	rowPitch = width * BytesPerPixel(fmt);
	slicePitch = rowPitch * uint64_t(height);
}

//-------------------------------------------------------------------------------------
// Encodes DDS file header (magic value, header, DX10 extended header)
//-------------------------------------------------------------------------------------
static HRESULT EncodeDDSHeader(
	uint32_t width,
	uint32_t height,
	DXGI_FORMAT format,
	uint8_t* pDestination,
	uint32_t& ddsRowPitch,
	uint32_t& ddsSlicePitch
) noexcept {
	*reinterpret_cast<uint32_t*>(pDestination) = DDS_MAGIC;

	auto header = reinterpret_cast<DDS_HEADER*>(static_cast<uint8_t*>(pDestination) + sizeof(uint32_t));
	assert(header);

	memset(header, 0, sizeof(DDS_HEADER));
	header->size = sizeof(DDS_HEADER);
	header->flags = DDS_HEADER_FLAGS_TEXTURE | DDS_HEADER_FLAGS_PITCH;
	header->caps = DDS_SURFACE_FLAGS_TEXTURE;
	header->height = height;
	header->width = width;
	header->depth = 1;

	size_t rowPitch, slicePitch;
	ComputePitch(format, width, height, rowPitch, slicePitch);

	if (slicePitch > UINT32_MAX || rowPitch > UINT32_MAX) {
		Logger::Get().Error("slicePitch 或 rowPitch 过大");
		return E_OUTOFMEMORY;
	}

	ddsRowPitch = (uint32_t)rowPitch;
	ddsSlicePitch = (uint32_t)slicePitch;

	header->pitchOrLinearSize = static_cast<uint32_t>(rowPitch);

	memcpy(&header->ddspf, &DDSPF_DX10, sizeof(DDS_PIXELFORMAT));

	auto ext = reinterpret_cast<DDS_HEADER_DXT10*>(reinterpret_cast<uint8_t*>(header) + sizeof(DDS_HEADER));
	assert(ext);

	memset(ext, 0, sizeof(DDS_HEADER_DXT10));
	ext->dxgiFormat = format;
	ext->resourceDimension = DDS_DIMENSION_TEXTURE2D;
	ext->arraySize = 1;

	return S_OK;
}

static HRESULT SaveToDDSFile(
	const wchar_t* fileName,
	SizeU size,
	DXGI_FORMAT format,
	std::span<uint8_t> pixelData,
	uint32_t rowPitch
) noexcept {
	// 创建 DDS 头
	uint8_t header[DDS_DX10_HEADER_SIZE];
	uint32_t ddsRowPitch;
	uint32_t ddsSlicePitch;
	HRESULT hr = EncodeDDSHeader(size.width, size.height, format, header, ddsRowPitch, ddsSlicePitch);
	if (FAILED(hr)) {
		Logger::Get().ComError("EncodeDDSHeader 失败", hr);
		return hr;
	}

	wil::unique_hfile hFile(CreateFile2(
		fileName,
		GENERIC_WRITE | DELETE, 0, CREATE_ALWAYS, nullptr)
	);
	if (!hFile) {
		hr = HRESULT_FROM_WIN32(GetLastError());
		Logger::Get().Win32Error("CreateFile2 失败");
		return hr;
	}

	if (!Win32Helper::WriteFile(hFile.get(), header)) {
		Logger::Get().Error("Win32Helper::WriteFile 失败");
		return E_FAIL;
	}

	// 写入图像
	if ((uint32_t)pixelData.size() == ddsSlicePitch) {
		if (!Win32Helper::WriteFile(hFile.get(), pixelData)) {
			Logger::Get().Error("Win32Helper::WriteFile 失败");
			return E_FAIL;
		}
	} else {
		// DDS 使用字节对齐，所以肯定是 rowPitch 错误
		assert(rowPitch >= ddsRowPitch);

		const uint8_t* __restrict sPtr = pixelData.data();

		for (uint32_t i = 0; i < size.height; ++i) {
			if (!Win32Helper::WriteFile(hFile.get(), std::span(sPtr, (DWORD)ddsRowPitch))) {
				Logger::Get().Error("Win32Helper::WriteFile 失败");
				return E_FAIL;
			}

			sPtr += rowPitch;
		}
	}

	return S_OK;
}

winrt::com_ptr<ID3D12Resource> TextureHelper::LoadFromFile(
	wil::zwstring_view fileName,
	const D3D12Context& d3d12Context,
	DXGI_FORMAT& format,
	SizeU& textureSize
) noexcept {
	if (fileName.ends_with(L".dds")) {
		return LoadTextureFromDDS(fileName.c_str(), format, d3d12Context, textureSize);
	} else {
		assert(format == DXGI_FORMAT_R8G8B8A8_UNORM || format == DXGI_FORMAT_R16G16B16A16_FLOAT ||
			format == DXGI_FORMAT_R32G32B32A32_FLOAT);
		return LoadTextureFromImage(fileName.c_str(), d3d12Context, format, textureSize);
	}
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

static std::array<uint8_t, 36> MakeMDCVChunk(const ColorInfo& colorInfo) noexcept {
	std::array<uint8_t, 36> chunk;

	// 数据长度
	*(uint32_t*)&chunk[0] = MakeUI4(0, 0, 0, uint8_t(chunk.size() - 12u));

	// Chunk 类型
	*(uint32_t*)&chunk[4] = MakeUI4('m', 'D', 'C', 'V');
	
	// 原色
	*(uint16_t*)&chunk[8] = EncodeChroma(colorInfo.redPrimary.X);
	*(uint16_t*)&chunk[10] = EncodeChroma(colorInfo.redPrimary.Y);
	*(uint16_t*)&chunk[12] = EncodeChroma(colorInfo.greenPrimary.X);
	*(uint16_t*)&chunk[14] = EncodeChroma(colorInfo.greenPrimary.Y);
	*(uint16_t*)&chunk[16] = EncodeChroma(colorInfo.bluePrimary.X);
	*(uint16_t*)&chunk[18] = EncodeChroma(colorInfo.bluePrimary.Y);
	// 白点
	*(uint16_t*)&chunk[20] = EncodeChroma(colorInfo.whitePoint.X);
	*(uint16_t*)&chunk[22] = EncodeChroma(colorInfo.whitePoint.Y);
	// 最高和最低亮度
	*(uint32_t*)&chunk[24] = EncodeLuminace(colorInfo.maxLuminance);
	*(uint32_t*)&chunk[28] = EncodeLuminace(colorInfo.minLuminance);

	// CRC32 校验
	*(uint32_t*)&chunk[32] = CalculateCRC32(&chunk[4], (uint32_t)chunk.size() - 8u);

	return chunk;
}

static HRESULT SavePng(
	wil::zwstring_view fileName,
	SizeU size,
	std::span<uint8_t> pixelData,
	uint32_t rowPitch,
	const ColorInfo& colorInfo
) noexcept {
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

		if (colorInfo.kind == winrt::AdvancedColorKind::StandardDynamicRange) {
			hr = wicStream->InitializeFromFilename(fileName.c_str(), GENERIC_WRITE);
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

	hr = frameEncoder->SetSize(size.width, size.height);
	if (FAILED(hr)) {
		Logger::Get().ComError("IWICBitmapFrameEncode::SetSize 失败", hr);
		return hr;
	}

	WICPixelFormatGUID destFormat = GUID_WICPixelFormat64bppRGBA;
	hr = frameEncoder->SetPixelFormat(&destFormat);
	if (FAILED(hr)) {
		Logger::Get().ComError("IWICBitmapFrameEncode::SetPixelFormat 失败", hr);
		return hr;
	}
	assert(destFormat == GUID_WICPixelFormat64bppRGBA);

	if (colorInfo.kind == winrt::AdvancedColorKind::WideColorGamut) {
		// 出于兼容性考虑为 WCG 截图嵌入 icc，这样不支持 cICP 的软件也能正确渲染。但 HDR 截图
		// 不嵌入，因为这不是推荐的做法，而且支持的软件很少，见 https://www.w3.org/TR/png-hdr-pq
		hr = EmbedBT2020ICC(wicFactory.get(), frameEncoder.get());
		if (FAILED(hr)) {
			Logger::Get().ComWarn("EmbedBT2020ICC 失败", hr);
		}
	}
	
	hr = frameEncoder->WritePixels(
		size.height,
		rowPitch,
		(UINT)pixelData.size(),
		pixelData.data()
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
		assert(colorInfo.kind != winrt::AdvancedColorKind::StandardDynamicRange);

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
			fileName.c_str(), GENERIC_WRITE, 0, CREATE_ALWAYS, &extendedParams));
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
		if (!Win32Helper::WriteFile(hFile.get(), MakeCICPChunk(colorInfo.kind))) {
			Logger::Get().Error("Win32Helper::WriteFile 失败");
			return E_FAIL;
		}

		if (colorInfo.kind == winrt::AdvancedColorKind::HighDynamicRange) {
			// 嵌入 mDCV 数据块
			auto chunk = MakeMDCVChunk(colorInfo);
			if (!Win32Helper::WriteFile(hFile.get(), chunk)) {
				Logger::Get().Error("Win32Helper::WriteFile 失败");
				return E_FAIL;
			}
		}
		
		if (!Win32Helper::WriteFile(hFile.get(),
			std::span(pMem + CICP_CHUNK_OFFSET, memSize - CICP_CHUNK_OFFSET))) 
		{
			Logger::Get().Error("Win32Helper::WriteFile 失败");
			return E_FAIL;
		}
	}
	
	return S_OK;
}

HRESULT TextureHelper::Save(
	wil::zwstring_view fileName,
	SizeU size,
	EffectTextureFormat textureFormat,
	std::span<uint8_t> pixelData,
	uint32_t rowPitch,
	const ColorInfo& colorInfo
) noexcept {
	if (fileName.ends_with(L".dds")) {
		assert(textureFormat > EffectTextureFormat::COLOR_SPACE_ADAPTIVE &&
			textureFormat < EffectTextureFormat::COUNT);
		DXGI_FORMAT dxgiFormat = EFFECT_TEXTURE_FORMAT_PROPS[(uint32_t)textureFormat].dxgiFormat;
		return SaveToDDSFile(fileName.c_str(), size, dxgiFormat, pixelData, rowPitch);
	} else {
		assert(fileName.ends_with(L".png"));
		assert(textureFormat == EffectTextureFormat::R16G16B16A16_UNORM);
		return SavePng(fileName, size, pixelData, rowPitch, colorInfo);
	}
}

}
