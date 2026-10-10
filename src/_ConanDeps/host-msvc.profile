[settings]
os=Windows
compiler=msvc
compiler.cppstd=17
compiler.runtime=static
compiler.version=195

[tool_requires]
cmake/[>=4.4 <4.5]

[options]
spdlog/*:header_only=True
spdlog/*:wchar_filenames=True
spdlog/*:no_exceptions=True

[conf]
tools.info.package_id:confs=["tools.build:cxxflags", "tools.build:defines", "user.magpie:msbuild_version"]
# 应同步更改项目的宏定义
imgui/*:tools.build:defines=["IMGUI_DISABLE_OBSOLETE_FUNCTIONS", "IMGUI_DISABLE_DEFAULT_FONT", "ImTextureID=ImU32", "ImTextureID_Invalid=((ImTextureID)-1)"]
