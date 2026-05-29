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
* @file utils.cpp
* @author Christophe Calmejane
*/

#include "utils.hpp"
#include <mutex>
#include <memory>
#include <vector>
#include <functional>
#include <fstream>
#include <cstdlib>
#include <iostream>

#if defined(USE_CURSES)
#	include <stdlib.h>
#	include <locale.h>
static WINDOW* s_Window = nullptr;
static SCREEN* s_Screen = nullptr;
#endif // USE_CURSES

// Optional log file mirror for outputText. Enabled by setting the AVDECC_EXAMPLE_LOG environment variable
// to a destination file path. This allows capturing the full log when the example runs under curses
// (where stdout is hijacked and the terminal cannot be scrolled).
static std::ofstream s_LogFile;

int getUserChoice()
{
#if defined(USE_CURSES)
	if (s_Window == nullptr)
		return 0;
	int c = wgetch(s_Window);
#else
	int c = std::cin.get();
#endif
	c -= '0';
	return c;
}

void initOutput()
{
	// Open the optional log file (mirror of outputText) if AVDECC_EXAMPLE_LOG is set.
	if (auto const* logPath = std::getenv("AVDECC_EXAMPLE_LOG"); logPath != nullptr && logPath[0] != '\0')
	{
		s_LogFile.open(logPath, std::ios::out | std::ios::trunc);
		if (s_LogFile.is_open())
		{
			// Inform the user on stderr (not captured by curses) where the log goes.
			std::cerr << "[avdecc-example] Mirroring outputText to: " << logPath << std::endl;
		}
		else
		{
			std::cerr << "[avdecc-example] Failed to open log file: " << logPath << std::endl;
		}
	}
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
	if (s_LogFile.is_open())
	{
		s_LogFile.flush();
		s_LogFile.close();
	}
}

void outputText(std::string const& str) noexcept
{
	try
	{
		static std::mutex mut;
		std::lock_guard<decltype(mut)> const lg(mut);

#if defined(USE_CURSES)
		wprintw(s_Window, "%s", str.c_str());
		wrefresh(s_Window);
#else // !USE_CURSES
		std::cout << str;
		std::flush(std::cout);
#endif // !USE_CURSES

		// Mirror to log file when enabled (perennial way to capture output under curses).
		if (s_LogFile.is_open())
		{
			s_LogFile << str;
			s_LogFile.flush();
		}
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
			if (/*intfc.type == la::networkInterface::Interface::Type::Ethernet && */ intfc.isConnected && !intfc.isVirtual)
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

la::networkInterface::Interface chooseSecondaryNetworkInterface(la::networkInterface::Interface const& primary)
{
	// Enumerate available interfaces, excluding the one already selected as primary.
	auto interfaces = std::vector<la::networkInterface::Interface>{};
	la::networkInterface::NetworkInterfaceHelper::getInstance().enumerateInterfaces(
		[&interfaces, &primary](la::networkInterface::Interface const& intfc)
		{
			if (intfc.isConnected && !intfc.isVirtual && intfc.id != primary.id)
			{
				interfaces.push_back(intfc);
			}
		});

	if (interfaces.empty())
	{
		outputText("No additional network interface available for redundancy.\n");
		return {};
	}

	outputText("Enable redundancy by selecting a SECONDARY interface (or 0 to skip):\n");
	outputText("0: Skip (single-interface mode)\n");
	auto intNum = 1u;
	for (auto const& intfc : interfaces)
	{
		outputText(std::to_string(intNum) + ": " + intfc.alias + " (" + intfc.description + ")\n");
		++intNum;
	}
	outputText("\n> ");

	auto index = -1;
	while (index == -1)
	{
		auto const c = getUserChoice();
		if (c == 0)
		{
			return {};
		}
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
		checkAndDisplayInterfaceType(avdecc_protocol_interface_type_macos_ncap);
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
