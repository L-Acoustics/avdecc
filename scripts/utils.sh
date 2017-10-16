#!/bin/bash
# Utility bash functions

getOS()
{
	local _retval="$1"
	local result=""

	case "$OSTYPE" in
		msys)
			result="win"
			;;
		cygwin)
			result="win"
			;;
		darwin*)
			result="mac"
			;;
		linux*)
			result="linux"
			;;
		*)
			echo "ERROR: Unknown OSTYPE: $OSTYPE"
			exit 127
			;;
	esac

	eval $_retval="'${result}'"
}

# Returns 0 if running OS is windows
isWindows()
{
	local osName
	getOS osName
	if [[ $osName = win ]]; then
		return 0
	fi
	return 1
}

# Returns 0 if running OS is mac
isMac()
{
	local osName
	getOS osName
	if [[ $osName = mac ]]; then
		return 0
	fi
	return 1
}

# Returns 0 if running OS is linux
isLinux()
{
	local osName
	getOS osName
	if [[ $osName = linux ]]; then
		return 0
	fi
	return 1
}

# Returns 0 if running OS is cygwin
isCygwin()
{
	if [[ $OSTYPE = cygwin ]]; then
		return 0
	fi
	return 1
}

getCcArch()
{
	local _retval="$1"
	
	if isMac; then
		ccCommand="clang"
	else
		ccCommand="gcc"
	fi
	if [ ! -z "$CC" ]; then
		ccCommand="$CC"
	fi

	eval $_retval="'$($ccCommand -dumpmachine)'"
}

