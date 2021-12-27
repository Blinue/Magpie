using Magpie.Properties;
using System.Windows;
using System.Windows.Controls;
using System.IO;
using System.Diagnostics;
using NLog;
using System;


namespace Magpie.Options {
	/// <summary>
	/// AdvancedOptionsPage.xaml 的交互逻辑
	/// </summary>
	public partial class AdvancedOptionsPage : Page {
		private static Logger Logger { get; } = LogManager.GetCurrentClassLogger();

		public AdvancedOptionsPage() {
			InitializeComponent();

			spDebugging.Visibility = Settings.Default.ShowDebuggingOptions ? Visibility.Visible : Visibility.Collapsed;

			ckbShowDebuggingOptions.Checked += CkbShowDebuggingOptions_Checked;
			ckbShowDebuggingOptions.Unchecked += CkbShowDebuggingOptions_Unchecked;
			ckbShowAllCaptureMethods.Checked += CkbShowAllCaptureMethods_Checked;
			ckbShowAllCaptureMethods.Unchecked += CkbShowAllCaptureMethods_Unchecked;
			cbbLoggingLevel.SelectionChanged += CbbLoggingLevel_SelectionChanged;
		}

		private void CkbShowAllCaptureMethods_Unchecked(object sender, RoutedEventArgs e) {
			((MainWindow)Application.Current.MainWindow).ShowAllCaptureMethods(false);
		}

		private void CkbShowAllCaptureMethods_Checked(object sender, RoutedEventArgs e) {
			((MainWindow)Application.Current.MainWindow).ShowAllCaptureMethods(true);
		}

		private void CbbLoggingLevel_SelectionChanged(object sender, SelectionChangedEventArgs e) {
			App.SetLogLevel(Settings.Default.LoggingLevel);
			((MainWindow)Application.Current.MainWindow).SetRuntimeLogLevel(Settings.Default.LoggingLevel);
		}

		private void BtnOpenLogsFolder_Click(object sender, RoutedEventArgs e) {
			if (!Directory.Exists(App.LOGS_FOLDER)) {
				Logger.Error("日志文件夹不存在");
				Debug.Assert(false);
				return;
			}

			ProcessStartInfo psi = new(App.LOGS_FOLDER) {
				UseShellExecute = true
			};

			try {
				_ = Process.Start(psi);
			} catch (Exception ex) {
				Logger.Error(ex, $"打开日志文件夹失败");
			}
		}

		private void CkbShowDebuggingOptions_Checked(object sender, RoutedEventArgs e) {
			spDebugging.Visibility = Visibility.Visible;
		}

		private void CkbShowDebuggingOptions_Unchecked(object sender, RoutedEventArgs e) {
			spDebugging.Visibility = Visibility.Collapsed;
		}
	}
}
