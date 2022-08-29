#include "pch.h"
#include "NewProfileViewModel.h"
#if __has_include("NewProfileViewModel.g.cpp")
#include "NewProfileViewModel.g.cpp"
#endif
#include "AppSettings.h"
#include "Win32Utils.h"
#include <Psapi.h>
#include "ScalingProfileService.h"
#include "AppXReader.h"


namespace winrt::Magpie::UI::implementation {

static bool IsCandidateWindow(HWND hWnd) {
	if (!IsWindowVisible(hWnd)) {
		return false;
	}

	RECT frameRect;
	if (!Win32Utils::GetWindowFrameRect(hWnd, frameRect)) {
		return false;
	}

	SIZE frameSize = Win32Utils::GetSizeOfRect(frameRect);
	if (frameSize.cx < 50 && frameSize.cy < 50) {
		return false;
	}

	// 标题不能为空
	if (Win32Utils::GetWndTitle(hWnd).empty()) {
		return false;
	}

	// 排除后台 UWP 窗口
	// https://stackoverflow.com/questions/43927156/enumwindows-returns-closed-windows-store-applications
	UINT isCloaked{};
	DwmGetWindowAttribute(hWnd, DWMWA_CLOAKED, &isCloaked, sizeof(isCloaked));
	if (isCloaked != 0) {
		return false;
	}

	std::wstring className = Win32Utils::GetWndClassName(hWnd);
	if (className == L"Progman" ||					// Program Manager
		className == L"Xaml_WindowedPopupClass"		// 主机弹出窗口
	) {
		return false;
	}

	DWORD dwProcId = 0;
	if (!GetWindowThreadProcessId(hWnd, &dwProcId)) {
		return false;
	}

	Win32Utils::ScopedHandle hProc(Win32Utils::SafeHandle(OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwProcId)));
	if (!hProc) {
		// 权限不足
		return false;
	}

	// 检查是否和已有配置重复
	AppXReader appxReader;
	if (appxReader.Initialize(hWnd)) {
		return ScalingProfileService::Get().TestNewProfile(true, appxReader.AUMID(), className);
	} else {
		std::wstring fileName(MAX_PATH, 0);
		DWORD size = GetModuleFileNameEx(hProc.get(), NULL, fileName.data(), (DWORD)fileName.size() + 1);
		if (size == 0) {
			return false;
		}
		fileName.resize(size);

		return ScalingProfileService::Get().TestNewProfile(false, fileName, className);
	}
}

static std::vector<HWND> GetDesktopWindows() {
	std::vector<HWND> windows;
	windows.reserve(1024);

	// EnumWindows 能否枚举到 UWP 窗口？
	// 虽然官方文档中明确指出不能，但我在 Win10/11 中测试是可以的
	// PowerToys 也依赖这个行为 https://github.com/microsoft/PowerToys/blob/d4b62d8118d49b2cc83c2a2126091378d0b5fec4/src/modules/launcher/Plugins/Microsoft.Plugin.WindowWalker/Components/OpenWindows.cs
	// 无法枚举到全屏状态下的 UWP 窗口
	EnumWindows(
		[](HWND hWnd, LPARAM lParam) {
			std::vector<HWND>& windows = *(std::vector<HWND>*)lParam;

			if (IsCandidateWindow(hWnd)) {
				windows.push_back(hWnd);
			}

			return TRUE;
		},
		(LPARAM)&windows
	);

	return windows;
}

template<typename It>
static void SortCandidateWindows(It begin, It end) {
	const std::collate<wchar_t>& col = std::use_facet<std::collate<wchar_t> >(std::locale());

	// 根据用户的区域设置排序，对于中文为拼音顺序
	std::sort(begin, end, [&col](Magpie::UI::CandidateWindowItem const& l, Magpie::UI::CandidateWindowItem const& r) {
		const hstring& titleL = l.Title();
		const hstring& titleR = r.Title();

		// 忽略大小写，忽略半/全角
		return CompareStringEx(
			LOCALE_NAME_USER_DEFAULT,
			NORM_IGNORECASE | NORM_IGNOREWIDTH | NORM_LINGUISTIC_CASING,
			titleL.c_str(), titleL.size(),
			titleR.c_str(), titleR.size(),
			nullptr, nullptr, 0
		) == CSTR_LESS_THAN;
	});
}

void NewProfileViewModel::PrepareForOpen(uint32_t dpi, bool isLightTheme, CoreDispatcher const& dispatcher) {
	std::vector<CandidateWindowItem> candidateWindows;
	for (HWND hWnd : GetDesktopWindows()) {
		candidateWindows.emplace_back((uint64_t)hWnd, dpi, isLightTheme, dispatcher);
	}

	SortCandidateWindows(candidateWindows.begin(), candidateWindows.end());

	std::vector<IInspectable> items;
	items.reserve(candidateWindows.size());
	std::copy(candidateWindows.begin(), candidateWindows.end(), std::insert_iterator(items, items.begin()));
	_candidateWindows = single_threaded_vector(std::move(items));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CandidateWindows"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsNoCandidateWindow"));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"IsAnyCandidateWindow"));

	CandidateWindowIndex(-1);
	if (_candidateWindows.Size() == 1) {
		_candidateWindows.GetAt(0)
			.as<CandidateWindowItem>()
			.PropertyChanged([this](IInspectable const&, PropertyChangedEventArgs const& args) {
			if (args.PropertyName() == L"DefaultProfileName") {
				CandidateWindowIndex(0);
				return;
			}
		});
	}

	std::vector<IInspectable> profiles;
	profiles.push_back(box_value(L"默认"));
	for (const ScalingProfile& profile : AppSettings::Get().ScalingProfiles()) {
		profiles.push_back(box_value(profile.name));
	}

	_profiles = single_threaded_vector(std::move(profiles));
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"Profiles"));

	ProfileIndex(0);
}

void NewProfileViewModel::CandidateWindowIndex(int32_t value) {
	_candidateWindowIndex = value;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"CandidateWindowIndex"));

	if (value >= 0) {
		Name(_candidateWindows.GetAt(value).as<CandidateWindowItem>().DefaultProfileName());
	} else {
		Name({});
	}
}

void NewProfileViewModel::Name(const hstring& value) noexcept {
	_name = value;
	_propertyChangedEvent(*this, PropertyChangedEventArgs(L"Name"));

	_IsConfirmButtonEnabled(!value.empty() && _candidateWindowIndex >= 0);
}

bool NewProfileViewModel::IsNotRunningAsAdmin() const noexcept {
	return !Win32Utils::IsProcessElevated();
}

void NewProfileViewModel::Confirm() const noexcept {
	if (_candidateWindowIndex < 0 || _name.empty()) {
		return;
	}

	CandidateWindowItem selectedItem = _candidateWindows.GetAt(_candidateWindowIndex).as<CandidateWindowItem>();
	hstring aumid = selectedItem.AUMID();
	ScalingProfileService::Get().AddProfile(!aumid.empty(), aumid.empty() ? selectedItem.Path() : aumid,
		selectedItem.ClassName(), _name, _profileIndex - 1);
}

}
