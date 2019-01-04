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

#include "networkInterfaceHelper_common.hpp"

#ifndef _GNU_SOURCE
#	define _GNU_SOURCE /* To get defns of NI_MAXSERV and NI_MAXHOST */
#endif // !_GNU_SOURCE
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/if_media.h>
#include <errno.h>
#include <unordered_map>
#include <memory>
#include <string>
#include <cstring> // memcpy

namespace la
{
namespace avdecc
{
namespace networkInterface
{
Interface::Type getInterfaceType(struct ifaddrs const* const ifa, int const ifm_options)
{
	// Check for loopback
	if ((ifa->ifa_flags & IFF_LOOPBACK) != 0)
		return Interface::Type::Loopback;

	// Check for WiFi
	if (ifm_options & IFM_IEEE80211)
		return Interface::Type::WiFi;

	// Check for Ethernet
	if (ifm_options & IFM_ETHER)
		return Interface::Type::Ethernet;

	// Not supported interface type
	return Interface::Type::None;
}

void refreshInterfaces(Interfaces& interfaces) noexcept
{
	std::unique_ptr<struct ifaddrs, std::function<void(struct ifaddrs*)>> scopedIfa{ nullptr, [](struct ifaddrs* ptr)
		{
			if (ptr != nullptr)
				freeifaddrs(ptr);
		} };

	struct ifaddrs* ifaddr{ nullptr };
	if (getifaddrs(&ifaddr) == -1)
	{
		return;
	}
	scopedIfa.reset(ifaddr);

	// We need a socket handle for ioctl calls
	int sck = socket(AF_INET, SOCK_DGRAM, 0);
	if (sck < 0)
	{
		return;
	}

	/* Walk through linked list, maintaining head pointer so we can free list later */
	for (auto ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
	{
		// Exclude ifaddr without addr field
		if (ifa->ifa_addr == nullptr)
			continue;

		/* Per interface, we first receive a AF_LINK then any number of AF_INET* (one per IP address) */
		int family = ifa->ifa_addr->sa_family;

		/* For AF_LINK, get the mac and setup the interface struct */
		if (family == AF_LINK)
		{
			la::avdecc::networkInterface::Interface interface;
			interface.name = ifa->ifa_name;
			interface.description = ifa->ifa_name;
			interface.alias = ifa->ifa_name;
			// Get media information
			struct ifmediareq ifmr;
			memset(&ifmr, 0, sizeof(ifmr));
			strncpy(ifmr.ifm_name, ifa->ifa_name, sizeof(ifmr.ifm_name));
			if (ioctl(sck, SIOCGIFMEDIA, &ifmr) != -1)
			{
				// Get media type
				interface.type = getInterfaceType(ifa, ifmr.ifm_current);
				if (interface.type != Interface::Type::None)
				{
					// Check if interface is active
					interface.isActive = (ifmr.ifm_status & IFM_ACTIVE) != 0;
					// Get the mac address contained in the AF_LINK specific data
					auto sdl = reinterpret_cast<struct sockaddr_dl*>(ifa->ifa_addr);
					if (sdl->sdl_alen == 6)
					{
						auto ptr = reinterpret_cast<unsigned char*>(LLADDR(sdl));
						std::memcpy(interface.macAddress.data(), ptr, 6);
					}
					// Add the interface to the list
					interfaces[ifa->ifa_name] = interface;
				}
			}
		}
		/* For an AF_INET* interface address, get the IP */
		else if (family == AF_INET || family == AF_INET6)
		{
			if (family == AF_INET6) // Right now, we don't want ipv6 addresses
				continue;

			// Check if interface has been recorded from AF_LINK
			auto intfcIt = interfaces.find(ifa->ifa_name);
			if (intfcIt != interfaces.end())
			{
				auto& interface = intfcIt->second;

				char host[NI_MAXHOST];
				int s = getnameinfo(ifa->ifa_addr, (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6), host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST);
				if (s != 0)
				{
					continue;
				}

				// Add the IP address of that interface
				interface.ipAddresses.push_back(host);
			}
		}
	}

	// Release the socket
	close(sck);
}

} // namespace networkInterface
} // namespace avdecc
} // namespace la
