using NLog;
using System;
using System.Threading;


namespace Magpie.CursorHook {
	// 启动时钩子
	internal class StartUpCursorHook : CursorHookBase {
		private static Logger Logger { get; } = LogManager.GetCurrentClassLogger();

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

				// Hook 除当前线程的所有线程，因为此时窗口线程未知
				setCursorHook.ThreadACL.SetExclusiveACL(new int[] { 0 });
			} catch (Exception e) {
				// 安装失败，直接退出
				Logger.Fatal(e, "安装钩子失败");
				return;
			}

			Logger.Info("SetCursor 钩子安装成功");

			// 启动时注入完成，唤醒注入进程
			EasyHook.RemoteHooking.WakeUpProcess();

			// 启动时注入永远不卸载钩子
			while (true) {
				// 查找全屏窗口
				IntPtr hwndHost = NativeMethods.FindWindow(HOST_WINDOW_CLASS_NAME, IntPtr.Zero);
				if (hwndHost != IntPtr.Zero) {
					if (hwndHost != base.hwndHost) {
						Logger.Info("检测到全屏窗口");

						base.hwndHost = hwndHost;
						// hwndSrc 为前台窗口
						hwndSrc = NativeMethods.GetForegroundWindow();

						ReportCursorMap();

						ReplaceHCursors();
					}
				} else if (base.hwndHost != IntPtr.Zero) {
					Logger.Info("全屏窗口已关闭");
					base.hwndHost = IntPtr.Zero;

					ReplaceHCursorsBack();
				}

				Thread.Sleep(200);
			}
		}
	}
}
