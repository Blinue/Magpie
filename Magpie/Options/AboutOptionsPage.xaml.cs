using System;
using System.Diagnostics;
using System.Windows.Controls;
using System.Windows.Navigation;


namespace Magpie.Options {
	/// <summary>
	/// AboutOptionsPage.xaml 的交互逻辑
	/// </summary>
	public partial class AboutOptionsPage : Page {
		private LicenseWindow licenseWindow = null;

		public AboutOptionsPage() {
			InitializeComponent();

			lblVersion.Content = $"{Properties.Resources.UI_Options_About_Version} {App.APP_VERSION.ToString(3)}";
		}

		private void Hyperlink_RequestNavigate(object sender, RequestNavigateEventArgs e) {
			ProcessStartInfo psi = new ProcessStartInfo(e.Uri.AbsoluteUri) {
				UseShellExecute = true
			};
			_ = Process.Start(psi);

			e.Handled = true;
		}

		private void BtnLicense_Click(object sender, System.Windows.RoutedEventArgs e) {
			if (licenseWindow == null) {
				licenseWindow = new LicenseWindow();
				licenseWindow.Closed += (object _, EventArgs _1) => {
					licenseWindow = null;
				};
			}

			licenseWindow.Show();
			_ = licenseWindow.Focus();
		}
	}
}
