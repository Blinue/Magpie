#include "pch.h"
#include "DDShelper.h"
#include "DDS.h"
#include "Logger.h"

///////////////////////////////////////////////////////////////////////
// 
// 读取和保存 DDS 文件的代码来自 https://github.com/microsoft/DirectXTK
// 
///////////////////////////////////////////////////////////////////////

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
	FILE_STANDARD_INFO fileInfo;
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

//--------------------------------------------------------------------------------------
// Return the BPP for a particular format
//--------------------------------------------------------------------------------------
static size_t BitsPerPixel(_In_ DXGI_FORMAT fmt) noexcept {
	switch (fmt) {
	case DXGI_FORMAT_R32G32B32A32_TYPELESS:
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
	case DXGI_FORMAT_R32G32B32A32_UINT:
	case DXGI_FORMAT_R32G32B32A32_SINT:
		return 128;

	case DXGI_FORMAT_R32G32B32_TYPELESS:
	case DXGI_FORMAT_R32G32B32_FLOAT:
	case DXGI_FORMAT_R32G32B32_UINT:
	case DXGI_FORMAT_R32G32B32_SINT:
		return 96;

	case DXGI_FORMAT_R16G16B16A16_TYPELESS:
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R16G16B16A16_UINT:
	case DXGI_FORMAT_R16G16B16A16_SNORM:
	case DXGI_FORMAT_R16G16B16A16_SINT:
	case DXGI_FORMAT_R32G32_TYPELESS:
	case DXGI_FORMAT_R32G32_FLOAT:
	case DXGI_FORMAT_R32G32_UINT:
	case DXGI_FORMAT_R32G32_SINT:
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
	case DXGI_FORMAT_Y416:
	case DXGI_FORMAT_Y210:
	case DXGI_FORMAT_Y216:
		return 64;

	case DXGI_FORMAT_R10G10B10A2_TYPELESS:
	case DXGI_FORMAT_R10G10B10A2_UNORM:
	case DXGI_FORMAT_R10G10B10A2_UINT:
	case DXGI_FORMAT_R11G11B10_FLOAT:
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_R8G8B8A8_SNORM:
	case DXGI_FORMAT_R8G8B8A8_SINT:
	case DXGI_FORMAT_R16G16_TYPELESS:
	case DXGI_FORMAT_R16G16_FLOAT:
	case DXGI_FORMAT_R16G16_UNORM:
	case DXGI_FORMAT_R16G16_UINT:
	case DXGI_FORMAT_R16G16_SNORM:
	case DXGI_FORMAT_R16G16_SINT:
	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R32_FLOAT:
	case DXGI_FORMAT_R32_UINT:
	case DXGI_FORMAT_R32_SINT:
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
	case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
	case DXGI_FORMAT_R8G8_B8G8_UNORM:
	case DXGI_FORMAT_G8R8_G8B8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
	case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8X8_TYPELESS:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
	case DXGI_FORMAT_AYUV:
	case DXGI_FORMAT_Y410:
	case DXGI_FORMAT_YUY2:
		return 32;

	case DXGI_FORMAT_P010:
	case DXGI_FORMAT_P016:
	case DXGI_FORMAT_V408:
		return 24;

	case DXGI_FORMAT_R8G8_TYPELESS:
	case DXGI_FORMAT_R8G8_UNORM:
	case DXGI_FORMAT_R8G8_UINT:
	case DXGI_FORMAT_R8G8_SNORM:
	case DXGI_FORMAT_R8G8_SINT:
	case DXGI_FORMAT_R16_TYPELESS:
	case DXGI_FORMAT_R16_FLOAT:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
	case DXGI_FORMAT_R16_UINT:
	case DXGI_FORMAT_R16_SNORM:
	case DXGI_FORMAT_R16_SINT:
	case DXGI_FORMAT_B5G6R5_UNORM:
	case DXGI_FORMAT_B5G5R5A1_UNORM:
	case DXGI_FORMAT_A8P8:
	case DXGI_FORMAT_B4G4R4A4_UNORM:
	case DXGI_FORMAT_P208:
	case DXGI_FORMAT_V208:
		return 16;

	case DXGI_FORMAT_NV12:
	case DXGI_FORMAT_420_OPAQUE:
	case DXGI_FORMAT_NV11:
		return 12;

	case DXGI_FORMAT_R8_TYPELESS:
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_R8_UINT:
	case DXGI_FORMAT_R8_SNORM:
	case DXGI_FORMAT_R8_SINT:
	case DXGI_FORMAT_A8_UNORM:
	case DXGI_FORMAT_BC2_TYPELESS:
	case DXGI_FORMAT_BC2_UNORM:
	case DXGI_FORMAT_BC2_UNORM_SRGB:
	case DXGI_FORMAT_BC3_TYPELESS:
	case DXGI_FORMAT_BC3_UNORM:
	case DXGI_FORMAT_BC3_UNORM_SRGB:
	case DXGI_FORMAT_BC5_TYPELESS:
	case DXGI_FORMAT_BC5_UNORM:
	case DXGI_FORMAT_BC5_SNORM:
	case DXGI_FORMAT_BC6H_TYPELESS:
	case DXGI_FORMAT_BC6H_UF16:
	case DXGI_FORMAT_BC6H_SF16:
	case DXGI_FORMAT_BC7_TYPELESS:
	case DXGI_FORMAT_BC7_UNORM:
	case DXGI_FORMAT_BC7_UNORM_SRGB:
	case DXGI_FORMAT_AI44:
	case DXGI_FORMAT_IA44:
	case DXGI_FORMAT_P8:
		return 8;

	case DXGI_FORMAT_R1_UNORM:
		return 1;

	case DXGI_FORMAT_BC1_TYPELESS:
	case DXGI_FORMAT_BC1_UNORM:
	case DXGI_FORMAT_BC1_UNORM_SRGB:
	case DXGI_FORMAT_BC4_TYPELESS:
	case DXGI_FORMAT_BC4_UNORM:
	case DXGI_FORMAT_BC4_SNORM:
		return 4;

	default:
		return 0;
	}
}

