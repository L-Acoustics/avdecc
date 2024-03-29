# AvdeccC target

cu_setup_project(la_avdecc_c ${LA_AVDECC_C_VERSION} "AVDECC C Bridge Library")

# Configure file based on CMakeLists.txt
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
	"${CMAKE_CURRENT_BINARY_DIR}/config.h"
)
if(WIN32)
	configure_file(
		"${CU_ROOT_DIR}/resources/win32/bindings/c/resources.rc.in"
		"${CMAKE_CURRENT_BINARY_DIR}/resources.rc"
	)
	set(SHARED_LIBRARY_FILES "${CMAKE_CURRENT_BINARY_DIR}/resources.rc")
endif()

set (PUBLIC_HEADER_FILES
	${CU_ROOT_DIR}/include/la/avdecc/avdecc.h
	${CU_ROOT_DIR}/include/la/avdecc/internals/exports.h
	${CU_ROOT_DIR}/include/la/avdecc/internals/typedefs.h
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
	executor_c.cpp
	utils.cpp
	protocolAcmpdu_c.cpp
	protocolAdpdu_c.cpp
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
source_group(TREE "${CU_ROOT_DIR}/include/la/avdecc/" PREFIX "Header Files\\Public" FILES ${PUBLIC_HEADER_FILES})
source_group("Header Files" FILES ${HEADER_FILES_COMMON})
#source_group("Header Files\\Protocol" FILES ${HEADER_FILES_PROTOCOL})
source_group("Source Files" FILES ${SOURCE_FILES_COMMON})
#source_group("Source Files\\Protocol" FILES ${SOURCE_FILES_PROTOCOL})

# Define target
add_library(${PROJECT_NAME} SHARED ${PUBLIC_HEADER_FILES} ${HEADER_FILES_COMMON} ${HEADER_FILES_PROTOCOL} ${SOURCE_FILES_COMMON} ${SOURCE_FILES_PROTOCOL} ${SHARED_LIBRARY_FILES})

if(ADD_PRIVATE_COMPILE_OPTIONS)
	target_compile_options(${PROJECT_NAME} PRIVATE ${ADD_PRIVATE_COMPILE_OPTIONS})
endif()

# Setup common options
cu_setup_library_options(${PROJECT_NAME})

# Link with avdecc
target_link_libraries(${PROJECT_NAME} PRIVATE la_avdecc_cxx)

# Setup install (and signing)
cu_setup_deploy_library(${PROJECT_NAME} ${INSTALL_BINDINGS_FLAG} ${SIGN_FLAG})

# Set installation rules
if(INSTALL_AVDECC_BINDINGS AND INSTALL_AVDECC_HEADERS)
	install(DIRECTORY ${CU_ROOT_DIR}/include/la/avdecc CONFIGURATIONS Release DESTINATION include/la FILES_MATCHING PATTERN "*.h")
endif()
