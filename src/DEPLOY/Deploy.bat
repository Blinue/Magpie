msbuild /p:Configuration=Release;Platform=x64;OutDir=..\..\publish\ ..\Magpie

IF %ERRORLEVEL% NEQ 0 (
    ECHO Error: Failed to build Magpie
    EXIT 1
)

REM 复制效果文件
REM msbuild /p:Configuration=Release;Platform=x64;OutDir=..\..\publish\ ..\Effects

REM IF %ERRORLEVEL% NEQ 0 (
REM     ECHO Error: Failed to build Effects
REM     EXIT 1
REM )

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
RD /S /Q Magpie.App
REM 删除所有 pri 文件，除了 resources.pri
FOR %%f IN ("*.pri") DO IF /i "%%~nf" NEQ "resources" DEL "%%f"
