include(./msvc.profile)

[settings]
compiler=clang
compiler.version=20
compiler.cppstd=gnu17

[conf]
tools.cmake.cmaketoolchain:generator=Visual Studio 18
