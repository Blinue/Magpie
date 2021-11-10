#include "pch.h"
#include "ImageReizer.h"


inline const BYTE* SampleNearest(const BYTE* img, SIZE size, POINTF pt) {
	pt.x = std::clamp(pt.x, 0.0f, (float)size.cy - 1);
	pt.y = std::clamp(pt.y, 0.0f, (float)size.cx - 1);

	return img + ((int)pt.x * size.cx + (int)pt.y) * 4;
}

void ImageReizer::Run(const BYTE* srcImg, SIZE srcSize, BYTE* destImg, SIZE destSize, FilterType filter) {
	assert(srcImg && destImg);
	assert(srcSize.cx > 0 && srcSize.cy > 0 && destSize.cx > 0 && destSize.cy > 0);

	float scalePtX = (float)srcSize.cx / destSize.cx;
	float scalePtY = (float)srcSize.cy / destSize.cy;
	for (int i = 0; i < destSize.cy; ++i) {
		for (int j = 0; j < destSize.cx; ++j) {
			*(INT*)destImg = *(INT*)SampleNearest(srcImg, srcSize,
				{ (i + 0.5f) * scalePtY, (j + 0.5f) * scalePtX });
			destImg += 4;
		}
	}
}
