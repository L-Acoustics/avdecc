/*
* Copyright (C) 2016-2026, L-Acoustics and its contributors

* This file is part of LA_avdecc.

* LA_avdecc is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.

* LA_avdecc is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.

* You should have received a copy of the GNU Lesser General Public License
* along with LA_avdecc.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
* @file avdecc/internals/exports.h
* @author Christophe Calmejane
* @brief OS specific defines for importing and exporting dynamic symbols.
*/

#pragma once

#ifdef __cplusplus
#	define LA_AVDECC_CPP_EXPORT extern "C"
#else // !__cplusplus
#	define LA_AVDECC_CPP_EXPORT
#endif // __cplusplus

#ifdef _WIN32

#	define LA_AVDECC_BINDINGS_C_CALL_CONVENTION __stdcall

#	if defined(la_avdecc_c_EXPORTS)
#		define LA_AVDECC_BINDINGS_C_API LA_AVDECC_CPP_EXPORT __declspec(dllexport)
#	elif defined(la_avdecc_c_STATICS)
#		define LA_AVDECC_BINDINGS_C_API LA_AVDECC_CPP_EXPORT
#	else // !la_avdecc_c_EXPORTS
#		define LA_AVDECC_BINDINGS_C_API LA_AVDECC_CPP_EXPORT __declspec(dllimport)
#	endif // la_avdecc_c_EXPORTS

#else // !_WIN32

#	define LA_AVDECC_BINDINGS_C_CALL_CONVENTION

#	if defined(la_avdecc_c_EXPORTS)
#		define LA_AVDECC_BINDINGS_C_API LA_AVDECC_CPP_EXPORT __attribute__((visibility("default")))
#	elif defined(la_avdecc_c_STATICS)
#		define LA_AVDECC_BINDINGS_C_API LA_AVDECC_CPP_EXPORT
#	else // !la_avdecc_c_EXPORTS
#		define LA_AVDECC_BINDINGS_C_API LA_AVDECC_CPP_EXPORT __attribute__((visibility("default")))
#	endif // la_avdecc_c_EXPORTS

#endif // _WIN32
