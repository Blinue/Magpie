[settings]
os=Windows
compiler=clang
compiler.version=19
compiler.runtime=static
compiler.cppstd=gnu17

[conf]
tools.cmake.cmaketoolchain:generator=Visual Studio 17
tools.info.package_id:confs=["tools.build:cxxflags"]
