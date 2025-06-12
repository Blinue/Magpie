#include "pch.h"
#include "TextureHelper.h"
#include "Logger.h"
#include "DDSHelper.h"
#include "DirectXHelper.h"
#include "EffectHelper.h"
#include <wincodec.h>

namespace Magpie {

static winrt::com_ptr<ID3D11Texture2D> LoadImg(const wchar_t* fileName, ID3D11Device* d3dDevice) noexcept {
	winrt::com_ptr<IWICImagingFactory2> wicImgFactory =
		winrt::try_create_instance<IWICImagingFactory2>(CLSID_WICImagingFactory);
	if (!wicImgFactory) {
		Logger::Get().Error("创建 WICImagingFactory 失败");
		return nullptr;
	}

	// 读取图像文件
	winrt::com_ptr<IWICBitmapDecoder> decoder;
	HRESULT hr = wicImgFactory->CreateDecoderFromFilename(fileName, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, decoder.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateDecoderFromFilename 失败", hr);
		return nullptr;
	}

	winrt::com_ptr<IWICBitmapFrameDecode> frame;
	hr = decoder->GetFrame(0, frame.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("IWICBitmapFrameDecode::GetFrame 失败", hr);
		return nullptr;
	}

	bool useFloatFormat = false;
	{
		WICPixelFormatGUID sourceFormat;
		hr = frame->GetPixelFormat(&sourceFormat);
		if (FAILED(hr)) {
			Logger::Get().ComError("GetPixelFormat 失败", hr);
			return nullptr;
		}

		winrt::com_ptr<IWICComponentInfo> cInfo;
		hr = wicImgFactory->CreateComponentInfo(sourceFormat, cInfo.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("CreateComponentInfo", hr);
			return nullptr;
		}
		winrt::com_ptr<IWICPixelFormatInfo2> formatInfo = cInfo.try_as<IWICPixelFormatInfo2>();
		if (!formatInfo) {
			Logger::Get().Error("IWICComponentInfo 转换为 IWICPixelFormatInfo2 时失败");
			return nullptr;
		}

		UINT bitsPerPixel;
		WICPixelFormatNumericRepresentation type;
		hr = formatInfo->GetBitsPerPixel(&bitsPerPixel);
		if (FAILED(hr)) {
			Logger::Get().ComError("GetBitsPerPixel", hr);
			return nullptr;
		}
		hr = formatInfo->GetNumericRepresentation(&type);
		if (FAILED(hr)) {
			Logger::Get().ComError("GetNumericRepresentation", hr);
			return nullptr;
		}

		useFloatFormat = bitsPerPixel > 32 || type == WICPixelFormatNumericRepresentationFixed || type == WICPixelFormatNumericRepresentationFloat;
	}

	// 转换格式
	winrt::com_ptr<IWICFormatConverter> formatConverter;
	hr = wicImgFactory->CreateFormatConverter(formatConverter.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("CreateFormatConverter 失败", hr);
		return nullptr;
	}

	WICPixelFormatGUID targetFormat = useFloatFormat ? GUID_WICPixelFormat64bppRGBAHalf : GUID_WICPixelFormat32bppRGBA;
	hr = formatConverter->Initialize(frame.get(), targetFormat, WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom);
	if (FAILED(hr)) {
		Logger::Get().ComError("IWICFormatConverter::Initialize 失败", hr);
		return nullptr;
	}

	// 检查 D3D 纹理尺寸限制
	UINT width, height;
	hr = formatConverter->GetSize(&width, &height);
	if (FAILED(hr)) {
		Logger::Get().ComError("GetSize 失败", hr);
		return nullptr;
	}

	if (width > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION || height > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION) {
		Logger::Get().Error("图像尺寸超出限制");
		return nullptr;
	}

	UINT stride = width * (useFloatFormat ? 8 : 4);
	UINT size = stride * height;
	std::unique_ptr<BYTE[]> buf(new BYTE[size]);

	hr = formatConverter->CopyPixels(nullptr, stride, size, buf.get());
	if (FAILED(hr)) {
		Logger::Get().ComError("CopyPixels 失败", hr);
		return nullptr;
	}

	D3D11_SUBRESOURCE_DATA initData{
		.pSysMem = buf.get(),
		.SysMemPitch = stride
	};
	winrt::com_ptr<ID3D11Texture2D> result = DirectXHelper::CreateTexture2D(
		d3dDevice,
		useFloatFormat ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_R8G8B8A8_UNORM,
		width,
		height,
		D3D11_BIND_SHADER_RESOURCE,
		D3D11_USAGE_IMMUTABLE,
		0,
		&initData
	);
	if (!result) {
		Logger::Get().Error("创建纹理失败");
		return nullptr;
	}

	return result;
}

winrt::com_ptr<ID3D11Texture2D> TextureHelper::LoadTexture(const wchar_t* fileName, ID3D11Device* d3dDevice) noexcept {
	std::wstring_view sv(fileName);
	size_t npos = sv.find_last_of(L'.');
	if (npos == std::wstring_view::npos) {
		Logger::Get().Error("文件名无后缀名");
		return nullptr;
	}

	std::wstring_view suffix = sv.substr(npos + 1);

	if (suffix == L"dds") {
		return DDSHelper::Load(fileName, d3dDevice);
	}

	if (suffix == L"bmp" || suffix == L"jpg" || suffix == L"jpeg"
		|| suffix == L"png" || suffix == L"tif" || suffix == L"tiff"
	) {
		return LoadImg(fileName, d3dDevice);
	}

	return nullptr;
}

static bool SavePng(
	const wchar_t* fileName,
	uint32_t width,
	uint32_t height,
	std::span<uint8_t> pixelData,
	uint32_t rowPitch
) {
	// 初始化 WIC
	winrt::com_ptr<IWICImagingFactory2> wicFactory =
		winrt::try_create_instance<IWICImagingFactory2>(CLSID_WICImagingFactory);
	if (!wicFactory) {
		Logger::Get().Error("创建 WICImagingFactory2 失败");
		return false;
	}

	winrt::com_ptr<IWICBitmapEncoder> imgEncoder;
	HRESULT hr = wicFactory->CreateEncoder(GUID_ContainerFormatPng, nullptr, imgEncoder.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("IWICImagingFactory::CreateEncoder 失败", hr);
		return false;
	}

	{
		winrt::com_ptr<IWICStream> wicStream;
		hr = wicFactory->CreateStream(wicStream.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("IWICImagingFactory::CreateStream 失败", hr);
			return false;
		}

		hr = wicStream->InitializeFromFilename(fileName, GENERIC_WRITE);
		if (FAILED(hr)) {
			Logger::Get().ComError("IWICStream::InitializeFromFilename 失败", hr);
			return false;
		}

		hr = imgEncoder->Initialize(wicStream.get(), WICBitmapEncoderNoCache);
		if (FAILED(hr)) {
			Logger::Get().ComError("IWICBitmapEncoder::Initialize 失败", hr);
			return false;
		}
	}

	winrt::com_ptr<IWICBitmapFrameEncode> frameEncoder;
	hr = imgEncoder->CreateNewFrame(frameEncoder.put(), nullptr);
	if (FAILED(hr)) {
		Logger::Get().ComError("IWICBitmapEncoder::CreateNewFrame 失败", hr);
		return false;
	}

	hr = frameEncoder->Initialize(nullptr);
	if (FAILED(hr)) {
		Logger::Get().ComError("IWICBitmapFrameEncode::Initialize 失败", hr);
		return false;
	}

	hr = frameEncoder->SetSize(width, height);
	if (FAILED(hr)) {
		Logger::Get().ComError("IWICBitmapFrameEncode::SetSize 失败", hr);
		return false;
	}

	const WICPixelFormatGUID& srcFormat = GUID_WICPixelFormat32bppRGBA;
	WICPixelFormatGUID destFormat = srcFormat;
	hr = frameEncoder->SetPixelFormat(&destFormat);
	if (FAILED(hr)) {
		Logger::Get().ComError("IWICBitmapFrameEncode::SetPixelFormat 失败", hr);
		return false;
	}

	// 写入像素数据
	if (destFormat == srcFormat) {
		hr = frameEncoder->WritePixels(
			height,
			rowPitch,
			(UINT)pixelData.size(),
			pixelData.data()
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("IWICBitmapFrameEncode::WritePixels 失败", hr);
			return false;
		}
	} else {
		// 需要转换格式
		winrt::com_ptr<IWICBitmap> memBmp;
		hr = wicFactory->CreateBitmapFromMemory(
			width,
			height,
			srcFormat,
			rowPitch,
			(UINT)pixelData.size(),
			pixelData.data(),
			memBmp.put()
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("IWICImagingFactory::CreateBitmapFromMemory 失败", hr);
			return false;
		}

		winrt::com_ptr<IWICFormatConverter> formatConverter;
		hr = wicFactory->CreateFormatConverter(formatConverter.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("IWICImagingFactory::CreateFormatConverter 失败", hr);
			return false;
		}

		hr = formatConverter->Initialize(
			memBmp.get(),
			destFormat,
			WICBitmapDitherTypeNone,
			nullptr,
			0.0,
			WICBitmapPaletteTypeMedianCut
		);
		if (FAILED(hr)) {
			Logger::Get().ComError("IWICFormatConverter::Initialize 失败", hr);
			return false;
		}

		hr = frameEncoder->WriteSource(formatConverter.get(), nullptr);
		if (FAILED(hr)) {
			Logger::Get().ComError("IWICBitmapFrameEncode::WriteSource 失败", hr);
			return false;
		}
	}

	hr = frameEncoder->Commit();
	if (FAILED(hr)) {
		Logger::Get().ComError("IWICBitmapFrameEncode::Commit 失败", hr);
		return false;
	}

	hr = imgEncoder->Commit();
	if (FAILED(hr)) {
		Logger::Get().ComError("IWICBitmapEncoder::Commit 失败", hr);
		return false;
	}

	return true;
}

bool TextureHelper::SaveTexture(
	const wchar_t* fileName,
	uint32_t width,
	uint32_t height,
	EffectIntermediateTextureFormat format,
	std::span<uint8_t> pixelData,
	uint32_t rowPitch
) noexcept {
	if (std::wstring_view(fileName).ends_with(L".dds")) {
		DXGI_FORMAT dxgiFormat = EffectHelper::FORMAT_DESCS[(uint32_t)format].dxgiFormat;
		return DDSHelper::Save(fileName, width, height, dxgiFormat, pixelData, rowPitch);
	} else {
		assert(std::wstring_view(fileName).ends_with(L".png"));
		assert(format == EffectIntermediateTextureFormat::R8G8B8A8_UNORM);
		return SavePng(fileName, width, height, pixelData, rowPitch);
	}
}

}
