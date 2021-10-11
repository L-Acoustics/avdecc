# avdecc examples

# RawMessageSend
add_executable(RawMessageSend rawMessageSenderCxx.cpp utils.cpp utils.hpp)
set_target_properties(RawMessageSend PROPERTIES FOLDER "Examples")
# Using avdecc library
target_link_libraries(RawMessageSend PRIVATE la_avdecc_cxx)
if(NOT WIN32)
	target_link_libraries(RawMessageSend PRIVATE ncurses)
endif()
# Setup common options
cu_setup_executable_options(RawMessageSend)
# Deploy and install target and its runtime dependencies (call this AFTER ALL dependencies have been added to the target)
cu_setup_deploy_runtime(RawMessageSend ${INSTALL_EXAMPLE_FLAG} ${SIGN_FLAG})

# SimpleController
add_executable(SimpleController simpleController.cpp utils.cpp utils.hpp)
set_target_properties(SimpleController PROPERTIES FOLDER "Examples")
# Using avdecc library
target_link_libraries(SimpleController PRIVATE la_avdecc_cxx)
if(NOT WIN32)
	target_link_libraries(SimpleController PRIVATE ncurses)
endif()
# Setup common options
cu_setup_executable_options(SimpleController)
# Deploy and install target and its runtime dependencies (call this AFTER ALL dependencies have been added to the target)
cu_setup_deploy_runtime(SimpleController ${INSTALL_EXAMPLE_FLAG} ${SIGN_FLAG})

# StreamFormatInfo
add_executable(StreamFormatInfo streamFormatInfo.cpp)
set_target_properties(StreamFormatInfo PROPERTIES FOLDER "Examples")
# Using avdecc library
target_link_libraries(StreamFormatInfo PRIVATE la_avdecc_cxx)
# Setup common options
cu_setup_executable_options(StreamFormatInfo)
# Deploy and install target and its runtime dependencies (call this AFTER ALL dependencies have been added to the target)
cu_setup_deploy_runtime(StreamFormatInfo ${INSTALL_EXAMPLE_FLAG} ${SIGN_FLAG})

# C bindings examples
if(BUILD_AVDECC_BINDINGS_C)
	# C bindings Example (using c++ code)
	add_executable(RawMessageSendC rawMessageSenderC.cpp utils.cpp utils.hpp)
	set_target_properties(RawMessageSendC PROPERTIES FOLDER "Examples/Bindings/C")
	# Use C bindings in utils.cpp
	target_compile_options(RawMessageSendC PRIVATE -DUSE_BINDINGS_C)
	# Using avdecc library
	target_link_libraries(RawMessageSendC PRIVATE la_avdecc_c)
	if(NOT WIN32)
		target_link_libraries(RawMessageSendC PRIVATE ncurses)
	endif()
	# Setup NIH
	target_link_libraries(RawMessageSendC PRIVATE la_networkInterfaceHelper_static)
	# Setup common options
	cu_setup_executable_options(RawMessageSendC)
	# Deploy and install target and its runtime dependencies (call this AFTER ALL dependencies have been added to the target)
	cu_setup_deploy_runtime(RawMessageSendC ${INSTALL_EXAMPLE_FLAG} ${SIGN_FLAG})
endif()

# Controller library examples
if(BUILD_AVDECC_CONTROLLER)
	# Discovery
	add_executable(Discovery discovery.cpp utils.cpp utils.hpp)
	set_target_properties(Discovery PROPERTIES FOLDER "Examples/Controller")
	# Using controller library
	target_link_libraries(Discovery PRIVATE la_avdecc_controller_cxx)
	if(NOT WIN32)
		target_link_libraries(Discovery PRIVATE ncurses)
	endif()
	# Setup common options
	cu_setup_executable_options(Discovery)
	# Deploy and install target and its runtime dependencies (call this AFTER ALL dependencies have been added to the target)
	cu_setup_deploy_runtime(Discovery ${INSTALL_EXAMPLE_FLAG} ${SIGN_FLAG})

	# EntityDumper
	add_executable(EntityDumper entityDumper.cpp utils.cpp utils.hpp)
	set_target_properties(EntityDumper PROPERTIES FOLDER "Examples/Controller")
	# Using controller library
	target_link_libraries(EntityDumper PRIVATE la_avdecc_controller_cxx)
	if(NOT WIN32)
		target_link_libraries(EntityDumper PRIVATE ncurses)
	endif()
	# Setup common options
	cu_setup_executable_options(EntityDumper)
	# Deploy and install target and its runtime dependencies (call this AFTER ALL dependencies have been added to the target)
	cu_setup_deploy_runtime(EntityDumper ${INSTALL_EXAMPLE_FLAG} ${SIGN_FLAG})
endif()
