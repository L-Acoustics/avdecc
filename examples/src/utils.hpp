/*
* Copyright (C) 2016-2018, L-Acoustics and its contributors

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
* @file utils.hpp
* @author Christophe Calmejane
*/

#pragma once

/** ************************************************************************ **/
/** UTILS METHODS TO HANDLE CURSES/NON-CURSES I/O FOR CONSOLE SAMPLES        **/
/** ************************************************************************ **/

#if (defined(__APPLE__) || defined(__linux__)) && !defined(USE_CURSES)
#	define USE_CURSES
#endif // __APPLE__

#include <la/avdecc/avdecc.hpp>
#include <la/avdecc/networkInterfaceHelper.hpp>
#include <string>

#if defined(USE_CURSES)
#	include <curses.h>
#else // !USE_CURSES
#	include <conio.h>
#	ifdef _WIN32
#		define getch _getch
#	endif // _WIN32
#endif // USE_CURSES

#define VENDOR_ID 0x001B92
#define DEVICE_ID 0x80
#define MODEL_ID 0x00000001

void initOutput();
void deinitOutput();
void outputText(std::string const& str) noexcept;
la::avdecc::protocol::ProtocolInterface::Type chooseProtocolInterfaceType();
la::avdecc::networkInterface::Interface chooseNetworkInterface();
