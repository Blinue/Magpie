#include "pch.h"
#include "ScreenshotHelper.h"
#include "Logger.h"
#include "StrHelper.h"
#include <wincodec.h>
#include <charconv>

namespace Magpie {

static bool ExtractNumber(std::wstring_view fileName, std::string& numStr, uint32_t& curNum) noexcept {
	assert(fileName.size() > 11);
	size_t len = fileName.size() - 11;
	numStr.resize(len);

	for (size_t i = 0; i < len; ++i) {
		wchar_t c = fileName[i + 7];
		if (c >= L'0' && c <= L'9') {
			numStr[i] = (char)c;
		} else {
			return false;
		}
	}

	return std::from_chars(numStr.c_str(), numStr.c_str() + numStr.size(), curNum).ec == std::errc{};
}

uint32_t ScreenshotHelper::FindUnusedScreenshotNum(const std::filesystem::path& screenshotsDir) noexcept {
	WIN32_FIND_DATA findData{};
	const std::wstring pattern = StrHelper::Concat(screenshotsDir.native(), L"\\Magpie_*.png");
	wil::unique_hfind hFind(FindFirstFileEx(
		pattern.c_str(), FindExInfoBasic, &findData, FindExSearchNameMatch, nullptr, FIND_FIRST_EX_LARGE_FETCH));
	if (!hFind) {
		Logger::Get().Win32Error("FindFirstFileEx 失败");
		return 0;
	}

	// 新截图应在所有现有截图之后，因此查找最大序号。如果最大序号是 UINT_MAX 则回落
	// 到查找最小的可用序号，不过这种数据除了特意构造不可能出现
	uint32_t result = 0;

	std::string numStr;
	std::vector<uint32_t> nums;
	bool shouldFindMin = false;
	do {
		uint32_t curNum;
		if (!ExtractNumber(findData.cFileName, numStr, curNum) || curNum == 0) {
			continue;
		}

		nums.push_back(curNum);

		if (shouldFindMin) {
			continue;
		}
		
		if (curNum == std::numeric_limits<uint32_t>::max()) {
			// 回落到查找最小的可用序号
			shouldFindMin = true;
			continue;
		}

		result = std::max(result, curNum + 1);
	} while (FindNextFile(hFind.get(), &findData));

	if (!shouldFindMin) {
		return result;
	}

	// 查找最小的可用序号
	std::sort(nums.begin(), nums.end());
	assert(nums.back() == std::numeric_limits<uint32_t>::max());

	if (nums[0] != 1) {
		return 1;
	}

	const size_t size = nums.size();
	for (size_t i = 1; i < size; ++i) {
		result = nums[i - 1] + 1;
		if (nums[i] > result) {
			return result;
		}
	}

	// 不可能执行到这里
	return 0;
}

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
