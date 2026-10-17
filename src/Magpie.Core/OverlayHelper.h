#pragma once
#include <imgui.h>

namespace Magpie {

struct OverlayHelper {
	struct SegoeIcons {
		static const ImWchar Cancel = 0xE711;
		static const ImWchar Camera = 0xE722;
		static const ImWchar Favicon = 0xE737;
		static const ImWchar Remove = 0xE738;
		static const ImWchar CheckboxIndeterminate = 0xE73C;
		static const ImWchar FullScreen = 0xE740;
		static const ImWchar Pinned = 0xE840;
		static const ImWchar Diagnostic = 0xE9D9;
#ifdef _DEBUG
		static const ImWchar Design = 0xEB3C;
#endif
	};

	static constexpr const ImColor TIMELINE_COLORS[] = {
		{229,57,53,255},
		{156,39,176,255},
		{63,81,181,255},
		{30,136,229,255},
		{0,137,123,255},
		{121,85,72,255},
		{117,117,117,255}
	};
};

}
