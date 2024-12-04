#pragma once
#include "KeyVisualState.g.h"

namespace winrt::Magpie::implementation {

// 用于 ShortcutControl 和 ShortcutDialog 绑定
struct KeyVisualState : KeyVisualStateT<KeyVisualState> {
    KeyVisualState(int key, bool isError) : _key(key), _isError(isError) {}

    int Key() const noexcept { return _key; }
    bool IsError() const noexcept { return _isError; }

private:
    int _key;
    bool _isError;
};

}

namespace winrt::Magpie::factory_implementation {

struct KeyVisualState : KeyVisualStateT<KeyVisualState, implementation::KeyVisualState> {
};

}
