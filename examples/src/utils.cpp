/*
* Copyright (C) 2016-2022, L-Acoustics and its contributors

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
#include <memory>
#include <vector>
#include <functional>

#if defined(USE_CURSES)
#	include <stdlib.h>
#	include <locale.h>
static WINDOW* s_Window = nullptr;
static SCREEN* s_Screen = nullptr;
#else // !USE_CURSES
#	include <iostream>
#endif // USE_CURSES

int getUserChoice()
{
#if defined(USE_CURSES)
	if (s_Window == nullptr)
		return 0;
	int c = wgetch(s_Window);
#else
	int c = getch();
#endif
	c -= '0';
	return c;
}

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
		std::flush(std::cout);
#else // !_WIN32
		wprintw(s_Window, str.c_str());
		wrefresh(s_Window);
#endif // _WIN32
	}
	catch (...)
	{
	}
}

la::networkInterface::Interface chooseNetworkInterface()
{
	// List of available interfaces
	std::vector<la::networkInterface::Interface> interfaces;

	// Enumerate available interfaces
	la::networkInterface::NetworkInterfaceHelper::getInstance().enumerateInterfaces(
		[&interfaces](la::networkInterface::Interface const& intfc)
		{
			// Only select connected, non virtual, ethernet interfaces
			if (intfc.type == la::networkInterface::Interface::Type::Ethernet && intfc.isConnected && !intfc.isVirtual)
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
		auto c = getUserChoice();
		if (c >= 1 && c <= static_cast<int>(interfaces.size()))
		{
			index = c - 1;
		}
	}

	return interfaces[index];
}

#ifdef USE_BINDINGS_C
template<typename ValueType>
constexpr size_t countBits(ValueType const value) noexcept
{
	return (value == 0u) ? 0u : 1u + countBits(value & (value - 1u));
}

avdecc_protocol_interface_type_t chooseProtocolInterfaceType()
{
	avdecc_protocol_interface_type_t protocolInterfaceType{ avdecc_protocol_interface_type_none };

	// Get the list of supported protocol interface types, and ask the user to choose one (if many available)
	auto protocolInterfaceTypes = LA_AVDECC_ProtocolInterface_getSupportedProtocolInterfaceTypes();
	if (protocolInterfaceTypes == avdecc_protocol_interface_type_none)
	{
		outputText(std::string("No protocol interface supported on this computer\n"));
		return protocolInterfaceType;
	}

	// Remove Virtual interface
	protocolInterfaceTypes &= ~avdecc_protocol_interface_type_virtual;

	if (countBits(protocolInterfaceTypes) == 1)
		protocolInterfaceType = protocolInterfaceTypes;
	else
	{
		outputText("Choose a protocol interface type:\n");

		std::vector<avdecc_protocol_interface_type_t> proposedInterfaces{};

		auto const checkAndDisplayInterfaceType = [protocolInterfaceTypes, &proposedInterfaces](avdecc_protocol_interface_type_t const interfaceType)
		{
			if ((protocolInterfaceTypes & interfaceType) == interfaceType)
			{
				proposedInterfaces.push_back(interfaceType);
				outputText(std::to_string(proposedInterfaces.size()) + ": " + LA_AVDECC_ProtocolInterface_typeToString(interfaceType) + "\n");
			}
		};

		checkAndDisplayInterfaceType(avdecc_protocol_interface_type_pcap);
		checkAndDisplayInterfaceType(avdecc_protocol_interface_type_macos_native);
		checkAndDisplayInterfaceType(avdecc_protocol_interface_type_proxy);

		outputText("\n> ");

		// Get user's choice
		int index = -1;
		while (index == -1)
		{
			int c = getch() - '0';
			if (c >= 1 && c <= static_cast<int>(proposedInterfaces.size()))
			{
				index = c - 1;
			}
		}
		protocolInterfaceType = proposedInterfaces[index];
	}

	return protocolInterfaceType;
}

#else // !USE_BINDINGS_C

la::avdecc::protocol::ProtocolInterface::Type chooseProtocolInterfaceType(la::avdecc::protocol::ProtocolInterface::SupportedProtocolInterfaceTypes const& allowedTypes)
{
	auto protocolInterfaceType{ la::avdecc::protocol::ProtocolInterface::Type::None };

	// Get the list of supported protocol interface types, and ask the user to choose one (if many available)
	auto const protocolInterfaceTypes = la::avdecc::protocol::ProtocolInterface::getSupportedProtocolInterfaceTypes() & allowedTypes;
	if (protocolInterfaceTypes.empty())
	{
		outputText(std::string("No protocol interface supported on this computer\n"));
		return protocolInterfaceType;
	}

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
			auto c = getUserChoice();
			if (c >= 1 && c <= static_cast<int>(protocolInterfaceTypes.count()))
			{
				index = c - 1;
			}
		}
		protocolInterfaceType = protocolInterfaceTypes.at(index);
	}

	return protocolInterfaceType;
}

#endif // USE_BINDINGS_C
