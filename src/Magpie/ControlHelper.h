#pragma once
#include "XamlHelper.h"

namespace Magpie {

// 用于修复 WinUI 控件存在的问题。因为官方毫无作为，我不得不使用这些 hack
struct ControlHelper {
	// 修复 ComboBox 下拉框的主题和位置
	static void ComboBox_DropDownOpened(const winrt::IInspectable& sender);

	// 设置 NumberBox 内部 TextBox 的右键菜单
	static void NumberBox_Loaded(const winrt::IInspectable& sender);
};

}
