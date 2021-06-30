#pragma once
#include <guiddef.h>


// {A9DF8B5B-B5C5-478F-B5B3-BC5C48E96EF5}
DEFINE_GUID(GUID_MAGPIE_MONOCHROME_CURSOR_SHADER,
    0xa9df8b5b, 0xb5c5, 0x478f, 0xb5, 0xb3, 0xbc, 0x5c, 0x48, 0xe9, 0x6e, 0xf5);

// {BF2F648B-CE26-4177-ACE1-CC0FB3DA9F52}
DEFINE_GUID(CLSID_MAGPIE_MONOCHROME_CURSOR_EFFECT,
    0xbf2f648b, 0xce26, 0x4177, 0xac, 0xe1, 0xcc, 0xf, 0xb3, 0xda, 0x9f, 0x52);


constexpr auto MAGPIE_MONOCHROME_CURSOR_SHADER = L"shaders/MonochromeCursorShader.cso";
