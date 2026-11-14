#pragma once

namespace Magpie {

struct RectHelper {
	static bool IsOverlap(const RectU& r1, const RectU& r2) noexcept {
		return r1.right > r2.left && r1.bottom > r2.top && r1.left < r2.right && r1.top < r2.bottom;
	}

	static bool Contains(const RectU& r1, const RectU& r2) noexcept {
		return r1.left <= r2.left && r1.top <= r2.top && r1.right >= r2.right && r1.bottom >= r2.bottom;
	}

	static bool Contains(const RectU& rect, PointU p) noexcept {
		return p.x >= rect.left && p.x < rect.right && p.y >= rect.top && p.y < rect.bottom;
	}

	static bool IsEmpty(const RectU& rect) noexcept {
		return rect.left == rect.right || rect.top == rect.bottom;
	}

	static bool Intersect(RectU& result, const RectU& r1, const RectU& r2) noexcept {
		// 计算重叠部分
		result.left = std::max(r1.left, r2.left);
		result.top = std::max(r1.top, r2.top);
		result.right = std::min(r1.right, r2.right);
		result.bottom = std::min(r1.bottom, r2.bottom);

		// 判断重叠部分是否是正面积
		return result.left < result.right && result.top < result.bottom;
	}

	static RectU Union(const RectU& r1, const RectU& r2) noexcept {
		return RectU{ std::min(r1.left, r2.left), std::min(r1.top, r2.top),
			std::max(r1.right, r2.right), std::max(r1.bottom, r2.bottom) };
	}

	static uint32_t CalcArea(const RectU& rect) noexcept {
		assert(rect.right >= rect.left && rect.bottom >= rect.top);
		return (rect.right - rect.left) * (rect.bottom - rect.top);
	}
};

}
