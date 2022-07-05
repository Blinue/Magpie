#pragma once
#include "pch.h"
#include <winrt/Magpie.Runtime.h>


namespace winrt::Magpie::App {

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

	Magpie::Runtime::MagSettings MagSettings() const noexcept {
		return _magSettings;
	}

private:
	std::wstring _name;

	std::wstring _pathRule;
	std::wstring _classNameRule;

	bool _isCroppingEnabled = false;

	Magpie::Runtime::MagSettings _magSettings;
};

}
