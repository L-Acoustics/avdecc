# avdecc examples

# RawMessageSend
add_executable(RawMessageSend rawMessageSenderCxx.cpp utils.cpp utils.hpp)
set_target_properties(RawMessageSend PROPERTIES FOLDER "Examples")
# Using avdecc library
target_link_libraries(RawMessageSend PRIVATE la_avdecc_cxx)
copy_runtime(RawMessageSend la_avdecc_cxx)
if(NOT WIN32)
	target_link_libraries(RawMessageSend PRIVATE ncurses)
endif()
# Setup common options
setup_executable_options(RawMessageSend)
# Set installation rule
if(INSTALL_AVDECC_EXAMPLES)
	install(TARGETS RawMessageSend RUNTIME CONFIGURATIONS Release DESTINATION bin)
endif()

# SimpleController
add_executable(SimpleController simpleController.cpp utils.cpp utils.hpp)
set_target_properties(SimpleController PROPERTIES FOLDER "Examples")
# Using avdecc library
target_link_libraries(SimpleController PRIVATE la_avdecc_cxx)
copy_runtime(SimpleController la_avdecc_cxx)
if(NOT WIN32)
	target_link_libraries(SimpleController PRIVATE ncurses)
endif()
# Setup common options
setup_executable_options(SimpleController)
# Set installation rule
if(INSTALL_AVDECC_EXAMPLES)
	install(TARGETS SimpleController RUNTIME CONFIGURATIONS Release DESTINATION bin)
endif()

# Network interfaces enumerator using la_avdecc_cxx
add_executable(NetworkInterfacesEnumeratorCxx networkInterfacesEnumeratorCxx.cpp)
set_target_properties(NetworkInterfacesEnumeratorCxx PROPERTIES FOLDER "Examples")
# Using avdecc library
target_link_libraries(NetworkInterfacesEnumeratorCxx PRIVATE la_avdecc_cxx)
copy_runtime(NetworkInterfacesEnumeratorCxx la_avdecc_cxx)
# Setup common options
setup_executable_options(NetworkInterfacesEnumeratorCxx)
# Set installation rule
if(INSTALL_AVDECC_EXAMPLES)
	install(TARGETS NetworkInterfacesEnumeratorCxx RUNTIME CONFIGURATIONS Release DESTINATION bin)
endif()

# C bindings examples
if(BUILD_AVDECC_BINDINGS_C)
	# Network interfaces enumerator using la_avdecc_c
	add_executable(NetworkInterfacesEnumeratorC networkInterfacesEnumerator.c)
	set_target_properties(NetworkInterfacesEnumeratorC PROPERTIES FOLDER "Examples")
	# Using avdecc C library
	target_link_libraries(NetworkInterfacesEnumeratorC PRIVATE la_avdecc_c)
	copy_runtime(NetworkInterfacesEnumeratorC la_avdecc_c)
	# Setup common options
	setup_executable_options(NetworkInterfacesEnumeratorC)
	# Set installation rule
	if(INSTALL_AVDECC_EXAMPLES)
		install(TARGETS NetworkInterfacesEnumeratorC RUNTIME CONFIGURATIONS Release DESTINATION bin)
	endif()

	# C bindings Example (using c++ code)
	add_executable(RawMessageSendC rawMessageSenderC.cpp utils.cpp utils.hpp)
	set_target_properties(RawMessageSendC PROPERTIES FOLDER "Examples")
	# Use C bindings in utils.cpp
	target_compile_options(RawMessageSendC PRIVATE -DUSE_BINDINGS_C)
	# Using avdecc library
	target_link_libraries(RawMessageSendC PRIVATE la_avdecc_c)
	copy_runtime(RawMessageSendC la_avdecc_c)
	if(NOT WIN32)
		target_link_libraries(RawMessageSendC PRIVATE ncurses)
	endif()
	# Setup common options
	setup_executable_options(RawMessageSendC)
	# Set installation rule
	if(INSTALL_AVDECC_EXAMPLES)
		install(TARGETS RawMessageSendC RUNTIME CONFIGURATIONS Release DESTINATION bin)
	endif()
endif()

# Controller library examples
if(BUILD_AVDECC_CONTROLLER)
	# Discovery
	add_executable(Discovery discovery.cpp utils.cpp utils.hpp)
	set_target_properties(Discovery PROPERTIES FOLDER "Examples")
	# Using controller library
	target_link_libraries(Discovery PRIVATE la_avdecc_controller_cxx)
	copy_runtime(Discovery la_avdecc_controller_cxx)
	copy_runtime(Discovery la_avdecc_cxx) # We need to copy avdecc library too (until it's transitively added to the export target)
	if(NOT WIN32)
		target_link_libraries(Discovery PRIVATE ncurses)
	endif()
	# Setup common options
	setup_executable_options(Discovery)
	# Set installation rule
	if(INSTALL_AVDECC_EXAMPLES)
		install(TARGETS Discovery RUNTIME CONFIGURATIONS Release DESTINATION bin)
	endif()

	# EntityDumper
	add_executable(EntityDumper entityDumper.cpp utils.cpp utils.hpp)
	set_target_properties(EntityDumper PROPERTIES FOLDER "Examples")
	# Using controller library
	target_link_libraries(EntityDumper PRIVATE la_avdecc_controller_cxx)
	copy_runtime(EntityDumper la_avdecc_controller_cxx)
	copy_runtime(EntityDumper la_avdecc_cxx) # We need to copy avdecc library too (until it's transitively added to the export target)
	if(NOT WIN32)
		target_link_libraries(EntityDumper PRIVATE ncurses)
	endif()
	# Setup common options
	setup_executable_options(EntityDumper)
	# Set installation rule
	if(INSTALL_AVDECC_EXAMPLES)
		install(TARGETS EntityDumper RUNTIME CONFIGURATIONS Release DESTINATION bin)
	endif()
endif()
