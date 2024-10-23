# Library CMake file

cu_setup_project(la_avdecc ${LA_AVDECC_VERSION} "AVDECC Library")

# Configure file based on CMakeLists.txt
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
	"${CMAKE_CURRENT_BINARY_DIR}/config.h"
)
if(WIN32)
	configure_file(
		"${CU_ROOT_DIR}/resources/win32/resources.rc.in"
		"${CMAKE_CURRENT_BINARY_DIR}/resources.rc"
	)
	set(SHARED_LIBRARY_FILES "${CMAKE_CURRENT_BINARY_DIR}/resources.rc")
endif()

set(ADD_LINK_LIBS "")
set(ADD_PRIVATE_COMPILE_OPTIONS "")
set(ADD_PUBLIC_COMPILE_OPTIONS "")
set(PCH_HEADER_NAME "pch.hpp")

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	# Add pthread library on linux
	list(APPEND ADD_LINK_LIBS "-lpthread")
endif()

# Disable windows MINMAX
if(WIN32)
	list(APPEND ADD_PUBLIC_COMPILE_OPTIONS "-DNOMINMAX")
endif()

set (PUBLIC_HEADER_FILES
	${CU_ROOT_DIR}/include/la/avdecc/avdecc.hpp
	${CU_ROOT_DIR}/include/la/avdecc/executor.hpp
	${CU_ROOT_DIR}/include/la/avdecc/logger.hpp
	${CU_ROOT_DIR}/include/la/avdecc/memoryBuffer.hpp
	${CU_ROOT_DIR}/include/la/avdecc/utils.hpp
	${CU_ROOT_DIR}/include/la/avdecc/watchDog.hpp
	${CU_ROOT_DIR}/include/la/avdecc/internals/aggregateEntity.hpp
	${CU_ROOT_DIR}/include/la/avdecc/internals/controllerEntity.hpp
	${CU_ROOT_DIR}/include/la/avdecc/internals/endian.hpp
	${CU_ROOT_DIR}/include/la/avdecc/internals/endStation.hpp
	${CU_ROOT_DIR}/include/la/avdecc/internals/entity.hpp
	${CU_ROOT_DIR}/include/la/avdecc/internals/entityAddressAccessTypes.hpp
	${CU_ROOT_DIR}/include/la/avdecc/internals/entityEnums.hpp
	${CU_ROOT_DIR}/include/la/avdecc/internals/entityModel.hpp
	${CU_ROOT_DIR}/include/la/avdecc/internals/entityModelControlValues.hpp
	${CU_ROOT_DIR}/include/la/avdecc/internals/entityModelControlValuesTraits.hpp
	${CU_ROOT_DIR}/include/la/avdecc/internals/entityModelTree.hpp
	${CU_ROOT_DIR}/include/la/avdecc/internals/entityModelTreeCommon.hpp
	${CU_ROOT_DIR}/include/la/avdecc/internals/entityModelTreeDynamic.hpp
	${CU_ROOT_DIR}/include/la/avdecc/internals/entityModelTreeStatic.hpp
	${CU_ROOT_DIR}/include/la/avdecc/internals/entityModelTypes.hpp
	${CU_ROOT_DIR}/include/la/avdecc/internals/exception.hpp
	${CU_ROOT_DIR}/include/la/avdecc/internals/exports.hpp
	${CU_ROOT_DIR}/include/la/avdecc/internals/instrumentationNotifier.hpp
	${CU_ROOT_DIR}/include/la/avdecc/internals/jsonSerialization.hpp
	${CU_ROOT_DIR}/include/la/avdecc/internals/logItems.hpp
	${CU_ROOT_DIR}/include/la/avdecc/internals/protocolAaAecpdu.hpp
	${CU_ROOT_DIR}/include/la/avdecc/internals/protocolAcmpdu.hpp
	${CU_ROOT_DIR}/include/la/avdecc/internals/protocolAdpdu.hpp
	${CU_ROOT_DIR}/include/la/avdecc/internals/protocolAecpdu.hpp
	${CU_ROOT_DIR}/include/la/avdecc/internals/protocolAemAecpdu.hpp
	${CU_ROOT_DIR}/include/la/avdecc/internals/protocolAemPayloadSizes.hpp
	${CU_ROOT_DIR}/include/la/avdecc/internals/protocolAvtpdu.hpp
	${CU_ROOT_DIR}/include/la/avdecc/internals/protocolDefines.hpp
	${CU_ROOT_DIR}/include/la/avdecc/internals/protocolMvuAecpdu.hpp
	${CU_ROOT_DIR}/include/la/avdecc/internals/protocolMvuPayloadSizes.hpp
	${CU_ROOT_DIR}/include/la/avdecc/internals/protocolVuAecpdu.hpp
	${CU_ROOT_DIR}/include/la/avdecc/internals/protocolInterface.hpp
	${CU_ROOT_DIR}/include/la/avdecc/internals/serialization.hpp
	${CU_ROOT_DIR}/include/la/avdecc/internals/streamFormatInfo.hpp
	${CU_ROOT_DIR}/include/la/avdecc/internals/uniqueIdentifier.hpp
)

