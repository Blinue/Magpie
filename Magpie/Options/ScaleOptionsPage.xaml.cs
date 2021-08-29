using NLog;
using System;
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
		}

		private void BtnScale_Click(object sender, RoutedEventArgs e) {
			ProcessStartInfo psi = new ProcessStartInfo(App.SCALE_MODELS_JSON_PATH) {
				UseShellExecute = true
			};

			try {
				_ = Process.Start(psi);
			} catch (Exception ex) {
				Logger.Error(ex, $"打开缩放配置失败\n\tSCALE_MODELS_JSON_PATH={App.SCALE_MODELS_JSON_PATH}");
			}
		}
	}
}
