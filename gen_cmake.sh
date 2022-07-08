#!/usr/bin/env bash
# Useful script to generate project files using cmake

# Override default cmake options
cmake_opt="-DBUILD_AVDECC_EXAMPLES=TRUE -DBUILD_AVDECC_TESTS=TRUE -DBUILD_AVDECC_LIB_SHARED_CXX=TRUE -DBUILD_AVDECC_LIB_STATIC_RT_SHARED=TRUE -DBUILD_AVDECC_DOC=TRUE -DBUILD_AVDECC_CONTROLLER=TRUE -DINSTALL_AVDECC_EXAMPLES=TRUE -DINSTALL_AVDECC_TESTS=TRUE -DINSTALL_AVDECC_LIB_SHARED_CXX=TRUE -DINSTALL_AVDECC_LIB_STATIC=TRUE -DINSTALL_AVDECC_HEADERS=TRUE -DINSTALL_AVDECC_DOC=TRUE -DENABLE_CODE_SIGNING=FALSE"

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

gen_doc=1

function extend_gc_fnc_help()
{
	echo " -no-doc -> Do not generate (nor install) documentation."
}

function extend_gc_fnc_unhandled_arg()
{
	case "$1" in
		-no-doc)
			gen_doc=0
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
}

# execute gen_cmake script from bashUtils
. "${selfFolderPath}scripts/bashUtils/gen_cmake.sh"
