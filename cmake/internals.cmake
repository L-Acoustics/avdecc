# Internal utility macros and functions for avdecc library

# Avoid multi inclusion of this file (cannot use include_guard as multiple copies of this file are included from multiple places)
if(LA_AVDECC_INTERNALS_INCLUDED)
	return()
endif()
set(LA_AVDECC_INTERNALS_INCLUDED true)

# Some global variables
set(LA_ROOT_DIR "${PROJECT_SOURCE_DIR}") # Folder containing the main CMakeLists.txt for the repository including this file
set(LA_TOP_LEVEL_BINARY_DIR "${PROJECT_BINARY_DIR}") # Folder containing the top level binary files (CMake root output folder)
set(CMAKE_MACROS_FOLDER "${CMAKE_CURRENT_LIST_DIR}")

# Include TargetSetupDeploy script from cmakeUtils
include(${CMAKE_CURRENT_LIST_DIR}/cmakeUtils/helpers/TargetSetupDeploy.cmake)

###############################################################################
# Set parallel build
# Sets the parallel build option for IDE that supports it
# This is overridden when compiling from command line with "cmake --build"
function(set_parallel_build TARGET_NAME)
	if(MSVC)
		target_compile_options(${TARGET_NAME} PRIVATE /MP)
	endif()
endfunction()

###############################################################################
# Set maximum warning level, and treat warnings as errors
# Applies on a target, must be called after target has been defined with
# 'add_library' or 'add_executable'.
function(set_maximum_warnings TARGET_NAME)
	if(MSVC)
		# Don't use Wall on MSVC, it prints too many stupid warnings
		target_compile_options(${TARGET_NAME} PRIVATE /W4 /WX)
		# Using clang-cl with MSVC (special case as MSBuild will convert MSVC Flags to Clang flags automatically)
		if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
			target_compile_options(${TARGET_NAME} PRIVATE -Wno-nonportable-include-path -Wno-microsoft-include)
		endif()
	elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		target_compile_options(${TARGET_NAME} PRIVATE -Wall -Werror -g)
	endif()
endfunction()

###############################################################################
# Set the DEBUG define in debug mode
# Applies on a target, must be called after target has been defined with
# 'add_library' or 'add_executable'.
function(set_debug_define TARGET_NAME)
	# Flags to add for DEBUG
	target_compile_options(${TARGET_NAME} PRIVATE $<$<CONFIG:Debug>:-DDEBUG>)
endfunction()

###############################################################################
# Remove VisualStudio useless deprecated warnings (CRT, CRT_SECURE, WINSOCK)
# Applies on a target, must be called after target has been defined with
# 'add_library' or 'add_executable'.
function(remove_vs_deprecated_warnings TARGET_NAME)
	if(MSVC)
		target_compile_options(${TARGET_NAME} PRIVATE -D_CRT_SECURE_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS -D_WINSOCK_DEPRECATED_NO_WARNINGS)
	endif()
endfunction()

###############################################################################
# Returns TRUE if TARGET is a macOS bundle application
function(is_macos_bundle TARGET_NAME IS_BUNDLE)
	if(APPLE)
		get_target_property(isBundle ${TARGET_NAME} MACOSX_BUNDLE)
		if(${isBundle})
			set(${IS_BUNDLE} TRUE PARENT_SCOPE)
			return()
		endif()
	endif()
	set(${IS_BUNDLE} FALSE PARENT_SCOPE)
endfunction()

