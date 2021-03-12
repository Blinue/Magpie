using System;
using System.Collections.Generic;
using System.Windows.Forms;
using Gma.System.MouseKeyHook;
using System.Drawing;
using Magpie.Properties;

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
            if (Settings.Default.FrameRate == 0) {
                ckbMaxFrameRate.Checked = true;
            } else {
                ckbMaxFrameRate.Checked = false;
                tkbFrameRate.Value = (int)Settings.Default.FrameRate;
            }
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
                        uint frameRate = Settings.Default.FrameRate;
                        string effectJson = Settings.Default.ScaleMode == 0
                            ? CommonEffectJson : AnimeEffectJson;

                        if(!NativeMethods.HasMagWindow()) {
                            if(!NativeMethods.CreateMagWindow(frameRate, effectJson, false)) {
                                MessageBox.Show("创建全屏窗口失败：" + NativeMethods.GetLastErrorMsg());
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

        private void CkbMaxFrameRate_CheckedChanged(object sender, EventArgs e) {
            tkbFrameRate.Enabled = !ckbMaxFrameRate.Checked;
            lblFrameRate.Enabled = !ckbMaxFrameRate.Checked;
            Settings.Default.FrameRate =
                ckbMaxFrameRate.Checked ? 0 : (uint)tkbFrameRate.Value;
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
            if(e.Button == MouseButtons.Left) {
                tsmiMainForm.PerformClick();
            }
        }

        private void TkbFrameRate_ValueChanged(object sender, EventArgs e) {
            lblFrameRate.Text = tkbFrameRate.Value.ToString();
            Settings.Default.FrameRate = (uint)tkbFrameRate.Value;
        }
    }
}
