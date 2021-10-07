using System.Windows;


namespace Magpie.Options {
	/// <summary>
	/// LicenseWindow.xaml 的交互逻辑
	/// </summary>
	public partial class LicenseWindow : Window {
		public LicenseWindow() {
			InitializeComponent();
		}

		private void BtnOK_Click(object sender, RoutedEventArgs e) {
			Close();
		}
	}
}
