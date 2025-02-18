# Override the default warning flags.
cu_set_warning_flags(TARGETS la_avdecc_cxx la_avdecc_static la_avdecc_controller_cxx la_avdecc_controller_static COMPILER CLANG PRIVATE -Wno-unused-parameter PUBLIC -Wno-gnu-zero-variadic-macro-arguments)
cu_set_warning_flags(TARGETS la_avdecc_cxx la_avdecc_static la_avdecc_controller_cxx la_avdecc_controller_static Tests COMPILER GCC PRIVATE -Wno-ignored-qualifiers)
