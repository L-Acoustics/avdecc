# Avdecc Library

# Minimum CMake version
cmake_minimum_required(VERSION 3.6)

############ Versioning

# Using Semantic Versioning (http://semver.org/spec/v2.0.0.html)
# Project version number x.y.z on main branch, x.y.z.w on dev and task branches
# Example: 1.5.0 is the upcoming release version (main branch) of current 1.5.0.3 devel version (dev/task)
set(LA_AVDECC_VERSION 2.0.10.1)
set(LA_AVDECC_CONTROLLER_VERSION 2.0.2.1)

############ Override from command line "CMake -D<OPTION>=TRUE/FALSE/0/1/ON/OFF"

# Build options
option(BUILD_AVDECC_EXAMPLES "Build examples." FALSE)
option(BUILD_AVDECC_TESTS "Build unit tests." FALSE)
option(BUILD_AVDECC_LIB_SHARED_CXX "Build C++ shared library." TRUE)
option(BUILD_AVDECC_LIB_STATIC_RT_SHARED "Build static library (runtime shared)." TRUE)
option(BUILD_AVDECC_DOC "Build documentation." FALSE)
option(BUILD_AVDECC_CONTROLLER "Build avdecc controller library." TRUE)
option(BUILD_AVDECC_INTERFACE_PCAP "Build the pcap protocol interface." TRUE)
option(BUILD_AVDECC_INTERFACE_PCAP_DYNAMIC_LINKING "Pcap protocol interface uses dynamic shared library linking (instead of static linking)." FALSE)
option(BUILD_AVDECC_INTERFACE_MAC "Build the macOS native protocol interface (macOS only)." TRUE)
option(BUILD_AVDECC_INTERFACE_PROXY "Build the proxy protocol interface." FALSE)
# Install options
option(INSTALL_AVDECC_EXAMPLES "Install examples." FALSE)
option(INSTALL_AVDECC_TESTS "Install unit tests." FALSE)
option(INSTALL_AVDECC_LIB_SHARED_CXX "Install C++ shared library." TRUE)
option(INSTALL_AVDECC_LIB_STATIC "Install static library." FALSE)
option(INSTALL_AVDECC_HEADERS "Install headers." TRUE)
option(INSTALL_AVDECC_DOC "Install documentation." TRUE)
# Signing options
option(ENABLE_AVDECC_SIGNING "Enable binaries signing." FALSE)
# Compatibility options
option(IGNORE_INVALID_CONTROL_DATA_LENGTH "Allow messages with an invalid advertised 'Control Data Length' field." TRUE) # Required for Motu devices sending invalid response messages
option(IGNORE_INVALID_NON_SUCCESS_AEM_RESPONSES "Allow invalid AEM non-success responses messages." TRUE) # Required for Motu/Tesira devices sending invalid error response messages

############ CMake configuration

project(LA_avdecc)

# Using c++14 standard for all targets
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Set minimum OSX version
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.9 CACHE INTERNAL "Force the target to be at least a Mac OS X 10.9" FORCE)

# Enable cmake folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Configure installation path: we override the default installation path.
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX "./Install" CACHE PATH "default install path" FORCE)
endif()

# Include our internal macros
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/internals.cmake)

# Include common properties overrides
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CommonProjectProperties.cmake)

############ Define some variables

set(TARGET_ARCH 32)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(TARGET_ARCH 64)
endif()
set(VS_USE_CLANG FALSE)
if(MSVC AND ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
	set(VS_USE_CLANG TRUE)
endif()

############ Check dependencies

# avdecc-tests needs avdecc.lib
if(BUILD_AVDECC_TESTS)
	set(BUILD_AVDECC_LIB_STATIC_RT_SHARED TRUE CACHE BOOL "Build avdecc static library (runtime shared)." FORCE)
endif()

# avdecc-examples needs avdecc.lib
if(BUILD_AVDECC_EXAMPLES)
	set(BUILD_AVDECC_LIB_STATIC_RT_SHARED TRUE CACHE BOOL "Build avdecc static library (runtime shared)." FORCE)
endif()

if(NOT CMAKE_HOST_APPLE AND BUILD_AVDECC_INTERFACE_MAC)
	set(BUILD_AVDECC_INTERFACE_MAC FALSE)
endif()

if(BUILD_AVDECC_INTERFACE_PROXY)
	message(FATAL_ERROR "Proxy interface not supported yet.")
endif()

if(NOT BUILD_AVDECC_INTERFACE_PCAP AND NOT BUILD_AVDECC_INTERFACE_MAC AND NOT BUILD_AVDECC_INTERFACE_PROXY)
	message(FATAL_ERROR "At least one valid protocol interface must be built.")
endif()

############ Define pcap variables

if(BUILD_AVDECC_INTERFACE_PCAP)
	if(WIN32)
		# Check if winpcap is installed
		if(NOT EXISTS "${CMAKE_SOURCE_DIR}/externals/3rdparty/winpcap/Include" OR NOT EXISTS "${CMAKE_SOURCE_DIR}/externals/3rdparty/winpcap/Lib")
			message(FATAL_ERROR "WinPCap Developer's Pack not properly installed. See ${CMAKE_SOURCE_DIR}/externals/3rdparty/winpcap/README.md for installation instructions")
		endif()
		# Define base folder for winpcap
		set(LA_WINPCAP_BASE_DIR "${CMAKE_SOURCE_DIR}/externals/3rdparty/winpcap")
		if(${TARGET_ARCH} EQUAL 32)
			set(LA_WINPCAP_LIB_DIR "${LA_WINPCAP_BASE_DIR}/Lib")
		else()
			set(LA_WINPCAP_LIB_DIR "${LA_WINPCAP_BASE_DIR}/Lib/x64")
		endif()
		# Link directory for wpcap
		if(NOT ${BUILD_AVDECC_INTERFACE_PCAP_DYNAMIC_LINKING})
			link_directories(${LA_WINPCAP_LIB_DIR})
		endif()
	endif()
endif()

############ Add projects

# Add main library
message(STATUS "Building Avdecc library")
add_subdirectory(src)

# Add controller library
if(BUILD_AVDECC_CONTROLLER)
	message(STATUS "Building Avdecc Controller library")
	add_subdirectory(src/controller)
endif()

# Add doc
if(BUILD_AVDECC_DOC)
	message(STATUS "Building documentation")
	add_subdirectory(doc)
endif()

# Add examples
if(BUILD_AVDECC_EXAMPLES)
	message(STATUS "Building examples")
	add_subdirectory(examples)
	# Force SimpleController as visual default target
	set_directory_properties(PROPERTIES VS_STARTUP_PROJECT SimpleController)
endif()

# Add tests
if(BUILD_AVDECC_TESTS)
	message(STATUS "Building unit tests")
	# Setup gtest options
	set(gtest_force_shared_crt ON CACHE BOOL "For shared crt for gtest" FORCE)
	# Include gtest framework
	add_subdirectory(externals/3rdparty/gtest/googletest)
	# Include our unit tests
	add_subdirectory(tests)
endif()
