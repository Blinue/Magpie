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

	IInspectable Icon() const noexcept;

	hstring DefaultProfileName() const noexcept {
		return _defaultProfileName;
	}

private:
	fire_and_forget _ResolveWindow(bool resolveIcon, bool resolveName);

	void _SetDefaultIcon();

	IAsyncAction _SetSoftwareBitmapIconAsync(Windows::Graphics::Imaging::SoftwareBitmap const& iconBitmap);

	void _SetIconPath(std::wstring_view iconPath);

	event<PropertyChangedEventHandler> _propertyChangedEvent;

	hstring _aumid;
	uint64_t _hWnd = 0;
	hstring _title;
	IInspectable _icon{ nullptr };
	hstring _defaultProfileName;

	bool _isLightTheme = false;
	uint32_t _dpi = 96;
	CoreDispatcher _dispatcher{ nullptr };
};

}

namespace winrt::Magpie::App::factory_implementation {

struct CandidateWindowItem : CandidateWindowItemT<CandidateWindowItem, implementation::CandidateWindowItem> {
};

}
