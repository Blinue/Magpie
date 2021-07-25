using System.Diagnostics;
using System.Windows;
using System.Windows.Controls;


namespace Magpie.Options {
    /// <summary>
    /// ScaleOptionsPage.xaml 的交互逻辑
    /// </summary>
    public partial class ScaleOptionsPage : Page {
        public ScaleOptionsPage() {
            InitializeComponent();
        }

        private void BtnScale_Click(object sender, RoutedEventArgs e) {
            _ = Process.Start(new ProcessStartInfo(App.SCALE_MODELS_JSON_PATH));
        }
    }
}