# Low level Protocol
set (HEADER_FILES_PROTOCOL
	protocol/protocolAemControlValuesPayloads.hpp
	protocol/protocolAemPayloads.hpp
	protocol/protocolMvuPayloads.hpp
)

set (SOURCE_FILES_PROTOCOL
	protocol/protocolAcmpdu.cpp
	protocol/protocolAdpdu.cpp
	protocol/protocolAecpdu.cpp
	protocol/protocolAemAecpdu.cpp
	protocol/protocolAemPayloads.cpp
	protocol/protocolAaAecpdu.cpp
	protocol/protocolAvtpdu.cpp
	protocol/protocolDefines.cpp
	protocol/protocolVuAecpdu.cpp
	protocol/protocolMvuAecpdu.cpp
	protocol/protocolMvuPayloads.cpp
)

# Protocol Interface
set (HEADER_FILES_PROTOCOL_INTERFACE
	protocolInterface/ethernetPacketDispatch.hpp
)

set (SOURCE_FILES_PROTOCOL_INTERFACE
	protocolInterface/protocolInterface.cpp
)

# State machines
set (HEADER_FILES_STATE_MACHINES
	stateMachine/advertiseStateMachine.hpp
	stateMachine/commandStateMachine.hpp
	stateMachine/discoveryStateMachine.hpp
	stateMachine/protocolInterfaceDelegate.hpp
	stateMachine/stateMachineManager.hpp
)

set (SOURCE_FILES_STATE_MACHINES
	stateMachine/advertiseStateMachine.cpp
	stateMachine/commandStateMachine.cpp
	stateMachine/discoveryStateMachine.cpp
	stateMachine/stateMachineManager.cpp
)

# Entity
set (HEADER_FILES_ENTITY
	entity/entityImpl.hpp
	entity/localEntityImpl.ipp
	entity/aemHandler.hpp
	entity/aggregateEntityImpl.hpp
	entity/controllerCapabilityDelegate.hpp
	entity/controllerEntityImpl.hpp
)

set (SOURCE_FILES_ENTITY
	entity/entityModelTypes.cpp
	entity/localEntityImpl.cpp
	entity/aemHandler.cpp
	entity/aggregateEntityImpl.cpp
	entity/controllerCapabilityDelegate.cpp
	entity/controllerEntityImpl.cpp
	entity/entityModelControlValues.cpp
)

# Common files
set (HEADER_FILES_COMMON
	${CMAKE_CURRENT_BINARY_DIR}/config.h
	endStationImpl.hpp
	logHelper.hpp
)

set (SOURCE_FILES_COMMON
	avdecc.cpp
	endStationImpl.cpp
	executor.cpp
	logger.cpp
	streamFormatInfo.cpp
	utils.cpp
	watchDog.cpp
)

# Pcap Protocol interface
if(BUILD_AVDECC_INTERFACE_PCAP)
	list(APPEND SOURCE_FILES_PROTOCOL_INTERFACE
		protocolInterface/protocolInterface_pcap.cpp
	)
	list(APPEND HEADER_FILES_PROTOCOL_INTERFACE
		protocolInterface/protocolInterface_pcap.hpp
	)
	list(APPEND ADD_PRIVATE_COMPILE_OPTIONS "-DHAVE_PROTOCOL_INTERFACE_PCAP")
	if(WIN32)
		# Winpcap include directories (always needed)
		include_directories(${LA_WINPCAP_BASE_DIR}/Include)
		# Define link flags for static linkage
		set(PCAP_STATIC_LINKAGE_LD_FLAGS "wpcap")
	else()
		# Define link flags for static linkage
		set(PCAP_STATIC_LINKAGE_LD_FLAGS "-lpcap")
		# Define link flags for dynamic linkage
		set(PCAP_DYNAMIC_LINKAGE_LD_FLAGS "-ldl")
	endif()

	# Pcap library helper
	list(APPEND HEADER_FILES_PROTOCOL_INTERFACE protocolInterface/pcapInterface.hpp)
	if(BUILD_AVDECC_INTERFACE_PCAP_DYNAMIC_LINKING)
		list(APPEND SOURCE_FILES_PROTOCOL_INTERFACE protocolInterface/pcapDynamicLinking.cpp)
		list(APPEND ADD_LINK_LIBS ${PCAP_DYNAMIC_LINKAGE_LD_FLAGS})
	else()
		list(APPEND SOURCE_FILES_PROTOCOL_INTERFACE protocolInterface/pcapStaticLinking.cpp)
		list(APPEND ADD_LINK_LIBS ${PCAP_STATIC_LINKAGE_LD_FLAGS})
	endif()
