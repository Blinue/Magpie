#pragma once
#include "pch.h"
#include <winrt/Magpie.Runtime.h>


namespace winrt::Magpie::App {

enum class CursorScaling {
	x0_5,
	x0_75,
	NoScaling,
	x1_25,
	x1_5,
	x2,
	Source,
	Custom
};

// 默认规则 Name、PathRule、ClassNameRule 均为空
class ScalingProfile {
public:
	const std::wstring& Name() const noexcept{
		return _name;
	}

	void Name(std::wstring_view value) noexcept {
		_name = value;
	}

	const std::wstring& PathRule() const noexcept {
		return _pathRule;
	}

	void PathRule(std::wstring_view value) noexcept {
		_pathRule = value;
	}

	bool IsPackaged() const noexcept {
		return _isPackaged;
	}

	void IsPackaged(bool value) noexcept {
		_isPackaged = value;
	}

	const std::wstring& ClassNameRule() const noexcept {
		return _classNameRule;
	}

	void ClassNameRule(std::wstring_view value) noexcept {
		_classNameRule = value;
	}

	bool IsCroppingEnabled() const noexcept {
		return _isCroppingEnabled;
	}

	void IsCroppingEnabled(bool value) noexcept {
		_isCroppingEnabled = value;
	}

	CursorScaling CursorScaling() const noexcept {
		return _cursorScaling;
	}

	void CursorScaling(Magpie::App::CursorScaling value) noexcept {
		_cursorScaling = value;
	}

	double CustomCursorScaling() const noexcept {
		return _customCursorScaling;
	}

	void CustomCursorScaling(double value) noexcept {
		_customCursorScaling = value;
	}

	Magpie::Runtime::MagSettings MagSettings() const noexcept {
		return _magSettings;
	}

private:
	std::wstring _name;

	// 若为打包应用，PathRule 存储 AUMID
	bool _isPackaged = false;
	std::wstring _pathRule;
	std::wstring _classNameRule;

	bool _isCroppingEnabled = false;
	Magpie::App::CursorScaling _cursorScaling = CursorScaling::NoScaling;
	double _customCursorScaling = 1.0;

	Magpie::Runtime::MagSettings _magSettings;
};

}
