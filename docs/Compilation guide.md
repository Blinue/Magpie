### Prerequisites

In order to compile Magpie, you need to first install:

1. The latest version of Visual Studio 2022. You need to install both "Desktop development with C++" and "Universal Windows Platform development" workloads and Windows SDK build 22621 or newer.
2. [CMake](https://cmake.org/)

   You can also use the built-in CMake of Visual Studio, which is located at `%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin`.
3. [Python](https://www.python.org/) 3.11+
4. [Conan](https://conan.io/)

   ```bash
   pip install conan
   ```
   
   Make sure that the above dependencies have been added to the system path, and use the following commands to check:
   ```bash
   cmake --version
   python --version
   conan --version
   ```

### Compiling

1. Clone the repo

   ```bash
   git clone https://github.com/Blinue/Magpie
   ```

2. Open the Magpie.sln in the root directory and build the solution.

### Enabling Touch Support

To enable touch input support, TouchHelper.exe needs to be signed. While signing is automatically done in the CI pipeline, you can also manually sign it. Follow these steps:

1. Create a self-signed certificate and export it as a pfx file.
2. Replace the `CERT_FINGERPRINT` constant in `src/Magpie/TouchHelper.cpp` with the SHA-1 hash (i.e., fingerprint) of your certificate.
3. Run the following command in the root directory of the repository:

```bash
python publish.py x64 unpackaged <pfx path> <pfx password>
```

This will compile Magpie and sign TouchHelper.exe. The compiled files will be located in `publish\x64`.
