using Gma.System.MouseKeyHook;
using Microsoft.Win32;
using Newtonsoft.Json.Linq;
using Magpie.Properties;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Interop;
using System.Windows.Threading;
using System.Windows.Media;

namespace Magpie {
    /// <summary>
    /// MainWindow.xaml 的交互逻辑
    /// </summary>
    public partial class MainWindow : Window {
        private readonly OpenFileDialog openFileDialog = new OpenFileDialog();
        private readonly DispatcherTimer timerScale = new DispatcherTimer();

        private IKeyboardMouseEvents keyboardEvents = null;
        private MagWindow magWindow;

        // 倒计时时间
        private const int DOWN_COUNT = 5;
        private int countDownNum;

        private (string Name, string Model)[] scaleModels;

        private const string SCALE_MODELS_JSON_PATH = "./ScaleModels.json";

        private IntPtr Handle;

        public MainWindow() {
            InitializeComponent();

            timerScale.Tick += TimerScale_Tick;
            timerScale.Interval = new TimeSpan(0, 0, 1);

            LoadScaleModels();

            // 加载设置
            txtHotkey.Text = Settings.Default.Hotkey;

            if (Settings.Default.ScaleMode >= cbbScaleMode.Items.Count) {
                Settings.Default.ScaleMode = 0;
            }
            cbbScaleMode.SelectedIndex = Settings.Default.ScaleMode;
            cbbInjectMode.SelectedIndex = Settings.Default.InjectMode;
            cbbCaptureMode.SelectedIndex = Settings.Default.CaptureMode;
        }

        private void TimerScale_Tick(object sender, EventArgs e) {
            if (--countDownNum != 0) {
                cmiScale.Header = btnScale.Content = countDownNum.ToString();
                return;
            }

            // 计时结束
            StopScaleTimer();

            ToggleMagWindow();
        }

        private void LoadScaleModels() {
            string json;
            if (File.Exists(SCALE_MODELS_JSON_PATH)) {
                json = File.ReadAllText(SCALE_MODELS_JSON_PATH);
            } else {
                json = Resources["BuiltInScaleModels"] as string;
                File.WriteAllText(SCALE_MODELS_JSON_PATH, json);
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

        private void BtnOptions_Click(object sender, RoutedEventArgs e) {
            new OptionsWindow().Show();
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e) {
            magWindow.Destory();
            Settings.Default.Save();
        }

        private void TxtHotkey_TextChanged(object sender, TextChangedEventArgs e) {
            keyboardEvents?.Dispose();
            keyboardEvents = Hook.GlobalEvents();

            string hotkey = txtHotkey.Text.Trim();

            try {
                keyboardEvents.OnCombination(new Dictionary<Combination, Action> {{
                    Combination.FromString(hotkey), () => ToggleMagWindow()
                }});

                txtHotkey.Foreground = Brushes.Black;
                Settings.Default.Hotkey = hotkey;

                cmiHotkey.Header = hotkey;
            } catch (ArgumentException) {
                txtHotkey.Foreground = Brushes.Red;
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

        private void Window_SourceInitialized(object sender, EventArgs e) {
            Handle = new WindowInteropHelper(this).Handle;

            HwndSource source = HwndSource.FromHwnd(Handle);
            source.AddHook(WndProc);

            magWindow = new MagWindow(this);
        }

        private IntPtr WndProc(IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam, ref bool handled) {
            if (msg == NativeMethods.MAGPIE_WM_SHOWME) {
                // 收到 WM_SHOWME 激活窗口
                if (WindowState == WindowState.Minimized) {
                    Show();
                    WindowState = WindowState.Normal;
                }

                _ = NativeMethods.SetForegroundWindow(Handle);
                handled = true;
            }
            return IntPtr.Zero;
        }

        private void CbbScaleMode_SelectionChanged(object sender, SelectionChangedEventArgs e) {
            Settings.Default.ScaleMode = cbbScaleMode.SelectedIndex;
        }

        private void StartScaleTimer() {
            countDownNum = DOWN_COUNT;
            cmiScale.Header = btnScale.Content = countDownNum.ToString();

            timerScale.Start();
        }

        private void StopScaleTimer() {
            timerScale.Stop();
            btnScale.Content = cmiScale.Header = "5秒后放大";
        }

        private void ToggleScaleTimer() {
            if (timerScale.IsEnabled) {
                StopScaleTimer();
            } else {
                StartScaleTimer();
            }
        }

        private void CbbCaptureMode_SelectionChanged(object sender, SelectionChangedEventArgs e) {
            Settings.Default.CaptureMode = cbbCaptureMode.SelectedIndex;
        }

        private void Window_StateChanged(object sender, EventArgs e) {
            if (WindowState == WindowState.Minimized) {
                Hide();
                notifyIcon.Visibility = Visibility.Visible;
            } else {
                notifyIcon.Visibility = Visibility.Hidden;
            }
        }

        private void CbbInjectMode_SelectionChanged(object sender, SelectionChangedEventArgs e) {
            if (cbbInjectMode.SelectedIndex == 2) {
                // 启动时注入
                if (!openFileDialog.ShowDialog().GetValueOrDefault(false)) {
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


        private void CmiExit_Click(object sender, RoutedEventArgs e) {
            Close();
        }

        private void CmiMainForm_Click(object sender, RoutedEventArgs e) {
            Show();
            WindowState = WindowState.Normal;
        }

        private void CmiScale_Click(object sender, RoutedEventArgs e) {
            ToggleScaleTimer();
        }

        private void BtnScale_Click(object sender, RoutedEventArgs e) {
            ToggleScaleTimer();
        }
    }

    public class NotifyIconLeftClickCommand : ICommand {
#pragma warning disable 67
        // 未使用
        public event EventHandler CanExecuteChanged;
#pragma warning restore 67

        public bool CanExecute(object _) {
            return true;
        }

        public void Execute(object parameter) {
            MainWindow window = parameter as MainWindow;
            window.Show();
            window.WindowState = WindowState.Normal;
        }
    }
}
