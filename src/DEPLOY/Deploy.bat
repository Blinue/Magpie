msbuild /p:Configuration=Release /p:Platform=x64 ..\CONAN_INSTALL

IF %ERRORLEVEL% NEQ 0 (
    ECHO Error: Failed to build CONAN_INSTALL
    EXIT 1
)

msbuild /p:Configuration=Release;Platform=x64;OutDir=..\..\publish\ ..\Effects

IF %ERRORLEVEL% NEQ 0 (
    ECHO Error: Failed to build Effects
    EXIT 1
)

msbuild /p:Configuration=Release;Platform=x64;OutDir=..\..\publish\ ..\Runtime

IF %ERRORLEVEL% NEQ 0 (
    ECHO Error: Failed to build Runtime
    EXIT 1
)

msbuild /p:Configuration=Release;Platform=x64;OutDir=..\..\publish\ ..\App

IF %ERRORLEVEL% NEQ 0 (
    ECHO Error: Failed to build App
    EXIT 1
)

msbuild /m /p:Configuration=Release;Platform=x64;BuildProjectReferences=false;OutDir=..\..\publish\ ..\Magpie

IF %ERRORLEVEL% NEQ 0 (
    ECHO Error: Failed to build Magpie
    EXIT 1
)

REM 清理不需要的文件
CD ..\..\publish
DEL *.pdb
DEL *.lib
DEL *.exp
DEL dummy.*
DEL *.winmd
DEL *.xml
DEL *.xbf
DEL Microsoft.Web.WebView2.Core.dll
RD /S /Q Microsoft.UI.Xaml
RD /S /Q App
REM 删除所有 pri 文件，除了 resources.pri
FOR %%f IN ("*.pri") DO IF /i "%%~nf" NEQ "resources" DEL "%%f"
