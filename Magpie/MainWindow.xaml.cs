using Gma.System.MouseKeyHook;
using Microsoft.Win32;
using Newtonsoft.Json.Linq;
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
using Magpie.Properties;
using Magpie.Options;
using System.Threading;


namespace Magpie {
	/// <summary>
	/// MainWindow.xaml 的交互逻辑
	/// </summary>
	public partial class MainWindow : Window {
		private OptionsWindow optionsWindow = null;
		private readonly OpenFileDialog openFileDialog = new OpenFileDialog();
		private readonly DispatcherTimer timerScale = new DispatcherTimer {
			Interval = new TimeSpan(0, 0, 1)
		};
		private readonly FileSystemWatcher scaleModelsWatcher = new FileSystemWatcher();

		private IKeyboardMouseEvents keyboardEvents = null;
		private MagWindow magWindow;

		// 倒计时时间
		private const int DOWN_COUNT = 5;
		private int countDownNum;

		private (string Name, string Model)[] scaleModels;

		private IntPtr Handle;

		// 不为零时表示全屏窗口不是因为热键关闭的
		private IntPtr prevSrcWindow = IntPtr.Zero;
		private readonly DispatcherTimer timerRestore = new DispatcherTimer {
			Interval = new TimeSpan(0, 0, 0, 0, 200)
		};

		public MainWindow() {
			InitializeComponent();

			timerScale.Tick += TimerScale_Tick;
			timerRestore.Tick += TimerRestore_Tick;

			LoadScaleModels();

			// 监视ScaleModels.json的更改
			scaleModelsWatcher.Path = App.APPLICATION_DIR;
			scaleModelsWatcher.NotifyFilter = NotifyFilters.LastWrite | NotifyFilters.FileName;
			scaleModelsWatcher.Filter = App.SCALE_MODELS_JSON_PATH.Substring(App.SCALE_MODELS_JSON_PATH.LastIndexOf('\\') + 1);
			scaleModelsWatcher.Changed += ScaleModelsWatcher_Changed;
			scaleModelsWatcher.Deleted += ScaleModelsWatcher_Changed;
			scaleModelsWatcher.EnableRaisingEvents = true;

			// 加载设置
			txtHotkey.Text = Settings.Default.Hotkey;

			if (Settings.Default.ScaleMode >= cbbScaleMode.Items.Count) {
				Settings.Default.ScaleMode = 0;
			}
			cbbScaleMode.SelectedIndex = Settings.Default.ScaleMode;
			cbbInjectMode.SelectedIndex = Settings.Default.InjectMode;
			cbbCaptureMode.SelectedIndex = Settings.Default.CaptureMode;
		}

		private void TimerRestore_Tick(object sender, EventArgs e) {
			if (!Settings.Default.AutoRestore || !NativeMethods.IsWindow(prevSrcWindow)) {
				StopWaitingForRestore();
				return;
			}

			if (NativeMethods.GetForegroundWindow() == prevSrcWindow) {
				ToggleMagWindow();
				return;
			}

			tbCurWndTitle.Text = $"{Properties.Resources.当前窗口}：{NativeMethods.GetWindowTitle(prevSrcWindow)}";
			gridAutoRestore.Visibility = Visibility.Visible;
		}

		private void ScaleModelsWatcher_Changed(object sender, FileSystemEventArgs e) {
			// 立即读取可能会访问冲突
			Thread.Sleep(10);
			Dispatcher.Invoke(() => {
				LoadScaleModels();
			});
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
			try {
				string json;
				if (File.Exists(App.SCALE_MODELS_JSON_PATH)) {
					json = File.ReadAllText(App.SCALE_MODELS_JSON_PATH);
				} else {
					json = Properties.Resources.BuiltInScaleModels;
					File.WriteAllText(App.SCALE_MODELS_JSON_PATH, json);
				}

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

				// 保留当前选择
				int idx = cbbScaleMode.SelectedIndex;
				cbbScaleMode.Items.Clear();
				foreach ((string Name, string Model) scaleModel in scaleModels) {
					_ = cbbScaleMode.Items.Add(scaleModel.Name);
				}

				if (idx >= cbbScaleMode.Items.Count) {
					idx = 0;
				}
				cbbScaleMode.SelectedIndex = idx;
			} catch (Exception) {
				scaleModels = null;

				cbbScaleMode.Items.Clear();
				_ = cbbScaleMode.Items.Add($"<{Properties.Resources.解析失败}>");
			}
		}

		private void BtnOptions_Click(object sender, RoutedEventArgs e) {
			if (optionsWindow == null) {
				optionsWindow = new OptionsWindow();
				optionsWindow.Closed += (object _, EventArgs _1) => {
					optionsWindow = null;
				};
			}

			optionsWindow.Show();
			_ = optionsWindow.Focus();
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
			if (Settings.Default.AutoRestore) {
				StopWaitingForRestore();
			}

			if (scaleModels == null || scaleModels.Length == 0) {
				return;
			}

			if (magWindow.Status == MagWindowStatus.Running) {
				// 通过热键关闭全屏窗口
				magWindow.Destory();
				return;
			}

			if (magWindow.Status == MagWindowStatus.Starting) {
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

			prevSrcWindow = magWindow.SrcWindow;
		}

		private void Window_SourceInitialized(object sender, EventArgs e) {
			Handle = new WindowInteropHelper(this).Handle;

			HwndSource source = HwndSource.FromHwnd(Handle);
			source.AddHook(WndProc);

			magWindow = new MagWindow(this);
			magWindow.Closed += MagWindow_Closed;

			// 检查命令行参数
			string[] args = Environment.GetCommandLineArgs();
			foreach (string arg in args) {
				if (arg == "-st") {
					// 启动到系统托盘
					WindowState = WindowState.Minimized;
					break;
				}
			}
		}

		private void StopWaitingForRestore() {
			gridAutoRestore.Visibility = Visibility.Hidden;
			tbCurWndTitle.Text = "";
			prevSrcWindow = IntPtr.Zero;
			timerRestore.Stop();
		}

		private void MagWindow_Closed() {
			if (!Settings.Default.AutoRestore) {
				return;
			}

			Dispatcher.Invoke(() => {
				if (NativeMethods.IsWindow(prevSrcWindow)) {
					timerRestore.Start();
				} else {
					StopWaitingForRestore();
				}
			});
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
			btnScale.Content = cmiScale.Header = Properties.Resources.秒后缩放;
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

		private void CmiOptions_Click(object sender, RoutedEventArgs e) {
			BtnOptions_Click(sender, e);
		}

		private void BtnCancelRestore_Click(object sender, RoutedEventArgs e) {
			StopWaitingForRestore();
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
