# avdecc examples

# Examples are using curses on Linux and macOS (but not if the generator is Xcode)
set(CURSES_COMPILE_OPTIONS "")
set(CURSES_LINK_OPTIONS "")
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	# Check if the generator is not Xcode
	if(NOT CMAKE_GENERATOR STREQUAL "Xcode")
		set(CURSES_COMPILE_OPTIONS -DUSE_CURSES)
		set(CURSES_LINK_OPTIONS ncurses)
	endif()
endif()

# RawMessageSend
add_executable(RawMessageSend rawMessageSenderCxx.cpp utils.cpp utils.hpp)
set_target_properties(RawMessageSend PROPERTIES FOLDER "Examples")
# Using avdecc library
target_link_libraries(RawMessageSend PRIVATE la_avdecc_cxx)
# Add (optional) curses support
target_compile_options(RawMessageSend PRIVATE ${CURSES_COMPILE_OPTIONS})
target_link_libraries(RawMessageSend PRIVATE ${CURSES_LINK_OPTIONS})
# Setup common options
cu_setup_executable_options(RawMessageSend)
# Deploy and install target and its runtime dependencies (call this AFTER ALL dependencies have been added to the target)
cu_setup_deploy_runtime(RawMessageSend ${INSTALL_EXAMPLE_FLAG} ${SIGN_FLAG})

# SimpleController
add_executable(SimpleController simpleController.cpp utils.cpp utils.hpp)
set_target_properties(SimpleController PROPERTIES FOLDER "Examples")
# Using avdecc library
target_link_libraries(SimpleController PRIVATE la_avdecc_cxx)
# Add (optional) curses support
target_compile_options(SimpleController PRIVATE ${CURSES_COMPILE_OPTIONS})
target_link_libraries(SimpleController PRIVATE ${CURSES_LINK_OPTIONS})
# Setup common options
cu_setup_executable_options(SimpleController)
# Deploy and install target and its runtime dependencies (call this AFTER ALL dependencies have been added to the target)
cu_setup_deploy_runtime(SimpleController ${INSTALL_EXAMPLE_FLAG} ${SIGN_FLAG})
# Copy data
add_custom_command(
	TARGET SimpleController
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/../data/SimpleControllerModelV2.json ${CMAKE_BINARY_DIR}/examples/src/SimpleControllerModelV2.json
	COMMENT "Copying Examples data to output folder"
	VERBATIM
)

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
	# Add (optional) curses support
	target_compile_options(RawMessageSendC PRIVATE ${CURSES_COMPILE_OPTIONS})
	target_link_libraries(RawMessageSendC PRIVATE ${CURSES_LINK_OPTIONS})
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
	# Add (optional) curses support
	target_compile_options(Discovery PRIVATE ${CURSES_COMPILE_OPTIONS})
	target_link_libraries(Discovery PRIVATE ${CURSES_LINK_OPTIONS})
	# Setup common options
	cu_setup_executable_options(Discovery)
	# Deploy and install target and its runtime dependencies (call this AFTER ALL dependencies have been added to the target)
	cu_setup_deploy_runtime(Discovery ${INSTALL_EXAMPLE_FLAG} ${SIGN_FLAG})

	if(ENABLE_AVDECC_FEATURE_JSON)
		# EntityDumper
		add_executable(EntityDumper entityDumper.cpp utils.cpp utils.hpp)
		set_target_properties(EntityDumper PROPERTIES FOLDER "Examples/Controller")
		# Using controller library
		target_link_libraries(EntityDumper PRIVATE la_avdecc_controller_cxx)
		# Add (optional) curses support
		target_compile_options(EntityDumper PRIVATE ${CURSES_COMPILE_OPTIONS})
		target_link_libraries(EntityDumper PRIVATE ${CURSES_LINK_OPTIONS})
		# Setup common options
		cu_setup_executable_options(EntityDumper)
		# Deploy and install target and its runtime dependencies (call this AFTER ALL dependencies have been added to the target)
		cu_setup_deploy_runtime(EntityDumper ${INSTALL_EXAMPLE_FLAG} ${SIGN_FLAG})
	endif()

	if(BUILD_AVDECC_SWIG AND AVDECC_SWIG_LANGUAGES MATCHES "csharp")
		# Only include if the CMake generator is single-config
		if(CMAKE_CONFIGURATION_TYPES)
			message("CMake generator is multi-config, skipping C# example")
		else()
			# Network interfaces enumerator using C# bindings
			add_subdirectory(discoveryCSharp)
			# StreamFormatInfo using C# bindings
			add_subdirectory(streamFormatInfoCSharp)
		endif()
	endif()
endif()
