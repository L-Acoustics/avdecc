# avdecc tests

# gtest include directories
include_directories(${gtest_SOURCE_DIR}/include)

# gtest link directories
link_directories(${gtest_BINARY_DIR}/src)

# Set gtest library
set(ADD_LINK_LIBRARIES GTest::gtest GTest::gmock)

### Unit Tests
set(TESTS_SOURCE
	main.cpp
	aatlv_tests.cpp
	aemPayloads_tests.cpp
	avdeccFixedString_tests.cpp
	controllerEntity_tests.cpp
	commandStateMachine_tests.cpp
	controllerCapabilityDelegate_tests.cpp
	endStation_tests.cpp
	enum_tests.cpp
	entity_tests.cpp
	instrumentationObserver.hpp
	logger_tests.cpp
	memoryBuffer_tests.cpp
	protocolAvtpdu_tests.cpp
	protocolInterface_pcap_tests.cpp
	protocolInterface_virtual_tests.cpp
	protocolVuAecpduProtocolIdentifier_tests.cpp
	streamFormat_tests.cpp
	uniqueIdentifier_tests.cpp
)
list(APPEND ADD_LINK_LIBRARIES la_avdecc_static)

if(BUILD_AVDECC_CONTROLLER)
	list(APPEND TESTS_SOURCE
		controller/avdeccController_tests.cpp
		controller/avdeccControlledEntity_tests.cpp
	)
	list(APPEND ADD_LINK_LIBRARIES la_avdecc_controller_static)
endif()

# Group source files
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source Files" FILES ${TESTS_SOURCE})

# Define target
add_executable(Tests ${TESTS_SOURCE})

# Setup common options
cu_setup_executable_options(Tests)

# Additional private include directory
target_include_directories(Tests PRIVATE "${CU_ROOT_DIR}/src")

# Set IDE folder
set_target_properties(Tests PROPERTIES FOLDER "Tests")

# Link with required libraries
target_link_libraries(Tests PRIVATE ${LINK_LIBRARIES} ${ADD_LINK_LIBRARIES})

# Copy test data
add_custom_command(
	TARGET Tests
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../data" "${CMAKE_BINARY_DIR}/tests/src/data"
	COMMENT "Copying Tests data to output folder"
	COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/../data/SimpleEntity.json" "${CMAKE_BINARY_DIR}/tests/src/data/テスト.json"
	COMMENT "Creating UTF-8 file"
	VERBATIM
)

# Deploy and install target and its runtime dependencies (call this AFTER ALL dependencies have been added to the target)
cu_setup_deploy_runtime(Tests ${INSTALL_TEST_FLAG} ${SIGN_FLAG})
