### Prerequisites

In order to compile Magpie, you need to first install:

1. The latest version of Visual Studio 2022. You need to install both "Desktop development with C++" and "Universal Windows Platform development" workloads and Windows SDK build 22621 or newer.
2. [CMake](https://cmake.org/)

   You can also use the built-in CMake of Visual Studio, which is located at `%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin`.
3. [Python](https://www.python.org/) 3.6+
4. [Conan](https://conan.io/) v1

   ```bash
   pip install conan
   ```
   
   Make sure that the above dependencies have been added to the system path, and use the following commands to check:
   ```bash
   cmake --version
   python --version
   conan --version
   ```

### Compile

1. Clone the repo

   ```bash
   git clone https://github.com/Blinue/Magpie
   ```

2. Open the Magpie.sln in the root directory and build the solution.
