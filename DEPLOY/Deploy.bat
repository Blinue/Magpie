REM 编译 Runtime
msbuild /p:Configuration=Release;Platform=x64;OutDir=../publish/ ../Runtime

IF %ERRORLEVEL% NEQ 0 (
    ECHO Error: MSBuild not found
    EXIT 1
)

REM 复制效果文件
msbuild /p:Configuration=Release;Platform=x64;OutDir=../publish/ ../Effects

IF %ERRORLEVEL% NEQ 0 (
    ECHO Error: MSBuild not found
    EXIT 1
)

REM 部署 .NET
msbuild -t:restore /t:Publish /p:Configuration=Release;Platform=x64;PublishDir=../publish ../Magpie/Magpie.csproj

IF %ERRORLEVEL% NEQ 0 (
    ECHO Error: MSBuild not found
    EXIT 1
)

REM 清理不需要的文件
cd ../publish/
del *.pdb
del *lib
del *.exp
del xaudio2_9redist.dll
