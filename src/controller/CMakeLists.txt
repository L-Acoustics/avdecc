# avdecc controller

cu_setup_project(la_avdecc_controller ${LA_AVDECC_CONTROLLER_VERSION} "AVDECC Controller Library")

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
set(PCH_HEADER_NAME "pch.hpp")

# Disable windows MINMAX
if(WIN32)
	list(APPEND ADD_PUBLIC_COMPILE_OPTIONS "-DNOMINMAX")
endif()

set (PUBLIC_HEADER_FILES
	${CU_ROOT_DIR}/include/la/avdecc/controller/avdeccController.hpp
	${CU_ROOT_DIR}/include/la/avdecc/controller/internals/avdeccControlledEntity.hpp
	${CU_ROOT_DIR}/include/la/avdecc/controller/internals/avdeccControlledEntityModel.hpp
	${CU_ROOT_DIR}/include/la/avdecc/controller/internals/exports.hpp
	${CU_ROOT_DIR}/include/la/avdecc/controller/internals/logItems.hpp
)

set (HEADER_FILES_COMMON
	${CMAKE_CURRENT_BINARY_DIR}/config.h
	avdeccControllerImpl.hpp
	avdeccControlledEntityImpl.hpp
	avdeccControllerLogHelper.hpp
	avdeccEntityModelCache.hpp
)

set (SOURCE_FILES_COMMON
	avdeccController.cpp
	avdeccControllerImpl.cpp
	avdeccControllerImplDelegateOverrides.cpp
	avdeccControllerImplHandlers.cpp
	avdeccControllerImplOverrides.cpp
	avdeccControlledEntityImpl.cpp
)

# Features
if(ENABLE_AVDECC_FEATURE_REDUNDANCY)
	list(APPEND ADD_PUBLIC_COMPILE_OPTIONS "-DENABLE_AVDECC_FEATURE_REDUNDANCY")
	if(ENABLE_AVDECC_STRICT_2018_REDUNDANCY)
		list(APPEND ADD_PUBLIC_COMPILE_OPTIONS "-DENABLE_AVDECC_STRICT_2018_REDUNDANCY")
	endif()
endif()
if(ENABLE_AVDECC_FEATURE_JSON)
	list(APPEND SOURCE_FILES_COMMON avdeccControlledEntityJsonSerializer.cpp)
	list(APPEND HEADER_FILES_COMMON avdeccControlledEntityJsonSerializer.hpp avdeccControllerJsonTypes.hpp)
	list(APPEND ADD_PRIVATE_COMPILE_OPTIONS "-DENABLE_AVDECC_FEATURE_JSON")
endif()

# Other options
if(ENABLE_AVDECC_CUSTOM_ANY)
	list(APPEND PUBLIC_HEADER_FILES
		${CU_ROOT_DIR}/include/la/avdecc/internals/any.hpp
	)
	list(APPEND ADD_PUBLIC_COMPILE_OPTIONS "-DENABLE_AVDECC_CUSTOM_ANY")
endif()
if(IGNORE_NEITHER_STATIC_NOR_DYNAMIC_MAPPINGS)
	list(APPEND ADD_PRIVATE_COMPILE_OPTIONS "-DIGNORE_NEITHER_STATIC_NOR_DYNAMIC_MAPPINGS")
endif()

set (LIB_HEADER_FILES ${HEADER_FILES_COMMON} ${PUBLIC_HEADER_FILES})
set (LIB_SOURCE_FILES ${SOURCE_FILES_COMMON})

source_group("Header Files\\Public\\Controller" FILES ${PUBLIC_HEADER_FILES})

# C++ dynamic library
if(BUILD_AVDECC_LIB_SHARED_CXX)
	set (LIB_SHARED_CXX_SOURCE_FILES ${LIB_SOURCE_FILES})
	add_library(la_avdecc_controller_cxx SHARED ${LIB_HEADER_FILES} ${LIB_SHARED_CXX_SOURCE_FILES} ${SHARED_LIBRARY_FILES})
	if(ADD_PRIVATE_COMPILE_OPTIONS)
		target_compile_options(la_avdecc_controller_cxx PRIVATE ${ADD_PRIVATE_COMPILE_OPTIONS})
	endif()
	if(ADD_PUBLIC_COMPILE_OPTIONS)
		target_compile_options(la_avdecc_controller_cxx PUBLIC ${ADD_PUBLIC_COMPILE_OPTIONS})
	endif()
	if(ADD_LINK_LIBS)
		target_link_libraries(la_avdecc_controller_cxx PRIVATE ${ADD_LINK_LIBS})
	endif()

	# Enable precompiled headers
	cu_set_precompiled_headers(la_avdecc_controller_cxx ${PCH_HEADER_NAME})

	# Setup common options
	cu_setup_library_options(la_avdecc_controller_cxx)

	# Link with avdecc using shared library (publicly because any executable using the controller needs low level library as well)
	target_link_libraries(la_avdecc_controller_cxx PUBLIC la_avdecc_cxx)

	# Setup libfmt
	if(ENABLE_AVDECC_USE_FMTLIB)
		target_compile_options(la_avdecc_controller_cxx PRIVATE -DHAVE_FMT)
		target_link_libraries(la_avdecc_controller_cxx PRIVATE fmt-header-only)
	endif()

	# Setup json
	if(ENABLE_AVDECC_FEATURE_JSON)
		target_link_libraries(la_avdecc_controller_cxx PRIVATE nlohmann_json)
	endif()

	# Setup install (and signing)
	cu_setup_deploy_library(la_avdecc_controller_cxx ${INSTALL_SHARED_FLAG} ${SIGN_FLAG})
endif()

# Runtime-shared static library
if(BUILD_AVDECC_LIB_STATIC_RT_SHARED)
	add_library(la_avdecc_controller_static STATIC ${LIB_HEADER_FILES} ${LIB_SOURCE_FILES})
	if(ADD_PRIVATE_COMPILE_OPTIONS)
		target_compile_options(la_avdecc_controller_static PRIVATE ${ADD_PRIVATE_COMPILE_OPTIONS})
	endif()
	if(ADD_PUBLIC_COMPILE_OPTIONS)
		target_compile_options(la_avdecc_controller_static PUBLIC ${ADD_PUBLIC_COMPILE_OPTIONS})
	endif()
	if(ADD_LINK_LIBS)
		target_link_libraries(la_avdecc_controller_static PUBLIC ${ADD_LINK_LIBS})
	endif()

	# Enable precompiled headers
	cu_set_precompiled_headers(la_avdecc_controller_static ${PCH_HEADER_NAME})

	# Setup common options
	cu_setup_library_options(la_avdecc_controller_static)

	# Setup json
	if(ENABLE_AVDECC_FEATURE_JSON)
		target_link_libraries(la_avdecc_controller_static PRIVATE nlohmann_json)
	endif()

	# Link with avdecc using static library
	target_link_libraries(la_avdecc_controller_static PUBLIC la_avdecc_static)

	# Setup install (and signing)
	cu_setup_deploy_library(la_avdecc_controller_static ${INSTALL_STATIC_FLAG} ${SIGN_FLAG})
endif()

# Set installation rules
if(INSTALL_AVDECC_HEADERS)
	cu_setup_headers_install_rules("${PUBLIC_HEADER_FILES}" "${CU_ROOT_DIR}/include")
endif()
