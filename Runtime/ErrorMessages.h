#pragma once
#include "pch.h"


struct ErrorMessages {
	static constexpr const char* GENERIC = "Msg_Error_Generic";
	static constexpr const char* INVALID_SOURCE_WINDOW = "Msg_Error_Invalid_Source_Window";
	static constexpr const char* VSYNC_OFF_NOT_SUPPORTED = "Msg_Error_Vsync_Off_Not_Supported";
	static constexpr const char* SRC_TOO_LARGE = "Msg_Error_Src_Too_Large";
	static constexpr const char* FAILED_TO_CROP = "Msg_Error_Failed_To_Crop";
	static constexpr const char* FAILED_TO_CAPTURE = "Msg_Error_Failed_To_Capture";
};
