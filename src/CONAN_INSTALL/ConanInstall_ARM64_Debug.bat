conan --version

IF %ERRORLEVEL% NEQ 0 (
    ECHO Error: Conan not found
    EXIT 1
)

conan config set storage.path=%CD%\..\..\.conan\data

conan install ..\conanfile.py --install-folder ..\..\.conan\ARM64\Debug --build=outdated -s build_type=Debug -s arch=armv8 -s compiler.version=17 -s compiler.runtime=MDd -s mimalloc:compiler.runtime=MTd --update
conan install ..\Updater\conanfile.txt --install-folder ..\..\.conan\ARM64\Debug\Updater --build=outdated -s build_type=Debug -s arch=armv8 -s compiler="Visual Studio" -s compiler.version=17 -s compiler.runtime=MTd --update
