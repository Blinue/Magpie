using EasyHook;
using Gma.System.MouseKeyHook;
using Magpie.CursorHook;
using Magpie.Properties;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Reflection;
using System.Runtime.Remoting;
using System.Windows.Forms;


namespace Magpie {
    public partial class MainForm : Form {
        public static readonly int WM_SHOWME = NativeMethods.RegisterWindowMessage("WM_SHOWME");

        private const string AnimeEffectJson = @"[
  {
    ""effect"": ""scale"",
    ""type"": ""Anime4KxDenoise""
  },
  {
    ""effect"": ""scale"",
    ""type"": ""mitchell"",
    ""scale"": [0,0],
    ""useSharperVersion"": true
  },
  {
    ""effect"": ""sharpen"",
    ""type"": ""adaptive"",
    ""curveHeight"": 0.2
  }
]";
        private const string CommonEffectJson = @"[
  {
    ""effect"": ""scale"",
    ""type"": ""lanczos6"",
    ""scale"": [0,0]
  },
  {
    ""effect"": ""sharpen"",
    ""type"": ""adaptive"",
    ""curveHeight"": 0.6
  },
  {
    ""effect"": ""sharpen"",
    ""type"": ""builtIn"",
    ""sharpness"": 0.5,
    ""threshold"": 0.5
  }
]";

        IKeyboardMouseEvents keyboardEvents = null;


        public MainForm() {
            InitializeComponent();

            // 加载设置
            txtHotkey.Text = Settings.Default.Hotkey;
            cbbScaleMode.SelectedIndex = Settings.Default.ScaleMode;
            ckbShowFPS.Checked = Settings.Default.ShowFPS;
            ckbNoVSync.Checked = Settings.Default.NoVSync;
            cbbInjectMode.SelectedIndex = Settings.Default.InjectMode;
            cbbCaptureMode.SelectedIndex = Settings.Default.CaptureMode;
        }

        protected override void WndProc(ref Message m) {
            if (m.Msg == WM_SHOWME) {
                // 收到 WM_SHOWME 激活窗口
                if (WindowState == FormWindowState.Minimized) {
                    Show();
                    WindowState = FormWindowState.Normal;
                }

                // 忽略错误
                NativeMethods.SetForegroundWindow(Handle);
            }
            base.WndProc(ref m);
        }

        private void MainForm_FormClosing(object sender, FormClosingEventArgs e) {
            NativeMethods.DestroyMagWindow();

            Settings.Default.Save();
        }

        private void TxtHotkey_TextChanged(object sender, EventArgs e) {
            keyboardEvents?.Dispose();
            keyboardEvents = Hook.GlobalEvents();

            string hotkey = txtHotkey.Text.Trim();

            try {
                keyboardEvents.OnCombination(new Dictionary<Combination, Action> {{
                    Combination.FromString(hotkey), () => {
                        string effectJson = Settings.Default.ScaleMode == 0
                            ? CommonEffectJson : AnimeEffectJson;
                        bool showFPS = Settings.Default.ShowFPS;
                        bool noVSync = Settings.Default.NoVSync;
                        int captureMode = Settings.Default.CaptureMode;

                        if(!NativeMethods.HasMagWindow()) {
                            if(!NativeMethods.CreateMagWindow(effectJson, captureMode, showFPS, noVSync, false)) {
                                MessageBox.Show("创建全屏窗口失败：" + NativeMethods.GetLastErrorMsg());
                                return;
                            }

                            if(cbbInjectMode.SelectedIndex == 1) {
                                HookCursorAtRuntime();
                            }
                        } else {
                            NativeMethods.DestroyMagWindow();
                        }
                    }
                }});

                txtHotkey.ForeColor = Color.Black;
                Settings.Default.Hotkey = hotkey;

                tsmiHotkey.Text = "热键：" + hotkey;
            } catch (ArgumentException) {
                txtHotkey.ForeColor = Color.Red;
            }
            
        }

        private void HookCursorAtRuntime() {
            IntPtr hwndSrc = NativeMethods.GetSrcWnd();
            int pid = NativeMethods.GetWindowProcessId(hwndSrc);
            if (pid == Process.GetCurrentProcess().Id) {
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
                NativeMethods.GetHostWnd(),
                hwndSrc
                );
            } catch (Exception e) {
                Console.WriteLine("CursorHook 注入失败：" + e.Message);
            }
        }

        private void HookCursorAtStartUp(string exePath) {
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

        private void CbbScaleMode_SelectedIndexChanged(object sender, EventArgs e) {
            Settings.Default.ScaleMode = cbbScaleMode.SelectedIndex;
        }

        private void MainForm_Resize(object sender, EventArgs e) {
            if(WindowState == FormWindowState.Minimized) {
                Hide();
                notifyIcon.Visible = true;
            } else {
                notifyIcon.Visible = false;
            }
        }

        private void TsmiMainForm_Click(object sender, EventArgs e) {
            Show();
            WindowState = FormWindowState.Normal;
        }

        private void TsmiExit_Click(object sender, EventArgs e) {
            Close();
        }

        private void NotifyIcon_MouseClick(object sender, MouseEventArgs e) {
            if (e.Button == MouseButtons.Left) {
                tsmiMainForm.PerformClick();
            }
        }

        private void CkbNoVSync_CheckedChanged(object sender, EventArgs e) {
            Settings.Default.NoVSync = ckbNoVSync.Checked;
        }

        private void CkbShowFPS_CheckedChanged(object sender, EventArgs e) {
            Settings.Default.ShowFPS = ckbShowFPS.Checked;
        }

        private void CbbInjectMode_SelectedIndexChanged(object sender, EventArgs e) {
            if(cbbInjectMode.SelectedIndex == 2) {
                // 启动时注入
                if(openFileDialog.ShowDialog() != DialogResult.OK) {
                    // 未选择文件，恢复原来的选项
                    cbbInjectMode.SelectedIndex = Settings.Default.InjectMode;
                    return;
                }

                HookCursorAtStartUp(openFileDialog.FileName);
            } else {
                // 不保存启动时注入的选项
                Settings.Default.InjectMode = cbbInjectMode.SelectedIndex;
            }
        }

        private void CbbCaptureMode_SelectedIndexChanged(object sender, EventArgs e) {
            Settings.Default.CaptureMode = cbbCaptureMode.SelectedIndex;
        }
    }
}
