using NLog;
using System;
using System.ComponentModel;
using System.Diagnostics;
using System.Windows;
using System.Windows.Controls;


namespace Magpie.Options {
	/// <summary>
	/// ScaleOptionsPage.xaml 的交互逻辑
	/// </summary>
	public partial class ScaleOptionsPage : Page {
		private static Logger Logger { get; } = LogManager.GetCurrentClassLogger();

		public ScaleOptionsPage() {
			InitializeComponent();

			if (NativeMethods.GetOSVersion() < new Version(10, 0, 22000)) {
				ccbDisableRoundCorner.Visibility = Visibility.Collapsed;
			}
		}

		private void BtnScale_Click(object sender, RoutedEventArgs e) {
			ProcessStartInfo psi = new ProcessStartInfo(App.SCALE_MODELS_JSON_PATH) {
				UseShellExecute = true
			};

			try {
				_ = Process.Start(psi);
			} catch (Win32Exception ex) {
				Logger.Warn(ex, "打开缩放配置失败，将尝试使用 Notepad 打开");

				psi.FileName = "notepad";
				psi.Arguments = App.SCALE_MODELS_JSON_PATH;
				try {
					_ = Process.Start(psi);
				} catch (Exception ex1) {
					Logger.Error(ex1, $"使用 Notepad 打开缩放配置失败\n\t缩放配置路径：{App.SCALE_MODELS_JSON_PATH}");
				}
			} catch (Exception ex) {
				Logger.Error(ex, $"打开缩放配置失败\n\t缩放配置路径：{App.SCALE_MODELS_JSON_PATH}");
			}
		}
	}
}
