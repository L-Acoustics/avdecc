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

set (PUBLIC_HEADER_FILES
	${LA_ROOT_DIR}/include/la/avdecc/controller/avdeccController.hpp
	${LA_ROOT_DIR}/include/la/avdecc/controller/internals/exports.hpp
	${LA_ROOT_DIR}/include/la/avdecc/controller/internals/avdeccControlledEntity.hpp
)
source_group("Header Files\\Public\\Controller" FILES ${PUBLIC_HEADER_FILES})

set (HEADER_FILES_COMMON
	avdeccControlledEntityImpl.hpp
)

set (SOURCE_FILES_COMMON
	avdeccController.cpp
	avdeccControlledEntityImpl.cpp
)

set (LIB_HEADER_FILES ${HEADER_FILES_COMMON} ${PUBLIC_HEADER_FILES})
set (LIB_SOURCE_FILES ${SOURCE_FILES_COMMON})

# C++ dynamic library
if(BUILD_AVDECC_LIB_SHARED_CXX)
	set (LIB_SHARED_CXX_SOURCE_FILES ${LIB_SOURCE_FILES})
	add_library(la_avdecc_controller_cxx SHARED ${LIB_HEADER_FILES} ${LIB_SHARED_CXX_SOURCE_FILES} ${SHARED_LIBRARY_FILES})

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
