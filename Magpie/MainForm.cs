using Gma.System.MouseKeyHook;
using Magpie.Properties;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;
using System.Text.Json;
using System.IO;


namespace Magpie {
    partial class MainForm : Form {
        private IKeyboardMouseEvents keyboardEvents = null;
        private readonly MagWindow magWindow = new MagWindow();

        // 倒计时时间
        private const int DOWN_COUNT = 5;
        private int countDownNum;

        private class ScaleModel {
            public string Name { get; set; }

            public JsonElement Model { get; set; }
        };

        private ScaleModel[] scaleModels;

        private const string SCALE_MODELS_JSON_PATH = "./ScaleModels.json";

        public MainForm() {
            InitializeComponent();
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
            ckbNoVSync.Checked = Settings.Default.NoVSync;
            cbbInjectMode.SelectedIndex = Settings.Default.InjectMode;
            cbbCaptureMode.SelectedIndex = Settings.Default.CaptureMode;
            ckbLowLatencyMode.Checked = Settings.Default.LowLatencyMode;
        }

        private void LoadScaleModels() {
            string json;
            if(File.Exists(SCALE_MODELS_JSON_PATH)) {
                json = File.ReadAllText(SCALE_MODELS_JSON_PATH);
            } else {
                File.WriteAllText(SCALE_MODELS_JSON_PATH, Resources.BuiltInScaleModels);
                json = Resources.BuiltInScaleModels;
            }

            try {
                scaleModels = JsonSerializer.Deserialize<ScaleModel[]>(
                    json,
                    new JsonSerializerOptions {
                        PropertyNameCaseInsensitive = true
                    }
                );
            } catch (Exception) {
                MessageBox.Show("读取 ScaleModel.json 失败");
                Environment.Exit(0);
            }

            if(scaleModels.Length == 0) {
                MessageBox.Show("非法的 ScaleModel.json");
                Environment.Exit(0);
            }

            foreach (var scaleModel in scaleModels) {
                cbbScaleMode.Items.Add(scaleModel.Name);
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

            string effectsJson = scaleModels[Settings.Default.ScaleMode].Model.GetRawText();
            bool showFPS = Settings.Default.ShowFPS;
            bool noVSync = Settings.Default.NoVSync;
            int captureMode = Settings.Default.CaptureMode;
            bool lowLatencyMode = Settings.Default.LowLatencyMode;

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

        private void StartScaleTimer() {
            if (timerScale.Enabled) {
                // 已经开始倒计时
                return;
            }

            countDownNum = DOWN_COUNT;
            btnScale.Text = countDownNum.ToString();
            btnScale.Enabled = false;

            timerScale.Interval = 1000;
            timerScale.Enabled = true;
        }

        private void BtnScale_Click(object sender, EventArgs e) {
            StartScaleTimer();
        }

        private void TimerScale_Tick(object sender, EventArgs e) {
            if(--countDownNum != 0) {
                btnScale.Text = countDownNum.ToString();
                return;
            }

            // 计时结束
            timerScale.Enabled = false;
            btnScale.Text = "5秒后放大";
            btnScale.Enabled = true;

            ToggleMagWindow();
        }

        private void TsmiScale_Click(object sender, EventArgs e) {
            StartScaleTimer();
        }
    }
}


