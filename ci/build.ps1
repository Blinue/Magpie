msbuild "/p:Configuration=Release;Platform=x64" "..\CONAN_INSTALL"
msbuild "/p:Configuration=Release;Platform=x64;OutDir=..\..\publish\" "..\Effects"
msbuild "/p:Configuration=Release;Platform=x64;OutDir=..\..\publish\" "..\Magpie.Core"
msbuild "/m /p:Configuration=Release;Platform=x64;BuildProjectReferences=false;OutDir=..\..\publish\" "..\Magpie"
msbuild "/p:Configuration=Release;Platform=x64;OutDir=..\..\publish\" "..\Updater"

COPY-ITEM -Path @("C:\Windows\System32\msvcp140.dll", "C:\Windows\System32\vcruntime140.dll", "C:\Windows\System32\vcruntime140_1.dll") -Destination "publish\"