###############################################################################
# Force symbols file generation for build configs (pdb or dSYM)
# Applies on a target, must be called after target has been defined with
# 'add_library' or 'add_executable'.
function(force_symbols_file TARGET_NAME)
	get_target_property(targetType ${TARGET_NAME} TYPE)

	if(MSVC)
		target_compile_options(${TARGET_NAME} PRIVATE /Zi)
		set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_RELEASE "/DEBUG /OPT:REF /OPT:ICF /INCREMENTAL:NO")
	elseif(APPLE)
		target_compile_options(${TARGET_NAME} PRIVATE -g)

		if("${CMAKE_GENERATOR}" STREQUAL "Xcode")
			if(${targetType} STREQUAL "STATIC_LIBRARY")
				# macOS do not support dSYM file for static libraries
				set_target_properties(${TARGET_NAME} PROPERTIES
					XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT[variant=Debug] "dwarf"
					XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT[variant=Release] "dwarf"
					XCODE_ATTRIBUTE_DEPLOYMENT_POSTPROCESSING[variant=Debug] "NO"
					XCODE_ATTRIBUTE_DEPLOYMENT_POSTPROCESSING[variant=Release] "NO"
				)
			else()
				set_target_properties(${TARGET_NAME} PROPERTIES
					XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT[variant=Debug] "dwarf-with-dsym"
					XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT[variant=Release] "dwarf-with-dsym"
					XCODE_ATTRIBUTE_DEPLOYMENT_POSTPROCESSING[variant=Debug] "YES"
					XCODE_ATTRIBUTE_DEPLOYMENT_POSTPROCESSING[variant=Release] "YES"
				)
			endif()
		else()
			# If not using Xcode, we have to do the dSYM/strip steps manually (but only for binary targets)
			if(${targetType} STREQUAL "SHARED_LIBRARY" OR ${targetType} STREQUAL "EXECUTABLE")
				add_custom_command(
					TARGET ${TARGET_NAME}
					POST_BUILD
					COMMAND dsymutil "$<TARGET_FILE:${TARGET_NAME}>"
					COMMENT "Extracting dSYM for ${TARGET_NAME}"
					VERBATIM
				)
				add_custom_command(
					TARGET ${TARGET_NAME}
					POST_BUILD
					COMMAND strip -x "$<TARGET_FILE:${TARGET_NAME}>"
					COMMENT "Stripping symbols from ${TARGET_NAME}"
					VERBATIM
				)
			endif()
		endif()
	endif()
endfunction()

###############################################################################
# Copy symbol files to a common location.
function(copy_symbols TARGET_NAME)
	set(SYMBOLS_DEST_PATH "${CMAKE_BINARY_DIR}/Symbols/$<CONFIG>/")
	get_target_property(targetType ${TARGET_NAME} TYPE)
	if(MSVC)
		# No pdb files for static libraries, symbols are embedded in the lib
		if(NOT ${targetType} STREQUAL "STATIC_LIBRARY")
			add_custom_command(
				TARGET ${TARGET_NAME}
				POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E make_directory "${SYMBOLS_DEST_PATH}"
				COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_PDB_FILE:${TARGET_NAME}>" "${SYMBOLS_DEST_PATH}"
				COMMENT "Copying ${TARGET_NAME} symbols"
				VERBATIM
			)
		endif()

	elseif(APPLE)
		if(${targetType} STREQUAL "SHARED_LIBRARY")
			install(FILES "$<TARGET_FILE:${TARGET_NAME}>.dSYM" DESTINATION "${SYMBOLS_DEST_PATH}" CONFIGURATIONS Release Debug)
		elseif(${targetType} STREQUAL "EXECUTABLE")
			is_macos_bundle(${TARGET_NAME} isBundle)
			if(${isBundle})
				install(FILES "${CMAKE_CURRENT_BINARY_DIR}/\${CMAKE_INSTALL_CONFIG_NAME}/$<TARGET_FILE_NAME:${TARGET_NAME}>.app.dSYM" DESTINATION "${SYMBOLS_DEST_PATH}" CONFIGURATIONS Release Debug)
			else()
				install(FILES "$<TARGET_FILE:${TARGET_NAME}>.dSYM" DESTINATION "${SYMBOLS_DEST_PATH}" CONFIGURATIONS Release Debug)
			endif()
		endif()
	endif()
endfunction()

###############################################################################
# Setup symbols for a target.
function(setup_symbols TARGET_NAME)
	# Force symbols file generation
	force_symbols_file(${TARGET_NAME})

	# Copy symbols to a common location
	copy_symbols(${TARGET_NAME})
endfunction()

