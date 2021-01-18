# AvdeccC target

setup_project(la_avdecc_c ${LA_AVDECC_C_VERSION} "Avdecc C Bridge Library")

# Configure file based on CMakeLists.txt
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
	"${CMAKE_CURRENT_BINARY_DIR}/config.h"
)
if(WIN32)
	configure_file(
		"${LA_ROOT_DIR}/resources/win32/bindings/c/resources.rc.in"
		"${CMAKE_CURRENT_BINARY_DIR}/resources.rc"
	)
	set(SHARED_LIBRARY_FILES "${CMAKE_CURRENT_BINARY_DIR}/resources.rc")
endif()

set (PUBLIC_HEADER_FILES
	${LA_ROOT_DIR}/include/la/avdecc/avdecc.h
	${LA_ROOT_DIR}/include/la/avdecc/networkInterfaceHelper.h
	${LA_ROOT_DIR}/include/la/avdecc/internals/exports.h
	${LA_ROOT_DIR}/include/la/avdecc/internals/typedefs.h
)

# Common files
set(HEADER_FILES_COMMON
	${CMAKE_CURRENT_BINARY_DIR}/config.h
	utils.hpp
	localEntity_c.hpp
	protocolInterface_c.hpp
)

set(SOURCE_FILES_COMMON
	avdecc_c.cpp
	utils.cpp
	protocolAcmpdu_c.cpp
	protocolAdpdu_c.cpp
	networkInterfaceHelper_c.cpp
	localEntity_c.cpp
	protocolInterface_c.cpp
)

# Other options
if(ALLOW_SEND_BIG_AECP_PAYLOADS)
	list(APPEND ADD_PRIVATE_COMPILE_OPTIONS "-DALLOW_SEND_BIG_AECP_PAYLOADS")
endif()
if(ALLOW_RECV_BIG_AECP_PAYLOADS)
	list(APPEND ADD_PRIVATE_COMPILE_OPTIONS "-DALLOW_RECV_BIG_AECP_PAYLOADS")
endif()

# Group sources
source_group("Header Files\\Public" FILES ${PUBLIC_HEADER_FILES})
source_group("Header Files" FILES ${HEADER_FILES_COMMON})
#source_group("Header Files\\Protocol" FILES ${HEADER_FILES_PROTOCOL})
source_group("Source Files" FILES ${SOURCE_FILES_COMMON})
#source_group("Source Files\\Protocol" FILES ${SOURCE_FILES_PROTOCOL})

# Define target
add_library(la_avdecc_c SHARED ${PUBLIC_HEADER_FILES} ${HEADER_FILES_COMMON} ${HEADER_FILES_PROTOCOL} ${SOURCE_FILES_COMMON} ${SOURCE_FILES_PROTOCOL})

if(ADD_PRIVATE_COMPILE_OPTIONS)
	target_compile_options(la_avdecc_c PRIVATE ${ADD_PRIVATE_COMPILE_OPTIONS})
endif()

# Setup common options
setup_library_options(la_avdecc_c "la_avdecc_c")

# Link with avdecc
target_link_libraries(la_avdecc_c PRIVATE la_avdecc_cxx)

# Setup install (and signing)
setup_deploy_library(la_avdecc_c ${INSTALL_BINDINGS_FLAG} ${SIGN_FLAG})

# Set installation rules
if(INSTALL_AVDECC_BINDINGS AND INSTALL_AVDECC_HEADERS)
	install(DIRECTORY ${LA_ROOT_DIR}/include/la/avdecc CONFIGURATIONS Release DESTINATION include/la FILES_MATCHING PATTERN "*.h")
endif()
