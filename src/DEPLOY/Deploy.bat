REM msbuild /p:Configuration=Release;Platform=x64;OutDir=..\..\publish\ ..\Magpie.App
devenv ..\..\Magpie.sln /Build "Release|x64"

IF %ERRORLEVEL% NEQ 0 (
    ECHO Error: Failed to build Magpie.App
    EXIT 1
)

REM 复制效果文件
REM msbuild /p:Configuration=Release;Platform=x64;OutDir=..\..\publish\ ..\Effects

REM IF %ERRORLEVEL% NEQ 0 (
REM     ECHO Error: Failed to build Effects
REM     EXIT 1
REM )

REM 清理不需要的文件
cd ..\..\build\x64\Release
del *.pdb
del *.lib
del *.exp
del dummy.*
del *.winmd
del *.xml