//--------------------------------------------------------------------------------------
// Get surface information for a particular format
//--------------------------------------------------------------------------------------
static HRESULT GetSurfaceInfo(
	_In_ size_t width,
	_In_ size_t height,
	_In_ DXGI_FORMAT fmt,
	_Out_opt_ size_t* outNumBytes,
	_Out_opt_ size_t* outRowBytes,
	_Out_opt_ size_t* outNumRows
) noexcept {
	uint64_t numBytes = 0;
	uint64_t rowBytes = 0;
	uint64_t numRows = 0;

	bool bc = false;
	bool packed = false;
	bool planar = false;
	size_t bpe = 0;
	switch (fmt) {
	case DXGI_FORMAT_UNKNOWN:
		return E_INVALIDARG;

	case DXGI_FORMAT_BC1_TYPELESS:
	case DXGI_FORMAT_BC1_UNORM:
	case DXGI_FORMAT_BC1_UNORM_SRGB:
	case DXGI_FORMAT_BC4_TYPELESS:
	case DXGI_FORMAT_BC4_UNORM:
	case DXGI_FORMAT_BC4_SNORM:
		bc = true;
		bpe = 8;
		break;

	case DXGI_FORMAT_BC2_TYPELESS:
	case DXGI_FORMAT_BC2_UNORM:
	case DXGI_FORMAT_BC2_UNORM_SRGB:
	case DXGI_FORMAT_BC3_TYPELESS:
	case DXGI_FORMAT_BC3_UNORM:
	case DXGI_FORMAT_BC3_UNORM_SRGB:
	case DXGI_FORMAT_BC5_TYPELESS:
	case DXGI_FORMAT_BC5_UNORM:
	case DXGI_FORMAT_BC5_SNORM:
	case DXGI_FORMAT_BC6H_TYPELESS:
	case DXGI_FORMAT_BC6H_UF16:
	case DXGI_FORMAT_BC6H_SF16:
	case DXGI_FORMAT_BC7_TYPELESS:
	case DXGI_FORMAT_BC7_UNORM:
	case DXGI_FORMAT_BC7_UNORM_SRGB:
		bc = true;
		bpe = 16;
		break;

	case DXGI_FORMAT_R8G8_B8G8_UNORM:
	case DXGI_FORMAT_G8R8_G8B8_UNORM:
	case DXGI_FORMAT_YUY2:
		packed = true;
		bpe = 4;
		break;

	case DXGI_FORMAT_Y210:
	case DXGI_FORMAT_Y216:
		packed = true;
		bpe = 8;
		break;

	case DXGI_FORMAT_NV12:
	case DXGI_FORMAT_420_OPAQUE:
		if ((height % 2) != 0) {
			// Requires a height alignment of 2.
			return E_INVALIDARG;
		}
		planar = true;
		bpe = 2;
		break;

	case DXGI_FORMAT_P208:
		planar = true;
		bpe = 2;
		break;

	case DXGI_FORMAT_P010:
	case DXGI_FORMAT_P016:
		if ((height % 2) != 0) {
			// Requires a height alignment of 2.
			return E_INVALIDARG;
		}
		planar = true;
		bpe = 4;
		break;

	default:
		break;
	}

	if (bc) {
		uint64_t numBlocksWide = 0;
		if (width > 0) {
			numBlocksWide = std::max<uint64_t>(1u, (uint64_t(width) + 3u) / 4u);
		}
		uint64_t numBlocksHigh = 0;
		if (height > 0) {
			numBlocksHigh = std::max<uint64_t>(1u, (uint64_t(height) + 3u) / 4u);
		}
		rowBytes = numBlocksWide * bpe;
		numRows = numBlocksHigh;
		numBytes = rowBytes * numBlocksHigh;
	} else if (packed) {
		rowBytes = ((uint64_t(width) + 1u) >> 1) * bpe;
		numRows = uint64_t(height);
		numBytes = rowBytes * height;
	} else if (fmt == DXGI_FORMAT_NV11) {
		rowBytes = ((uint64_t(width) + 3u) >> 2) * 4u;
		numRows = uint64_t(height) * 2u; // Direct3D makes this simplifying assumption, although it is larger than the 4:1:1 data
		numBytes = rowBytes * numRows;
	} else if (planar) {
		rowBytes = ((uint64_t(width) + 1u) >> 1) * bpe;
		numBytes = (rowBytes * uint64_t(height)) + ((rowBytes * uint64_t(height) + 1u) >> 1);
		numRows = height + ((uint64_t(height) + 1u) >> 1);
	} else {
		const size_t bpp = BitsPerPixel(fmt);
		if (!bpp)
			return E_INVALIDARG;

		rowBytes = (uint64_t(width) * bpp + 7u) / 8u; // round up to nearest byte
		numRows = uint64_t(height);
		numBytes = rowBytes * height;
	}

#if defined(_M_IX86) || defined(_M_ARM) || defined(_M_HYBRID_X86_ARM64)
	static_assert(sizeof(size_t) == 4, "Not a 32-bit platform!");
	if (numBytes > UINT32_MAX || rowBytes > UINT32_MAX || numRows > UINT32_MAX)
		return HRESULT_E_ARITHMETIC_OVERFLOW;
#else
	static_assert(sizeof(size_t) == 8, "Not a 64-bit platform!");
#endif

	if (outNumBytes) {
		*outNumBytes = static_cast<size_t>(numBytes);
	}
	if (outRowBytes) {
		*outRowBytes = static_cast<size_t>(rowBytes);
	}
	if (outNumRows) {
		*outNumRows = static_cast<size_t>(numRows);
	}

	return S_OK;
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

			if (ISBITMASK(0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000)) {
				return DXGI_FORMAT_B8G8R8A8_UNORM;
			}

			if (ISBITMASK(0x00ff0000, 0x0000ff00, 0x000000ff, 0)) {
				return DXGI_FORMAT_B8G8R8X8_UNORM;
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
			if (ISBITMASK(0x7c00, 0x03e0, 0x001f, 0x8000)) {
				return DXGI_FORMAT_B5G5R5A1_UNORM;
			}
			if (ISBITMASK(0xf800, 0x07e0, 0x001f, 0)) {
				return DXGI_FORMAT_B5G6R5_UNORM;
			}

			// No DXGI format maps to ISBITMASK(0x7c00,0x03e0,0x001f,0) aka D3DFMT_X1R5G5B5

			if (ISBITMASK(0x0f00, 0x00f0, 0x000f, 0xf000)) {
				return DXGI_FORMAT_B4G4R4A4_UNORM;
			}

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
	} else if (ddpf.flags & DDS_ALPHA) {
		if (8 == ddpf.RGBBitCount) {
			return DXGI_FORMAT_A8_UNORM;
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
		if (MAKEFOURCC('D', 'X', 'T', '1') == ddpf.fourCC) {
			return DXGI_FORMAT_BC1_UNORM;
		}
		if (MAKEFOURCC('D', 'X', 'T', '3') == ddpf.fourCC) {
			return DXGI_FORMAT_BC2_UNORM;
		}
		if (MAKEFOURCC('D', 'X', 'T', '5') == ddpf.fourCC) {
			return DXGI_FORMAT_BC3_UNORM;
		}

		// While pre-multiplied alpha isn't directly supported by the DXGI formats,
		// they are basically the same as these BC formats so they can be mapped
		if (MAKEFOURCC('D', 'X', 'T', '2') == ddpf.fourCC) {
			return DXGI_FORMAT_BC2_UNORM;
		}
		if (MAKEFOURCC('D', 'X', 'T', '4') == ddpf.fourCC) {
			return DXGI_FORMAT_BC3_UNORM;
		}

		if (MAKEFOURCC('A', 'T', 'I', '1') == ddpf.fourCC) {
			return DXGI_FORMAT_BC4_UNORM;
		}
		if (MAKEFOURCC('B', 'C', '4', 'U') == ddpf.fourCC) {
			return DXGI_FORMAT_BC4_UNORM;
		}
		if (MAKEFOURCC('B', 'C', '4', 'S') == ddpf.fourCC) {
			return DXGI_FORMAT_BC4_SNORM;
		}

		if (MAKEFOURCC('A', 'T', 'I', '2') == ddpf.fourCC) {
			return DXGI_FORMAT_BC5_UNORM;
		}
		if (MAKEFOURCC('B', 'C', '5', 'U') == ddpf.fourCC) {
			return DXGI_FORMAT_BC5_UNORM;
		}
		if (MAKEFOURCC('B', 'C', '5', 'S') == ddpf.fourCC) {
			return DXGI_FORMAT_BC5_SNORM;
		}

		// BC6H and BC7 are written using the "DX10" extended header

		if (MAKEFOURCC('R', 'G', 'B', 'G') == ddpf.fourCC) {
			return DXGI_FORMAT_R8G8_B8G8_UNORM;
		}
		if (MAKEFOURCC('G', 'R', 'G', 'B') == ddpf.fourCC) {
			return DXGI_FORMAT_G8R8_G8B8_UNORM;
		}

		if (MAKEFOURCC('Y', 'U', 'Y', '2') == ddpf.fourCC) {
			return DXGI_FORMAT_YUY2;
		}

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

static DXGI_FORMAT MakeSRGB(_In_ DXGI_FORMAT format) noexcept {
	switch (format) {
	case DXGI_FORMAT_R8G8B8A8_UNORM:
		return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	case DXGI_FORMAT_BC1_UNORM:
		return DXGI_FORMAT_BC1_UNORM_SRGB;

	case DXGI_FORMAT_BC2_UNORM:
		return DXGI_FORMAT_BC2_UNORM_SRGB;

	case DXGI_FORMAT_BC3_UNORM:
		return DXGI_FORMAT_BC3_UNORM_SRGB;

	case DXGI_FORMAT_B8G8R8A8_UNORM:
		return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

	case DXGI_FORMAT_B8G8R8X8_UNORM:
		return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;

	case DXGI_FORMAT_BC7_UNORM:
		return DXGI_FORMAT_BC7_UNORM_SRGB;

	default:
		return format;
	}
}

static HRESULT CreateD3DResources(
	_In_ ID3D11Device* d3dDevice,
	_In_ uint32_t resDim,
	_In_ size_t width,
	_In_ size_t height,
	_In_ size_t depth,
	_In_ size_t mipCount,
	_In_ size_t arraySize,
	_In_ DXGI_FORMAT format,
	_In_ D3D11_USAGE usage,
	_In_ unsigned int bindFlags,
	_In_ unsigned int cpuAccessFlags,
	_In_ unsigned int miscFlags,
	_In_ bool forceSRGB,
	_In_ bool isCubeMap,
	_In_reads_opt_(mipCount* arraySize) const D3D11_SUBRESOURCE_DATA* initData,
	_Outptr_opt_ ID3D11Resource** texture
) noexcept {
	if (!d3dDevice)
		return E_POINTER;

	HRESULT hr = E_FAIL;

	if (forceSRGB) {
		format = MakeSRGB(format);
	}

	switch (resDim) {
	case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
	{
		D3D11_TEXTURE1D_DESC desc = {};
		desc.Width = static_cast<UINT>(width);
		desc.MipLevels = static_cast<UINT>(mipCount);
		desc.ArraySize = static_cast<UINT>(arraySize);
		desc.Format = format;
		desc.Usage = usage;
		desc.BindFlags = bindFlags;
		desc.CPUAccessFlags = cpuAccessFlags;
		desc.MiscFlags = miscFlags & ~static_cast<unsigned int>(D3D11_RESOURCE_MISC_TEXTURECUBE);

		ID3D11Texture1D* tex = nullptr;
		hr = d3dDevice->CreateTexture1D(&desc,
			initData,
			&tex
		);
		if (SUCCEEDED(hr) && tex) {
			if (texture) {
				*texture = tex;
			} else {
				tex->Release();
			}
		}
	}
	break;

	case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
	{
		D3D11_TEXTURE2D_DESC desc = {
			.Width = static_cast<UINT>(width),
			.Height = static_cast<UINT>(height),
			.MipLevels = static_cast<UINT>(mipCount),
			.ArraySize = static_cast<UINT>(arraySize),
			.Format = format,
			.SampleDesc = {
				.Count = 1
			},
			.Usage = usage,
			.BindFlags = bindFlags,
			.CPUAccessFlags = cpuAccessFlags,
		};
		if (isCubeMap) {
			desc.MiscFlags = miscFlags | D3D11_RESOURCE_MISC_TEXTURECUBE;
		} else {
			desc.MiscFlags = miscFlags & ~static_cast<unsigned int>(D3D11_RESOURCE_MISC_TEXTURECUBE);
		}

		ID3D11Texture2D* tex = nullptr;
		hr = d3dDevice->CreateTexture2D(&desc,
			initData,
			&tex
		);
		if (SUCCEEDED(hr) && tex) {
			if (texture) {
				*texture = tex;
			} else {
				tex->Release();
			}
		}
	}
	break;

	case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
	{
		D3D11_TEXTURE3D_DESC desc = {
			.Width = static_cast<UINT>(width),
			.Height = static_cast<UINT>(height),
			.Depth = static_cast<UINT>(depth),
			.MipLevels = static_cast<UINT>(mipCount),
			.Format = format,
			.Usage = usage,
			.BindFlags = bindFlags,
			.CPUAccessFlags = cpuAccessFlags,
			.MiscFlags = miscFlags & ~UINT(D3D11_RESOURCE_MISC_TEXTURECUBE)
		};

		ID3D11Texture3D* tex = nullptr;
		hr = d3dDevice->CreateTexture3D(&desc,
			initData,
			&tex
		);
		if (SUCCEEDED(hr) && tex) {
			if (texture) {
				*texture = tex;
			} else {
				tex->Release();
			}
		}
	}
	break;
	}

	return hr;
}

static HRESULT FillInitData(
	_In_ size_t width,
	_In_ size_t height,
	_In_ size_t depth,
	_In_ size_t mipCount,
	_In_ size_t arraySize,
	_In_ DXGI_FORMAT format,
	_In_ size_t maxsize,
	_In_ size_t bitSize,
	_In_reads_bytes_(bitSize) const uint8_t* bitData,
	_Out_ size_t& twidth,
	_Out_ size_t& theight,
	_Out_ size_t& tdepth,
	_Out_ size_t& skipMip,
	_Out_writes_(mipCount* arraySize) D3D11_SUBRESOURCE_DATA* initData
) noexcept {
	if (!bitData || !initData) {
		return E_POINTER;
	}

	skipMip = 0;
	twidth = 0;
	theight = 0;
	tdepth = 0;

	size_t NumBytes = 0;
	size_t RowBytes = 0;
	const uint8_t* pSrcBits = bitData;
	const uint8_t* pEndBits = bitData + bitSize;

	size_t index = 0;
	for (size_t j = 0; j < arraySize; j++) {
		size_t w = width;
		size_t h = height;
		size_t d = depth;
		for (size_t i = 0; i < mipCount; i++) {
			HRESULT hr = GetSurfaceInfo(w, h, format, &NumBytes, &RowBytes, nullptr);
			if (FAILED(hr))
				return hr;

			if (NumBytes > UINT32_MAX || RowBytes > UINT32_MAX)
				return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);

			if ((mipCount <= 1) || !maxsize || (w <= maxsize && h <= maxsize && d <= maxsize)) {
				if (!twidth) {
					twidth = w;
					theight = h;
					tdepth = d;
				}

				assert(index < mipCount * arraySize);
				_Analysis_assume_(index < mipCount * arraySize);
				initData[index].pSysMem = pSrcBits;
				initData[index].SysMemPitch = static_cast<UINT>(RowBytes);
				initData[index].SysMemSlicePitch = static_cast<UINT>(NumBytes);
				++index;
			} else if (!j) {
				// Count number of skipped mipmaps (first item only)
				++skipMip;
			}

			if (pSrcBits + (NumBytes * d) > pEndBits) {
				return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);
			}

			pSrcBits += NumBytes * d;

			w = w >> 1;
			h = h >> 1;
			d = d >> 1;
			if (w == 0) {
				w = 1;
			}
			if (h == 0) {
				h = 1;
			}
			if (d == 0) {
				d = 1;
			}
		}
	}

	return (index > 0) ? S_OK : E_FAIL;
}

static HRESULT CreateTextureFromDDS(
	_In_ ID3D11Device* d3dDevice,
	_In_ const DDS_HEADER* header,
	_In_reads_bytes_(bitSize) const uint8_t* bitData,
	_In_ size_t bitSize,
	_In_ size_t maxsize,
	_In_ D3D11_USAGE usage,
	_In_ unsigned int bindFlags,
	_In_ unsigned int cpuAccessFlags,
	_In_ unsigned int miscFlags,
	_In_ bool forceSRGB,
	_Outptr_opt_ ID3D11Resource** texture
) noexcept {
	HRESULT hr = S_OK;

	const UINT width = header->width;
	UINT height = header->height;
	UINT depth = header->depth;

	uint32_t resDim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
	UINT arraySize = 1;
	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
	bool isCubeMap = false;

	size_t mipCount = header->mipMapCount;
	if (0 == mipCount) {
		mipCount = 1;
	}

	if ((header->ddspf.flags & DDS_FOURCC) &&
		(MAKEFOURCC('D', 'X', '1', '0') == header->ddspf.fourCC)) {
		auto d3d10ext = reinterpret_cast<const DDS_HEADER_DXT10*>(reinterpret_cast<const char*>(header) + sizeof(DDS_HEADER));

		arraySize = d3d10ext->arraySize;
		if (arraySize == 0) {
			return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		}

		switch (d3d10ext->dxgiFormat) {
		case DXGI_FORMAT_AI44:
		case DXGI_FORMAT_IA44:
		case DXGI_FORMAT_P8:
		case DXGI_FORMAT_A8P8:
			Logger::Get().Error("ERROR: DDSTextureLoader does not support video textures. Consider using DirectXTex instead.\n");
			return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

		default:
			if (BitsPerPixel(d3d10ext->dxgiFormat) == 0) {
				Logger::Get().Error(fmt::format("ERROR: Unknown DXGI format ({})\n", static_cast<uint32_t>(d3d10ext->dxgiFormat)));
				return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
			}
		}

		format = d3d10ext->dxgiFormat;

		switch (d3d10ext->resourceDimension) {
		case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
			// D3DX writes 1D textures with a fixed Height of 1
			if ((header->flags & DDS_HEIGHT) && height != 1) {
				return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
			}
			height = depth = 1;
			break;

		case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
			if (d3d10ext->miscFlag & D3D11_RESOURCE_MISC_TEXTURECUBE) {
				arraySize *= 6;
				isCubeMap = true;
			}
			depth = 1;
			break;

		case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
			if (!(header->flags & DDS_HEADER_FLAGS_VOLUME)) {
				return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
			}

			if (arraySize > 1) {
				Logger::Get().Error("ERROR: Volume textures are not texture arrays\n");
				return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
			}
			break;

		case D3D11_RESOURCE_DIMENSION_BUFFER:
			Logger::Get().Error("ERROR: Resource dimension buffer type not supported for textures\n");
			return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

		case D3D11_RESOURCE_DIMENSION_UNKNOWN:
		default:
			Logger::Get().Error(fmt::format("ERROR: Unknown resource dimension ({})\n", static_cast<uint32_t>(d3d10ext->resourceDimension)));
			return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
		}

		resDim = d3d10ext->resourceDimension;
	} else {
		format = GetDXGIFormat(header->ddspf);

		if (format == DXGI_FORMAT_UNKNOWN) {
			Logger::Get().Error("ERROR: DDSTextureLoader does not support all legacy DDS formats. Consider using DirectXTex.\n");
			return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
		}

		if (header->flags & DDS_HEADER_FLAGS_VOLUME) {
			resDim = D3D11_RESOURCE_DIMENSION_TEXTURE3D;
		} else {
			if (header->caps2 & DDS_CUBEMAP) {
				// We require all six faces to be defined
				if ((header->caps2 & DDS_CUBEMAP_ALLFACES) != DDS_CUBEMAP_ALLFACES) {
					Logger::Get().Error("ERROR: DirectX 11 does not support partial cubemaps\n");
					return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
				}

				arraySize = 6;
				isCubeMap = true;
			}

			depth = 1;
			resDim = D3D11_RESOURCE_DIMENSION_TEXTURE2D;

			// Note there's no way for a legacy Direct3D 9 DDS to express a '1D' texture
		}

		assert(BitsPerPixel(format) != 0);
	}

	if ((miscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE)
		&& (resDim == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
		&& ((arraySize % 6) == 0)) {
		isCubeMap = true;
	}

	// Bound sizes (for security purposes we don't trust DDS file metadata larger than the Direct3D hardware requirements)
	if (mipCount > D3D11_REQ_MIP_LEVELS) {
		Logger::Get().Error(fmt::format("ERROR: Too many mipmap levels defined for DirectX 11 ({}).\n", mipCount));
		return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
	}

	switch (resDim) {
	case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
		if ((arraySize > D3D11_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION) ||
			(width > D3D11_REQ_TEXTURE1D_U_DIMENSION)) {
			Logger::Get().Error(fmt::format("ERROR: Resource dimensions too large for DirectX 11 (1D: array {}, size {})\n", arraySize, width));
			return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
		}
		break;

	case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
		if (isCubeMap) {
			// This is the right bound because we set arraySize to (NumCubes*6) above
			if ((arraySize > D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION) ||
				(width > D3D11_REQ_TEXTURECUBE_DIMENSION) ||
				(height > D3D11_REQ_TEXTURECUBE_DIMENSION)) {
				Logger::Get().Error(fmt::format("ERROR: Resource dimensions too large for DirectX 11 (2D cubemap: array {}, size {} by {})\n", arraySize, width, height));
				return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
			}
		} else if ((arraySize > D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION) ||
			(width > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION) ||
			(height > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION)) {
			Logger::Get().Error(fmt::format("ERROR: Resource dimensions too large for DirectX 11 (2D: array {}, size {} by {})\n", arraySize, width, height));
			return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
		}
		break;

	case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
		if ((arraySize > 1) ||
			(width > D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION) ||
			(height > D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION) ||
			(depth > D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION)) {
			Logger::Get().Error(fmt::format("ERROR: Resource dimensions too large for DirectX 11 (3D: array {}, size {} by {} by {})\n", arraySize, width, height, depth));
			return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
		}
		break;

	case D3D11_RESOURCE_DIMENSION_BUFFER:
		Logger::Get().Error("ERROR: Resource dimension buffer type not supported for textures\n");
		return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

	case D3D11_RESOURCE_DIMENSION_UNKNOWN:
	default:
		Logger::Get().Error(fmt::format("ERROR: Unknown resource dimension ({})\n", static_cast<uint32_t>(resDim)));
		return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
	}

	// Create the texture
	std::unique_ptr<D3D11_SUBRESOURCE_DATA[]> initData(new (std::nothrow) D3D11_SUBRESOURCE_DATA[mipCount * arraySize]);
	if (!initData) {
		return E_OUTOFMEMORY;
	}

	size_t skipMip = 0;
	size_t twidth = 0;
	size_t theight = 0;
	size_t tdepth = 0;
	hr = FillInitData(width, height, depth, mipCount, arraySize, format,
		maxsize, bitSize, bitData,
		twidth, theight, tdepth, skipMip, initData.get());

	if (SUCCEEDED(hr)) {
		hr = CreateD3DResources(d3dDevice,
			resDim, twidth, theight, tdepth, mipCount - skipMip, arraySize,
			format,
			usage, bindFlags, cpuAccessFlags, miscFlags,
			forceSRGB,
			isCubeMap,
			initData.get(),
			texture
		);

		if (FAILED(hr) && !maxsize && (mipCount > 1)) {
			// Retry with a maxsize determined by feature level
			maxsize = (resDim == D3D11_RESOURCE_DIMENSION_TEXTURE3D)
				? D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION
				: D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;

			hr = FillInitData(width, height, depth, mipCount, arraySize, format,
				maxsize, bitSize, bitData,
				twidth, theight, tdepth, skipMip, initData.get());
			if (SUCCEEDED(hr)) {
				hr = CreateD3DResources(d3dDevice,
					resDim, twidth, theight, tdepth, mipCount - skipMip, arraySize,
					format,
					usage, bindFlags, cpuAccessFlags, miscFlags,
					forceSRGB,
					isCubeMap,
					initData.get(),
					texture
				);
			}
		}
	}

	return hr;
}

static DDS_ALPHA_MODE GetAlphaMode(_In_ const DDS_HEADER* header) noexcept {
	if (header->ddspf.flags & DDS_FOURCC) {
		if (MAKEFOURCC('D', 'X', '1', '0') == header->ddspf.fourCC) {
			auto d3d10ext = reinterpret_cast<const DDS_HEADER_DXT10*>(reinterpret_cast<const uint8_t*>(header) + sizeof(DDS_HEADER));
			const auto mode = static_cast<DDS_ALPHA_MODE>(d3d10ext->miscFlags2 & DDS_MISC_FLAGS2_ALPHA_MODE_MASK);
			switch (mode) {
			case DDS_ALPHA_MODE_STRAIGHT:
			case DDS_ALPHA_MODE_PREMULTIPLIED:
			case DDS_ALPHA_MODE_OPAQUE:
			case DDS_ALPHA_MODE_CUSTOM:
				return mode;

			case DDS_ALPHA_MODE_UNKNOWN:
			default:
				break;
			}
		} else if ((MAKEFOURCC('D', 'X', 'T', '2') == header->ddspf.fourCC)
			|| (MAKEFOURCC('D', 'X', 'T', '4') == header->ddspf.fourCC)) {
			return DDS_ALPHA_MODE_PREMULTIPLIED;
		}
	}

	return DDS_ALPHA_MODE_UNKNOWN;
}

static HRESULT CreateDDSTextureFromFileEx(
	ID3D11Device* d3dDevice,
	const wchar_t* fileName,
	size_t maxsize,
	D3D11_USAGE usage,
	unsigned int bindFlags,
	unsigned int cpuAccessFlags,
	unsigned int miscFlags,
	bool forceSRGB,
	ID3D11Resource** texture,
	DDS_ALPHA_MODE* alphaMode
) noexcept {
	if (texture) {
		*texture = nullptr;
	}
	if (alphaMode) {
		*alphaMode = DDS_ALPHA_MODE_UNKNOWN;
	}

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
		return hr;
	}

	hr = CreateTextureFromDDS(
		d3dDevice,
		header, bitData, bitSize,
		maxsize,
		usage, bindFlags, cpuAccessFlags, miscFlags,
		forceSRGB,
		texture
	);

	if (SUCCEEDED(hr)) {
		if (alphaMode)
			*alphaMode = GetAlphaMode(header);
	}

	return hr;
}

//-------------------------------------------------------------------------------------
// Computes the image row pitch in bytes, and the slice ptich (size in bytes of the image)
// based on DXGI format, width, and height
//-------------------------------------------------------------------------------------
static bool ComputePitch(DXGI_FORMAT fmt, size_t width, size_t height, size_t& rowPitch, size_t& slicePitch) noexcept {
	size_t bpp = BitsPerPixel(fmt);
	if (!bpp) {
		Logger::Get().Error("不支持的格式");
		return false;
	}

	// Default byte alignment
	rowPitch = (uint64_t(width) * bpp + 7u) / 8u;
	slicePitch = rowPitch * uint64_t(height);

	return true;
}

//-------------------------------------------------------------------------------------
// Encodes DDS file header (magic value, header, DX10 extended header)
//-------------------------------------------------------------------------------------
static bool EncodeDDSHeader(
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
	if (!ComputePitch(format, width, height, rowPitch, slicePitch)) {
		Logger::Get().Error("ComputePitch 失败");
		return false;
	}

	if (slicePitch > UINT32_MAX || rowPitch > UINT32_MAX) {
		Logger::Get().Error("slicePitch 或 rowPitch 过大");
		return false;
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

	return true;
}

//-------------------------------------------------------------------------------------
// Save a DDS file to disk
//-------------------------------------------------------------------------------------
static bool SaveToDDSFile(
	const wchar_t* fileName,
	uint32_t width,
	uint32_t height,
	DXGI_FORMAT format,
	std::span<uint8_t> pixelData,
	uint32_t rowPitch
) noexcept {
	// 创建 DDS 头
	uint8_t header[DDS_DX10_HEADER_SIZE];
	uint32_t ddsRowPitch;
	uint32_t ddsSlicePitch;
	if (!EncodeDDSHeader(width, height, format, header, ddsRowPitch, ddsSlicePitch)) {
		Logger::Get().Error("EncodeDDSHeader 失败");
		return false;
	}

	wil::unique_hfile hFile(CreateFile2(
		fileName,
		GENERIC_WRITE | DELETE, 0, CREATE_ALWAYS, nullptr)
	);
	if (!hFile) {
		Logger::Get().Win32Error("CreateFile2 失败");
		return false;
	}

	DWORD bytesWritten;
	if (!WriteFile(hFile.get(), header, (DWORD)DDS_DX10_HEADER_SIZE, &bytesWritten, nullptr) ||
		bytesWritten != DDS_DX10_HEADER_SIZE) {
		Logger::Get().Win32Error("WriteFile 失败");
		return false;
	}

	// 写入图像
	if ((uint32_t)pixelData.size() == ddsSlicePitch) {
		if (!WriteFile(hFile.get(), pixelData.data(), (DWORD)ddsSlicePitch, &bytesWritten, nullptr) ||
			bytesWritten != ddsSlicePitch) {
			Logger::Get().Win32Error("WriteFile 失败");
			return false;
		}
	} else {
		if (rowPitch < ddsRowPitch) {
			// DDS 使用字节对齐，所以肯定是 rowPitch 错误
			Logger::Get().Win32Error("rowPitch 参数非法");
			return false;
		}

		const uint8_t* __restrict sPtr = pixelData.data();

		for (uint32_t i = 0; i < height; ++i) {
			if (!WriteFile(hFile.get(), sPtr, (DWORD)ddsRowPitch, &bytesWritten, nullptr) ||
				bytesWritten != ddsRowPitch) {
				Logger::Get().Win32Error("WriteFile 失败");
				return false;
			}

			sPtr += rowPitch;
		}
	}

	return true;
}

winrt::com_ptr<ID3D11Texture2D> DDSHelper::Load(const wchar_t* fileName, ID3D11Device* d3dDevice) noexcept {
	winrt::com_ptr<ID3D11Resource> result;

	DDS_ALPHA_MODE alphaMode = DDS_ALPHA_MODE_STRAIGHT;
	HRESULT hr = CreateDDSTextureFromFileEx(
		d3dDevice,
		fileName,
		0,
		D3D11_USAGE_IMMUTABLE,
		D3D11_BIND_SHADER_RESOURCE,
		0,
		0,
		false,
		result.put(),
		&alphaMode
	);
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateDDSTextureFromFile 失败", hr);
		return nullptr;
	}

	winrt::com_ptr<ID3D11Texture2D> tex = result.try_as<ID3D11Texture2D>();
	if (!tex) {
		Logger::Get().Error("从 ID3D11Resource 获取 ID3D11Texture2D 失败");
		return nullptr;
	}

	return tex;
}

bool DDSHelper::Save(
	const wchar_t* fileName,
	uint32_t width,
	uint32_t height,
	DXGI_FORMAT format,
	std::span<uint8_t> pixelData,
	uint32_t rowPitch
) {
	if (!SaveToDDSFile(fileName, width, height, format, pixelData, rowPitch)) {
		DeleteFile(fileName);
		return false;
	}

	return true;
}

}
