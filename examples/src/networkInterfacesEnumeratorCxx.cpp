/*
* Copyright (C) 2016-2021, L-Acoustics and its contributors

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
* @file networkInterfaceEnumeratorCxx.cpp
* @author Christophe Calmejane
* @brief Example enumerating all detected network interfaces on the local computer (using C++ Library).
*/

//#include <la/avdecc/utils.hpp>
#include <la/avdecc/networkInterfaceHelper.hpp>

#include <iostream>
#include <string>

std::ostream& operator<<(std::ostream& stream, la::avdecc::networkInterface::MacAddress const& macAddress)
{
	stream << la::avdecc::networkInterface::macAddressToString(macAddress);
	return stream;
}

std::ostream& operator<<(std::ostream& stream, la::avdecc::networkInterface::Interface::Type const type)
{
	switch (type)
	{
		case la::avdecc::networkInterface::Interface::Type::Loopback:
			stream << "Loopback";
			break;
		case la::avdecc::networkInterface::Interface::Type::Ethernet:
			stream << "Ethernet";
			break;
		case la::avdecc::networkInterface::Interface::Type::WiFi:
			stream << "WiFi";
			break;
		case la::avdecc::networkInterface::Interface::Type::AWDL:
			stream << "AWDL";
			break;
		default:
			break;
	}
	return stream;
}

int displayInterfaces()
{
	std::cout << "Available interfaces:\n\n";

	unsigned int intNum = 1;
	// Enumerate available interfaces
	la::avdecc::networkInterface::enumerateInterfaces(
		[&intNum](la::avdecc::networkInterface::Interface const& intfc) noexcept
		{
			try
			{
				std::cout << intNum << ": " << intfc.id << std::endl;
				std::cout << "  Description:  " << intfc.description << std::endl;
				std::cout << "  Alias:        " << intfc.alias << std::endl;
				std::cout << "  MacAddress:   " << intfc.macAddress << std::endl;
				std::cout << "  Type:         " << intfc.type << std::endl;
				std::cout << "  Enabled:      " << (intfc.isEnabled ? "YES" : "NO") << std::endl;
				std::cout << "  Connected:    " << (intfc.isConnected ? "YES" : "NO") << std::endl;
				std::cout << "  Virtual:      " << (intfc.isVirtual ? "YES" : "NO") << std::endl;
				if (!intfc.ipAddressInfos.empty())
				{
					std::cout << "  IP Addresses: " << std::endl;
					for (auto const& info : intfc.ipAddressInfos)
					{
						std::cout << "    " << static_cast<std::string>(info.address) << " (" << static_cast<std::string>(info.netmask) << ") -> " << static_cast<std::string>(info.getNetworkBaseAddress()) << " / " << static_cast<std::string>(info.getBroadcastAddress()) << std::endl;
					}
				}
				if (!intfc.gateways.empty())
				{
					std::cout << "  Gateways:     " << std::endl;
					for (auto const& ip : intfc.gateways)
					{
						std::cout << "    " << static_cast<std::string>(ip) << std::endl;
					}
				}
			}
			catch (std::exception const& e)
			{
				std::cout << "Got exception: " << e.what() << std::endl;
			}

			std::cout << std::endl;
			++intNum;
		});

	return 0;
}

int main()
{
	return displayInterfaces();
}
