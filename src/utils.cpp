/*
* Copyright (C) 2016-2017, L-Acoustics and its contributors

* This file is part of LA_avdecc.

* LA_avdecc is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.

* LA_avdecc is distributed in the hope that it will be usefu_state,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.

* You should have received a copy of the GNU Lesser General Public License
* along with LA_avdecc.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
* @file utils.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/utils.hpp"
#if defined(_WIN32)
#include <Windows.h>
#endif // _WIN32
#if defined(__APPLE__)
#include <pthread.h>
#endif // __APPLE__
#if defined(__unix__)
#include <pthread.h>
#endif // __unix__
#include <iostream>
#include <cstdio>

namespace la
{
namespace avdecc
{

bool LA_AVDECC_CALL_CONVENTION setCurrentThreadName(std::string const& name)
{
#if defined(_WIN32)
	struct
	{
		DWORD dwType;
		LPCSTR szName;
		DWORD dwThreadID;
		DWORD dwFlags;
	} info;

	info.dwType = 0x1000;
	info.szName = name.c_str();
	info.dwThreadID = GetCurrentThreadId();
	info.dwFlags = 0;

	__try
	{
		RaiseException(0x406d1388 /*MS_VC_EXCEPTION*/, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
	}
	__except (EXCEPTION_CONTINUE_EXECUTION)
	{
	}
	return true;

#elif defined(__APPLE__)
	pthread_setname_np(name.c_str());
	return true;

#elif defined(__unix__) && !defined(__CYGWIN__)
#if (__GLIBC__ * 1000 + __GLIBC_MINOR__) >= 2012
	pthread_setname_np(pthread_self(), name.c_str());
	return true;
#else // !GLIBC >= 2012
	prctl(PR_SET_NAME, name.c_str(), 0, 0, 0);
	return true;
#endif

#else
	(void)name;
	return false;
#endif // !WIN32 && ! __APPLE__ && ! __unix__
}

static bool s_enableAssert = true;

void LA_AVDECC_CALL_CONVENTION enableAssert() noexcept
{
	s_enableAssert = true;
}

void LA_AVDECC_CALL_CONVENTION disableAssert() noexcept
{
	s_enableAssert = false;
}

bool LA_AVDECC_CALL_CONVENTION isAssertEnabled() noexcept
{
	return s_enableAssert;
}

void LA_AVDECC_CALL_CONVENTION displayAssertDialog(char const* const file, unsigned const line, char const* const message, va_list arg) noexcept
{
	bool shouldBreak{ true };
	bool shouldAbort{ true };
	try
	{
		char buffer[2048];
		auto offset = std::snprintf(buffer, sizeof(buffer), "Debug Assertion Failed!\n\nFile: %s\nLine: %u\n\n\t", file, line);
		if (offset < sizeof(buffer) - 1)
		{
			offset += std::vsnprintf(buffer + offset, sizeof(buffer) - offset, message, arg);
			buffer[sizeof(buffer) - 1] = 0; // Contrary to std::snprintf, std::vsnprintf does not add \0 if there is not enough room

			std::cerr << buffer << std::endl;

			if (offset < sizeof(buffer) - 1)
			{
				offset += std::snprintf(buffer + offset, sizeof(buffer) - offset,
																"\n"
																"\n"
																"Press 'Abort' to abort immediately\n"
																"Press 'Retry' to debug the program\n"
																"Press 'Ignore' to try to continue normal execution\n");
			}
		}
#ifdef _WIN32
		if (IsDebuggerPresent())
		{
			shouldBreak = true; // Always call DebugBreak if debugger is attached
			shouldAbort = false; // Always try to continue if debugger is attached
		}
		else
		{
			auto const value = MessageBox(nullptr, buffer, "Assert", MB_ABORTRETRYIGNORE | MB_ICONERROR);
			shouldBreak = (value == IDRETRY);
			shouldAbort = (value == IDABORT);
		}
#endif // _WIN32
	}
	catch (...)
	{
		std::cerr << "Assert at " << file << ":" << std::to_string(line) << " -> " << message << " (WARNING: Exception during va_arg expansion)" << std::endl;
	}
	if (shouldBreak)
	{
#ifdef _WIN32
		DebugBreak();
#endif
	}
	if (shouldAbort)
	{
		std::abort();
	}
}

} // namespace avdecc
} // namespace la
