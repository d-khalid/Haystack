# Haystack
### Introduction
Pretend like this is a well written introduction.

## Build Dependencies
This section will provide the appropriate commands and/or links for installing the build prerequisites of this project on Windows 10/11 and Fedora. The current main requirements are CMake 3.16 or later, make/ninja and a C++ compiler that supports the C++17 standard.  

CLion is the recommended IDE for working on this project as it has great CMake integration and is generally a great IDE.

### Windows 
Install the ``scoop`` package manager by following the instructions on their [website](https://scoop.sh/). Then run:
```
$ scoop install cmake make ninja mingw
```
### Fedora
```
$ sudo dnf install cmake make ninja g++
```
## Build Instructions
1- Clone the repository
```
git clone git@github.com:d-khalid/Haystack.git
```
2- Make a build folder for CMake
```
cd Haystack & mkdir cmake-build
cd cmake-build
```
3- Use CMake to generate a Makefile  
```
# This will build all the build options (currently there's only one, will add new instructions here when there's multiple)
cmake ..
```
4- Use make to compile the executable binaries (cli executable will be in 'cli' folder)
```
make
```


