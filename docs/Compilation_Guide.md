### Prerequisites

In order to compile Magpie, you need to first install:

1. The latest version of Visual Studio 2022. You need to install both "Desktop development with C++" and "Universal Windows Platform development" workloads and Windows SDK build 22621 or newer.
2. [Python](https://www.python.org/) 3.6+
3. [Conan](https://conan.io/) v1

   ```bash
   pip install conan<2.0
   ```
   To execute in cmd, double quotes should be added around `conan<2.0`. Make sure Conan has been added to the system path and use the following command to check:
   ```bash
   conan --version
   ```

### Compile and Run

1. Clone the repo

   ```bash
   git clone https://github.com/Blinue/Magpie
   ```

2. Open the Magpie.sln in the root directory using Visual Studio 2022. This solution contains multiple projects, among which the "Magpie" project is the program's entry point. It should already be the startup project, but if it isn't, please set it manually.

3. First, build the CONAN_INSTALL project, which will install the dependencies.

5. Compile and run Magpie.
