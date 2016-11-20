## Installing

This library is tested on Debian, Fedora and Ubuntu Linux, as well as Microsoft Windows.

**Note: Currently, it builds, but does not properly run, on Mac OS X and FreeBSD and OpenBSD.
This is a work in progress.**

**Windows Note:**
The build process on Windows is rather involved (i.e. it is relatively difficult to set up the build system for first-time users). For convenience, the binaries/win64-msvc-1900-relwithdebinfo subdirectory contains a prebuilt version of the C++ example application and associated library. It was built with Microsoft Visual Studio 2015. Because of how Microsoft packages its C++ libraries on Windows, it might be necessary to first install the C++ runtime for Windows, [available from here](https://www.microsoft.com/en-us/download/details.aspx?id=48145).

Requirements:
--------------

- A C++ 2011-compatible compiler. This includes any recent version of gcc/g++, LLVM/clang or Visual C++ 2015.
- CMake (generates the build scripts and finds library locations)
- Doxygen (generates html documentation; optional, but highly recommended)
- Boost C++ libraries (provides lots of code features)
- Eigen3 libraries (linear algebra library)
- environment-modules (manipulates shell environment variables; optional, but highly recommended)
- HDF5 libraries (storage backend)
- git (needed to checkout the code; required because it provides information to the build scripts)
- ZLIB library (needed for compression of stored data)

On Debian and Ubuntu, the necessary dependencies may be installed using this command:
```
sudo apt install cmake doxygen environment-modules libboost-all-dev libeigen3-dev libhdf5-dev hdf5-tools git zlib1g-dev
```
On Fedora, this command may be used:
```
sudo dnf install cmake doxygen environment-modules boost-devel eigen3-devel hdf5-devel hdf5 git zlib-devel
```
On FreeBSD, use this command:
```
sudo pkg install hdf5 cmake doxygen modules boost-all eigen git
```

Building:
-------------

- Download (and perhaps extract) the source code package. 
- Create a new build directory. It can be anywhere. Switch into this directory.
- Run cmake to generate the build scripts. e.g.:
```
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX={install path} {Path to source directory}
```
- This command, on Linux, typically defaults to generating Makefiles and will use gcc as the compiler. Consult the CMake
   documentation for details regarding how to change the compiler or other settings.
   - To change the compiler, prefix the CMake command with CMAKE_CXX_COMPILER='path to compiler'.
- If cmake is set to generate Makefiles, run:
```
make
```
- If the build is successful, binaries and libraries should be in the ./Debug directory. These can all be copied
to the install directory using:
```
sudo make install
```
