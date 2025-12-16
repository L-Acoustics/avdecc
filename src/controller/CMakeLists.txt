# avdecc controller

cu_setup_project(la_avdecc_controller ${LA_AVDECC_CONTROLLER_VERSION} "AVDECC Controller Library" MARKETING_VERSION_DIGITS ${MARKETING_VERSION_DIGITS} MARKETING_VERSION_POSTFIX ${MARKETING_VERSION_POSTFIX} MARKETING_VERSION ${MARKETING_VERSION})

# Configure file based on CMakeLists.txt
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
	"${CMAKE_CURRENT_BINARY_DIR}/config.h"
)
if(WIN32)
	configure_file(
		"${CU_ROOT_DIR}/resources/win32/controller/resources.rc.in"
		"${CMAKE_CURRENT_BINARY_DIR}/resources.rc"
	)
	set(SHARED_LIBRARY_FILES "${CMAKE_CURRENT_BINARY_DIR}/resources.rc")
endif()

set(ADD_LINK_LIBS "")
set(ADD_PRIVATE_COMPILE_OPTIONS "")
set(ADD_PUBLIC_COMPILE_OPTIONS "")
set(SWIG_INTERFACE_FILE_COMPILE_OPTIONS "")
set(PCH_HEADER_NAME "pch.hpp")

# Disable windows MINMAX
if(WIN32)
	list(APPEND ADD_PUBLIC_COMPILE_OPTIONS "-DNOMINMAX")
endif()

set (PUBLIC_HEADER_FILES
	${CU_ROOT_DIR}/include/la/avdecc/controller/avdeccController.hpp
	${CU_ROOT_DIR}/include/la/avdecc/controller/internals/avdeccControlledEntity.hpp
	${CU_ROOT_DIR}/include/la/avdecc/controller/internals/avdeccControlledEntityModel.hpp
	${CU_ROOT_DIR}/include/la/avdecc/controller/internals/virtualEntityBuilder.hpp
	${CU_ROOT_DIR}/include/la/avdecc/controller/internals/exports.hpp
	${CU_ROOT_DIR}/include/la/avdecc/controller/internals/logItems.hpp
)

set (HEADER_FILES_COMMON
	${CMAKE_CURRENT_BINARY_DIR}/config.h
	avdeccControllerImpl.hpp
	avdeccControlledEntityImpl.hpp
	avdeccControllerLogHelper.hpp
	avdeccControllerProxy.hpp
	avdeccEntityModelCache.hpp
	entityModelChecksum.hpp
	treeModelAccessStrategy.hpp
	treeModelAccessTraverseStrategy.hpp
	treeModelAccessCacheStrategy.hpp
	virtualEntityModelVisitor.hpp
)

set (SOURCE_FILES_COMMON
	avdeccController.cpp
	avdeccControllerImpl.cpp
	avdeccControllerImplDelegateOverrides.cpp
	avdeccControllerImplHandlers.cpp
	avdeccControllerImplOverrides.cpp
	avdeccControllerProxy.cpp
	avdeccControlledEntityImpl.cpp
	entityModelChecksum.cpp
	virtualEntityModelVisitor.cpp
)

# 64 bits visual studio requires /bigobj to be used for this file (apperently there are too many symbols in the file), but let's set the flag for 32 bits as well
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	set_source_files_properties(avdeccControllerImpl.cpp PROPERTIES COMPILE_FLAGS "/bigobj")
endif()

# Features
if(ENABLE_AVDECC_FEATURE_REDUNDANCY)
	list(APPEND ADD_PUBLIC_COMPILE_OPTIONS "-DENABLE_AVDECC_FEATURE_REDUNDANCY")
	list(APPEND SWIG_INTERFACE_FILE_COMPILE_OPTIONS "-DENABLE_AVDECC_FEATURE_REDUNDANCY")
	if(ENABLE_AVDECC_STRICT_2018_REDUNDANCY)
		list(APPEND ADD_PUBLIC_COMPILE_OPTIONS "-DENABLE_AVDECC_STRICT_2018_REDUNDANCY")
	endif()
endif()
if(ENABLE_AVDECC_FEATURE_JSON)
	list(APPEND SOURCE_FILES_COMMON avdeccControlledEntityJsonSerializer.cpp)
	list(APPEND HEADER_FILES_COMMON avdeccControlledEntityJsonSerializer.hpp avdeccControllerJsonTypes.hpp)
	list(APPEND ADD_PUBLIC_COMPILE_OPTIONS "-DENABLE_AVDECC_FEATURE_JSON")
