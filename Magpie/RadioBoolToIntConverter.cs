using System;
using System.Globalization;
using System.Windows;
using System.Windows.Data;


namespace Magpie {
	internal class RadioBoolToIntConverter : IValueConverter {
		public object Convert(object value, Type targetType, object parameter, CultureInfo culture) {
			return value == null ? DependencyProperty.UnsetValue : (int)value == int.Parse(parameter.ToString());
		}

		public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture) {
			return (bool)value ? parameter : null;
		}
	}
}
