using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;


namespace Magpie.CursorHook {
    // 运行时钩子
    class RuntimeCursorHook : CursorHookBase {
        public RuntimeCursorHook(IntPtr hwndSrc, IpcServer server) : base(server) {
            hwndHost = NativeMethods.FindWindow(HOST_WINDOW_CLASS_NAME, IntPtr.Zero);
            if (hwndHost == IntPtr.Zero) {
                throw new Exception("无法找到全屏窗口");
            }

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
                ReportIfFalse(false, "安装钩子失败：" + e.Message);
                return;
            }

            ReportToServer("SetCursor钩子安装成功");

            ReplaceHCursors();

            while (NativeMethods.IsWindow(hwndHost)) {
                SendMessages();
                Thread.Sleep(200);
            }

            // 退出前重置窗口类的光标
            ReplaceHCursorsBack();

            // 卸载钩子
            setCursorHook.Dispose();
            EasyHook.LocalHook.Release();
        }
    }
}
