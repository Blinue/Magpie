#include "pch.h"
#include "DirtyRectsOptimizer.h"
#include "DebugInfo.h"
#include "RectHelper.h"

namespace Magpie {

static bool IsCornerInRect(PointU p, const RectU& r) noexcept {
	return p.x >= r.left && p.x <= r.right && p.y >= r.top && p.y <= r.bottom;
}

static bool OptimizeDirtyRectPair(RectU& rect1, RectU& rect2, bool reversed = false) noexcept {
	if (RectHelper::IsEmpty(rect1) || RectHelper::IsEmpty(rect2)) {
		return false;
	}

	// 计算 rect2 有几个角在 rect1 内
	bool lt = IsCornerInRect(PointU{ rect2.left, rect2.top }, rect1);
	bool rt = IsCornerInRect(PointU{ rect2.right, rect2.top }, rect1);
	bool rb = IsCornerInRect(PointU{ rect2.right, rect2.bottom }, rect1);
	bool lb = IsCornerInRect(PointU{ rect2.left, rect2.bottom }, rect1);
	uint32_t count = (uint32_t)lt + (uint32_t)rt + (uint32_t)rb + (uint32_t)lb;

	if (count <= 1) {
		if (!reversed) {
			// 尝试反向
			return OptimizeDirtyRectPair(rect2, rect1, true);
		}

		if (count == 0) {
			// 有小间隙也合并，因为检查重复帧使用 16x16 分块，而且多余的像素因为纹理缓存复制代价很小
			constexpr uint32_t MERGE_THRESHOLD = DUP_FRAME_DISPATCH_BLOCK_SIZE / 2;

			if (rect1.top == rect2.top && rect1.bottom == rect2.bottom) {
				if (rect1.right < rect2.left) {
					if (rect1.right + MERGE_THRESHOLD >= rect2.left) {
						// rect2 合并进 rect1
						rect1.right = rect2.right;
						rect2.right = rect2.left;
						return true;
					}
				} else {
					assert(rect1.left > rect2.right);
					if (rect2.right + MERGE_THRESHOLD >= rect1.left) {
						rect1.left = rect2.left;
						rect2.right = rect2.left;
						return true;
					}
				}
			} else if (rect1.left == rect2.left && rect1.right == rect2.right) {
				if (rect1.bottom < rect2.top) {
					if (rect1.bottom + MERGE_THRESHOLD >= rect2.top) {
						rect1.bottom = rect2.bottom;
						rect2.right = rect2.left;
						return true;
					}
				} else {
					assert(rect1.top > rect2.bottom);
					if (rect2.bottom + MERGE_THRESHOLD >= rect1.top) {
						rect1.top = rect2.top;
						rect2.right = rect2.left;
						return true;
					}
				}
			}
		}
	} else if (count == 2) {
		// rect2 有两个角在 rect1 内时可以合并或裁剪
		if (lt) {
			if (rt) {
				if (rect2.left == rect1.left && rect2.right == rect1.right) {
					// rect2 合并进 rect1
					rect1.bottom = rect2.bottom;
					rect2.right = rect2.left;
					return true;
				} else if (rect2.top != rect1.bottom) {
					// 裁剪 rect2
					rect2.top = rect1.bottom;
					assert(rect2.bottom >= rect2.top);
					return true;
				}
			} else {
				assert(lb);
				if (rect2.top == rect1.top && rect2.bottom == rect1.bottom) {
					rect1.right = rect2.right;
					rect2.right = rect2.left;
					return true;
				} else if (rect2.left != rect1.right) {
					rect2.left = rect1.right;
					assert(rect2.right >= rect2.left);
					return true;
				}
			}
		} else {
			assert(rb);
			if (rt) {
				if (rect2.top == rect1.top && rect2.bottom == rect1.bottom) {
					rect1.left = rect2.left;
					rect2.right = rect2.left;
					return true;
				} else if (rect2.right != rect1.left) {
					rect2.right = rect1.left;
					assert(rect2.right >= rect2.left);
					return true;
				}
			} else {
				if (rect2.left == rect1.left && rect2.right == rect1.right) {
					rect1.top = rect2.top;
					rect2.right = rect2.left;
					return true;
				} else if (rect2.bottom != rect1.top) {
					rect2.bottom = rect1.top;
					assert(rect2.bottom >= rect2.top);
					return true;
				}
			}
		}
	} else if (count == 4) {
		// rect2 在 rect1 内
		rect2.right = rect2.left;
		return true;
	}

	return false;
}

static void BasicOptimize(SmallVectorImpl<RectU>& dirtyRects) noexcept {
	// 持续循环直到不再能优化
	while (true) {
		const uint32_t count = (uint32_t)dirtyRects.size();
		assert(count > 0);

		bool optimized = false;
		for (uint32_t i = 0; i < count; ++i) {
			for (uint32_t j = i + 1; j < count; ++j) {
				if (OptimizeDirtyRectPair(dirtyRects[i], dirtyRects[j])) {
					optimized = true;
				}
			}
		}

		if (!optimized) {
			return;
		}

		// 从后向前删除空矩形
		for (int i = int(count - 1); i >= 0; --i) {
			const RectU& rect = dirtyRects[i];
			if (RectHelper::IsEmpty(rect)) {
				dirtyRects.erase(dirtyRects.begin() + i);
			}
		}
	}
}

static uint32_t CalcTotalPixels(const SmallVectorImpl<RectU>& rects) noexcept {
	uint32_t result = 0;
	for (const RectU& rect : rects) {
		result += RectHelper::CalcArea(rect);
	}
	return result;
}

#ifdef MP_DEBUG_INFO
// 验证优化算法的正确性
static void ValidateOptimize(
	const SmallVectorImpl<RectU>& originRects,
	const SmallVectorImpl<RectU>& newRects
) noexcept {
	if (originRects.empty()) {
		return;
	}

	std::vector<bool> pixels;
	for (const RectU& originRect : originRects) {
		// 作为优化先检查有没有被优化后的某个矩形包含
		bool contained = false;
		for (const RectU& newRect : newRects) {
			if (RectHelper::Contains(newRect, originRect)) {
				contained = true;
				break;
			}
		}
		if (contained) {
			continue;
		}

		// 可能被多个矩形共同包含，需要逐像素检查
		const uint32_t originWidth = originRect.right - originRect.left;
		pixels.assign(size_t((originRect.bottom - originRect.top) * originWidth), false);

		for (RectU newRect : newRects) {
			if (!RectHelper::Intersect(newRect, newRect, originRect)) {
				continue;
			}

			for (uint32_t i = newRect.top; i < newRect.bottom; ++i) {
				uint32_t start = (i - originRect.top) * originWidth;
				std::fill(pixels.begin() + size_t(start + newRect.left - originRect.left),
					pixels.begin() + size_t(start + newRect.right - originRect.left), true);
			}
		}

		if (std::find(pixels.begin(), pixels.end(), false) != pixels.end()) {
			OutputDebugString(L"优化脏矩形算法错误！\n");
			// 打印脏矩形供调试
			for (const RectU& rect : originRects) {
				OutputDebugString(fmt::format(L"{},{},{},{}\n",
					rect.left, rect.top, rect.right, rect.bottom).c_str());
			}
			return;
		}
	}
}
#endif

void DirtyRectsOptimizer::Execute(SmallVectorImpl<RectU>& dirtyRects) noexcept {
	uint32_t rectCount = (uint32_t)dirtyRects.size();
	if (rectCount <= 1) {
		return;
	}

#ifdef MP_DEBUG_INFO
	auto se = wil::scope_exit(std::bind_front(ValidateOptimize, DEBUG_INFO.validateDirtyRectsOptimizer ?
		SmallVector<RectU>(dirtyRects.begin(), dirtyRects.end()) : SmallVector<RectU>(), std::ref(dirtyRects)));
#endif

	if (rectCount <= MAX_CAPTURE_DIRTY_RECT_COUNT * 4) {
		BasicOptimize(dirtyRects);
		rectCount = (uint32_t)dirtyRects.size();
	}

	// 深度优化的复杂度为 n^4，输入矩形数量太多时应削减。花太多时间优化脏矩形是得不偿失的
	constexpr uint32_t DEEP_OPTIMIZE_LIMIT = MAX_CAPTURE_DIRTY_RECT_COUNT * 2;
	if (rectCount > DEEP_OPTIMIZE_LIMIT) {
		RectU& lastRect = dirtyRects[DEEP_OPTIMIZE_LIMIT - 1];
		for (auto it = dirtyRects.begin() + DEEP_OPTIMIZE_LIMIT; it != dirtyRects.end(); ++it) {
			lastRect = RectHelper::Union(lastRect, *it);
		}
		dirtyRects.erase(dirtyRects.begin() + DEEP_OPTIMIZE_LIMIT, dirtyRects.end());

		BasicOptimize(dirtyRects);
		rectCount = (uint32_t)dirtyRects.size();
	}

	if (rectCount == 1) {
		return;
	}

	uint32_t totalPixels = CalcTotalPixels(dirtyRects);
	
	while (true) {
		uint32_t minTotalPixels = std::numeric_limits<uint32_t>::max();
		uint32_t targetRectCount = 0;
		bool targetCanOptimize = false;
		uint32_t targetIdx1 = 0;
		uint32_t targetIdx2 = 0;
		// 遍历所有的两两合并找出总像素数最少的
		for (uint32_t i = 0; i < rectCount; ++i) {
			for (uint32_t j = i + 1; j < rectCount; ++j) {
				const RectU& rect1 = dirtyRects[i];
				const RectU& rect2 = dirtyRects[j];

				// 两个矩形必须相交才有优化的可能，但脏矩形数量过多时需要强制合并
				if (!RectHelper::IsOverlap(rect1, rect2) && rectCount <= MAX_CAPTURE_DIRTY_RECT_COUNT) {
					continue;
				}

				RectU unionedRect = RectHelper::Union(rect1, rect2);
				uint32_t newTotalPixels = 0;
				uint32_t newRectCount = 0;
				bool optimized = false;

				// 这里只优化一轮而不是调用 OptimizeDirtyRects，既降低复杂度又能避免堆分配
				for (uint32_t k = 0; k < rectCount; ++k) {
					if (k == i || k == j) {
						continue;
					}

					RectU curRect = dirtyRects[k];
					if (OptimizeDirtyRectPair(curRect, unionedRect)) {
						optimized = true;
					}

					if (!RectHelper::IsEmpty(curRect)) {
						newTotalPixels += RectHelper::CalcArea(curRect);
						++newRectCount;
					}
				}

				if (!RectHelper::IsEmpty(unionedRect)) {
					newTotalPixels += RectHelper::CalcArea(unionedRect);
					++newRectCount;
				}

				if (newTotalPixels < minTotalPixels ||
					(newTotalPixels == minTotalPixels && newRectCount < targetRectCount)) {
					minTotalPixels = newTotalPixels;
					targetRectCount = newRectCount;
					targetCanOptimize = optimized;
					targetIdx1 = i;
					targetIdx2 = j;
				}
			}
		}

		// 总像素数持平也采用，因为脏矩形数量减少了
		if (minTotalPixels > totalPixels && rectCount <= MAX_CAPTURE_DIRTY_RECT_COUNT) {
			return;
		}

		assert(targetIdx1 < targetIdx2);
		dirtyRects[targetIdx1] = RectHelper::Union(dirtyRects[targetIdx1], dirtyRects[targetIdx2]);
		dirtyRects.erase(dirtyRects.begin() + targetIdx2);

		if (targetCanOptimize) {
			BasicOptimize(dirtyRects);
			totalPixels = CalcTotalPixels(dirtyRects);
		} else {
			totalPixels = minTotalPixels;
		}

		rectCount = (uint32_t)dirtyRects.size();
		if (rectCount == 1) {
			return;
		}
	}
}

#ifdef _DEBUG
static Ignore _ = [] {
	auto rectComp = [](const RectU& l, const RectU& r) {
		return std::tuple(l.left, l.top, l.right, l.bottom) <
			std::tuple(r.left, r.top, r.right, r.bottom);
	};

	SmallVector<RectU, 0> dirtyRects;
	dirtyRects.reserve(16);

	dirtyRects.emplace_back(0, 0, 2, 2);
	dirtyRects.emplace_back(1, 1, 3, 4);
	dirtyRects.emplace_back(2, 1, 4, 3);
	dirtyRects.emplace_back(0, 1, 3, 2);
	dirtyRects.emplace_back(3, 3, 4, 4);
	BasicOptimize(dirtyRects);
	std::sort(dirtyRects.begin(), dirtyRects.end(), rectComp);
	assert(dirtyRects.size() == 2);
	assert((dirtyRects[0] == RectU{ 0, 0, 2, 2 }));
	assert((dirtyRects[1] == RectU{ 1, 1, 4, 4 }));
	
	dirtyRects.clear();
	dirtyRects.emplace_back(0, 0, 1, 1);
	dirtyRects.emplace_back(0, 0, 2, 2);
	BasicOptimize(dirtyRects);
	assert(dirtyRects.size() == 1);
	assert((dirtyRects[0] == RectU{ 0, 0, 2, 2 }));
	
	return Ignore();
}();
#endif

}
