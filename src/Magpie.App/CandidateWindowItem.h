#pragma once
#include "CandidateWindowItem.g.h"

namespace winrt::Magpie::App::implementation {

struct CandidateWindowItem : CandidateWindowItemT<CandidateWindowItem>,
                             wil::notify_property_changed_base<CandidateWindowItem> {
    CandidateWindowItem(uint64_t hWnd, uint32_t dpi, bool isLightTheme, CoreDispatcher const& dispatcher);

	hstring Title() const noexcept {
		return _title;
	}

	Controls::IconElement Icon() const noexcept;

	hstring DefaultProfileName() const noexcept {
		return _defaultProfileName;
	}

	hstring AUMID() const noexcept {
		return _aumid;
	}

	hstring Path() const noexcept {
		return _path;
	}

	hstring ClassName() const noexcept {
		return _className;
	}

private:
	fire_and_forget _ResolveWindow(bool resolveIcon, bool resolveName, HWND hWnd, bool isLightTheme, uint32_t dpi, CoreDispatcher dispatcher);

	hstring _title;
	Controls::IconElement _icon{ nullptr };
	hstring _defaultProfileName;

	hstring _aumid;
	hstring _path;
	hstring _className;
};

}

namespace winrt::Magpie::App::factory_implementation {

struct CandidateWindowItem : CandidateWindowItemT<CandidateWindowItem, implementation::CandidateWindowItem> {
};

}
