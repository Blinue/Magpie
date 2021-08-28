conan --version

IF %ERRORLEVEL% NEQ 0 (
    ECHO Error: Conan not found
    EXIT 1
)

conan install ..\Runtime\conanfile.py -g visual_studio --install-folder ..\.conan\Debug\Runtime -s arch=x86_64 -s build_type=Debug --build=missing --update
conan install ..\Runtime\conanfile.py -g visual_studio --install-folder ..\.conan\Release\Runtime -s arch=x86_64 -s build_type=Release --build=missing --update
