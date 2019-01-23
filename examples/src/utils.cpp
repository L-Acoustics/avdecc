/*
* Copyright (C) 2016-2019, L-Acoustics and its contributors

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
* @file utils.cpp
* @author Christophe Calmejane
*/

#include "utils.hpp"
#include <mutex>

#if defined(USE_CURSES)
#	include <stdlib.h>
#	include <locale.h>
static WINDOW* s_Window = nullptr;
static SCREEN* s_Screen = nullptr;
#else // !USE_CURSES
#	include <iostream>
#endif // USE_CURSES


void initOutput()
{
#if defined(USE_CURSES)
	auto term = getenv("TERM");
	if (term == nullptr)
		s_Screen = newterm((char*)"xterm", stdout, stdin);
	else
		initscr();
	raw(); // Disable buffering
	nonl(); // We don't want new line chars
	timeout(-1);
	noecho(); // We don't want to echo what we type
	setlocale(LC_CTYPE, "");
	cbreak();
	s_Window = newwin(LINES, COLS, 0, 0);
	scrollok(s_Window, true);
	keypad(s_Window, true);
	wrefresh(s_Window);
#endif // USE_CURSES
}

void deinitOutput()
{
#if defined(USE_CURSES)
	if (s_Screen != nullptr)
		delscreen(s_Screen);
	else
		endwin();
#endif // USE_CURSES
}

void outputText(std::string const& str) noexcept
{
	try
	{
		static std::mutex mut;
		std::lock_guard<decltype(mut)> const lg(mut);

#ifdef _WIN32
		std::cout << str;
#else // !_WIN32
		wprintw(s_Window, str.c_str());
		wrefresh(s_Window);
#endif // _WIN32
	}
	catch (...)
	{
	}
}

la::avdecc::protocol::ProtocolInterface::Type chooseProtocolInterfaceType()
{
	auto protocolInterfaceType{ la::avdecc::protocol::ProtocolInterface::Type::None };

	// Get the list of supported protocol interface types, and ask the user to choose one (if many available)
	auto protocolInterfaceTypes = la::avdecc::protocol::ProtocolInterface::getSupportedProtocolInterfaceTypes();
	if (protocolInterfaceTypes.empty())
	{
		outputText(std::string("No protocol interface supported on this computer\n"));
		return protocolInterfaceType;
	}

	// Remove Virtual interface
	protocolInterfaceTypes.reset(la::avdecc::protocol::ProtocolInterface::Type::Virtual);

	if (protocolInterfaceTypes.count() == 1)
		protocolInterfaceType = protocolInterfaceTypes.at(0);
	else
	{
		outputText("Choose a protocol interface type:\n");
		unsigned int intNum = 1;
		for (auto const type : protocolInterfaceTypes)
		{
			outputText(std::to_string(intNum) + ": " + la::avdecc::protocol::ProtocolInterface::typeToString(type) + "\n");
			++intNum;
		}
		outputText("\n> ");

		// Get user's choice
		int index = -1;
		while (index == -1)
		{
			int c = getch() - '0';
			if (c >= 1 && c <= static_cast<int>(protocolInterfaceTypes.count()))
			{
				index = c - 1;
			}
		}
		protocolInterfaceType = protocolInterfaceTypes.at(index);
	}

	return protocolInterfaceType;
}

la::avdecc::networkInterface::Interface chooseNetworkInterface()
{
	// List of available interfaces
	std::vector<la::avdecc::networkInterface::Interface> interfaces;

	// Enumerate available interfaces
	la::avdecc::networkInterface::enumerateInterfaces(
		[&interfaces](la::avdecc::networkInterface::Interface const& intfc)
		{
			// Only select interfaces that is not loopback and has at least one IP address
			if (intfc.type != la::avdecc::networkInterface::Interface::Type::Loopback && !intfc.ipAddresses.empty() && intfc.isActive)
				interfaces.push_back(intfc);
		});

	if (interfaces.empty())
	{
		outputText(std::string("No valid network interface found on this computer\n"));
		return {};
	}

	// Let the user choose an interface
	outputText("Choose an interface:\n");
	unsigned int intNum = 1;
	for (auto const& intfc : interfaces)
	{
		outputText(std::to_string(intNum) + ": " + intfc.alias + " (" + intfc.description + ")\n");
		++intNum;
	}
	outputText("\n> ");

	// Get user's choice
	int index = -1;
	while (index == -1)
	{
		int c = getch() - '0';
		if (c >= 1 && c <= static_cast<int>(interfaces.size()))
		{
			index = c - 1;
		}
	}

	return interfaces[index];
}
