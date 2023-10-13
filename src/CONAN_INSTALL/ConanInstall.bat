conan --version

IF %ERRORLEVEL% NEQ 0 (
	ECHO Error: Conan not found
	EXIT 1
)

conan config set storage.path=%CD%\..\..\.conan\data

IF %1 == Debug (
	IF %2 == x64 (
		conan install ..\Magpie\conanfile.txt --install-folder ..\..\.conan\x64\Debug\Magpie --build=outdated -s build_type=Debug -s arch=x86_64 -s compiler.version=17 -s compiler.runtime=MTd --update
		conan install ..\Magpie.Core\conanfile.txt --install-folder ..\..\.conan\x64\Debug\Magpie.Core --build=outdated -s build_type=Debug -s arch=x86_64 -s compiler.version=17 -s compiler.runtime=MTd --update
		conan install ..\Magpie.App\conanfile.txt --install-folder ..\..\.conan\x64\Debug\Magpie.App --build=outdated -s build_type=Debug -s arch=x86_64 -s compiler.version=17 -s compiler.runtime=MTd --update
	) ELSE (
		conan install ..\Magpie\conanfile.txt --install-folder ..\..\.conan\ARM64\Debug\Magpie --build=outdated -s build_type=Debug -s arch=armv8 -s compiler.version=17 -s compiler.runtime=MTd --update
		conan install ..\Magpie.Core\conanfile.txt --install-folder ..\..\.conan\ARM64\Debug\Magpie.Core --build=outdated -s build_type=Debug -s arch=armv8 -s compiler.version=17 -s compiler.runtime=MTd --update
		conan install ..\Magpie.App\conanfile.txt --install-folder ..\..\.conan\ARM64\Debug\Magpie.App --build=outdated -s build_type=Debug -s arch=armv8 -s compiler.version=17 -s compiler.runtime=MTd --update
	)
) ELSE (
	IF %2 == x64 (
		conan install ..\Magpie\conanfile.txt --install-folder ..\..\.conan\x64\Release\Magpie --build=outdated -s build_type=Release -s arch=x86_64 -s compiler.version=17 -s compiler.runtime=MT --update
		conan install ..\Magpie.Core\conanfile.txt --install-folder ..\..\.conan\x64\Release\Magpie.Core --build=outdated -s build_type=Release -s arch=x86_64 -s compiler.version=17 -s compiler.runtime=MT --update
		conan install ..\Magpie.App\conanfile.txt --install-folder ..\..\.conan\x64\Release\Magpie.App --build=outdated -s build_type=Release -s arch=x86_64 -s compiler.version=17 -s compiler.runtime=MT --update
	) ELSE (
		conan install ..\Magpie\conanfile.txt --install-folder ..\..\.conan\ARM64\Release\Magpie --build=outdated -s build_type=Release -s arch=armv8 -s compiler.version=17 -s compiler.runtime=MT --update
		conan install ..\Magpie.Core\conanfile.txt --install-folder ..\..\.conan\ARM64\Release\Magpie.Core --build=outdated -s build_type=Release -s arch=armv8 -s compiler.version=17 -s compiler.runtime=MT --update
		conan install ..\Magpie.App\conanfile.txt --install-folder ..\..\.conan\ARM64\Release\Magpie.App --build=outdated -s build_type=Release -s arch=armv8 -s compiler.version=17 -s compiler.runtime=MT --update
	)
)
