#pragma once
#include "CandidateWindowItem.g.h"

namespace winrt::Magpie::App::implementation {

struct CandidateWindowItem : CandidateWindowItemT<CandidateWindowItem> {
    CandidateWindowItem(uint64_t hWnd, uint32_t dpi, bool isLightTheme, CoreDispatcher const& dispatcher);

	event_token PropertyChanged(PropertyChangedEventHandler const& handler) {
		return _propertyChangedEvent.add(handler);
	}

	void PropertyChanged(event_token const& token) noexcept {
		_propertyChangedEvent.remove(token);
	}

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

	event<PropertyChangedEventHandler> _propertyChangedEvent;

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
