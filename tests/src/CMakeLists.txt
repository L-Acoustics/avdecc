# avdecc tests

# gtest include directories
include_directories(${gtest_SOURCE_DIR}/include)

# gtest link directories
link_directories(${gtest_BINARY_DIR}/src)

# Set gtest library
set(ADD_LINK_LIBRARIES gtest)

### Unit Tests
set(TESTS_SOURCE
	main.cpp
	aatlv_tests.cpp
	aemPayloads_tests.cpp
	any_tests.cpp
	avdeccFixedString_tests.cpp
	controllerEntity_tests.cpp
	controllerStateMachine_tests.cpp
	instrumentationObserver.hpp
	logger_tests.cpp
	memoryBuffer_tests.cpp
	protocolAvtpdu_tests.cpp
	protocolInterface_pcap_tests.cpp
	protocolInterface_virtual_tests.cpp
	streamFormat_tests.cpp
	uniqueIdentifier_tests.cpp
)
list(APPEND ADD_LINK_LIBRARIES la_avdecc_static)

if(BUILD_AVDECC_CONTROLLER)
	list(APPEND TESTS_SOURCE
		avdeccController_tests.cpp
	)
	list(APPEND ADD_LINK_LIBRARIES la_avdecc_controller_static)
endif()

# Define target
add_executable(Tests ${TESTS_SOURCE})

# Setup common options
setup_executable_options(Tests)

# Set IDE folder
set_target_properties(Tests PROPERTIES FOLDER "Tests")

# Link with required libraries
target_link_libraries(Tests PRIVATE ${LINK_LIBRARIES} ${ADD_LINK_LIBRARIES})

# Set installation rule
if(INSTALL_AVDECC_TESTS)
	install(TARGETS Tests RUNTIME CONFIGURATIONS Release DESTINATION bin)
endif()
