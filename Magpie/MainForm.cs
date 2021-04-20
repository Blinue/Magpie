using Gma.System.MouseKeyHook;
using Magpie.Properties;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;


namespace Magpie {
    partial class MainForm : Form {
        
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
    ""scale"": [0,0],
    ""ARStrength"": 0.7
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

        private IKeyboardMouseEvents keyboardEvents = null;
        private readonly MagWindow magWindow = new MagWindow();

        public MainForm() {
            InitializeComponent();

            // 加载设置
            txtHotkey.Text = Settings.Default.Hotkey;
            cbbScaleMode.SelectedIndex = Settings.Default.ScaleMode;
            ckbShowFPS.Checked = Settings.Default.ShowFPS;
            ckbNoVSync.Checked = Settings.Default.NoVSync;
            cbbInjectMode.SelectedIndex = Settings.Default.InjectMode;
            cbbCaptureMode.SelectedIndex = Settings.Default.CaptureMode;
            ckbLowLatencyMode.Checked = Settings.Default.LowLatencyMode;

        }

        protected override void WndProc(ref Message m) {
            if (m.Msg == NativeMethods.MAGPIE_WM_SHOWME) {
                // 收到 WM_SHOWME 激活窗口
                if (WindowState == FormWindowState.Minimized) {
                    Show();
                    WindowState = FormWindowState.Normal;
                }

                _ = NativeMethods.SetForegroundWindow(Handle);
            }
            base.WndProc(ref m);
        }

        private void MainForm_FormClosing(object sender, FormClosingEventArgs e) {
            magWindow.Destory();
            Settings.Default.Save();
        }

        private void TxtHotkey_TextChanged(object sender, EventArgs e) {
            keyboardEvents?.Dispose();
            keyboardEvents = Hook.GlobalEvents();

            string hotkey = txtHotkey.Text.Trim();

            try {
                keyboardEvents.OnCombination(new Dictionary<Combination, Action> {{
                    Combination.FromString(hotkey), () => {
                        string effectsJson = Settings.Default.ScaleMode == 0
                            ? CommonEffectJson : AnimeEffectJson;
                        bool showFPS = Settings.Default.ShowFPS;
                        bool noVSync = Settings.Default.NoVSync;
                        int captureMode = Settings.Default.CaptureMode;
                        bool lowLatencyMode = Settings.Default.LowLatencyMode;

                        if(magWindow.IsExist) {
                            magWindow.Destory();
                        } else {
                            magWindow.Create(
                                effectsJson,
                                captureMode,
                                showFPS,
                                lowLatencyMode,
                                noVSync,
                                cbbInjectMode.SelectedIndex == 1,
                                false
                            );
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

                magWindow.HookCursorAtStartUp(openFileDialog.FileName);
            } else {
                // 不保存启动时注入的选项
                Settings.Default.InjectMode = cbbInjectMode.SelectedIndex;
            }
        }

        private void CbbCaptureMode_SelectedIndexChanged(object sender, EventArgs e) {
            Settings.Default.CaptureMode = cbbCaptureMode.SelectedIndex;
        }


        private void CkbLowLatencyMode_CheckedChanged(object sender, EventArgs e) {
            Settings.Default.LowLatencyMode = ckbLowLatencyMode.Checked;
        }
    }
}


