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

        private static readonly string AnimeEffectJson = @"[
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
        private static readonly string CommonEffectJson = @"[
  {
    ""effect"": ""scale"",
    ""type"": ""jinc2"",
    ""scale"": [0,0],
    ""windowSinc"": 0.35,
    ""sinc"": 0.825,
    ""ARStrength"": 0.7
  },
  {
    ""effect"": ""sharpen"",
    ""type"": ""adaptive"",
    ""curveHeight"": 0.3
  }
]";

        IKeyboardMouseEvents keyboardEvents = null;

        public MainForm() {
            InitializeComponent();

            // 加载设置
            txtHotkey.Text = Settings.Default.Hotkey;
            cbbScaleMode.SelectedIndex = Settings.Default.ScaleMode;
            ckbShowFPS.Checked = Settings.Default.showFPS;
            ckbNoVSync.Checked = Settings.Default.noVSync;
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
                        bool showFPS = Settings.Default.showFPS;
                        bool noVSync = Settings.Default.noVSync;

                        if(!NativeMethods.HasMagWindow()) {
                            if(!NativeMethods.CreateMagWindow(effectJson, showFPS, noVSync, false)) {
                                MessageBox.Show("创建全屏窗口失败：" + NativeMethods.GetLastErrorMsg());
                                return;
                            }

                            HookCursor();
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

        private void HookCursor() {
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
            IntPtr hwndSrc = NativeMethods.GetSrcWnd();
            int pid = NativeMethods.GetWindowProcessId(hwndSrc);
            if(pid == Process.GetCurrentProcess().Id) {
                // 不能 hook 本进程
                return;
            }

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
            Settings.Default.noVSync = ckbNoVSync.Checked;
        }

        private void CkbShowFPS_CheckedChanged(object sender, EventArgs e) {
            Settings.Default.showFPS = ckbShowFPS.Checked;
        }
    }
}
