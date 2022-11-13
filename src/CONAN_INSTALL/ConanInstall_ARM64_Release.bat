conan --version

IF %ERRORLEVEL% NEQ 0 (
    ECHO Error: Conan not found
    EXIT 1
)

conan config set storage.path=%CD%\..\..\.conan\data

conan install ..\conanfile.txt --install-folder ..\..\.conan\ARM64\Release --build=outdated -s build_type=Release -s arch=armv8 -s compiler="Visual Studio" -s compiler.version=17 -s compiler.runtime=MD --update
conan install ..\Updater\conanfile.txt --install-folder ..\..\.conan\ARM64\Release\Updater --build=outdated -s build_type=Release -s arch=armv8 -s compiler="Visual Studio" -s compiler.version=17 -s compiler.runtime=MT --update
