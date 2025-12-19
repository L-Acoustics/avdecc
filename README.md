# LA AVDECC
[![Coverity Scan Build Status](https://img.shields.io/coverity/scan/14038.svg)](https://scan.coverity.com/projects/l-acoustics-avdecc)
[![Format](https://github.com/L-Acoustics/avdecc/actions/workflows/format.yml/badge.svg)](https://github.com/L-Acoustics/avdecc/actions/workflows/format.yml)
Copyright (C) 2016-2025, L-Acoustics and its contributors

## What is LA_avdecc
LA_avdecc is a set of open source libraries for controlling and monitoring AVB entities using the AVDECC protocol (IEEE 1722.1) compliant to Avnu Milan.

These libraries are written in pure C++17. They can be compiled on Windows, Linux and macOS, using standard development tools (procedure below). Unit tests and sample programs are also provided.

These libraries have already been used indirectly in many musical events throughout the world to control all kinds of AVB entities ([list below](#compatibleEntities)). L-Acoustics' Network Manager 2.5 (and up) now relies on them for all its AVDECC functionalities in compliance to the Avnu Milan Specifications.

Another benefit is the support of Apple’s native API, which allows control of the input and output AVB streams of a Mac from itself (which is not possible with the libraries using PCAP).
Bindings to other languages are also provided, and will continue in the future.

We use GitHub issues for tracking requests and bugs.

### Optional dependencies:
* [Google's C++ test framework](https://github.com/google/googletest) to build unit tests
* [WinPcap Developer's Pack](externals/3rdparty/winpcap/README.md) to build on Windows platform
* [libfmt](https://github.com/fmtlib/fmt) to format log messages
* [nlohmann JSON](https://github.com/nlohmann/json) to read and write JSON files

### <a name="compatibleEntities"></a>Tested AVB entities:
* L-Acoustics: LA4X, LA12X, LA2Xi, P1
* Biamp: Tesira Forte
* Avid: S6L
* MOTU: 112D, 828, Traveller, StageBox16
* MeyerSound: Galileo GALAXY, CAL
* QSC: Q-SYS Cores
* Apple: macOS Talker, Listener and Controller (El Capitan and later)
* AudioScience: Hono AVB Mini
* d&b audiotechnik: DS20

## la_avdecc library

Implementation of the IEEE Std 1722.1-2013 specification.  
Also implementing most of IEEE Std 1722.1-Corrigendum1-2018.
Also implementing AVnu Alliance Milan.
Also implementing AVnu Alliance Network Redundancy.

The library exposes APIs needed to create AVDECC entities on the local computer, and to interact with other entities on the network.

## la_avdecc_controller library

This is a simple library to create an AVDECC controller entity on the local computer. This controller automatically listens to and keeps track of the other entities on the network using the IEEE Std 1722.1 protocol.

The controller API has 2 interfaces:
- An observer interface to monitor all changes on discovered entities
- An interaction interface to send enumeration and control (AECP) or connection management (ACMP) requests to an entity

## la_avdecc_c library

C language bindings over la_avdecc library.

## Minimum requirements for compilation

### All platforms
- CMake 3.29

### Windows
- Windows 10
- Visual Studio 2022 v17.4 or greater (using platform toolset v143)
- WinPcap 4.1.2 Developer's Pack (see [this file](externals/3rdparty/winpcap/README.md) for more details)
- GitBash or cygwin

### macOS
- macOS 10.13
- Xcode 14

### Linux
- C++17 compliant compiler (for g++, v11.0 or greater)
- Make
- pcap developer package
- ncurses developer package (optional, for examples)

## Compilation

### All platforms
- Clone this repository
- Update submodules: *git submodule update --init --recursive*

### Windows
- [Install WinPcap Developer's Pack](externals/3rdparty/winpcap/README.md)
- Using the provided bash script (*gen_cmake.sh*):
  * Run the script with whatever optional parameters required (run *gen_cmake.sh -h* to display the help)
  * Go into the generated output folder
  * Open the generated Visual Studio solution *LA_avdecc.sln*
  * Compile everything from Visual Studio
- Manually issuing a CMake command:
  * Run a proper CMake command to generate a Visual Studio solution (or any other CMake generator matching your build toolchain)
  * Open the generated Visual Studio solution (or your other CMake generated files)
  * Compile everything from Visual Studio (or compile using your toolchain)
 
### macOS
- Using the provided bash script (*gen_cmake.sh*):
  * Run the script with whatever optional parameters required (run *gen_cmake.sh -h* to display the help)
  * Go into the generated output folder
  * Open the generated Xcode solution *LA_avdecc.xcodeproj*
  * Compile everything from Xcode
- Manually issuing a CMake command:
  * Run a proper CMake command to generate a Xcode solution (or any other CMake generator matching your build toolchain)
  * Open the generated Xcode solution (or your other CMake generated files)
  * Compile everything from Xcode (or compile using your toolchain)

### Linux
- Using the provided bash script (*gen_cmake.sh*):
  * Run the script with either *-debug* or *-release* and whatever optional parameters required (run *gen_cmake.sh -h* to display the help)
  * Go into the generated output folder
  * Run *make* to compile everything
- Manually issuing a CMake command:
  * Run a proper CMake command to generate Unix Makefiles (or any other CMake generator matching your build toolchain)
  * Go into the folder where the Unix Makefiles have been generated
  * Run *make* to compile everything (or compile using your toolchain)
- pcap library override:
  * If you need to override the pcap library name or path used for dynamic linking in the pcap protocol interface (default is 'libpcap.so'), you can pass the CMake parameter `-DOVERRIDE_PCAP_LIB_PATH="<your_pcap_library>"` when generating the build system (e.g. `-DOVERRIDE_PCAP_LIB_PATH="libpcap_custom.so"`)

## Cross-compilation using Docker

- Requires `docker` and `docker-compose` to be installed
- Go to the `Docker` folder
- Build the docker builder image: _docker-compose build_
- Generate the build solution: _docker-compose run --rm gen_cmake -debug -c Ninja_
  - You may change parameters to your convenience
- Build the solution: _docker-compose run --rm build --target install_
- You can then run any of the compiled examples or unit tests from the docker container: _APP=Tests-d docker-compose run --rm run_

## Known limitations

- [Windows] When plugging in a USB ethernet card for the first time, you either have to reboot the computer or restart the WinPCap driver (*net stop npf* then *net start npf*, from an elevated command prompt). Doesn't apply when using npcap.
- [Linux] Administrative privileges are required to run PCap applications. You can either directly run samples as root with `sudo` or setup capabilities using `sudo setcap cap_net_raw+ep <application path>` then directly run the application

## Upcoming features

- Better unit testing using a virtual protocol interface and virtual entities
- Ability to preload AEMXML files, and not enumerate AEM for devices with identical vendorEntityModelId
- Talker and Listener state machines (low level library)
- Creation of a DiscoveryStateMachine so it can be used by Talker/Listener entities (moving code out of ControllerStateMachine)
- Bindings libraries:
  * Lua (public and private APIs)

## Contributing code

[Please read this file](CONTRIBUTING.md)

## Trademark legal notice
All product names, logos, brands and trademarks are property of their respective owners. All company, product and service names used in this library are for identification purposes only. Use of these names, logos, and brands does not imply endorsement.
