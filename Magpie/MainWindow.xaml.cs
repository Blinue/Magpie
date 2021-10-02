using Gma.System.MouseKeyHook;
using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Interop;
using System.Windows.Threading;
using System.Windows.Media;
using Magpie.Properties;
using Magpie.Options;


namespace Magpie {
	/// <summary>
	/// MainWindow.xaml 的交互逻辑
	/// </summary>
	public partial class MainWindow : Window {
		private static NLog.Logger Logger { get; } = NLog.LogManager.GetCurrentClassLogger();

		private OptionsWindow optionsWindow = null;
		private readonly DispatcherTimer timerScale = new DispatcherTimer {
			Interval = new TimeSpan(0, 0, 1)
		};

		private IKeyboardMouseEvents keyboardEvents = null;
		private MagWindow magWindow = null;

		private readonly ScaleModelManager scaleModelManager = new ScaleModelManager();

		// 倒计时时间
		private const int DOWN_COUNT = 5;
		private int countDownNum;

		private IntPtr Handle;

		// 不为零时表示全屏窗口不是因为Hotkey关闭的
		private IntPtr prevSrcWindow = IntPtr.Zero;
		private readonly DispatcherTimer timerRestore = new DispatcherTimer {
			Interval = new TimeSpan(0, 0, 0, 0, 300)
		};

		private void Application_Closing() {
			magWindow.Destory();

			if (optionsWindow != null) {
				optionsWindow.Close();
			}
		}

		public MainWindow() {
			InitializeComponent();

			BindScaleModels();
			scaleModelManager.ScaleModelsChanged += BindScaleModels;

			timerScale.Tick += TimerScale_Tick;
			timerRestore.Tick += TimerRestore_Tick;

			// 加载设置
			txtHotkey.Text = Settings.Default.Hotkey;

			if (Settings.Default.ScaleMode >= cbbScaleMode.Items.Count) {
				Settings.Default.ScaleMode = 0;
			}
			cbbScaleMode.SelectedIndex = Settings.Default.ScaleMode;

			// 延迟绑定，防止加载时改变设置
			cbbScaleMode.SelectionChanged += CbbScaleMode_SelectionChanged;
		}

		private void BindScaleModels() {
			int oldIdx = cbbScaleMode.SelectedIndex;
			cbbScaleMode.Items.Clear();

			ScaleModelManager.ScaleModel[] scaleModels = scaleModelManager.GetScaleModels();
			if (scaleModels == null || scaleModels.Length == 0) {
				_ = cbbScaleMode.Items.Add($"<{Properties.Resources.UI_Main_Parse_Failure}>");
				cbbScaleMode.SelectedIndex = 0;
			} else {
				foreach (ScaleModelManager.ScaleModel m in scaleModels) {
					_ = cbbScaleMode.Items.Add(m.Name);
				}

				if (oldIdx < 0 || oldIdx >= scaleModels.Length) {
					oldIdx = 0;
				}
				cbbScaleMode.SelectedIndex = oldIdx;
			}
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

			tbCurWndTitle.Text = $"{Properties.Resources.UI_Main_Current_Window}{NativeMethods.GetWindowTitle(prevSrcWindow)}";
			if (WindowState == WindowState.Normal) {
				btnForgetCurrentWnd.Visibility = gridCurWnd.Visibility = Visibility.Visible;
			}
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
			magWindow.Dispose();

			if (Settings.Default.MinimizeWhenClose) {
				WindowState = WindowState.Minimized;
				e.Cancel = true;
			} else {
				Application_Closing();
			}
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

				Logger.Info($"当前热键：{txtHotkey.Text}");
			} catch (ArgumentException ex) {
				Logger.Error(ex, $"解析快捷键失败：{txtHotkey.Text}");
				txtHotkey.Foreground = Brushes.Red;
			}
		}

		private void ToggleMagWindow() {
			if (Settings.Default.AutoRestore) {
				StopWaitingForRestore();
				// 立即更新布局，因为窗口大小可能改变，如果接下来放大 Magpie 本身会立即退出
				UpdateLayout();
			}

			if (!scaleModelManager.IsValid()) {
				return;
			}

			if (magWindow.Running) {
				Logger.Info("通过热键退出全屏");
				magWindow.Destory();
				return;
			}

			string effectsJson = scaleModelManager.GetScaleModels()[Settings.Default.ScaleMode].Model;
			bool showFPS = Settings.Default.ShowFPS;
			int captureMode = Settings.Default.CaptureMode;
			bool adjustCursorSpeed = Settings.Default.AdjustCursorSpeed;
			bool noVsync = Settings.Default.NoVsync;

			magWindow.Create(
				effectsJson,
				captureMode,
				showFPS,
				adjustCursorSpeed,
				noVsync
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
					Logger.Info("已指定启动时缩放到系统托盘");
					WindowState = WindowState.Minimized;
					break;
				}
			}
		}

		private void StopWaitingForRestore() {
			if (WindowState == WindowState.Normal) {
				btnForgetCurrentWnd.Visibility = gridCurWnd.Visibility = Visibility.Collapsed;
			}
			tbCurWndTitle.Text = "";
			prevSrcWindow = IntPtr.Zero;
			timerRestore.Stop();
		}

		private void MagWindow_Closed() {
			// 不监视 Magpie 主窗口
			if (!Settings.Default.AutoRestore || prevSrcWindow == Handle) {
				return;
			}

			Dispatcher.Invoke(() => {
				if (NativeMethods.IsWindow(prevSrcWindow)) {
					Logger.Info("正在监视源窗口是否为前台窗口");
					timerRestore.Start();
				} else {
					Logger.Info("停止监视源窗口是否为前台窗口");
					StopWaitingForRestore();
				}
			});
		}

		private IntPtr WndProc(IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam, ref bool handled) {
			if (msg == NativeMethods.MAGPIE_WM_SHOWME) {
				Logger.Info("收到 WM_SHOWME 消息");

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
			btnScale.Content = Properties.Resources.UI_Main_Scale_After_5S;
			cmiScale.Header = Properties.Resources.UI_SysTray_Scale_After_5S;
		}

		private void ToggleScaleTimer() {
			if (timerScale.IsEnabled) {
				StopScaleTimer();
			} else {
				StartScaleTimer();
			}
		}

		private void Window_StateChanged(object sender, EventArgs e) {
			if (WindowState == WindowState.Minimized) {
				Hide();
				notifyIcon.Visibility = Visibility.Visible;
			} else {
				notifyIcon.Visibility = Visibility.Hidden;
				btnForgetCurrentWnd.Visibility = gridCurWnd.Visibility = timerRestore.IsEnabled ? Visibility.Visible : Visibility.Collapsed;
			}
		}

		private void CmiExit_Click(object sender, RoutedEventArgs e) {
			Application_Closing();
			Closing -= Window_Closing;
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
