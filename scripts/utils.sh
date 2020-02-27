#!/usr/bin/env bash
# Utility bash functions

getOutputFolder()
{
	local _retval="$1"
	local basePath="$2"
	local arch="$3"
	local toolset="$4"
	local config="$5"
	local result=""

	if isMac; then
		result="${basePath}_${arch}"
	elif isWindows; then
		result="${basePath}_${arch}_${toolset}"
	else
		result="${basePath}_${arch}_${config}"
	fi

	eval $_retval="'${result}'"
}

getFileSize()
{
	local filePath="$1"
	local _retval="$2"
	local result=""

	if isMac;
	then
		result=$(stat -f%z "$filePath")
	else
		result=$(stat -c%s "$filePath")
	fi

	eval $_retval="'${result}'"
}

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
			# We have to check for WSL
			if [[ `uname -r` == *"Microsoft"* ]]; then
				result="win"
			else
				result="linux"
			fi
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

