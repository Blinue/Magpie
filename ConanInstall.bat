conan --version

conan install .\EffectCommon\conanfile.py -g visual_studio --install-folder .\.conan\Debug\EffectCommon -s arch=x86_64 -s build_type=Debug --build=missing --update
conan install .\EffectCommon\conanfile.py -g visual_studio --install-folder .\.conan\Release\EffectCommon -s arch=x86_64 -s build_type=Release --build=missing --update

conan install .\MODULE_ACNet\conanfile.py -g visual_studio --install-folder .\.conan\Debug\MODULE_ACNet -s arch=x86_64 -s build_type=Debug --build=missing --update
conan install .\MODULE_ACNet\conanfile.py -g visual_studio --install-folder .\.conan\Release\MODULE_ACNet -s arch=x86_64 -s build_type=Release --build=missing --update

conan install .\MODULE_Anime4K\conanfile.py -g visual_studio --install-folder .\.conan\Debug\MODULE_Anime4K -s arch=x86_64 -s build_type=Debug --build=missing --update
conan install .\MODULE_Anime4K\conanfile.py -g visual_studio --install-folder .\.conan\Release\MODULE_Anime4K -s arch=x86_64 -s build_type=Release --build=missing --update

conan install .\MODULE_Common\conanfile.py -g visual_studio --install-folder .\.conan\Debug\MODULE_Common -s arch=x86_64 -s build_type=Debug --build=missing --update
conan install .\MODULE_Common\conanfile.py -g visual_studio --install-folder .\.conan\Release\MODULE_Common -s arch=x86_64 -s build_type=Release --build=missing --update

conan install .\MODULE_FFX\conanfile.py -g visual_studio --install-folder .\.conan\Debug\MODULE_FFX -s arch=x86_64 -s build_type=Debug --build=missing --update
conan install .\MODULE_FFX\conanfile.py -g visual_studio --install-folder .\.conan\Release\MODULE_FFX -s arch=x86_64 -s build_type=Release --build=missing --update

conan install .\MODULE_FSRCNNX\conanfile.py -g visual_studio --install-folder .\.conan\Debug\MODULE_FSRCNNX -s arch=x86_64 -s build_type=Debug --build=missing --update
conan install .\MODULE_FSRCNNX\conanfile.py -g visual_studio --install-folder .\.conan\Release\MODULE_FSRCNNX -s arch=x86_64 -s build_type=Release --build=missing --update

conan install .\MODULE_RAVU\conanfile.py -g visual_studio --install-folder .\.conan\Debug\MODULE_RAVU -s arch=x86_64 -s build_type=Debug --build=missing --update
conan install .\MODULE_RAVU\conanfile.py -g visual_studio --install-folder .\.conan\Release\MODULE_RAVU -s arch=x86_64 -s build_type=Release --build=missing --update

conan install .\MODULE_SSIM\conanfile.py -g visual_studio --install-folder .\.conan\Debug\MODULE_SSIM -s arch=x86_64 -s build_type=Debug --build=missing --update
conan install .\MODULE_SSIM\conanfile.py -g visual_studio --install-folder .\.conan\Release\MODULE_SSIM -s arch=x86_64 -s build_type=Release --build=missing --update

conan install .\Runtime\conanfile.py -g visual_studio --install-folder .\.conan\Debug\Runtime -s arch=x86_64 -s build_type=Debug --build=missing --update
conan install .\Runtime\conanfile.py -g visual_studio --install-folder .\.conan\Release\Runtime -s arch=x86_64 -s build_type=Release --build=missing --update

pause