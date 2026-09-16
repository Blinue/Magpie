#include "pch.h"
#include "WICImageLoader.h"
#include "Logger.h"
#include <propkey.h>

namespace Magpie {

static void QueryExifInfos(IWICBitmapFrameDecode* frame, bool& isSRGB, uint16_t& orientation) noexcept {
	winrt::com_ptr<IWICMetadataQueryReader> metaReader;
	HRESULT hr = frame->GetMetadataQueryReader(metaReader.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("IWICBitmapFrameDecode::GetMetadataQueryReader 失败", hr);
		return;
	}

	GUID containerFormat;
	hr = metaReader->GetContainerFormat(&containerFormat);
	if (FAILED(hr)) {
		Logger::Get().ComError("IWICMetadataQueryReader::GetContainerFormat 失败", hr);
		return;
	}

	wil::unique_prop_variant value;

	// 检索色域的代码参考自
	// https://github.com/microsoft/DirectXTK12/blob/3fdbcb5a6b73994a52c0b0d7a0c5743287004dad/Src/WICTextureLoader.cpp#L350-L417
	if (containerFormat == GUID_ContainerFormatPng) {
		if (SUCCEEDED(metaReader->GetMetadataByName(L"/sRGB/RenderingIntent", &value)) && value.vt == VT_UI1) {
			isSRGB = true;
		} else if (SUCCEEDED(metaReader->GetMetadataByName(L"/gAMA/ImageGamma", &value)) && value.vt == VT_UI4) {
			isSRGB = value.uintVal == 45455;
		}
	} else {
		if (SUCCEEDED(metaReader->GetMetadataByName(L"System.Image.ColorSpace", &value)) && value.vt == VT_UI2) {
			isSRGB = value.uiVal == 1;
		}
	}

	// 检索旋转信息
	if (value.vt != VT_EMPTY) {
		PropVariantClear(&value);
	}
	if (SUCCEEDED(metaReader->GetMetadataByName(L"System.Photo.Orientation", &value)) && value.vt == VT_UI2) {
		orientation = value.uiVal;
	}
}

winrt::com_ptr<IWICBitmapSource> WICImageLoader::LoadFromFile(
	const wchar_t* fileName,
	WICPixelFormatGUID targetFormat,
	bool& isSRGB
) noexcept {
	winrt::com_ptr<IWICImagingFactory2> wicImgFactory =
		winrt::try_create_instance<IWICImagingFactory2>(CLSID_WICImagingFactory);
	if (!wicImgFactory) {
		Logger::Get().Error("创建 WICImagingFactory 失败");
		return nullptr;
	}

	// 读取图像文件
	winrt::com_ptr<IWICBitmapDecoder> decoder;
	HRESULT hr = wicImgFactory->CreateDecoderFromFilename(
		fileName, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, decoder.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("IWICImagingFactory::CreateDecoderFromFilename 失败", hr);
		return nullptr;
	}

	winrt::com_ptr<IWICBitmapFrameDecode> frame;
	hr = decoder->GetFrame(0, frame.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("IWICBitmapDecoder::GetFrame 失败", hr);
		return nullptr;
	}

	// 默认视为 sRGB
	isSRGB = true;
	uint16_t orientation = PHOTO_ORIENTATION_NORMAL;
	QueryExifInfos(frame.get(), isSRGB, orientation);

	winrt::com_ptr<IWICBitmapSource> source = std::move(frame);

	// 处理旋转
	if (orientation >= 2 && orientation <= 8) {
		// 来自 https://github.com/microsoft/Win2D/blob/25680382dd2136779e10ea6084f0c5ba437ae288/winrt/lib/images/CanvasBitmap.cpp#L209
		WICBitmapTransformOptions transformOptions;
		switch (orientation) {
		case PHOTO_ORIENTATION_ROTATE90:
			transformOptions = WICBitmapTransformRotate270;
			break;
		case PHOTO_ORIENTATION_ROTATE180:
			transformOptions = WICBitmapTransformRotate180;
			break;
		case PHOTO_ORIENTATION_ROTATE270:
			transformOptions = WICBitmapTransformRotate90;
			break;
		case PHOTO_ORIENTATION_FLIPHORIZONTAL:
			transformOptions = WICBitmapTransformFlipHorizontal;
			break;
		case PHOTO_ORIENTATION_FLIPVERTICAL:
			transformOptions = WICBitmapTransformFlipVertical;
			break;
		case PHOTO_ORIENTATION_TRANSPOSE:
			transformOptions = WICBitmapTransformOptions(WICBitmapTransformRotate270 | WICBitmapTransformFlipHorizontal);
			break;
		default:
			transformOptions = WICBitmapTransformOptions(WICBitmapTransformRotate90 | WICBitmapTransformFlipHorizontal);
			break;
		}
		
		// 先将图片数据读到内存，否则旋转会非常慢
		winrt::com_ptr<IWICBitmap> memBitmap;
		hr = wicImgFactory->CreateBitmapFromSource(source.get(), WICBitmapCacheOnDemand, memBitmap.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("IWICImagingFactory::CreateBitmapFromSource 失败", hr);
			return nullptr;
		}

		winrt::com_ptr<IWICBitmapFlipRotator> flipRotator;
		hr = wicImgFactory->CreateBitmapFlipRotator(flipRotator.put());
		if (FAILED(hr)) {
			Logger::Get().ComError("IWICImagingFactory::CreateBitmapFlipRotator 失败", hr);
			return nullptr;
		}

		hr = flipRotator->Initialize(memBitmap.get(), transformOptions);
		if (FAILED(hr)) {
			Logger::Get().ComError("IWICBitmapFlipRotator::Initialize 失败", hr);
			return nullptr;
		}

		source = std::move(flipRotator);
	}

	WICPixelFormatGUID sourceWicFormat;
	hr = source->GetPixelFormat(&sourceWicFormat);
	if (FAILED(hr)) {
		Logger::Get().ComError("IWICBitmapSource::GetPixelFormat 失败", hr);
		return nullptr;
	}

	if (sourceWicFormat == targetFormat) {
		return std::move(source);
	}

	// 转换格式
	winrt::com_ptr<IWICFormatConverter> formatConverter;
	hr = wicImgFactory->CreateFormatConverter(formatConverter.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("IWICImagingFactory::CreateFormatConverter 失败", hr);
		return nullptr;
	}

	hr = formatConverter->Initialize(source.get(), targetFormat,
		WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom);
	if (FAILED(hr)) {
		Logger::Get().ComError("IWICFormatConverter::Initialize 失败", hr);
		return nullptr;
	}

	return std::move(formatConverter);
}

}
