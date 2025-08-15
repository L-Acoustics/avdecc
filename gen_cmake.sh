#!/usr/bin/env bash
# Useful script to generate project files using cmake

# Override default cmake options
cmake_opt="-DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE -DBUILD_AVDECC_EXAMPLES=TRUE -DBUILD_AVDECC_TESTS=TRUE -DBUILD_AVDECC_LIB_SHARED_CXX=TRUE -DBUILD_AVDECC_LIB_STATIC_RT_SHARED=TRUE -DBUILD_AVDECC_DOC=TRUE -DBUILD_AVDECC_CONTROLLER=TRUE -DINSTALL_AVDECC_EXAMPLES=TRUE -DINSTALL_AVDECC_TESTS=TRUE -DINSTALL_AVDECC_LIB_SHARED_CXX=TRUE -DINSTALL_AVDECC_LIB_STATIC=TRUE -DINSTALL_AVDECC_HEADERS=TRUE -DINSTALL_AVDECC_DOC=TRUE -DENABLE_CODE_SIGNING=FALSE -DFETCHCONTENT_TRY_FIND_PACKAGE_MODE=NEVER"

# Get absolute folder for this script
selfFolderPath="`cd "${BASH_SOURCE[0]%/*}"; pwd -P`/" # Command to get the absolute path

# Include util functions
. "${selfFolderPath}scripts/bashUtils/utils.sh"

if isMac; then
	cmake_opt="$cmake_opt -DBUILD_AVDECC_INTERFACE_MAC=TRUE -DBUILD_AVDECC_INTERFACE_PCAP_DYNAMIC_LINKING=FALSE -DBUILD_AVDECC_INTERFACE_VIRTUAL=TRUE"
elif isWindows; then
	cmake_opt="$cmake_opt -DBUILD_AVDECC_INTERFACE_PCAP=TRUE -DBUILD_AVDECC_INTERFACE_PCAP_DYNAMIC_LINKING=TRUE -DBUILD_AVDECC_INTERFACE_VIRTUAL=TRUE"
else
	cmake_opt="$cmake_opt -DBUILD_AVDECC_INTERFACE_PCAP=TRUE -DBUILD_AVDECC_INTERFACE_PCAP_DYNAMIC_LINKING=FALSE -DBUILD_AVDECC_INTERFACE_VIRTUAL=TRUE"
fi

############################ DO NOT MODIFY AFTER THAT LINE #############

# Include default values
if [ -f "${selfFolderPath}.defaults.sh" ]; then
	. "${selfFolderPath}.defaults.sh"
fi

# Parse variables
gen_doc=1
gen_c=0
gen_csharp=0

function extend_gc_fnc_help()
{
	echo " -no-doc -> Do not generate (nor install) documentation."
	echo " -build-c -> Build the C bindings library."
	echo " -build-csharp -> Build the C# bindings library."
}

function extend_gc_fnc_unhandled_arg()
{
	case "$1" in
		-no-doc)
			gen_doc=0
			return 1
			;;
		-build-c)
			gen_c=1
			return 1
			;;
		-build-csharp)
			gen_csharp=1
			return 1
			;;
	esac
	return 0
}

# function extend_gc_fnc_postparse()
# {
# }

function extend_gc_fnc_precmake()
{
	if [ $gen_doc -eq 0 ]; then
		add_cmake_opt+=("-DBUILD_AVDECC_DOC=FALSE")
		add_cmake_opt+=("-DINSTALL_AVDECC_DOC=FALSE")
	fi
	if [ $gen_c -eq 1 ]; then
		add_cmake_opt+=("-DBUILD_AVDECC_BINDINGS_C=TRUE")
		add_cmake_opt+=("-DINSTALL_AVDECC_BINDINGS=TRUE")
	fi
	if [ $gen_csharp -eq 1 ]; then
		add_cmake_opt+=("-DBUILD_AVDECC_BINDINGS_CSHARP=TRUE")
		add_cmake_opt+=("-DINSTALL_AVDECC_BINDINGS=TRUE")
		add_cmake_opt+=("-DAVDECC_SWIG_LANGUAGES=csharp")
	fi
}

function boolToString()
{
	if [ $1 -eq 1 ]; then
		echo "true"
	else
		echo "false"
	fi
}

function extend_gc_fnc_props_summary()
{
	echo "| - Doc: $(boolToString $gen_doc)"
	echo "| - C Bindings: $(boolToString $gen_c)"
	echo "| - C# Bindings: $(boolToString $gen_csharp)"
}

# execute gen_cmake script from bashUtils
. "${selfFolderPath}scripts/bashUtils/gen_cmake.sh"
