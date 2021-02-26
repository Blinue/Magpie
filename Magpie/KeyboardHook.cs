using System;
using System.Collections.Generic;
using System.Windows.Forms;
using System.Linq;
using System.Diagnostics;

namespace Magpie {
	// 注册全局键盘钩子
	// 使用原生 API 太麻烦
	public class KeyboardHook {
		// 保存关注的按键的状态
		readonly Dictionary<Keys, bool> keyStates = new Dictionary<Keys, bool>();

		public event KeyEventHandler KeyDown;
		public event KeyEventHandler KeyUp;
		
		private Action unhookCallback;

		public bool IsHooked { get => unhookCallback != null; }

		public IEnumerable<Keys> PressedKeys {
			get => keyStates
				.Where(pair => pair.Value)
				.Select(pair => pair.Key);
		}

		~KeyboardHook() {
			Unhook();
		}


		public bool IsKeyDown(Keys key) {
			if (!keyStates.TryGetValue(key, out bool down)) {
				return false;
			}
			return down;
		}

		// 注册钩子
		public void Hook(IEnumerable<Keys> keys) {
			if (IsHooked) {
				Unhook();
			}

			foreach (var key in keys) {
				keyStates[key] = false;
			}

			Win32.SetGlobalKeyboardHook((Win32.WM_ENUM wparam, ref Win32.KBDLLHOOKSTRUCT lparam) => {
				Keys key = (Keys)lparam.vkCode;

				if (!keyStates.ContainsKey(key)) {
					return;
				}

				KeyEventArgs args = new KeyEventArgs(key);
				
				if (wparam == Win32.WM_ENUM.WM_KEYDOWN || wparam == Win32.WM_ENUM.WM_SYSKEYDOWN) {
					keyStates[key] = true;
                    KeyDown?.Invoke(this, args);
                } else if (wparam == Win32.WM_ENUM.WM_KEYUP || wparam == Win32.WM_ENUM.WM_SYSKEYUP) {
					keyStates[key] = false;
                    KeyUp?.Invoke(this, args);
                }
			});
		}

		// 释放钩子
		public void Unhook() {
			if (!IsHooked) {
				return;
			}

			unhookCallback();
			unhookCallback = null;

			keyStates.Clear();
		}
	}
}