endif()

# Mac Protocol interface
if(BUILD_AVDECC_INTERFACE_MAC)
	list(APPEND SOURCE_FILES_PROTOCOL_INTERFACE
		protocolInterface/protocolInterface_macNative.mm
	)
	list(APPEND HEADER_FILES_PROTOCOL_INTERFACE
		protocolInterface/protocolInterface_macNative.hpp
	)
	list(APPEND ADD_PRIVATE_COMPILE_OPTIONS "-DHAVE_PROTOCOL_INTERFACE_MAC")
	list(APPEND ADD_LINK_LIBS "-framework AudioVideoBridging")
endif()

# Proxy Protocol interface
if(BUILD_AVDECC_INTERFACE_PROXY)
	message(FATAL_ERROR "Not supported yet")
	list(APPEND ADD_PRIVATE_COMPILE_OPTIONS "-DHAVE_PROTOCOL_INTERFACE_PROXY")
endif()

# Virtual Protocol interface
if(BUILD_AVDECC_INTERFACE_VIRTUAL)
	list(APPEND SOURCE_FILES_PROTOCOL_INTERFACE
		protocolInterface/protocolInterface_virtual.cpp
	)
	list(APPEND HEADER_FILES_PROTOCOL_INTERFACE
		protocolInterface/protocolInterface_virtual.hpp
	)
	list(APPEND ADD_PRIVATE_COMPILE_OPTIONS "-DHAVE_PROTOCOL_INTERFACE_VIRTUAL")
endif()

# Features
if(ENABLE_AVDECC_FEATURE_REDUNDANCY)
	list(APPEND ADD_PUBLIC_COMPILE_OPTIONS "-DENABLE_AVDECC_FEATURE_REDUNDANCY")
endif()
if(ENABLE_AVDECC_FEATURE_JSON)
	list(APPEND PUBLIC_HEADER_FILES
		${CU_ROOT_DIR}/include/la/avdecc/internals/jsonTypes.hpp
	)
	list(APPEND SOURCE_FILES_ENTITY
		entity/entityModelJsonSerializer.cpp
	)
	list(APPEND ADD_PUBLIC_COMPILE_OPTIONS "-DENABLE_AVDECC_FEATURE_JSON")
	# 64 bits visual studio requires /bigobj to be used for this file (apperently there are too many symbols in the file), but let's set the flag for 32 bits as well
	if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
		set_source_files_properties(entity/entityModelJsonSerializer.cpp PROPERTIES COMPILE_FLAGS "/bigobj")
	endif()
endif()

# Other options
if(IGNORE_INVALID_CONTROL_DATA_LENGTH)
	list(APPEND ADD_PRIVATE_COMPILE_OPTIONS "-DIGNORE_INVALID_CONTROL_DATA_LENGTH")
endif()
if(IGNORE_INVALID_NON_SUCCESS_AEM_RESPONSES)
	list(APPEND ADD_PRIVATE_COMPILE_OPTIONS "-DIGNORE_INVALID_NON_SUCCESS_AEM_RESPONSES")
endif()
if(ALLOW_GET_AUDIO_MAP_UNSOL)
	list(APPEND ADD_PRIVATE_COMPILE_OPTIONS "-DALLOW_GET_AUDIO_MAP_UNSOL")
endif()
if(ALLOW_SEND_BIG_AECP_PAYLOADS)
	list(APPEND ADD_PUBLIC_COMPILE_OPTIONS "-DALLOW_SEND_BIG_AECP_PAYLOADS")
endif()
if(ALLOW_RECV_BIG_AECP_PAYLOADS)
	list(APPEND ADD_PUBLIC_COMPILE_OPTIONS "-DALLOW_RECV_BIG_AECP_PAYLOADS")
endif()