###############################################################################
# Setup Xcode automatic codesigning (required since Catalina).
function(setup_xcode_codesigning TARGET_NAME)
	# Set codesigning for macOS
	if(APPLE)
		if("${CMAKE_GENERATOR}" STREQUAL "Xcode")
			# Force Xcode signing identity but only if defined to something valid (we will re-sign later anyway)
			if(NOT "${LA_TEAM_IDENTIFIER}" STREQUAL "-")
				set_target_properties(${TARGET_NAME} PROPERTIES XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${LA_TEAM_IDENTIFIER}")
			endif()
			# For xcode code signing to go deeply so all our dylibs are signed as well (will fail with xcode >= 11 otherwise)
			set_target_properties(${TARGET_NAME} PROPERTIES XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS "--deep --strict --force --options=runtime")
			# Enable Hardened Runtime (required to notarize applications)
			set_target_properties(${TARGET_NAME} PROPERTIES XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME YES)
		endif()
	endif()
endfunction()

###############################################################################
# Set Precompiled Headers on a target
function(set_precompiled_headers TARGET_NAME HEADER_NAME SOURCE_NAME)
	if(CMAKE_HOST_WIN32 AND MSVC)
		# -Yu: Use Precompiled Header; -FI: Force Include Precompiled Header
		target_compile_options(${TARGET_NAME} PRIVATE -Yu${HEADER_NAME} -FI${HEADER_NAME})
		# -Yc: Create Precompiled Headers, needed to create the PCH itself
		set_source_files_properties(${SOURCE_NAME} PROPERTIES COMPILE_FLAGS -Yc${HEADER_NAME})
	endif()
endfunction()

###############################################################################
# Setup common options for a library target
function(setup_library_options TARGET_NAME BASE_LIB_NAME)
	# Get target type for specific options
	get_target_property(targetType ${TARGET_NAME} TYPE)

	if(MSVC)
		# Set WIN32 version since we want to target WinVista minimum
		target_compile_options(${TARGET_NAME} PRIVATE -D_WIN32_WINNT=0x0600)
	endif()

	if(NOT APPLE AND NOT WIN32)
		# Build using fPIC
		target_compile_options(${TARGET_NAME} PRIVATE -fPIC)
		# Add pthread library
		set(ADD_LINK_LIBRARIES pthread)
	endif()

	# Static library special options
	if(${targetType} STREQUAL "STATIC_LIBRARY")
		target_link_libraries(${TARGET_NAME} PUBLIC ${LINK_LIBRARIES} ${ADD_LINK_LIBRARIES})
		# Compile c++ as static exports
		target_compile_options(${TARGET_NAME} PUBLIC "-D${BASE_LIB_NAME}_cxx_STATICS")

	# Shared library special options
	elseif(${targetType} STREQUAL "SHARED_LIBRARY")
		target_link_libraries(${TARGET_NAME} PRIVATE ${LINK_LIBRARIES} ${ADD_LINK_LIBRARIES})
		# Gcc/Clang needs specific flags for shared libraries
		if(NOT MSVC)
			if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
				target_compile_options(${TARGET_NAME} PRIVATE -fvisibility=hidden)
			endif()
		endif()
		if(WIN32)
			# Touch the import library so even if we don't change exported symbols, the lib is 'changed' and anything depending on the dll will be relinked
			add_custom_command(
				TARGET ${TARGET_NAME}
				POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E touch_nocreate "$<TARGET_LINKER_FILE:${TARGET_NAME}>"
				COMMAND ${CMAKE_COMMAND} -E echo "Touching $<TARGET_LINKER_FILE:${TARGET_NAME}> import library"
				VERBATIM
			)
		endif()
		# Generate so-version on Linux and macOS
		if(NOT WIN32)
			set_target_properties(${TARGET_NAME} PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})
		endif()

	# Unsupported target type
	else()
		message(FATAL_ERROR "Unsupported target type for setup_library_options: ${targetType}")
	endif()

	# Set full warnings (including treat warnings as error)
	set_maximum_warnings(${TARGET_NAME})
	
	# Set parallel build
	set_parallel_build(${TARGET_NAME})

	# Set the "DEBUG" define in debug compilation mode
	set_debug_define(${TARGET_NAME})
	
	# Prevent visual studio deprecated warnings about CRT and Sockets
	remove_vs_deprecated_warnings(${TARGET_NAME})
	
	# Add a postfix in debug mode
	set_target_properties(${TARGET_NAME} PROPERTIES DEBUG_POSTFIX "-d")

	# Set xcode automatic codesigning
	setup_xcode_codesigning(${TARGET_NAME})

	# Use cmake folders
	set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Libraries")

	# Additional include directories
	target_include_directories(${TARGET_NAME} PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${LA_ROOT_DIR}/include> PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
	
	# Setup debug symbols
	setup_symbols(${TARGET_NAME})

endfunction()

###############################################################################
# Setup install rules for header files
function(setup_headers_install_rules FILES_LIST INCLUDE_ABSOLUTE_BASE_FOLDER)
	foreach(f ${FILES_LIST})
		get_filename_component(dir ${f} DIRECTORY)
		file(RELATIVE_PATH dir ${INCLUDE_ABSOLUTE_BASE_FOLDER} ${dir})
		install(FILES ${f} CONFIGURATIONS Release DESTINATION include/${dir})
	endforeach()
endfunction()

###############################################################################
# Internal function
function(get_sign_command_options OUT_VAR)
	set(${OUT_VAR} SIGNTOOL_OPTIONS ${LA_SIGNTOOL_OPTIONS} /d \"${LA_COMPANY_NAME} ${PROJECT_NAME}\" CODESIGN_OPTIONS --timestamp --deep --strict --force --options=runtime CODESIGN_IDENTITY \"${LA_BINARY_SIGNING_IDENTITY}\" PARENT_SCOPE)
endfunction()

#
function(setup_signing_command TARGET_NAME)
	# Xcode already forces automatic signing, so only sign for the other cases
	if(NOT "${CMAKE_GENERATOR}" STREQUAL "Xcode")
		# Get signing options
		get_sign_command_options(SIGN_COMMAND_OPTIONS)

		# Expand options to individual parameters
		string(REPLACE ";" " " SIGN_COMMAND_OPTIONS "${SIGN_COMMAND_OPTIONS}")

		is_macos_bundle(${TARGET_NAME} isBundle)
		if(${isBundle})
			set(binary_path "$<TARGET_BUNDLE_DIR:${TARGET_NAME}>")
		else()
			set(binary_path "$<TARGET_FILE:${TARGET_NAME}>")
		endif()

		# Generate code-signing code
		string(CONCAT CODESIGNING_CODE
			"include(\"${CMAKE_MACROS_FOLDER}/cmakeUtils/helpers/SignBinary.cmake\")\n"
			"cu_sign_binary(BINARY_PATH \"${binary_path}\" ${SIGN_COMMAND_OPTIONS})\n"
		)
		# Write to a cmake file
		set(CODESIGN_SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/codesign_$<CONFIG>_${TARGET_NAME}.cmake)
		file(GENERATE
			OUTPUT ${CODESIGN_SCRIPT}
			CONTENT ${CODESIGNING_CODE}
		)
		# Run the codesign script as POST_BUILD command on the target
		add_custom_command(TARGET ${TARGET_NAME}
			POST_BUILD
			COMMAND ${CMAKE_COMMAND} -P ${CODESIGN_SCRIPT}
			VERBATIM
		)
	endif()
endfunction()

###############################################################################
# Setup install rules for a library target, as well a signing if specified
# Optional parameters:
#  - INSTALL -> Generate CMake install rules
#  - SIGN -> Code sign (ignored for everything but SHARED_LIBRARY)
function(setup_deploy_library TARGET_NAME)
	# Get target type for specific options
	get_target_property(targetType ${TARGET_NAME} TYPE)

	# Parse arguments
	cmake_parse_arguments(SDL "INSTALL;SIGN" "" "" ${ARGN})

	if(SDL_INSTALL)
		# Static library install rules
		if(${targetType} STREQUAL "STATIC_LIBRARY")
			install(TARGETS ${TARGET_NAME} EXPORT ${TARGET_NAME} ARCHIVE DESTINATION lib)
			install(EXPORT ${TARGET_NAME} DESTINATION cmake)

		# Shared library install rules
		elseif(${targetType} STREQUAL "SHARED_LIBRARY")
			# Check for SIGN option
			if(SDL_SIGN)
				setup_signing_command(${TARGET_NAME})
			endif()

			install(TARGETS ${TARGET_NAME} EXPORT ${TARGET_NAME} RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
			install(EXPORT ${TARGET_NAME} DESTINATION cmake)

		# Interface library install rules
		elseif(${targetType} STREQUAL "INTERFACE_LIBRARY")
			install(TARGETS ${TARGET_NAME} EXPORT ${TARGET_NAME})
			install(EXPORT ${TARGET_NAME} DESTINATION cmake)

		# Unsupported target type
		else()
			message(FATAL_ERROR "Unsupported target type for setup_deploy_library macro: ${targetType}")
		endif()
	endif()
endfunction()

###############################################################################
# Setup macOS bundle information
# Applies on a target, must be called after target has been defined with
# 'add_executable'.
function(setup_bundle_information TARGET_NAME)
	if(APPLE)
		set_target_properties(${TARGET_NAME} PROPERTIES
				MACOSX_BUNDLE_INFO_STRING "${LA_PROJECT_PRODUCTDESCRIPTION}"
				MACOSX_BUNDLE_ICON_FILE "AppIcon"
				MACOSX_BUNDLE_GUI_IDENTIFIER "${LA_PROJECT_BUNDLEIDENTIFIER}"
				MACOSX_BUNDLE_BUNDLE_NAME "${PROJECT_NAME}"
				MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
				MACOSX_BUNDLE_COPYRIGHT "${LA_PROJECT_READABLE_COPYRIGHT}")
	endif()
endfunction()

###############################################################################
# Setup common options for an executable target.
function(setup_executable_options TARGET_NAME)
	if(MSVC)
		# Set WIN32 version since we want to target WinVista minimum
		target_compile_options(${TARGET_NAME} PRIVATE -D_WIN32_WINNT=0x0600)
	endif()

	# Add link libraries
	target_link_libraries(${TARGET_NAME} PRIVATE ${LINK_LIBRARIES})

	# Set full warnings (including treat warnings as error)
	set_maximum_warnings(${TARGET_NAME})
	
	# Set parallel build
	set_parallel_build(${TARGET_NAME})

	# Set the "DEBUG" define in debug compilation mode
	set_debug_define(${TARGET_NAME})
	
	# Prevent visual studio deprecated warnings about CRT and Sockets
	remove_vs_deprecated_warnings(${TARGET_NAME})
	
	# Add a postfix in debug mode (but only if not a macOS bundle as it is not supported and will cause error in other parts of the scripts)
	is_macos_bundle(${TARGET_NAME} isBundle)
	if(NOT ${isBundle})
		set_target_properties(${TARGET_NAME} PROPERTIES DEBUG_POSTFIX "-d")
	endif()

	# Set target properties
	setup_bundle_information(${TARGET_NAME})

	# Setup debug symbols
	setup_symbols(${TARGET_NAME})

	# Set rpath for macOS
	if(APPLE)
		if(${isBundle})
			set_target_properties(${TARGET_NAME} PROPERTIES INSTALL_RPATH "@executable_path/../Frameworks")
			# Directly use install rpath for app bundles, since we copy dylibs into the bundle during post build
			set_target_properties(${TARGET_NAME} PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
		else()
			set_target_properties(${TARGET_NAME} PROPERTIES INSTALL_RPATH "@executable_path/../lib")
			# Directly use install rpath for command line apps too
			set_target_properties(${TARGET_NAME} PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
		endif()

	# Set rpath for linux
	elseif(NOT WIN32)
		set_target_properties(${TARGET_NAME} PROPERTIES INSTALL_RPATH "../lib")
	endif()
	
	# Set xcode automatic codesigning
	setup_xcode_codesigning(${TARGET_NAME})

	target_include_directories(${TARGET_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}" "${LA_ROOT_DIR}/include")

endfunction()

###############################################################################
# Setup runtime deployment rules for an executable target, for easy debug and install (if specified)
# Optional parameters:
#  - INSTALL -> Generate CMake install rules
#  - SIGN -> Code sign all binaries
function(setup_deploy_runtime TARGET_NAME)
	# Get target type for specific options
	get_target_property(targetType ${TARGET_NAME} TYPE)

	# Only for executables
	if(NOT ${targetType} STREQUAL "EXECUTABLE")
		message(FATAL_ERROR "Unsupported target type for setup_deploy_runtime macro: ${targetType}")
	endif()

	# Get signing options
	get_sign_command_options(SIGN_COMMAND_OPTIONS)

	# cmakeUtils deploy runtime
	cu_deploy_runtime_target(${ARGV} ${SIGN_COMMAND_OPTIONS})

	# Check for install and sign of the binary itself
	cmake_parse_arguments(SDR "INSTALL;SIGN" "" "" ${ARGN})

	if(SDR_SIGN)
		setup_signing_command(${TARGET_NAME})
	endif()

	if(SDR_INSTALL)
		install(TARGETS ${TARGET_NAME} BUNDLE DESTINATION . RUNTIME DESTINATION bin)
	endif()
endfunction()

###############################################################################
# Setup common variables for a project
macro(setup_project PRJ_NAME PRJ_VERSION PRJ_DESC)
	project(${PRJ_NAME} LANGUAGES C CXX VERSION ${PRJ_VERSION})

	set(LA_PROJECT_PRODUCTDESCRIPTION ${PRJ_DESC})

	set(LA_PROJECT_VERSIONMAJ ${PROJECT_VERSION_MAJOR})
	set(LA_PROJECT_VERSIONMIN ${PROJECT_VERSION_MINOR})
	set(LA_PROJECT_VERSIONPATCH ${PROJECT_VERSION_PATCH})
	set(LA_PROJECT_VERSIONBETA ${PROJECT_VERSION_TWEAK})
	if(NOT LA_PROJECT_VERSIONBETA)
		set(LA_PROJECT_VERSIONBETA 0)
		set(LA_PROJECT_VERSION_STRING "${LA_PROJECT_VERSIONMAJ}.${LA_PROJECT_VERSIONMIN}.${LA_PROJECT_VERSIONPATCH}")
		set(LA_PROJECT_CMAKEVERSION_STRING "${LA_PROJECT_VERSIONMAJ}.${LA_PROJECT_VERSIONMIN}.${LA_PROJECT_VERSIONPATCH}")
	else()
		set(LA_PROJECT_VERSION_STRING "${LA_PROJECT_VERSIONMAJ}.${LA_PROJECT_VERSIONMIN}.${LA_PROJECT_VERSIONPATCH}-beta${LA_PROJECT_VERSIONBETA}")
		set(LA_PROJECT_CMAKEVERSION_STRING "${LA_PROJECT_VERSIONMAJ}.${LA_PROJECT_VERSIONMIN}.${LA_PROJECT_VERSIONPATCH}.${LA_PROJECT_VERSIONBETA}")
	endif()

	string(TOLOWER "com.${LA_COMPANY_NAME}" LA_PROJECT_BUNDLEIDENTIFIER)
	set(LA_PROJECT_BUNDLEIDENTIFIER "${LA_PROJECT_BUNDLEIDENTIFIER}.${PRJ_NAME}")
	set(LA_PROJECT_PRODUCTVERSION "${LA_PROJECT_VERSIONMAJ}.${LA_PROJECT_VERSIONMIN}.${LA_PROJECT_VERSIONPATCH}")
	set(LA_PROJECT_FILEVERSION_STRING "${LA_PROJECT_VERSIONMAJ}.${LA_PROJECT_VERSIONMIN}.${LA_PROJECT_VERSIONPATCH}.${LA_PROJECT_VERSIONBETA}")
endmacro(setup_project)
