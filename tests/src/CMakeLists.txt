# avdecc tests

# avdecc include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# avdecc source directories
include_directories(${PROJECT_SOURCE_DIR}/src)

# gtest include directories
include_directories(${gtest_SOURCE_DIR}/include)

# gtest link directories
link_directories(${gtest_BINARY_DIR}/src)

# Set gtest library
set(ADD_LINK_LIBRARIES gtest)

### Unit Tests
set(TESTS_SOURCE
	main.cpp
	aemPayloads_tests.cpp
	any_tests.cpp
	avdeccFixedString_tests.cpp
	controllerStateMachine_tests.cpp
	protocolAvtpdu_tests.cpp
	protocolInterface_pcap_tests.cpp
	streamFormat_tests.cpp
)
list(APPEND ADD_LINK_LIBRARIES la_avdecc_static)

#if(BUILD_AVDECC_CONTROLLER)
#	list(APPEND TESTS_SOURCE
#		controller_tests.cpp
#	)
#	list(APPEND ADD_LINK_LIBRARIES la_avdecc_controller_static)
#endif()

add_executable(Tests ${TESTS_SOURCE})
set_target_properties(Tests PROPERTIES FOLDER "Tests")
target_link_libraries(Tests PRIVATE ${LINK_LIBRARIES} ${ADD_LINK_LIBRARIES})
# Set installation rule
if(INSTALL_AVDECC_TESTS)
	install(TARGETS Tests RUNTIME CONFIGURATIONS Release DESTINATION bin)
endif()
