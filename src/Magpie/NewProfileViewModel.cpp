#include "pch.h"
#include "NewProfileViewModel.h"
#if __has_include("NewProfileViewModel.g.cpp")
#include "NewProfileViewModel.g.cpp"
#endif
#include "AppSettings.h"
#include "Win32Helper.h"
#include <Psapi.h>
#include "ProfileService.h"
#include "AppXReader.h"
#include "CommonSharedConstants.h"

using namespace Magpie::Core;

namespace winrt::Magpie::implementation {

static bool IsCandidateWindow(HWND hWnd) noexcept {
	// 跳过不可见的窗口
	if (!IsWindowVisible(hWnd)) {
		return false;
	}

	// 跳过不接受点击的窗口
	if (GetWindowLongPtr(hWnd, GWL_EXSTYLE) & WS_EX_TRANSPARENT) {
		return false;
	}

	RECT frameRect{};

	HRESULT hr = DwmGetWindowAttribute(hWnd,
		DWMWA_EXTENDED_FRAME_BOUNDS, &frameRect, sizeof(frameRect));
	if (FAILED(hr)) {
		return false;
	}

	SIZE frameSize = Win32Helper::GetSizeOfRect(frameRect);
	if (frameSize.cx < 50 && frameSize.cy < 50) {
		return false;
	}

	// 标题不能为空
	if (Win32Helper::GetWndTitle(hWnd).empty()) {
		return false;
	}

	// 排除后台 UWP 窗口
	// https://stackoverflow.com/questions/43927156/enumwindows-returns-closed-windows-store-applications
	UINT isCloaked{};
	DwmGetWindowAttribute(hWnd, DWMWA_CLOAKED, &isCloaked, sizeof(isCloaked));
	if (isCloaked != 0) {
		return false;
	}

	std::wstring className = Win32Helper::GetWndClassName(hWnd);
	if (className == L"Progman" ||					// Program Manager
		className == L"Xaml_WindowedPopupClass"		// 主机弹出窗口
	) {
		return false;
	}

	// 检查是否和已有配置重复
	AppXReader appxReader;
	if (appxReader.Initialize(hWnd)) {
		return ProfileService::Get().TestNewProfile(true, appxReader.AUMID(), className);
	} else {
		std::wstring fileName = Win32Helper::GetPathOfWnd(hWnd);
		if (fileName.empty()) {
			return false;
		}

		return ProfileService::Get().TestNewProfile(false, fileName, className);
	}
}

static SmallVector<HWND> GetDesktopWindows() noexcept {
	SmallVector<HWND> windows;
	
	// EnumWindows 可以枚举到 UWP 窗口，官方文档已经过时。无法枚举到全屏状态下的 UWP 窗口
	EnumWindows(
		[](HWND hWnd, LPARAM lParam) {
			if (IsCandidateWindow(hWnd)) {
				((SmallVector<HWND>*)lParam)->push_back(hWnd);
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
	std::sort(begin, end, [&col](Magpie::CandidateWindowItem const& l, Magpie::CandidateWindowItem const& r) {
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
	RaisePropertyChanged(L"CandidateWindows");
	RaisePropertyChanged(L"IsNoCandidateWindow");
	RaisePropertyChanged(L"IsAnyCandidateWindow");

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
	profiles.push_back(box_value(ResourceLoader::GetForCurrentView(
		CommonSharedConstants::APP_RESOURCE_MAP_ID).GetString(L"Root_Defaults/Content")));
	for (const Profile& profile : AppSettings::Get().Profiles()) {
		profiles.push_back(box_value(profile.name));
	}

	_profiles = single_threaded_vector(std::move(profiles));
	RaisePropertyChanged(L"Profiles");

	ProfileIndex(0);
}

void NewProfileViewModel::CandidateWindowIndex(int value) {
	_candidateWindowIndex = value;
	RaisePropertyChanged(L"CandidateWindowIndex");

	if (value >= 0) {
		Name(_candidateWindows.GetAt(value).as<CandidateWindowItem>().DefaultProfileName());
	} else {
		Name({});
	}
}

void NewProfileViewModel::Name(const hstring& value) noexcept {
	_name = value;
	RaisePropertyChanged(L"Name");

	_IsConfirmButtonEnabled(!value.empty() && _candidateWindowIndex >= 0);
}

void NewProfileViewModel::Confirm() const noexcept {
	if (_candidateWindowIndex < 0 || _name.empty()) {
		return;
	}

	CandidateWindowItem selectedItem = _candidateWindows.GetAt(_candidateWindowIndex).as<CandidateWindowItem>();
	hstring aumid = selectedItem.AUMID();
	ProfileService::Get().AddProfile(!aumid.empty(), aumid.empty() ? selectedItem.Path() : aumid,
		selectedItem.ClassName(), _name, _profileIndex - 1);
}

}
