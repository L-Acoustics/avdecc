/*
* Copyright (C) 2016-2019, L-Acoustics and its contributors

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
* @file networkInterfaceHelper_win32.cpp
* @author Christophe Calmejane
*/

#include "networkInterfaceHelper_common.hpp"

#include <memory>
#include <cstdint> // std::uint8_t
#include <cstring> // memcpy
#include <WinSock2.h>
#include <Iphlpapi.h>
#pragma comment(lib, "KERNEL32.lib")
#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "IPHLPAPI.lib")

namespace la
{
namespace avdecc
{
namespace networkInterface
{
std::string getStringFromWide(PWCHAR wide)
{
	char multiByte[1024];
	if (WideCharToMultiByte(CP_ACP, 0, wide, -1, multiByte, sizeof(multiByte), NULL, NULL) == 0)
		return std::string();
	return std::string(multiByte);
}

Interface::Type getInterfaceType(IFTYPE const ifType)
{
	switch (ifType)
	{
		case IF_TYPE_ETHERNET_CSMACD:
			return Interface::Type::Ethernet;
		case IF_TYPE_SOFTWARE_LOOPBACK:
			return Interface::Type::Loopback;
		case IF_TYPE_IEEE80211:
			return Interface::Type::WiFi;
		default:
			break;
	}

	// Not supported type
	return Interface::Type::None;
}

void refreshInterfaces(Interfaces& interfaces) noexcept
{
	ULONG ulSize = 0;
	ULONG family = AF_INET; // AF_UNSPEC for ipV4 and ipV6, AF_INET6 for ipV6 only

	GetAdaptersAddresses(family, GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_INCLUDE_GATEWAYS, nullptr, nullptr, &ulSize); // Make an initial call to get the needed size to allocate
	auto buffer = std::make_unique<std::uint8_t[]>(ulSize);
	if (GetAdaptersAddresses(family, GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_INCLUDE_GATEWAYS, nullptr, (PIP_ADAPTER_ADDRESSES)buffer.get(), &ulSize) != ERROR_SUCCESS)
	{
		return;
	}

	for (auto adapter = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.get()); adapter != nullptr; adapter = adapter->Next)
	{
		auto const type = getInterfaceType(adapter->IfType);
		// Only process supported interface types
		if (type != Interface::Type::None)
		{
			Interface i;
			i.name = std::string("\\Device\\NPF_") + adapter->AdapterName;
			i.description = getStringFromWide(adapter->Description);
			i.alias = getStringFromWide(adapter->FriendlyName);
			if (adapter->PhysicalAddressLength == i.macAddress.size())
				std::memcpy(i.macAddress.data(), adapter->PhysicalAddress, adapter->PhysicalAddressLength);
			i.type = type;
			i.isActive = adapter->OperStatus == IfOperStatusUp;

			for (auto ua = adapter->FirstUnicastAddress; ua != nullptr; ua = ua->Next)
			{
				//bool isIPv6 = ua->Address.lpSockaddr->sa_family == AF_INET6;
				auto ip = std::string(inet_ntoa(reinterpret_cast<struct sockaddr_in*>(ua->Address.lpSockaddr)->sin_addr));
				i.ipAddresses.push_back(ip);
			}

			for (auto ga = adapter->FirstGatewayAddress; ga != nullptr; ga = ga->Next)
			{
				//bool isIPv6 = ua->Address.lpSockaddr->sa_family == AF_INET6;
				auto ip = std::string(inet_ntoa(reinterpret_cast<struct sockaddr_in*>(ga->Address.lpSockaddr)->sin_addr));
				i.gateways.push_back(ip);
			}

			interfaces[i.name] = i;
		}
	}
}

} // namespace networkInterface
} // namespace avdecc
} // namespace la
