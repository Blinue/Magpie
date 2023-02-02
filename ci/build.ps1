# 出错时终止 CI
$ErrorActionPreference = "Stop"

msbuild /p:Configuration=Release`;Platform=x64 src\CONAN_INSTALL
if ($LastExitCode -ne 0) {
	throw '编译 CONAN_INSTALL 失败'
}
msbuild /p:Configuration=Release`;Platform=x64`;OutDir=..\..\publish\ src\Effects
if ($LastExitCode -ne 0) {
	throw '编译 Effects 失败'
}
msbuild /p:Configuration=Release`;Platform=x64`;OutDir=..\..\publish\ src\Magpie.Core
if ($LastExitCode -ne 0) {
	throw '编译 Magpie.Core 失败'
}
msbuild /m /p:Configuration=Release`;Platform=x64`;BuildProjectReferences=false`;OutDir=..\..\publish\ src\Magpie
if($LastExitCode -ne 0) {
	throw '编译 Magpie 失败'
}
msbuild /p:Configuration=Release`;Platform=x64`;OutDir=..\..\publish\ src\Updater
if ($LastExitCode -ne 0) {
	throw '编译 Updater 失败'
}

# 清理不需要的文件
Set-Location .\publish\
Remove-Item @("*.pdb", "*.lib", "*.exp", "*.winmd", "*.xml", "*.xbf", "dummy.*", "Microsoft.Web.WebView2.Core.dll")
Remove-Item @("Microsoft.UI.Xaml", "Magpie.App") -Recurse
Remove-Item *.pri -Exclude resources.pri
