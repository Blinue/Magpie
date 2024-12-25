#pragma once
#include "CandidateWindowItem.g.h"

namespace winrt::Magpie::implementation {

struct CandidateWindowItem : CandidateWindowItemT<CandidateWindowItem>,
                             wil::notify_property_changed_base<CandidateWindowItem> {
    CandidateWindowItem(HWND hWnd);

	hstring Title() const noexcept {
		return _title;
	}

	IconElement Icon() const noexcept;

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
	fire_and_forget _ResolveWindow(bool resolveIcon, bool resolveName, HWND hWnd);

	hstring _title;
	IconElement _icon{ nullptr };
	hstring _defaultProfileName;

	hstring _aumid;
	hstring _path;
	hstring _className;
};

}
