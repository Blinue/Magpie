[settings]
os=Windows
compiler=clang
compiler.version=20
compiler.runtime=static
compiler.cppstd=gnu17

[conf]
tools.cmake.cmaketoolchain:generator=Visual Studio 18
tools.info.package_id:confs=["tools.build:cxxflags", "user.magpie:msbuild_version"]
