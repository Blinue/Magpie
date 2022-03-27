### Prerequisites

In order to compile Magpie, you need to first install:

1. The latest version of Visual Studio 2022. You need to install both ".NET Desktop Development" and "Desktop Development with C++" workloads and Windows SDK build 22000 or newer.
2. [Conan](https://conan.io/) and its runtime environment [Python](https://www.python.org/). Make sure Conan is added to the PATH.

### Compile and Run

1. Clone the repo

   ```bash
   git clone https://github.com/Blinue/Magpie
   ```

2. Open `Magpie.sln` with Visual Studio 2022. The solution includes multiple projects. The "Magpie" project is the entrypoint of the program. The rest are launching projects. Please set them manually is they are not.

3. Generate the CONAN_INSTALL project, which will install all the Conan dependencies for the C++ projects.

4. Compile the Magpie project. The NuGet dependency will be install automatically in this process.

5. Run Magpie.

6. There might be errors raised by the IntelliSense for C++ projects. This is because the caches haven't been updated. You can try re-scanning the solutions or deleting the `.vs` folder.
