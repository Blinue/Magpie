using System;
using System.IO;
using System.Windows;
using System.Windows.Resources;

namespace Magpie.Options {
	/// <summary>
	/// LicenseWindow.xaml 的交互逻辑
	/// </summary>
	public partial class LicenseWindow : Window {
		private static NLog.Logger Logger { get; } = NLog.LogManager.GetCurrentClassLogger();

		public LicenseWindow() {
			InitializeComponent();

			Uri uri = new Uri("pack://application:,,,/Magpie;component/Resources/Licenses.txt", UriKind.Absolute);
			try {
				StreamResourceInfo info = Application.GetResourceStream(uri);
				using (StreamReader reader = new StreamReader(info.Stream)) {
					tbLicenses.Text = reader.ReadToEnd();
				}
			} catch (Exception e) {
				Logger.Error(e, $"读取 {uri} 失败");
			}
		}

		private void BtnOK_Click(object sender, RoutedEventArgs e) {
			Close();
		}
	}
}
