using EasyHook;
using Magpie.CursorHook;
using System;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Runtime.Remoting;
using System.Threading;


namespace Magpie {
    class MagWindow {
        private Thread magThread = null;

        public bool IsExist {
            get => magThread != null && magThread.IsAlive;
        }

        public void Create(
            string effectsJson,
            int captureMode,
            bool showFPS,
            bool lowLatencyMode,
            bool noVSync,
            bool hookCursorAtRuntime,
            bool noDisturb = false
        ) {
            if (IsExist) {
                Destory();
            }

            // 使用 WinRT Capturer API 需要在 MTA 中
            // C# 窗体必须使用 STA，因此将全屏窗口创建在新的线程里
            magThread = new Thread(() => {
                NativeMethods.RunMagWindow(
                    effectsJson,    // 缩放模式
                    captureMode,    // 抓取模式
                    showFPS,        // 显示 FPS
                    lowLatencyMode, // 低延迟模式
                    noVSync,        // 关闭垂直同步
                    noDisturb       // 用于调试
                );
            });
            magThread.SetApartmentState(ApartmentState.MTA);
            magThread.Start();

            if (hookCursorAtRuntime) {
                HookCursorAtRuntime();
            }
        }

        public void Destory() {
            NativeMethods.BroadcastMessage(NativeMethods.MAGPIE_WM_DESTORYMAG);
            if (magThread != null) {
                magThread.Abort();
                magThread = null;
            }
        }

        private void HookCursorAtRuntime() {
            IntPtr hwndSrc = NativeMethods.GetForegroundWindow();
            int pid = NativeMethods.GetWindowProcessId(hwndSrc);
            if (pid == 0 || pid == Process.GetCurrentProcess().Id) {
                // 不能 hook 本进程
                return;
            }

#if DEBUG
            string channelName = null;
            // DEBUG 时创建 IPC server
            RemoteHooking.IpcCreateServer<ServerInterface>(ref channelName, WellKnownObjectMode.Singleton);
#endif

            // 获取 CursorHook.dll 的绝对路径
            string injectionLibrary = Path.Combine(
                Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location),
                "CursorHook.dll"
            );

            // 使用 EasyHook 注入
            try {
                EasyHook.RemoteHooking.Inject(
                pid,                // 要注入的进程的 ID
                injectionLibrary,   // 32 位 DLL
                injectionLibrary,   // 64 位 DLL
                // 下面为传递给注入 DLL 的参数
#if DEBUG
                channelName,
#endif
                hwndSrc
                );
            } catch (Exception e) {
                Console.WriteLine("CursorHook 注入失败：" + e.Message);
            }
        }

        public void HookCursorAtStartUp(string exePath) {
#if DEBUG
            string channelName = null;
            // DEBUG 时创建 IPC server
            RemoteHooking.IpcCreateServer<ServerInterface>(ref channelName, WellKnownObjectMode.Singleton);
#endif

            // 获取 CursorHook.dll 的绝对路径
            string injectionLibrary = Path.Combine(
                Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location),
                "CursorHook.dll"
            );

            try {
                EasyHook.RemoteHooking.CreateAndInject(
                    exePath,    // 可执行文件路径
                    "",         // 命令行参数
                    0,          // 传递给 CreateProcess 的标志
                    injectionLibrary,   // 32 位 DLL
                    injectionLibrary,   // 64 位 DLL
                    out int _  // 忽略进程 ID
                               // 下面为传递给注入 DLL 的参数
#if DEBUG
                    , channelName
#endif
                );
            } catch (Exception e) {
                Console.WriteLine("CursorHook 注入失败：" + e.Message);
            }
        }
    }
}
