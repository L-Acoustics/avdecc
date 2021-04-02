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
#include <linux/if_link.h>
#include <linux/if_packet.h>
#include <linux/wireless.h>
#include <netinet/in.h>
#include <linux/if.h>
#include <unordered_map>
#include <memory>
#include <string>
#include <cstring> // memcpy
#include <atomic>
#include <thread>
#include <chrono>

namespace la
{
namespace avdecc
{
namespace networkInterface
{
Interface::Type getInterfaceType(struct ifaddrs const* const ifa, int const sock)
{
	// Check for loopback
	if ((ifa->ifa_flags & IFF_LOOPBACK) != 0)
		return Interface::Type::Loopback;

	// Check for WiFi
	{
		struct iwreq wrq;
		memset(&wrq, 0, sizeof(wrq));
		strncpy(wrq.ifr_name, ifa->ifa_name, IFNAMSIZ - 1);
		if (ioctl(sock, SIOCGIWNAME, &wrq) != -1)
		{
			// TODO: Might not be 802.11 only, find a way to differenciate wireless protocols... maybe with  "wrq.u.name" with partial string match, but it may not be standard
			return Interface::Type::WiFi;
		}
	}

	// It's an Ethernet interface
	return Interface::Type::Ethernet;
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

		/* Per interface, we first receive a AF_PACKET then any number of AF_INET* (one per IP address) */
		int family = ifa->ifa_addr->sa_family;

		/* For an AF_PACKET, get the mac and setup the interface struct */
		if (family == AF_PACKET && ifa->ifa_data != nullptr)
		{
			la::avdecc::networkInterface::Interface interface;
			interface.id = ifa->ifa_name;
			interface.description = ifa->ifa_name;
			interface.alias = ifa->ifa_name;
			interface.type = getInterfaceType(ifa, sck);
			// Check if interface is enabled
			interface.isEnabled = (ifa->ifa_flags & IFF_UP) == IFF_UP;
			// Check if interface is connected
			interface.isConnected = (ifa->ifa_flags & (IFF_UP | IFF_RUNNING)) == (IFF_UP | IFF_RUNNING);
			// Is interface Virtual (TODO: Try to detect for other kinds)
			interface.isVirtual = interface.type == Interface::Type::Loopback;

			// Get the mac address contained in the AF_PACKET specific data
			auto sll = reinterpret_cast<struct sockaddr_ll*>(ifa->ifa_addr);
			if (sll->sll_halen == 6)
			{
				std::memcpy(interface.macAddress.data(), sll->sll_addr, 6);
			}
			interfaces[ifa->ifa_name] = interface;
		}
		/* For an AF_INET* interface address, get the IP */
		else if (family == AF_INET || family == AF_INET6)
		{
			if (family == AF_INET6) // Right now, we don't want ipv6 addresses
				continue;

			// Check if interface has been recorded from AF_PACKET
			auto intfcIt = interfaces.find(ifa->ifa_name);
			if (intfcIt != interfaces.end())
			{
				auto& interface = intfcIt->second;

				char host[NI_MAXHOST];
				auto ret = getnameinfo(ifa->ifa_addr, (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6), host, sizeof(host) - 1, nullptr, 0, NI_NUMERICHOST);
				if (ret != 0)
				{
					continue;
				}
				host[NI_MAXHOST - 1] = 0;

				char mask[NI_MAXHOST];
				ret = getnameinfo(ifa->ifa_netmask, (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6), mask, sizeof(mask) - 1, nullptr, 0, NI_NUMERICHOST);
				if (ret != 0)
				{
					continue;
				}
				mask[NI_MAXHOST - 1] = 0;

				// Add the IP address of that interface
				try
				{
					interface.ipAddressInfos.emplace_back(IPAddressInfo{ IPAddress{ host }, IPAddress{ mask } });
				}
				catch (...)
				{
				}
			}
		}
	}

	// Release the socket
	close(sck);
}

void waitForFirstEnumeration() noexcept
{
	static std::once_flag once;
	std::call_once(once,
		[]()
		{
			auto newList = Interfaces{};
			refreshInterfaces(newList);
			onNewInterfacesList(std::move(newList));
		});
}

static auto s_shouldTerminate = std::atomic_bool{ false };
static auto s_observerThread = std::thread{};

void onFirstObserverRegistered() noexcept
{
	s_shouldTerminate = false;

	s_observerThread = std::thread(
		[]()
		{
			utils::setCurrentThreadName("networkInterfaceHelper::ObserverPolling");
			auto previousList = Interfaces{};
			auto nextCheck = std::chrono::time_point<std::chrono::system_clock>{};
			while (!s_shouldTerminate)
			{
				auto const now = std::chrono::system_clock::now();
				if (now >= nextCheck)
				{
					auto newList = Interfaces{};
					refreshInterfaces(newList);

					// Process previous list and check if some property changed
					for (auto const& [name, previousIntfc] : previousList)
					{
						auto const newIntfcIt = newList.find(name);
						if (newIntfcIt != newList.end())
						{
							auto const& newIntfc = newIntfcIt->second;
							if (previousIntfc.isEnabled != newIntfc.isEnabled)
							{
								notifyEnabledStateChanged(newIntfc, newIntfc.isEnabled);
							}
							if (previousIntfc.isConnected != newIntfc.isConnected)
							{
								notifyConnectedStateChanged(newIntfc, newIntfc.isConnected);
							}
							if (previousIntfc.alias != newIntfc.alias)
							{
								notifyAliasChanged(newIntfc, newIntfc.alias);
							}
							if (previousIntfc.ipAddressInfos != newIntfc.ipAddressInfos)
							{
								notifyIPAddressInfosChanged(newIntfc, newIntfc.ipAddressInfos);
							}
							if (previousIntfc.gateways != newIntfc.gateways)
							{
								notifyGatewaysChanged(newIntfc, newIntfc.gateways);
							}
						}
					}

					// Copy the list before it's moved
					previousList = newList;

					// Check for change in Interface count
					onNewInterfacesList(std::move(newList));

					// Setup next check time
					nextCheck = now + std::chrono::milliseconds(1000);
				}

				// Wait a little bit so we don't burn the CPU
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		});
}

void onLastObserverUnregistered() noexcept
{
	s_shouldTerminate = true;
	if (s_observerThread.joinable())
	{
		s_observerThread.join();
		s_observerThread = {};
	}
}

} // namespace networkInterface
} // namespace avdecc
} // namespace la
