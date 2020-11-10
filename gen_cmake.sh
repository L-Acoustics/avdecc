#!/usr/bin/env bash
# Useful script to generate project files using cmake

# Get absolute folder for this script
selfFolderPath="`cd "${BASH_SOURCE[0]%/*}"; pwd -P`/" # Command to get the absolute path

# Include util functions
. "${selfFolderPath}scripts/bashUtils/utils.sh"

# Override default cmake options
cmake_opt="-DBUILD_AVDECC_EXAMPLES=TRUE -DBUILD_AVDECC_TESTS=TRUE -DBUILD_AVDECC_LIB_SHARED_CXX=TRUE -DBUILD_AVDECC_LIB_STATIC_RT_SHARED=TRUE -DBUILD_AVDECC_DOC=TRUE -DBUILD_AVDECC_CONTROLLER=TRUE -DINSTALL_AVDECC_EXAMPLES=TRUE -DINSTALL_AVDECC_TESTS=TRUE -DINSTALL_AVDECC_LIB_SHARED_CXX=TRUE -DINSTALL_AVDECC_LIB_STATIC=TRUE -DINSTALL_AVDECC_HEADERS=TRUE -DINSTALL_AVDECC_DOC=TRUE -DENABLE_AVDECC_SIGNING=FALSE"

if isMac; then
	cmake_opt="$cmake_opt -DBUILD_AVDECC_INTERFACE_MAC=TRUE -DBUILD_AVDECC_INTERFACE_PCAP_DYNAMIC_LINKING=FALSE -DBUILD_AVDECC_INTERFACE_VIRTUAL=TRUE"
elif isWindows; then
	cmake_opt="$cmake_opt -DBUILD_AVDECC_INTERFACE_PCAP=TRUE -DBUILD_AVDECC_INTERFACE_PCAP_DYNAMIC_LINKING=TRUE -DBUILD_AVDECC_INTERFACE_VIRTUAL=TRUE"
else
	cmake_opt="$cmake_opt -DBUILD_AVDECC_INTERFACE_PCAP=TRUE -DBUILD_AVDECC_INTERFACE_PCAP_DYNAMIC_LINKING=FALSE -DBUILD_AVDECC_INTERFACE_VIRTUAL=TRUE"
fi

############################ DO NOT MODIFY AFTER THAT LINE #############

# Default values
default_VisualGenerator="Visual Studio 16 2019"
default_VisualGeneratorArch="Win32"
default_VisualToolset="v142"
default_VisualToolchain="x64"
default_VisualArch="x86"
default_VisualSdk="8.1"
default_signtoolOptions="/a /sm /q /fd sha256 /tr http://timestamp.sectigo.com"

# 
cmake_generator=""
generator_arch=""
arch=""
toolset=""
cmake_config=""
outputFolderBasePath="_build"
if isMac; then
	cmake_path="/Applications/CMake.app/Contents/bin/cmake"
	# CMake.app not found, use cmake from the path
	if [ ! -f "${cmake_path}" ]; then
		cmake_path="cmake"
	fi
	generator="Xcode"
	getCcArch arch
	defaultOutputFolder="${outputFolderBasePath}_<arch>"
else
	# Use cmake from the path
	cmake_path="cmake"
	if isWindows; then
		generator="$default_VisualGenerator"
		generator_arch="$default_VisualGeneratorArch"
		toolset="$default_VisualToolset"
		toolchain="$default_VisualToolchain"
		platformSdk="$default_VisualSdk"
		arch="$default_VisualArch"
		defaultOutputFolder="${outputFolderBasePath}_<arch>_<toolset>"
	else
		generator="Unix Makefiles"
		getCcArch arch
		defaultOutputFolder="${outputFolderBasePath}_<arch>_<config>"
	fi
fi

which "${cmake_path}" &> /dev/null
if [ $? -ne 0 ]; then
	echo "CMake not found. Please add CMake binary folder in your PATH environment variable."
	exit 1
fi

outputFolder=""
outputFolderForced=0
add_cmake_opt=()
useVSclang=0
useVS2017=0
hasTeamId=0
signingId=""
doSign=0
signtoolOptions="$default_signtoolOptions"

while [ $# -gt 0 ]
do
	case "$1" in
		-h)
			echo "Usage: gen_cmake.sh [options]"
			echo " -h -> Display this help"
			echo " -o <folder> -> Output folder (Default: ${defaultOutputFolder})"
			echo " -f <flags> -> Force all cmake flags (Default: $cmake_opt)"
			echo " -a <flags> -> Add cmake flags to default ones (or to forced ones with -f option)"
			echo " -b <cmake path> -> Force cmake binary path (Default: $cmake_path)"
			echo " -c <cmake generator> -> Force cmake generator (Default: $generator)"
			if isWindows; then
				echo " -t <visual toolset> -> Force visual toolset (Default: $toolset)"
				echo " -tc <visual toolchain> -> Force visual toolchain (Default: $toolchain)"
				echo " -64 -> Generate the 64 bits version of the project (Default: 32)"
				echo " -vs2017 -> Compile using VS 2017 compiler instead of the default one"
				echo " -clang -> Compile using clang for VisualStudio"
				echo " -signtool-opt <options> -> Windows code signing options (Default: $default_signtoolOptions)"
			fi
			if isMac; then
				echo " -id <Signing Identity> -> Signing identity for binary signing (full identity name inbetween the quotes, see -ids to get the list)"
				echo " -ids -> List signing identities"
			fi
			echo " -debug -> Force debug configuration (Single-Configuration generators only)"
			echo " -release -> Force release configuration (Single-Configuration generators only)"
			echo " -sign -> Sign binaries (Default: No signing)"
			exit 3
			;;
		-o)
			shift
			if [ $# -lt 1 ]; then
				echo "ERROR: Missing parameter for -o option, see help (-h)"
				exit 4
			fi
			outputFolder="$1"
			outputFolderForced=1
			;;
		-f)
			shift
			if [ $# -lt 1 ]; then
				echo "ERROR: Missing parameter for -f option, see help (-h)"
				exit 4
			fi
			cmake_opt="$1"
			;;
		-a)
			shift
			if [ $# -lt 1 ]; then
				echo "ERROR: Missing parameter for -a option, see help (-h)"
				exit 4
			fi
			IFS=' ' read -r -a tokens <<< "$1"
			for token in ${tokens[@]}
			do
				add_cmake_opt+=("$token")
			done
			;;
		-b)
			shift
			if [ $# -lt 1 ]; then
				echo "ERROR: Missing parameter for -b option, see help (-h)"
				exit 4
			fi
			if [ ! -x "$1" ]; then
				echo "ERROR: Specified cmake binary is not valid (not found or not executable): $1"
				exit 4
			fi
			cmake_path="$1"
			;;
		-c)
			shift
			if [ $# -lt 1 ]; then
				echo "ERROR: Missing parameter for -c option, see help (-h)"
				exit 4
			fi
			cmake_generator="$1"
			;;
		-t)
			if isWindows; then
				shift
				if [ $# -lt 1 ]; then
					echo "ERROR: Missing parameter for -t option, see help (-h)"
					exit 4
				fi
				toolset="$1"
			else
				echo "ERROR: -t option is only supported on Windows platform"
				exit 4
			fi
			;;
		-tc)
			if isWindows; then
				shift
				if [ $# -lt 1 ]; then
					echo "ERROR: Missing parameter for -tc option, see help (-h)"
					exit 4
				fi
				toolchain="$1"
			else
				echo "ERROR: -tc option is only supported on Windows platform"
				exit 4
			fi
			;;
		-64)
			if isWindows; then
				generator="Visual Studio 15 2017 Win64"
				arch="x64"
			else
				echo "ERROR: -64 option is only supported on Windows platform"
				exit 4
			fi
			;;
		-vs2017)
			if isWindows; then
				useVS2017=1
			else
				echo "ERROR: -vs2017 option is only supported on Windows platform"
				exit 4
			fi
			;;
		-clang)
			if isWindows; then
				useVSclang=1
			else
				echo "ERROR: -clang option is only supported on Windows platform"
				exit 4
			fi
			;;
		-signtool-opt)
			if isWindows; then
				shift
				if [ $# -lt 1 ]; then
					echo "ERROR: Missing parameter for -signtool-opt option, see help (-h)"
					exit 4
				fi
				signtoolOptions="$1"
			else
				echo "ERROR: -signtool-opt option is only supported on Windows platform"
				exit 4
			fi
			;;
		-id)
			if isMac; then
				shift
				if [ $# -lt 1 ]; then
					echo "ERROR: Missing parameter for -id option, see help (-h)"
					exit 4
				fi
				signingId="$1"
			else
				echo "ERROR: -id option is only supported on macOS platform"
				exit 4
			fi
			;;
		-ids)
			if isMac; then
				security find-identity -v -p codesigning | grep -Po "^[[:space:]]+[0-9]+\)[[:space:]]+[0-9A-Z]+[[:space:]]+\"\KDeveloper ID Application: [^(]+\([^)]+\)(?=\")"
				if [ $? -ne 0 ]; then
					echo "ERROR: No valid signing identity found. You must install a 'Developer ID Application' certificate"
					exit 4
				fi
				exit 0
			else
				echo "ERROR: -ids option is only supported on macOS platform"
				exit 4
			fi
			;;
		-debug)
			cmake_config="Debug"
			;;
		-release)
			cmake_config="Release"
			;;
		-sign)
			doSign=1
			;;
		*)
			echo "ERROR: Unknown option '$1' (use -h for help)"
			exit 4
			;;
	esac
	shift
done

if [ ! -z "$cmake_generator" ]; then
	echo "Overriding default cmake generator ($generator) with: $cmake_generator"
	generator="$cmake_generator"
fi

# Signing is now mandatory for macOS
if isMac; then
	# No signing identity provided, try to autodetect
	if [ "$signingId" == "" ]; then
		echo -n "No signing identity provided, autodetecting... "
		signingId="$(security find-identity -v -p codesigning | grep -Po "^[[:space:]]+[0-9]+\)[[:space:]]+[0-9A-Z]+[[:space:]]+\"\KDeveloper ID Application: [^(]+\([^)]+\)(?=\")" -m1)"
		if [ "$signingId" == "" ]; then
			echo "ERROR: Cannot autodetect a valid signing identity, please use -id option"
			exit 4
		fi
		echo "using identity: '$signingId'"
	fi
	# Validate signing identity exists
	subSign="${signingId//(/\\(}" # Need to escape ( and )
	subSign="${subSign//)/\\)}"
	security find-identity -v -p codesigning | grep -Po "^[[:space:]]+[0-9]+\)[[:space:]]+[0-9A-Z]+[[:space:]]+\"${subSign}" &> /dev/null
	if [ $? -ne 0 ]; then
		echo "ERROR: Signing identity '${signingId}' not found, use the full identity name inbetween the quotes (see -ids to get a list of valid identities)"
		exit 4
	fi
	# Get Team Identifier from signing identity (for xcode)
	teamRegEx="[^(]+\(([^)]+)"
	if ! [[ $signingId =~ $teamRegEx ]]; then
		echo "ERROR: Failed to find Team Identifier in signing identity: $signingId"
		exit 4
	fi
	teamId="${BASH_REMATCH[1]}"
	if [ $doSign -eq 0 ]; then
		echo "Binary signing is mandatory since macOS Catalina, forcing it using ID '$signingId' (TeamID '$teamId')"
		doSign=1
	fi
	add_cmake_opt+=("-DLA_BINARY_SIGNING_IDENTITY=$signingId")
	add_cmake_opt+=("-DLA_TEAM_IDENTIFIER=$teamId")
fi

if [ $doSign -eq 1 ]; then
	add_cmake_opt+=("-DENABLE_AVDECC_SIGNING=TRUE")
	# Set signtool options if signing enabled on windows
	if isWindows; then
		if [ ! -z "$signtoolOptions" ]; then
			add_cmake_opt+=("-DLA_SIGNTOOL_OPTIONS=$signtoolOptions")
		fi
	fi
fi

# Using -vs2017 option
if [ $useVS2017 -eq 1 ]; then
	generator="Visual Studio 15 2017"
	toolset="v141"
fi

# Using -clang option (shortcut to auto-define the toolset)
if [ $useVSclang -eq 1 ]; then
	toolset="ClangCL"
fi

# Check if at least a -debug or -release option has been passed for Single-Configuration generators
if isSingleConfigurationGenerator "$generator"; then
	if [ -z $cmake_config ]; then
		echo "ERROR: Single-Configuration generator '$generator' requires either -debug or -release option to be specified"
		exit 4
	fi
	add_cmake_opt+=("-DCMAKE_BUILD_TYPE=${cmake_config}")
else
	# Clear any -debug or -release passed to a Multi-Configurations generator
	cmake_config=""
fi

if [ $outputFolderForced -eq 0 ]; then
	getOutputFolder outputFolder "${outputFolderBasePath}" "${arch}" "${toolset}" "${cmake_config}" "${generator}"
fi

generator_arch_option=""
if [ ! -z "${generator_arch}" ]; then
	generator_arch_option="-A${generator_arch} "
fi

toolset_option=""
if [ ! -z "${toolset}" ]; then
	if [ ! -z "${toolchain}" ]; then
		toolset_option="-T${toolset},host=${toolchain} "
	else
		toolset_option="-T${toolset} "
	fi
fi

sdk_option=""
if [ ! -z "${platformSdk}" ]; then
	sdk_option="-DCMAKE_SYSTEM_VERSION=$platformSdk"
fi

echo "Generating cmake project..."
"$cmake_path" -H. -B"${outputFolder}" "-G${generator}" $generator_arch_option $toolset_option $sdk_option $cmake_opt "${add_cmake_opt[@]}"

echo ""
echo "All done, generated project lies in ${outputFolder}"
