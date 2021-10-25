# Documentation CMake file

if(WIN32)
	set(LA_HHC_SUBSTITUTE_SCRIPT "${CU_ROOT_DIR}/scripts/hhcSubstitute.bat")
endif()

# Configure files based on CMakeLists.txt
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in"
	"${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
	@ONLY
)
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/footer.html.in"
	"${CMAKE_CURRENT_BINARY_DIR}/footer.html"
	@ONLY
)

# add a target to generate API documentation with Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
	add_custom_target(avdecclib_doc ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} COMMENT "Generating documentation with Doxygen" VERBATIM)
	set_target_properties(avdecclib_doc PROPERTIES FOLDER "Doc")
endif(DOXYGEN_FOUND)

if(WIN32)
	# Set installation rules
	if(INSTALL_AVDECC_DOC)
		install(FILES ${CMAKE_CURRENT_BINARY_DIR}/avdecc.chm CONFIGURATIONS Release DESTINATION doc)
	endif()
endif()
