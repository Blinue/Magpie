#include "pch.h"
#include <map>


class KeyBoardHook {
public:
	KeyBoardHook() = delete;

	static bool hook(std::initializer_list<int> keys) {
		if (isHooked()) {
			unHook();
		}

		_hHook = SetWindowsHookEx(WH_KEYBOARD_LL, _hookProc, NULL, NULL);
		if (_hHook == NULL) {
			return false;
		}

		for (int i : keys) {
			_keyStates[i] = false;
		}

		return true;
	}

	static bool isHooked() {
		return _hHook != NULL;
	}

	static bool unHook() {
		return UnhookWindowsHookEx(_hHook);
	}

	static std::vector<int> getPressedKeys() {
		std::vector<int> _result;
		for (const auto& pair : _keyStates) {
			if (pair.second) {
				_result.push_back(pair.first);
			}
		}
		return std::move(_result);
	}

	static void setKeyDownCallback(std::function<void(int key)> callback) {
		_downCallback = callback;
	}

	static void setKeyUpCallback(std::function<void(int key)> callback) {
		_upCallback = callback;
	}
private:
	static LRESULT CALLBACK _hookProc(int code, WPARAM wParam, LPARAM lParam) {
		if (code >= 0) {
			int key = ((KBDLLHOOKSTRUCT*)lParam)->vkCode;
			if (_keyStates.find(key) != _keyStates.end()) {
				if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
					_keyStates[key] = true;

					if (_downCallback) {
						_downCallback(key);
					}
				} else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
					_keyStates[key] = false;

					if (_upCallback) {
						_upCallback(key);
					}
				}
			}
		}

		return CallNextHookEx(_hHook, code, wParam, lParam);
	}

	static std::map<int, bool> _keyStates;
	static HHOOK _hHook;
	static std::function<void(int key)> _downCallback;
	static std::function<void(int key)> _upCallback;
};