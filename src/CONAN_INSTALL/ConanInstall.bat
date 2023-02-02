conan --version

IF %ERRORLEVEL% NEQ 0 (
	ECHO Error: Conan not found
	EXIT 1
)

conan config set storage.path=%CD%\..\..\.conan\data

IF %1 == Debug (
	IF %2 == x64 (
		conan install ..\conanfile.py --install-folder ..\..\.conan\x64\Debug --build=outdated -s build_type=Debug -s arch=x86_64 -s compiler.version=17 -s compiler.runtime=MTd --update
	) ELSE (
		conan install ..\conanfile.py --install-folder ..\..\.conan\ARM64\Debug --build=outdated -s build_type=Debug -s arch=armv8 -s compiler.version=17 -s compiler.runtime=MTd --update
	)
) ELSE (
	IF %2 == x64 (
		conan install ..\conanfile.py --install-folder ..\..\.conan\x64\Release --build=outdated -s build_type=Release -s arch=x86_64 -s compiler.version=17 -s compiler.runtime=MT --update
	) ELSE (
		conan install ..\conanfile.py --install-folder ..\..\.conan\ARM64\Release --build=outdated -s build_type=Release -s arch=armv8 -s compiler.version=17 -s compiler.runtime=MT --update
	)
)
