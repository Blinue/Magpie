#include "pch.h"
#include "ScreenshotHelper.h"
#include "Logger.h"
#include <wincodec.h>

namespace Magpie {

bool ScreenshotHelper::SavePng(
	uint32_t width,
	uint32_t height,
	std::span<uint8_t> pixelData,
	uint32_t rowPitch,
	const wchar_t* fileName
) noexcept {
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

	const WICPixelFormatGUID srcFormat = GUID_WICPixelFormat32bppRGBA;
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

}
