@ECHO OFF

python texture2Tiff.py %1 %2.tiff

IF %ERRORLEVEL% NEQ 0 (
    ECHO Error: texture2Tiff.py failed
    PAUSE
    EXIT 1
)

texconv -f R16G16B16A16_FLOAT -m 1 -fl 11.1 -y -nologo %2.tiff

IF %ERRORLEVEL% NEQ 0 (
    ECHO "Error: Failed to convert .tiff to .dds"
    PAUSE
    EXIT 1
)

DEL %2.tiff
