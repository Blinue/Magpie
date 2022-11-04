$ErrorActionPreference = "Stop"

msbuild /p:Configuration=Release`;Platform=x64 src\CONAN_INSTALL
msbuild /p:Configuration=Release`;Platform=x64`;OutDir=..\..\publish\ src\Effects
msbuild /p:Configuration=Release`;Platform=x64`;OutDir=..\..\publish\ src\Magpie.Core
msbuild /m /p:Configuration=Release`;Platform=x64`;BuildProjectReferences=false`;OutDir=..\..\publish\ src\Magpie
msbuild /p:Configuration=Release`;Platform=x64`;OutDir=..\..\publish\ src\Updater

cd .\publish\
Remove-Item @("*.pdb", "*.lib", "*.exp", "*.winmd", "*.xml", "*.xbf", "dummy.*", "Microsoft.Web.WebView2.Core.dll")
Remove-Item @("Microsoft.UI.Xaml", "Magpie.UI") -Recurse
Remove-Item *.pri -Exclude resources.pri

COPY-ITEM -Path @("C:\Windows\System32\msvcp140.dll", "C:\Windows\System32\vcruntime140.dll", "C:\Windows\System32\vcruntime140_1.dll")
