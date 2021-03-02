using System;
using System.Collections.Generic;
using System.Windows.Forms;
using Gma.System.MouseKeyHook;
using System.Drawing;
using Magpie.Properties;

namespace Magpie {
    public partial class MainForm : Form {
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
            if (m.Msg == NativeMethods.WM_SHOWME) {
                if (WindowState == FormWindowState.Minimized) {
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

        private void TkbFrameRate_Scroll(object sender, EventArgs e) {
            lblFrameRate.Text = tkbFrameRate.Value.ToString();
            Settings.Default.FrameRate = (uint)tkbFrameRate.Value;
        }

        private void TxtHotkey_TextChanged(object sender, EventArgs e) {
            keyboardEvents?.Dispose();
            keyboardEvents = Hook.GlobalEvents();

            try {
                keyboardEvents.OnCombination(new Dictionary<Combination, Action> {{
                    Combination.FromString(txtHotkey.Text.Trim()), () => {
                        uint frameRate = Settings.Default.FrameRate;
                        string effectJson = Settings.Default.ScaleMode == 0
                            ? Resources.CommonEffectJson : Resources.AnimeEffectJson;

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
                Settings.Default.Hotkey = txtHotkey.Text.Trim();
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
    }
}
