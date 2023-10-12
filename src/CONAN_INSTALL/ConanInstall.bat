cd ..\Magpie.Core
python ..\conan_install.py %2 %1

cd ..\Magpie.App
python ..\conan_install.py %2 %1

cd ..\Magpie
python ..\conan_install.py %2 %1
