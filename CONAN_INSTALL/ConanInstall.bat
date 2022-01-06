conan --version

IF %ERRORLEVEL% NEQ 0 (
    ECHO Error: Conan not found
    EXIT 1
)

conan config set storage.path=%CD%\..\.conan\data

conan install ..\Runtime\conanfile.txt --profile ../conanprofile.txt -g visual_studio --install-folder ..\.conan\Debug\Runtime -s build_type=Debug --build=outdated --update
conan install ..\Runtime\conanfile.txt --profile ../conanprofile.txt -g visual_studio --install-folder ..\.conan\Release\Runtime -s build_type=Release --build=outdated --update
