# libcadence [![C++ standard][c++-standard-shield]][c++-standard-link] [![License][license-shield]][license-link]
---
[![TravisCI][travis-shield]][travis-link]
[![Codecov][codecov-shield]][codecov-link]
[![Coverity][coverity-shield]][coverity-link]
[![Coverage Status][coveralls-shield]][coveralls-link]

[c++-standard-shield]: https://img.shields.io/badge/c%2B%2B-14/17/20-blue.svg
[c++-standard-link]: https://en.wikipedia.org/wiki/C%2B%2B#Standardization
[license-shield]: https://img.shields.io/badge/License-Apache%202.0-blue.svg
[license-link]: https://opensource.org/licenses/Apache-2.0
[travis-shield]: https://travis-ci.org/abbyssoul/libcadence.png?branch=master
[travis-link]: https://travis-ci.org/abbyssoul/libcadence
[codecov-shield]: https://codecov.io/gh/abbyssoul/libcadence/branch/master/graph/badge.svg
[codecov-link]: https://codecov.io/gh/abbyssoul/libcadence
[coverity-shield]: https://scan.coverity.com/projects/9728/badge.svg
[coverity-link]: https://scan.coverity.com/projects/abbyssoul-libcadence
[coveralls-shield]: https://coveralls.io/repos/github/abbyssoul/libcadence/badge.svg?branch=master
[coveralls-link]: https://coveralls.io/github/abbyssoul/libcadence?branch=master

libcadence is a _library_ of IPC components.
> library: a collection of types, functions, classes, etc. implementing a set of facilities (abstractions) meant to be potentially used as part of more that one program. From [Cpp Code guidelines gloassay](http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#glossary)

See [examples](docs/examples.md) for some examples of using this library.


## Dependencies
The implementation is based on [ASIO](https://github.com/chriskohlhoff/asio) for async communication.
The long term plan is to switch to STD networking module.

It also depends on [libsolace](https://github.com/abbyssoul/libsolace) for low level data manipulation primitives
such as ByteReader/ByteWriter and Result<> types.


### GTest
Note test framework used is *gtest* and it is managed via git modules.
Don't forget to do `git submodule update --init --recursive` on a new checkout to pull sub-module dependencies.


## Contributing changes
The framework is work in progress and contributions are very welcomed.
Please see  [`CONTRIBUTING.md`](CONTRIBUTING.md) for details on how to contribute to
this project.

Please note that in order to maintain code quality a set of static code analysis tools is used as part of the build process.
Thus all contributions must be verified by this tools before PR can be accepted.


# Building

### Build tool dependencies
In order to build this project following tools must be present in the system:
* git (to check out project and it’s external modules, see dependencies section)
* doxygen (for documentation)
* cppcheck (static code analysis, latest version from git is used as part of the 'codecheck' step)
* cpplint (for static code analysis in addition to cppcheck)
* valgrind (for runtime code quality verification)

This project is using C++17 features extensively. The minimal tested/required version of gcc is gcc-7.
[CI](https://travis-ci.org/abbyssoul/libcadence) is using clang-6 and gcc-7.
To install build tools on Debian based Linux distribution:
```shell
sudo apt-get update -qq
sudo apt-get install git doxygen python-pip valgrind ggcov
sudo pip install cpplint
```

The library has one external dependency: [libsolace](https://github.com/abbyssoul/libsolace).
Please make sure it is installed and available in the build path before building the project.

## Building the project
```shell
# In the project check-out directory:
# To build debug version with sanitizer enabled (recommended for development)
./configure --enable-debug --enable-sanitizer

# To build the library it self
make

# To build and run unit tests:
make test

# To run valgrind on test suit:
# Note: `valgrind` doesn’t work with ./configure --enable-sanitize option
make verify

# To build API documentation using doxygen:
make doc
```

To install locally for testing:
```shell
make --prefix=/user/home/<username>/test/lib install
```
To install system wide (as root):
```shell
make install
```
To run code quality check before submission of a patch:
```shell
# Verify code quality before submission
make codecheck
```

## License
Please see LICENSE file for details


## Authors
Please see AUTHORS file for the list of contributors

