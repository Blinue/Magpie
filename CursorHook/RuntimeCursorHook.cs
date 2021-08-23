using NLog;
using System;
using System.Threading;


namespace Magpie.CursorHook {
	// 运行时钩子
	internal class RuntimeCursorHook : CursorHookBase {
		private static Logger Logger { get; } = LogManager.GetCurrentClassLogger();

		public RuntimeCursorHook(IntPtr hwndSrc) {
			hwndHost = NativeMethods.FindWindow(HOST_WINDOW_CLASS_NAME, IntPtr.Zero);
			if (hwndHost == IntPtr.Zero) {
				Logger.Warn("未找到全屏窗口，将等待3秒");

				// 未找到全屏窗口，在 3 秒内多次尝试
				for (int i = 0; i < 10; ++i) {
					Thread.Sleep(300);

					hwndHost = NativeMethods.FindWindow(HOST_WINDOW_CLASS_NAME, IntPtr.Zero);
					if (hwndHost != IntPtr.Zero) {
						break;
					}
				}

				if (hwndHost == IntPtr.Zero) {
					Logger.Fatal("全屏窗口不存在，取消运行时注入");
					throw new Exception("全屏窗口不存在"); ;
				}
			}

			Logger.Info($"全屏窗口：{hwndSrc}");
			this.hwndSrc = hwndSrc;
		}

		public override void Run() {
			// 安装钩子
			EasyHook.LocalHook setCursorHook = null;
			try {
				// 截获 SetCursor
				setCursorHook = EasyHook.LocalHook.Create(
					EasyHook.LocalHook.GetProcAddress("user32.dll", "SetCursor"),
					new SetCursorDelegate(SetCursorHook),
					this
				);

				// 只 Hook 窗口线程，因为 SetCursor 必须在窗口线程上调用
				setCursorHook.ThreadACL.SetInclusiveACL(
					new int[] { NativeMethods.GetWindowThreadId(hwndSrc) }
				);
			} catch (Exception e) {
				// 安装失败，直接退出
				Logger.Fatal(e, "SetCursor 钩子安装失败");
				return;
			}

			Logger.Info("SetCursor 钩子安装成功");

			ReplaceHCursors();

			while (NativeMethods.IsWindow(hwndHost)) {
				Thread.Sleep(200);
			}

			Logger.Info("全屏窗口已关闭，即将卸载钩子");

			// 退出前重置窗口类的光标
			ReplaceHCursorsBack();

			// 卸载钩子
			setCursorHook.Dispose();
			EasyHook.LocalHook.Release();

			Logger.Info("已卸载钩子");
		}
	}
}
