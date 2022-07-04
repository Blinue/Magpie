#include "pch.h"
#include "HomePage.h"
#if __has_include("HomePage.g.cpp")
#include "HomePage.g.cpp"
#endif
#include "StrUtils.h"
#include "MagService.h"


namespace winrt::Magpie::App::implementation {

HomePage::HomePage() {
	InitializeComponent();

	MagService& magService = MagService::Get();

	_wndToRestoreChangedRevoker = magService.WndToRestoreChanged(
		auto_revoke,
		{ this, &HomePage::_MagService_WndToRestoreChanged}
	);

	_UpdateAutoRestoreState();
}

void HomePage::_MagService_WndToRestoreChanged(uint64_t) {
	_UpdateAutoRestoreState();
}

void HomePage::_UpdateAutoRestoreState() {
	HWND wndToRestore = (HWND)MagService::Get().WndToRestore();
	if (wndToRestore) {
		std::wstring title(GetWindowTextLength(wndToRestore), L'\0');
		GetWindowText(wndToRestore, title.data(), (int)title.size() + 1);

		AutoRestoreExpandedSettingItem().Title(
			StrUtils::ConcatW(L"当前窗口：", title.empty() ? L"<标题为空>" : title));
	}
}

}
