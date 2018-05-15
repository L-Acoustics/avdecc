# avdecc controller

setup_project(la_avdecc_controller ${LA_AVDECC_CONTROLLER_VERSION} "Avdecc Controller Library")

# Configure file based on CMakeLists.txt
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
	"${CMAKE_CURRENT_BINARY_DIR}/config.h"
)
if(WIN32)
	configure_file(
		"${LA_ROOT_DIR}/resources/win32/controller/resources.rc.in"
		"${CMAKE_CURRENT_BINARY_DIR}/resources.rc"
	)
	set(SHARED_LIBRARY_FILES "${CMAKE_CURRENT_BINARY_DIR}/resources.rc")
endif()

set(ADD_LINK_LIBS "")
set(ADD_PRIVATE_COMPILE_OPTIONS "")
set(ADD_PUBLIC_COMPILE_OPTIONS "")

set (PUBLIC_HEADER_FILES
	${LA_ROOT_DIR}/include/la/avdecc/controller/avdeccController.hpp
	${LA_ROOT_DIR}/include/la/avdecc/controller/internals/exports.hpp
	${LA_ROOT_DIR}/include/la/avdecc/controller/internals/avdeccControlledEntity.hpp
	${LA_ROOT_DIR}/include/la/avdecc/controller/internals/avdeccControlledEntityModel.hpp
	${LA_ROOT_DIR}/include/la/avdecc/controller/internals/avdeccControlledEntityCommonModel.hpp
	${LA_ROOT_DIR}/include/la/avdecc/controller/internals/avdeccControlledEntityDynamicModel.hpp
	${LA_ROOT_DIR}/include/la/avdecc/controller/internals/avdeccControlledEntityStaticModel.hpp
)
source_group("Header Files\\Public\\Controller" FILES ${PUBLIC_HEADER_FILES})

set (HEADER_FILES_COMMON
	${CMAKE_CURRENT_BINARY_DIR}/config.h
	avdeccControllerImpl.hpp
	avdeccControlledEntityImpl.hpp
	avdeccControlledEntityModelTree.hpp
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

# Other options
if(ENABLE_AVDECC_CUSTOM_ANY)
	list(APPEND PUBLIC_HEADER_FILES
		${LA_ROOT_DIR}/include/la/avdecc/internals/any.hpp
	)
	list(APPEND ADD_PUBLIC_COMPILE_OPTIONS "-DENABLE_AVDECC_CUSTOM_ANY")
endif()
if(IGNORE_NEITHER_STATIC_NOR_DYNAMIC_MAPPINGS)
	list(APPEND ADD_PRIVATE_COMPILE_OPTIONS "-DIGNORE_NEITHER_STATIC_NOR_DYNAMIC_MAPPINGS")
endif()

set (LIB_HEADER_FILES ${HEADER_FILES_COMMON} ${PUBLIC_HEADER_FILES})
set (LIB_SOURCE_FILES ${SOURCE_FILES_COMMON})

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

	# Setup common options
	setup_library_options(la_avdecc_controller_cxx "la_avdecc_controller")

	# Link with avdecc using shared library
	target_link_libraries(la_avdecc_controller_cxx PUBLIC la_avdecc_cxx)

	# Set installation rule
	if(INSTALL_AVDECC_LIB_SHARED_CXX)
		# Sign binary (this is done during installation phase)
		if(ENABLE_AVDECC_SIGNING)
			sign_target(la_avdecc_controller_cxx)
		endif()
		setup_library_install_rules(la_avdecc_controller_cxx)
	endif()
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

	# Setup common options
	setup_library_options(la_avdecc_controller_static "la_avdecc_controller")

	# Link with avdecc using static library
	target_link_libraries(la_avdecc_controller_static PUBLIC la_avdecc_static)

	# Set installation rule
	if(INSTALL_AVDECC_LIB_STATIC)
		setup_library_install_rules(la_avdecc_controller_static)
	endif()
endif()

# Set installation rules
if(INSTALL_AVDECC_HEADERS)
	install(DIRECTORY ${LA_ROOT_DIR}/include/la/avdecc CONFIGURATIONS Release DESTINATION include/la)
endif()
