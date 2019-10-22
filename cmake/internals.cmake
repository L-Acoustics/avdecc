# Internal utility macros and functions for avdecc library

###############################################################################
# Set maximum warning level, and treat warnings as errors
# Applies on a target, must be called after target has been defined with
# 'add_library' or 'add_executable'.
function(set_maximum_warnings TARGET_NAME)
	if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX OR APPLE OR VS_USE_CLANG)
		target_compile_options(${TARGET_NAME} PRIVATE -Wall -Werror -g)
	elseif(MSVC)
		# Don't use Wall on MSVC, it prints too many stupid warnings
		target_compile_options(${TARGET_NAME} PRIVATE /W4 /WX)
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
		if(VS_USE_CLANG)
			target_compile_options(${TARGET_NAME} PRIVATE -g2 -gdwarf-2)
		else()
			target_compile_options(${TARGET_NAME} PRIVATE /Zi)
		endif()
		set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS_RELEASE "/DEBUG /OPT:REF /OPT:ICF /INCREMENTAL:NO")
	elseif(APPLE)
		target_compile_options(${TARGET_NAME} PRIVATE -g)

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
	if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
		if(NOT WIN32)
			# Build using fPIC
			target_compile_options(${TARGET_NAME} PRIVATE -fPIC)
		endif()
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
		if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX OR CMAKE_HOST_APPLE)
			target_compile_options(${TARGET_NAME} PRIVATE -fvisibility=hidden)
		endif()
		if(WIN32)
			# Touch the import library so even if we don't change exported symbols, the lib is 'changed' and anything depending on the dll will be relinked
			add_custom_command(
				TARGET ${TARGET_NAME}
				POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E touch_nocreate "$<TARGET_LINKER_FILE:${TARGET_NAME}>"
				COMMAND ${CMAKE_COMMAND} -E echo "Touching $<TARGET_LINKER_FILE:${TARGET_NAME}> import library"
				COMMENT "Touching $<TARGET_LINKER_FILE:${TARGET_NAME}> import library"
				VERBATIM
			)
		endif()
		# Generate so-version on Gcc (actually we only want this on linux, so we'll have to better test the condition someday)
		if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
			set_target_properties(${TARGET_NAME} PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})
		endif()

	# Unsupported target type
	else()
		message(FATAL_ERROR "Unsupported target type for setup_library_options: ${targetType}")
	endif()

	# Set full warnings (including treat warnings as error)
	set_maximum_warnings(${TARGET_NAME})
	
	# Set the "DEBUG" define in debug compilation mode
	set_debug_define(${TARGET_NAME})
	
	# Prevent visual studio deprecated warnings about CRT and Sockets
	remove_vs_deprecated_warnings(${TARGET_NAME})
	
	# Add a postfix in debug mode
	set_target_properties(${TARGET_NAME} PROPERTIES DEBUG_POSTFIX "-d")

	# Use cmake folders
	set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Libraries")

	# Additional include directories
	target_include_directories(${TARGET_NAME} PUBLIC $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${LA_ROOT_DIR}/include> PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
	
	# Setup debug symbols
	setup_symbols(${TARGET_NAME})

endfunction()

###############################################################################
# Setup common install rules for a library target
function(setup_library_install_rules TARGET_NAME)
	# Get target type for specific options
	get_target_property(targetType ${TARGET_NAME} TYPE)

	# Static library install rules
	if(${targetType} STREQUAL "STATIC_LIBRARY")
		install(TARGETS ${TARGET_NAME} EXPORT ${TARGET_NAME} ARCHIVE DESTINATION lib)
		install(EXPORT ${TARGET_NAME} DESTINATION cmake)

	# Shared library install rules
	elseif(${targetType} STREQUAL "SHARED_LIBRARY")
		install(TARGETS ${TARGET_NAME} EXPORT ${TARGET_NAME} RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)
		install(EXPORT ${TARGET_NAME} DESTINATION cmake)

	# Unsupported target type
	else()
		message(FATAL_ERROR "Unsupported target type for setup_library_install_rules macro: ${targetType}")
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
	
	# Set the "DEBUG" define in debug compilation mode
	set_debug_define(${TARGET_NAME})
	
	# Prevent visual studio deprecated warnings about CRT and Sockets
	remove_vs_deprecated_warnings(${TARGET_NAME})
	
	# Add a postfix in debug mode
	set_target_properties(${TARGET_NAME} PROPERTIES DEBUG_POSTFIX "-d")

	# Set target properties
	setup_bundle_information(${TARGET_NAME})

	# Setup debug symbols
	setup_symbols(${TARGET_NAME})

	# Set rpath for macOS
	if(APPLE)
		is_macos_bundle(${TARGET_NAME} isBundle)
		if(${isBundle})
			set_target_properties(${TARGET_NAME} PROPERTIES INSTALL_RPATH "@executable_path/../Frameworks")
			# Directly use install rpath for app bundles, since we copy dylibs into the bundle during post build
			set_target_properties(${TARGET_NAME} PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
			# For xcode automatic code signing to go deeply so all our dylibs are signed as well (will fail with xcode >= 11 otherwise)
			set_target_properties(${TARGET_NAME} PROPERTIES XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS "--deep --force")
			# Enable Hardened Runtime (required to notarize applications)
			set_target_properties(${TARGET_NAME} PROPERTIES XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME YES)
		else()
			set_target_properties(${TARGET_NAME} PROPERTIES INSTALL_RPATH "@executable_path/../lib")
			# Directly use install rpath for command line apps too
			set_target_properties(${TARGET_NAME} PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
		endif()
	endif()
	# Set rpath for linux
	if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
		set_target_properties(${TARGET_NAME} PROPERTIES INSTALL_RPATH "../lib")
	endif()
	
	target_include_directories(${TARGET_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}" "${LA_ROOT_DIR}/include")

endfunction()

###############################################################################
# Sign a binary.
function(sign_target TARGET_NAME)
	# Get the postfix for each configuration, in case we want to sign the target with another target than Release (someday)
	foreach(VAR ${CMAKE_CONFIGURATION_TYPES})
		set(confPostfix "${VAR}_POSTFIX")
		string(TOUPPER "${VAR}_POSTFIX" upperConfPostfix)
		get_target_property(${TARGET_NAME}_${confPostfix} ${TARGET_NAME} ${upperConfPostfix}) # Postfix property must be get using uppercase configuration name
		if(NOT ${TARGET_NAME}_${confPostfix})
			set(${TARGET_NAME}_${confPostfix} "")
		endif()
		# Use the install target to set a variable containing the postfix, since during install we cannot access variable from the cache or the global scope
		install(CODE "set(${TARGET_NAME}_${confPostfix} \"${${TARGET_NAME}_${confPostfix}}\")")
	endforeach()

	get_target_property(targetType ${TARGET_NAME} TYPE)

	if(targetType STREQUAL "UTILITY")
		get_target_property(targetType ${TARGET_NAME} LA_FORCED_TYPE)
		if(NOT targetType)
			message(FATAL_ERROR "LA_FORCED_TYPE property must be set for UTILITY target (example SHARED_LIBRARY)")
		endif()
	endif()

	if(WIN32)
		install(
			CODE "\
				if(NOT \${CMAKE_INSTALL_CONFIG_NAME} STREQUAL \"Debug\")\n\
					set(targetLocation \"${CMAKE_CURRENT_BINARY_DIR}/\${CMAKE_INSTALL_CONFIG_NAME}/${CMAKE_${targetType}_PREFIX}${TARGET_NAME}\${${TARGET_NAME}_\${CMAKE_INSTALL_CONFIG_NAME}_POSTFIX}${CMAKE_${targetType}_SUFFIX}\")\n\
					execute_process(COMMAND \"${CMAKE_COMMAND}\" -E echo \"Signing ${TARGET_NAME}\")\n\
					execute_process(COMMAND signtool sign /a /sm /q /fd sha1 /t http://timestamp.digicert.com /d \"${LA_COMPANY_NAME} ${PROJECT_NAME}\" \"\${targetLocation}\")\n\
					execute_process(COMMAND signtool sign /a /sm /as /q /fd sha256 /tr http://timestamp.digicert.com /d \"${LA_COMPANY_NAME} ${PROJECT_NAME}\" \"\${targetLocation}\")\n\
				endif()"
		)

	elseif(APPLE)
		if(NOT LA_TEAM_IDENTIFIER)
			message(FATAL_ERROR "LA_TEAM_IDENTIFIER variable must be set before trying to sign a binary with sign_target().")
		endif()
		is_macos_bundle(${TARGET_NAME} isBundle)
		if(${isBundle})
			set(addTargetPath ".app")
			# MacOS Catalina requires code signing all the time
			add_custom_command(
				TARGET ${TARGET_NAME}
				POST_BUILD
				COMMAND codesign -s ${LA_TEAM_IDENTIFIER} --timestamp --deep --strict --force "$<TARGET_FILE_DIR:${TARGET_NAME}>/../.."
				COMMENT "Signing Bundle ${TARGET_NAME} for easy debug"
				VERBATIM
			)

		else()
			set(addTargetPath "")
		endif()
		install(
			CODE "\
				set(targetLocation \"${CMAKE_CURRENT_BINARY_DIR}/\${CMAKE_INSTALL_CONFIG_NAME}/${CMAKE_${targetType}_PREFIX}${TARGET_NAME}\${${TARGET_NAME}_\${CMAKE_INSTALL_CONFIG_NAME}_POSTFIX}${CMAKE_${targetType}_SUFFIX}${addTargetPath}\")\n\
				execute_process(COMMAND \"${CMAKE_COMMAND}\" -E echo \"Signing ${TARGET_NAME}\")\n\
				execute_process(COMMAND codesign -s \"${LA_TEAM_IDENTIFIER}\" --timestamp --deep --strict --force \"\${targetLocation}\")\n\
		")
	endif()

endfunction()

###############################################################################
# Copy the runtime part of MODULE_NAME to the output folder of TARGET_NAME for
# easy debugging from the IDE.
function(copy_runtime TARGET_NAME MODULE_NAME)
	# Get module type
	get_target_property(moduleType ${MODULE_NAME} TYPE)
	# Module is not a shared library, no need to copy
	if(NOT ${moduleType} STREQUAL "SHARED_LIBRARY")
		return()
	endif()

	is_macos_bundle(${TARGET_NAME} isBundle)
	# For mac non-bundle apps, we copy the dylibs to the lib sub folder, so it matches the same rpath than when installing (since we use install_rpath)
	if(APPLE AND NOT ${isBundle})
		set(addSubDestPath "/../lib")
	else()
		set(addSubDestPath "")
	endif()

	# Copy shared library to output folder as post-build (for easy test/debug)
	add_custom_command(
		TARGET ${TARGET_NAME}
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${TARGET_NAME}>${addSubDestPath}"
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${MODULE_NAME}> "$<TARGET_FILE_DIR:${TARGET_NAME}>${addSubDestPath}"
		COMMENT "Copying ${MODULE_NAME} shared library to ${TARGET_NAME} output folder for easy debug"
		VERBATIM
	)
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

###############################################################################
# Global variables (must stay at the end of the file)
set(LA_ROOT_DIR "${PROJECT_SOURCE_DIR}") # Folder containing the main CMakeLists.txt for the repository including this file
set(LA_TOP_LEVEL_BINARY_DIR "${PROJECT_BINARY_DIR}") # Folder containing the top level binary files (CMake root output folder)
