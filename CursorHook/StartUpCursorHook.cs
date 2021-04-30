using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace Magpie.CursorHook {
    // 启动时钩子
    class StartUpCursorHook : CursorHookBase {
        public StartUpCursorHook(IpcServer server): base(server) {
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

                // Hook 除当前线程的所有线程，因为此时窗口线程未知
                setCursorHook.ThreadACL.SetExclusiveACL(new int[] { 0 });
            } catch (Exception e) {
                // 安装失败，直接退出
                ReportIfFalse(false, "安装钩子失败：" + e.Message);
                return;
            }

            ReportToServer("SetCursor钩子安装成功");


            // 启动时注入完成，唤醒注入进程
            EasyHook.RemoteHooking.WakeUpProcess();

            // 启动时注入永远不卸载钩子
            while (true) {
                // 查找全屏窗口
                IntPtr hwndHost = NativeMethods.FindWindow(HOST_WINDOW_CLASS_NAME, IntPtr.Zero);
                if (hwndHost != IntPtr.Zero) {
                    if (hwndHost != base.hwndHost) {
                        ReportToServer("检测到全屏窗口");

                        base.hwndHost = hwndHost;
                        // hwndSrc 为前台窗口
                        hwndSrc = NativeMethods.GetForegroundWindow();

                        ReportCursorMap();

                        ReplaceHCursors();
                    }
                } else {
                    if (base.hwndHost != IntPtr.Zero) {
                        ReportToServer("全屏窗口已关闭");
                        base.hwndHost = IntPtr.Zero;

                        ReplaceHCursorsBack();
                    }
                }

                SendMessages();
                Thread.Sleep(200);
            }
        }
    }
}
