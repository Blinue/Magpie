# ±‡“Î Runtime
msbuild /p:Configuration=Release;Platform=x64;OutDir=../publish/ ../Runtime

IF %ERRORLEVEL% NEQ 0 (
    ECHO Error: MSBuild not found
    EXIT 1
)

# ±‡“Î Effects
msbuild /p:Configuration=Release;Platform=x64;OutDir=../publish/ ../Effects

IF %ERRORLEVEL% NEQ 0 (
    ECHO Error: MSBuild not found
    EXIT 1
)

# ≤ø  Magpie
msbuild ../Magpie/Magpie.csproj -t:restore /t:Publish /p:PublishDir=../publish/

IF %ERRORLEVEL% NEQ 0 (
    ECHO Error: MSBuild not found
    EXIT 1
)

cd ../publish/
del *.pdb
del *lib
del *.exp
del xaudio2_9redist.dll
