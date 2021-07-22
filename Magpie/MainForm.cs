using Gma.System.MouseKeyHook;
using Magpie.Properties;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;
using Newtonsoft.Json.Linq;
using System.IO;
using System.Linq;


namespace Magpie {
    internal partial class MainForm : Form {
        private IKeyboardMouseEvents keyboardEvents = null;
        private readonly MagWindow magWindow;

        // 倒计时时间
        private const int DOWN_COUNT = 5;
        private int countDownNum;

        private (string Name, string Model)[] scaleModels;

        private const string SCALE_MODELS_JSON_PATH = "./ScaleModels.json";

        public MainForm() {
            InitializeComponent();

            magWindow = new MagWindow(this);
        }

        private void MainForm_Load(object sender, EventArgs e) {
            LoadScaleModels();

            // 加载设置
            txtHotkey.Text = Settings.Default.Hotkey;

            if (Settings.Default.ScaleMode >= cbbScaleMode.Items.Count) {
                Settings.Default.ScaleMode = 0;
            }
            cbbScaleMode.SelectedIndex = Settings.Default.ScaleMode;
            ckbShowFPS.Checked = Settings.Default.ShowFPS;
            cbbInjectMode.SelectedIndex = Settings.Default.InjectMode;
            cbbCaptureMode.SelectedIndex = Settings.Default.CaptureMode;
        }

        private void LoadScaleModels() {
            string json;
            if (File.Exists(SCALE_MODELS_JSON_PATH)) {
                json = File.ReadAllText(SCALE_MODELS_JSON_PATH);
            } else {
                File.WriteAllText(SCALE_MODELS_JSON_PATH, Resources.BuiltInScaleModels);
                json = Resources.BuiltInScaleModels;
            }

            try {
                scaleModels = JArray.Parse(json)
                     .Select(t => {
                         string name = t["name"]?.ToString();
                         string model = t["model"]?.ToString();
                         return name == null || model == null ? throw new Exception() : (name, model);
                     })
                     .ToArray();

                if (scaleModels.Length == 0) {
                    throw new Exception();
                }
            } catch (Exception) {
                _ = MessageBox.Show("非法的 ScaleModel.json");
                Environment.Exit(0);
            }

            foreach ((string Name, string Model) scaleModel in scaleModels) {
                _ = cbbScaleMode.Items.Add(scaleModel.Name);
            }
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
                    Combination.FromString(hotkey), () => ToggleMagWindow()
                }});

                txtHotkey.ForeColor = Color.Black;
                Settings.Default.Hotkey = hotkey;

                tsmiHotkey.Text = "热键：" + hotkey;
            } catch (ArgumentException) {
                txtHotkey.ForeColor = Color.Red;
            }
        }

        private void ToggleMagWindow() {
            if (magWindow.Status == MagWindowStatus.Starting) {
                return;
            } else if (magWindow.Status == MagWindowStatus.Running) {
                magWindow.Destory();
                return;
            }

            string effectsJson = scaleModels[Settings.Default.ScaleMode].Model;
            bool showFPS = Settings.Default.ShowFPS;
            int captureMode = Settings.Default.CaptureMode;

            magWindow.Create(
                effectsJson,
                captureMode,
                showFPS,
                cbbInjectMode.SelectedIndex == 1,
                false
            );
        }

        private void CbbScaleMode_SelectedIndexChanged(object sender, EventArgs e) {
            Settings.Default.ScaleMode = cbbScaleMode.SelectedIndex;
        }

        private void MainForm_Resize(object sender, EventArgs e) {
            if (WindowState == FormWindowState.Minimized) {
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

        private void CkbShowFPS_CheckedChanged(object sender, EventArgs e) {
            Settings.Default.ShowFPS = ckbShowFPS.Checked;
        }

        private void CbbInjectMode_SelectedIndexChanged(object sender, EventArgs e) {
            if (cbbInjectMode.SelectedIndex == 2) {
                // 启动时注入
                if (openFileDialog.ShowDialog() != DialogResult.OK) {
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

        private void StartScaleTimer() {
            countDownNum = DOWN_COUNT;
            tsmiScale.Text = btnScale.Text = countDownNum.ToString();

            timerScale.Interval = 1000;
            timerScale.Start();
        }

        private void StopScaleTimer() {
            timerScale.Stop();
            tsmiScale.Text = btnScale.Text = "5秒后放大";
        }

        private void ToggleScaleTimer() {
            if (timerScale.Enabled) {
                StopScaleTimer();
            } else {
                StartScaleTimer();
            }
        }

        private void BtnScale_Click(object sender, EventArgs e) {
            ToggleScaleTimer();
        }

        private void TimerScale_Tick(object sender, EventArgs e) {
            if (--countDownNum != 0) {
                tsmiScale.Text = btnScale.Text = countDownNum.ToString();
                return;
            }

            // 计时结束
            StopScaleTimer();

            ToggleMagWindow();
        }

        private void TsmiScale_Click(object sender, EventArgs e) {
            ToggleScaleTimer();
        }
    }
}


