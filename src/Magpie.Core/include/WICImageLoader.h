#pragma once
#include <wincodec.h>

namespace Magpie {

struct WICImageLoader {
	static winrt::com_ptr<IWICBitmapSource> LoadFromFile(
		const wchar_t* fileName,
		WICPixelFormatGUID targetFormat,
		bool& isSRGB
	) noexcept;
};

}
