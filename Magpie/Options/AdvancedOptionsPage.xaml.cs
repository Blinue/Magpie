using Magpie.Properties;
using System.Windows;
using System.Windows.Controls;


namespace Magpie.Options {
	/// <summary>
	/// AdvancedOptionsPage.xaml 的交互逻辑
	/// </summary>
	public partial class AdvancedOptionsPage : Page {
		public AdvancedOptionsPage() {
			InitializeComponent();

#if DEBUG
			spDebug.Visibility = Visibility.Visible;
			ckbShowAllCaptureMethods.Checked += CkbShowAllCaptureMethods_Checked;
			ckbShowAllCaptureMethods.Unchecked += CkbShowAllCaptureMethods_Unchecked;
#else
			spDebug.Visibility = Visibility.Collapsed;
#endif

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
	}
}
