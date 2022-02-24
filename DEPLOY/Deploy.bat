REM 编译 Runtime
msbuild /p:Configuration=Release;Platform=x64;OutDir=../publish/ ../Runtime

IF %ERRORLEVEL% NEQ 0 (
    ECHO Error: Failed to build Runtime
    EXIT 1
)

REM 复制效果文件
msbuild /p:Configuration=Release;Platform=x64;OutDir=../publish/ ../Effects

IF %ERRORLEVEL% NEQ 0 (
    ECHO Error: Failed to build Effects
    EXIT 1
)

REM 部署 .NET
msbuild -t:restore /t:Publish /p:Configuration=Release;Platform=x64 ../Magpie/Magpie.csproj

REM 包含 .NET 运行时的部署版本
REM msbuild -t:restore /t:Publish /p:Configuration=Release;Platform=x64;SelfContained=true;IncludeAllContentForSelfExtract=true;EnableCompressionInSingleFile=true ../Magpie/Magpie.csproj

IF %ERRORLEVEL% NEQ 0 (
    ECHO Error: Failed to build Magpie
    EXIT 1
)

REM 清理不需要的文件
cd ../publish/
del *.pdb
del *lib
del *.exp