endif()
if(ENABLE_AVDECC_FEATURE_CBR)
	list(APPEND ADD_PUBLIC_COMPILE_OPTIONS "-DENABLE_AVDECC_FEATURE_CBR")
endif()

# Other options
if(IGNORE_NEITHER_STATIC_NOR_DYNAMIC_MAPPINGS)
	list(APPEND ADD_PRIVATE_COMPILE_OPTIONS "-DIGNORE_NEITHER_STATIC_NOR_DYNAMIC_MAPPINGS")
endif()
if(CONTINUE_MISBEHAVE_AEM_RESPONSES)
	list(APPEND ADD_PRIVATE_COMPILE_OPTIONS "-DCONTINUE_MISBEHAVE_AEM_RESPONSES")
endif()
if(IGNORE_MISMATCHING_MVU_RESPONSES)
	list(APPEND ADD_PRIVATE_COMPILE_OPTIONS "-DIGNORE_MISMATCHING_MVU_RESPONSES")
endif()

set (LIB_HEADER_FILES ${HEADER_FILES_COMMON} ${PUBLIC_HEADER_FILES})
set (LIB_SOURCE_FILES ${SOURCE_FILES_COMMON})

source_group(TREE "${CU_ROOT_DIR}/include/la/avdecc/" PREFIX "Header Files\\Public" FILES ${PUBLIC_HEADER_FILES})

# C++ dynamic library
if(BUILD_AVDECC_LIB_SHARED_CXX)
	set (LIB_SHARED_CXX_SOURCE_FILES ${LIB_SOURCE_FILES})
	add_library(${PROJECT_NAME}_cxx SHARED ${LIB_HEADER_FILES} ${LIB_SHARED_CXX_SOURCE_FILES} ${SHARED_LIBRARY_FILES})
	if(ADD_PRIVATE_COMPILE_OPTIONS)
		target_compile_options(${PROJECT_NAME}_cxx PRIVATE ${ADD_PRIVATE_COMPILE_OPTIONS})
	endif()
	if(ADD_PUBLIC_COMPILE_OPTIONS)
		target_compile_options(${PROJECT_NAME}_cxx PUBLIC ${ADD_PUBLIC_COMPILE_OPTIONS})
	endif()
	if(ADD_LINK_LIBS)
		target_link_libraries(${PROJECT_NAME}_cxx PRIVATE ${ADD_LINK_LIBS})
	endif()

	# Enable precompiled headers
	cu_set_precompiled_headers(${PROJECT_NAME}_cxx ${PCH_HEADER_NAME})

	# Setup common options
	cu_setup_library_options(${PROJECT_NAME}_cxx)

	# Link with avdecc using shared library (publicly because any executable using the controller needs low level library as well)
	target_link_libraries(${PROJECT_NAME}_cxx PUBLIC la_avdecc_cxx)

	# Setup libfmt
	if(ENABLE_AVDECC_USE_FMTLIB)
		target_compile_options(${PROJECT_NAME}_cxx PRIVATE -DHAVE_FMT)
		target_link_libraries(${PROJECT_NAME}_cxx PRIVATE fmt-header-only)
	endif()

	# Setup json
	if(ENABLE_AVDECC_FEATURE_JSON)
		target_link_libraries(${PROJECT_NAME}_cxx PRIVATE nlohmann_json::nlohmann_json)
	endif()

	# Setup install (and signing)
	cu_setup_deploy_library(${PROJECT_NAME}_cxx ${INSTALL_SHARED_FLAG} ${SIGN_FLAG})
endif()

# Runtime-shared static library
if(BUILD_AVDECC_LIB_STATIC_RT_SHARED)
	add_library(${PROJECT_NAME}_static STATIC ${LIB_HEADER_FILES} ${LIB_SOURCE_FILES})
	if(ADD_PRIVATE_COMPILE_OPTIONS)
		target_compile_options(${PROJECT_NAME}_static PRIVATE ${ADD_PRIVATE_COMPILE_OPTIONS})
	endif()
	if(ADD_PUBLIC_COMPILE_OPTIONS)
		target_compile_options(${PROJECT_NAME}_static PUBLIC ${ADD_PUBLIC_COMPILE_OPTIONS})
	endif()
	if(ADD_LINK_LIBS)
		target_link_libraries(${PROJECT_NAME}_static PUBLIC ${ADD_LINK_LIBS})
	endif()

	# Enable precompiled headers
	cu_set_precompiled_headers(${PROJECT_NAME}_static ${PCH_HEADER_NAME})

	# Setup common options
	cu_setup_library_options(${PROJECT_NAME}_static)

	# Setup json
	if(ENABLE_AVDECC_FEATURE_JSON)
		target_link_libraries(${PROJECT_NAME}_static PRIVATE nlohmann_json::nlohmann_json)
	endif()

	# Link with avdecc using static library
	target_link_libraries(${PROJECT_NAME}_static PUBLIC la_avdecc_static)

	# Setup install (and signing)
	cu_setup_deploy_library(${PROJECT_NAME}_static ${INSTALL_STATIC_FLAG} ${SIGN_FLAG})
