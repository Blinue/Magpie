#pragma once
#include "BoolToObjectConverter.g.h"

namespace winrt::Magpie::implementation {

struct BoolToObjectConverter : BoolToObjectConverterT<BoolToObjectConverter> {
	// 需定义 DependencyProperty 才能绑定 ThemeResource
	DEFINE_DEPENDENCY_PROPERTY(IInspectable, TrueObject, _trueObjectProperty)
	DEFINE_DEPENDENCY_PROPERTY(IInspectable, FalseObject, _falseObjectProperty)

public:
	BoolToObjectConverter();

	IInspectable Convert(const IInspectable& value, const Interop::TypeName&, const IInspectable&, const hstring&);

	IInspectable ConvertBack(const IInspectable&, const Interop::TypeName&, const IInspectable&, const hstring&);

private:
	static void _RegisterDependencyProperties();
};

}

BASIC_FACTORY(BoolToObjectConverter)
