#!/usr/bin/env bash

# External header lists
declare -A stlHeaders=()
declare -A qtHeaders=()
declare -A otherHeaders=()

# Internal header lists
declare -A publicHeaders=()
declare -A privateHeaders=()

function addToExtensionLessHeaderList()
{
	local include=$1

	# Is it a Qt header
	local qtRegex="^<Q((t[A-Z])|([A-Z]))"
	if [[ $include =~ $qtRegex ]]
	then
		qtHeaders[$include]=1
	else
		stlHeaders[$include]=1
	fi
}

function processPrivateHeaders()
{
	local privateFolder="$1"
	local publicFolder="$2"
	shift
	shift

	# We want to match both <> and "" includes
	local includes=$(find "$privateFolder" "$@" -iname "*.[hc]pp" | xargs grep -hPo "^#[t]*include[ ]*\K[\"<][^\">]+[\">]" | sort -u)
	local noExtRegex="^<[^\.]+>$"
	local otherRegex="^<.+>$"
	for include in $includes
	do
		# Process extension-less headers (Qt, STL)
		if [[ $include =~ $noExtRegex ]]
		then
			addToExtensionLessHeaderList $include
		else
			# Process other headers (included as <>)
			if [[ $include =~ $otherRegex ]]
			then
				otherHeaders[$include]=1
			else
				# Extract the actual file
				local fileName="${include//\"/}"
				# Found the file in own public includes
				if [ -f "$publicFolder/$fileName" ]
				then
					publicHeaders[$include]=1
				elif [ $fileName != "pch.hpp" ]
				then
					# Check if the header is found in the path (otherwise if might fail to be included)
					if [ -f "$privateFolder/$fileName" ]
					then
						privateHeaders[$include]=1
					else
						echo "WARNING: Header is not PCH compliant (path not direct from '$privateFolder'): $include"
					fi
				fi
			fi
		fi
	done
}

function processPublicHeaders()
{
	local includeFolder="$1"
	shift

	# We don't want to match "" includes
	local includes=$(find "$includeFolder" "$@" -iname "*.[hc]pp" | xargs grep -hPo "^#[t]*include[ ]*\K<[^>]+[>]" | sort -u)
	local noExtRegex="^<[^\.]+>$"
	for include in $includes
	do
		# Process extension-less headers (Qt, STL)
		if [[ $include =~ $noExtRegex ]]
		then
			addToExtensionLessHeaderList $include
		else
			otherHeaders[$include]=1
		fi
	done

	# List all public headers and add them
	includes=$(find "$includeFolder" "$@" -iname "*.[hc]pp")
	for include in $includes
	do
		publicHeaders[\"${include/include\//}\"]=1
	done
}

function sortAndDump()
{
	local tab
	eval "declare -A tab="${1#*=}

	# Alphabetically sort
	IFS=$'\n' sorted=($(sort <<<"${!tab[*]}"))

	# Slashes count sort
	for i in {0..10}
	do
		for include in ${sorted[@]}
		do
			local slashes="${include//[^\/]}"
			if [ ${#slashes} -eq $i ]
			then
				echo "#include $include"
			fi
		done
	done
}

declare -A publicFolders=()
declare -A privateFolders=()
currentPublicFolder=""
currentPrivateFolder=""

if [ $# -eq 0 ]
then
	echo "WARNING: No folder specified, nothing to do. See help (-h)"
	exit 1
fi

while [ $# -gt 0 ]
do
	case "$1" in
		-h)
			echo "Usage: list_includes.sh [options]"
			echo " -h -> Display this help"
			echo " -public <public source folder> [exclusion path] -> Add a public source folder to process, with an optional exclusion path (* possible)"
			echo " -private <private source folder> <public source folder> [exclusion path] -> Add a private source folder with it's matching public source folder, with an optional exclusion path (* possible)"
			exit 3
			;;
		-public)
			shift
			if [ $# -lt 1 ]; then
				echo "ERROR: Missing parameter for -public option, see help (-h)"
				exit 4
			fi
			currentPrivateFolder=""
			currentPublicFolder="$1"
			publicFolders["$currentPublicFolder"]=""
			;;
		-private)
			shift
			if [ $# -lt 2 ]; then
				echo "ERROR: Missing parameter for -private option, see help (-h)"
				exit 4
			fi
			currentPublicFolder=""
			currentPrivateFolder="$1;$2"
			privateFolders["$currentPrivateFolder"]=""
			shift
			;;
		*)
			if [[ "$currentPublicFolder" == "" ]] && [[ "$currentPrivateFolder" == "" ]]
			then
				echo "ERROR: Unknown option '$1' (use -h for help)"
				exit 4
			fi
			if [[ "$currentPublicFolder" != "" ]]
			then
				publicFolders["$currentPublicFolder"]="$1"
				currentPublicFolder=""
			elif [[ "$currentPrivateFolder" != "" ]]
			then
				privateFolders["$currentPrivateFolder"]="$1"
				currentPrivateFolder=""
			else
				echo "INTERNAL ERROR"
			fi
			;;
	esac
	shift
done

if [ ${#publicFolders[*]} -gt 0 ]
then
	declare -a publicParams=()

	for path in "${!publicFolders[@]}"
	do
		publicParams+=("$path")
		if [[ ${publicFolders["$path"]} != "" ]]
		then
			publicParams+=("-not")
			publicParams+=("-path")
			publicParams+=("${publicFolders["$path"]}")
		fi
	done

	processPublicHeaders "${publicParams[@]}"
fi

if [ ${#privateFolders[*]} -gt 0 ]
then
	declare -a privateParams=()

	for path in "${!privateFolders[@]}"
	do
		privateParams+=("${path%;*}")
		privateParams+=("${path##*;}")
		if [[ ${privateFolders["$path"]} != "" ]]
		then
			privateParams+=("-not")
			privateParams+=("-path")
			privateParams+=("${privateFolders["$path"]}")
		fi
	done

	processPrivateHeaders "${privateParams[@]}"
fi

# Dump headers
if [ ${#publicHeaders[*]} -gt 0 ]
then
	echo "// Public Headers"
	sortAndDump "$(declare -p publicHeaders)"
	echo ""
fi

if [ ${#privateHeaders[*]} -gt 0 ]
then
	echo "// Private Headers"
	sortAndDump "$(declare -p privateHeaders)"
	echo ""
fi

if [ ${#otherHeaders[*]} -gt 0 ]
then
	echo "// Other Headers"
	sortAndDump "$(declare -p otherHeaders)"
	echo ""
fi

if [ ${#qtHeaders[*]} -gt 0 ]
then
	echo "// Qt Headers"
	sortAndDump "$(declare -p qtHeaders)"
	echo ""
fi

if [ ${#stlHeaders[*]} -gt 0 ]
then
	echo "// STL Headers"
	sortAndDump "$(declare -p stlHeaders)"
	echo ""
fi

echo "CAUTION: It's currently not possible to differentiate OS-Dependant includes, so you'll probably have to tweak the list manually!"