# Group sources
source_group(TREE "${CU_ROOT_DIR}/include/la/avdecc/" PREFIX "Header Files\\Public" FILES ${PUBLIC_HEADER_FILES})
source_group("Header Files\\Protocol" FILES ${HEADER_FILES_PROTOCOL})
source_group("Header Files\\Protocol Interface" FILES ${HEADER_FILES_PROTOCOL_INTERFACE})
source_group("Header Files\\State Machines" FILES ${HEADER_FILES_STATE_MACHINES})
source_group("Header Files\\Entity" FILES ${HEADER_FILES_ENTITY})
source_group("Source Files\\Protocol" FILES ${SOURCE_FILES_PROTOCOL})
source_group("Source Files\\Protocol Interface" FILES ${SOURCE_FILES_PROTOCOL_INTERFACE})
source_group("Source Files\\State Machines" FILES ${SOURCE_FILES_STATE_MACHINES})
source_group("Source Files\\Entity" FILES ${SOURCE_FILES_ENTITY})

set (LIB_HEADER_FILES ${HEADER_FILES_COMMON} ${HEADER_FILES_PROTOCOL} ${HEADER_FILES_PROTOCOL_INTERFACE} ${HEADER_FILES_STATE_MACHINES} ${HEADER_FILES_ENTITY} ${PUBLIC_HEADER_FILES})
set (LIB_SOURCE_FILES ${SOURCE_FILES_COMMON} ${SOURCE_FILES_PROTOCOL} ${SOURCE_FILES_PROTOCOL_INTERFACE} ${SOURCE_FILES_STATE_MACHINES} ${SOURCE_FILES_ENTITY})

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

	# Additional private include directory
	target_include_directories(${PROJECT_NAME}_cxx PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}) # Include root source directory for access to private headers
	
	# Setup NIH
	target_link_libraries(${PROJECT_NAME}_cxx PUBLIC la_networkInterfaceHelper_static)

	# Setup libfmt
	if(ENABLE_AVDECC_USE_FMTLIB)
		target_compile_options(${PROJECT_NAME}_cxx PRIVATE -DHAVE_FMT -DFMT_HEADER_ONLY)
	endif()

	# Setup json
	if(ENABLE_AVDECC_FEATURE_JSON)
		target_link_libraries(${PROJECT_NAME}_cxx PUBLIC nlohmann_json)
	endif()

	# Setup install (and signing)
	cu_setup_deploy_library(${PROJECT_NAME}_cxx ${INSTALL_SHARED_FLAG} ${SIGN_FLAG})
endif()

# Runtime-shared static library
if(BUILD_AVDECC_LIB_STATIC_RT_SHARED)
	add_library(${PROJECT_NAME}_static STATIC ${LIB_HEADER_FILES} ${LIB_SOURCE_FILES})
	if(ADD_PRIVATE_COMPILE_OPTIONS)
		target_compile_options(${PROJECT_NAME}_static PUBLIC ${ADD_PRIVATE_COMPILE_OPTIONS}) # For a static library, no compile option is really private (using private APIs is possible, thus we need to know the options)
	endif()
	if(ADD_PUBLIC_COMPILE_OPTIONS)
		target_compile_options(${PROJECT_NAME}_static PUBLIC ${ADD_PUBLIC_COMPILE_OPTIONS})
	endif()
	if(ADD_LINK_LIBS)
		target_link_libraries(${PROJECT_NAME}_static PUBLIC ${ADD_LINK_LIBS})
	endif()

	# Enable precompiled headers
	cu_set_precompiled_headers(${PROJECT_NAME}_static ${PCH_HEADER_NAME})

	# Setup common library options
	cu_setup_library_options(${PROJECT_NAME}_static)

	# Additional include directory (only for build interface)
	target_include_directories(${PROJECT_NAME}_static PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>) # Include root source directory for access to private headers
	
	# Setup NIH
	target_link_libraries(${PROJECT_NAME}_static PUBLIC la_networkInterfaceHelper_static)

	# Setup json
	if(ENABLE_AVDECC_FEATURE_JSON)
		target_link_libraries(${PROJECT_NAME}_static PUBLIC nlohmann_json)
	endif()

	# Setup install (and signing)
	cu_setup_deploy_library(${PROJECT_NAME}_static ${INSTALL_STATIC_FLAG} ${SIGN_FLAG})

	# We must install wpcap link library for windows, so the target using static library can properly link
	if(INSTALL_AVDECC_LIB_STATIC AND NOT ${BUILD_AVDECC_INTERFACE_PCAP_DYNAMIC_LINKING} AND WIN32)
		install(FILES "${LA_WINPCAP_LIB_DIR}/wpcap.lib" DESTINATION lib)
	endif()
endif()

# Set installation rules
if(INSTALL_AVDECC_HEADERS)
	cu_setup_headers_install_rules("${PUBLIC_HEADER_FILES}" "${CU_ROOT_DIR}/include")
endif()