endif()

# Set installation rules
if(INSTALL_AVDECC_HEADERS)
	cu_setup_headers_install_rules("${PUBLIC_HEADER_FILES}" "${CU_ROOT_DIR}/include" CONFIGURATIONS Release Debug)
endif()

if(BUILD_AVDECC_SWIG)
	include(${CU_BASE_DIR}/helpers/SetupSwig.cmake REQUIRED)

	# avdecc controller library
	set(TARGET_PREFIX ${PROJECT_NAME})
	set(SWIG_INTERFACE_FILE "${CU_ROOT_DIR}/include/la/avdecc/controller/avdeccController.i")
	set(SWIG_FILE_DEPENDENCIES
		"${CU_ROOT_DIR}/include/la/avdecc/utils.i"
		"${CU_ROOT_DIR}/include/la/avdecc/memoryBuffer.i"
		"${CU_ROOT_DIR}/include/la/avdecc/internals/chrono.i"
		"${CU_ROOT_DIR}/include/la/avdecc/internals/swig_foreach.i"
		"${CU_ROOT_DIR}/include/la/avdecc/internals/std_function.i"
		"${CU_ROOT_DIR}/include/la/avdecc/internals/std_tuple.i"
		"${CU_ROOT_DIR}/include/la/avdecc/internals/std_unordered_map.i"
		"${CU_ROOT_DIR}/include/la/avdecc/internals/entityModel.i"
		"${CU_ROOT_DIR}/include/la/avdecc/avdecc.i"
	)
	cu_setup_swig_target(VERSION 4.4 TARGET_NAME ${PROJECT_NAME}::shared INTERFACE_FILE "${SWIG_INTERFACE_FILE}" LANGUAGES ${AVDECC_SWIG_LANGUAGES} SWIG_TARGET_PREFIX ${TARGET_PREFIX} REQUIRED INSTALL_SUPPORT_FILES FILE_DEPENDENCIES ${SWIG_FILE_DEPENDENCIES} INSTALL_CONFIGURATIONS Release Debug OUTVAR_PREFIX_SUPPORT_FILES_FOLDER SUPPORT_FILES_FOLDER SUPPORT_FILES_FOLDER INTERFACE_FILE_COMPILE_OPTIONS ${SWIG_INTERFACE_FILE_COMPILE_OPTIONS})

	# Post process generated targets
	foreach(SWIG_LANG ${AVDECC_SWIG_LANGUAGES})
		set(SWIG_TARGET_NAME ${TARGET_PREFIX}-${SWIG_LANG})

		# Add all public header files as dependency, so that wrapper files are re-generated whenever they change
		set_property(TARGET ${SWIG_TARGET_NAME} PROPERTY SWIG_DEPENDS ${PUBLIC_HEADER_FILES})
		# Setup common options like any other project
		cu_setup_library_options(${SWIG_TARGET_NAME} ALIAS_NAME ${SWIG_LANG} NO_ALIAS_TARGET)
		if(${SWIG_LANG} STREQUAL "csharp")
			include(${CU_ROOT_DIR}/cmake/cmakeUtils/helpers/GenerateNugetTarget.cmake REQUIRED)
			cu_generate_csharp_nuget_target(TARGET_NAME ${SWIG_TARGET_NAME} NUGET_SOURCE_URL "${NUGET_PUBLISH_SOURCE_URL}" NUGET_API_KEY "${NUGET_PUBLISH_API_KEY}" CS_SOURCE_FOLDERS "${SUPPORT_FILES_FOLDER_csharp}" CS_PACKAGE_DEPENDENCIES "la_avdecc:${AVDECC_PROJECT_FRIENDLY_VERSION}" NUGET_PACK_TARGET_DEPENDENCIES la_avdecc-csharp-nuget-push)
		elseif(${SWIG_LANG} STREQUAL "lua")
			target_link_libraries(${SWIG_TARGET_NAME} PRIVATE liblua)
		endif()

		# Setup install (and signing)
		cu_setup_deploy_library(${SWIG_TARGET_NAME} ${INSTALL_BINDINGS_FLAG} ${SIGN_FLAG})

	endforeach()
endif()
