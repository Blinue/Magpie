using System.Reflection;
using System.Resources;
using System.Runtime.InteropServices;
using System.Windows;


[assembly: AssemblyTitle("Magpie")]
[assembly: AssemblyProduct("Magpie")]
[assembly: AssemblyCopyright("Copyright (C) 2021 Liu Xu")]


// 程序集的版本信息由下列四个值组成: 
//
//      主版本
//      次版本
//      生成号
//      修订号
//
//可以指定所有这些值，也可以使用“生成号”和“修订号”的默认值
//通过使用 "*"，如下所示:
// [assembly: AssemblyVersion("1.0.*")]
[assembly: AssemblyVersion("0.8.1.0")]
[assembly: AssemblyFileVersion("0.8.1.0")]


// 将 ComVisible 设置为 false 会使此程序集中的类型
//对 COM 组件不可见。如果需要从 COM 访问此程序集中的类型
//请将此类型的 ComVisible 特性设置为 true。
[assembly: ComVisible(false)]

// 如果不支持系统的语言/区域，回落到英语
[assembly: NeutralResourcesLanguage("en-US", UltimateResourceFallbackLocation.MainAssembly)]

[assembly: ThemeInfo(
	ResourceDictionaryLocation.None, //主题特定资源词典所处位置
									 //(未在页面中找到资源时使用，
									 //或应用程序资源字典中找到时使用)
	ResourceDictionaryLocation.SourceAssembly //常规资源词典所处位置
											  //(未在页面中找到资源时使用，
											  //、应用程序或任何主题专用资源字典中找到时使用)
)]


// 提示编译器只支持 Windows，从而消除 CA1416 警告
// 因为 GenerateAssemblyInfo 设为 false，因此需要显式设置此属性
[assembly: System.Runtime.Versioning.SupportedOSPlatform("windows")]
