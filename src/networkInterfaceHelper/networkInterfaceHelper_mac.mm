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
#include <mutex> // once
#include <cstring> // memcpy

#import <SystemConfiguration/SystemConfiguration.h>
#import <Foundation/Foundation.h>

#define DYNAMIC_STORE_NETWORK_STATE_STRING @"State:/Network/Interface/"
#define DYNAMIC_STORE_LINK_STRING @"/Link"

namespace la
{
namespace avdecc
{
namespace networkInterface
{
Interface::Type getInterfaceType(struct ifaddrs const* const ifa, int const ifm_options)
{
	// Check for AWDL
	if (strncmp(ifa->ifa_name, "awdl", 4) == 0)
		return Interface::Type::AWDL;

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

void interfaceLinkStatusChanged(SCDynamicStoreRef store, CFArrayRef changedKeys, void* ctx)
{
	auto const count = CFArrayGetCount(changedKeys);

	for (auto index = CFIndex{ 0 }; index < count; ++index)
	{
		auto const* const keyRef = static_cast<CFStringRef>(CFArrayGetValueAtIndex(changedKeys, index));
		auto const* const key = (__bridge NSString const*)keyRef;

		if ([key hasPrefix:DYNAMIC_STORE_NETWORK_STATE_STRING])
		{
			auto const prefixLength = [DYNAMIC_STORE_NETWORK_STATE_STRING length];
			auto const suffixLength = [DYNAMIC_STORE_LINK_STRING length];
			auto* interfaceName = [key substringWithRange:NSMakeRange(prefixLength, [key length] - prefixLength - suffixLength)];
			auto const* const valueRef = SCDynamicStoreCopyValue(store, keyRef);
			if (valueRef)
			{
				auto const isConnected = [(NSString*)[(__bridge NSDictionary const*)valueRef valueForKey:@"Active"] boolValue];
				onConnectedStateChanged(std::string{ [interfaceName UTF8String] }, isConnected);
				CFRelease(valueRef);
			}
		}
	}
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
			interface.id = ifa->ifa_name;
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
					// Declared macOS interfaces are always enabled
					interface.isEnabled = true;
					// Check if interface is connected
					interface.isConnected = (ifmr.ifm_status & IFM_ACTIVE) != 0;
					// Is interface Virtual (TODO: Try to detect for other kinds)
					interface.isVirtual = interface.type == Interface::Type::Loopback;
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
				interface.ipAddressInfos.emplace_back(IPAddressInfo{ IPAddress{ host }, IPAddress{ mask } });
			}
		}
	}

	// Release the socket
	close(sck);

	// If we have a recent version of macOS, get the Interface localized display name
	if (@available(macOS 10.14, *))
	{
		// Get alias name for registered interfaces (using macOS native API)
		auto const* const interfacesArrayRef = SCNetworkInterfaceCopyAll();

		if (interfacesArrayRef)
		{
			auto const count = CFArrayGetCount(interfacesArrayRef);

			for (auto index = CFIndex{ 0 }; index < count; ++index)
			{
				auto const* const interfaceRef = static_cast<SCNetworkInterfaceRef>(CFArrayGetValueAtIndex(interfacesArrayRef, index));

				auto const* const bsdName = static_cast<CFStringRef>(SCNetworkInterfaceGetBSDName(interfaceRef));
				if (!bsdName)
				{
					continue;
				}

				// Only process interfaces that has been recorded from AF_LINK
				auto const intName = [(__bridge NSString*)bsdName UTF8String];
				auto const intfcIt = interfaces.find(intName);
				if (intfcIt == interfaces.end())
				{
					continue;
				}
				auto& interface = intfcIt->second;

				// Get alias/description
				auto const* const localizedRef = static_cast<CFStringRef>(SCNetworkInterfaceGetLocalizedDisplayName(interfaceRef));
				if (localizedRef)
				{
					interface.description = [(__bridge NSString*)localizedRef UTF8String];
					interface.alias = interface.description + " (" + interface.id + ")";
				}
			}
			CFRelease(interfacesArrayRef);
		}
	}
}

void waitForFirstEnumeration() noexcept
{
	// Always force a refresh
	auto newList = Interfaces{};
	refreshInterfaces(newList);
	onNewInterfacesList(std::move(newList));
}

static IONotificationPortRef s_notificationPort = nullptr;
static io_iterator_t s_controllerMatchIterator = 0;
static io_iterator_t s_controllerTerminateIterator = 0;
static CFRunLoopSourceRef s_runLoopRef = nullptr;

static void clearIterator(io_iterator_t iterator)
{
	@autoreleasepool
	{
		io_service_t service;
		while ((service = IOIteratorNext(iterator)))
		{
			IOObjectRelease(service);
		}
	}
}

static void onIONetworkControllerListChanged(void* refcon, io_iterator_t iterator)
{
	auto newList = Interfaces{};
	refreshInterfaces(newList);
	onNewInterfacesList(std::move(newList));
	clearIterator(iterator);
}

void onFirstObserverRegistered() noexcept
{
	// Register for Added/Removed interfaces notification
	{
		mach_port_t masterPort = 0;
		IOMasterPort(mach_task_self(), &masterPort);
		s_notificationPort = IONotificationPortCreate(masterPort);
		IONotificationPortSetDispatchQueue(s_notificationPort, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0));

		IOServiceAddMatchingNotification(s_notificationPort, kIOMatchedNotification, IOServiceMatching("IONetworkController"), onIONetworkControllerListChanged, nullptr, &s_controllerMatchIterator);
		IOServiceAddMatchingNotification(s_notificationPort, kIOTerminatedNotification, IOServiceMatching("IONetworkController"), onIONetworkControllerListChanged, nullptr, &s_controllerTerminateIterator);

		// Clear the iterators discarding already discovered adapters, we'll manually list them anyway
		clearIterator(s_controllerMatchIterator);
		clearIterator(s_controllerTerminateIterator);
	}

	// Register for State change notification on all interfaces
	{
		auto* scKeys = [[NSMutableArray alloc] init];
#if !__has_feature(objc_arc)
		[scKeys autorelease];
#endif
		[scKeys addObject:DYNAMIC_STORE_NETWORK_STATE_STRING @"[^\\]+" DYNAMIC_STORE_LINK_STRING];

		/* Connect to the dynamic store */
		auto ctx = SCDynamicStoreContext{ 0, NULL, NULL, NULL, NULL };
		auto const store = SCDynamicStoreCreate(nullptr, CFSTR("networkInterfaceHelper"), interfaceLinkStatusChanged, &ctx);

		/* Start monitoring */
		if (SCDynamicStoreSetNotificationKeys(store, nullptr, (__bridge CFArrayRef)scKeys))
		{
			s_runLoopRef = SCDynamicStoreCreateRunLoopSource(kCFAllocatorDefault, store, 0);
			CFRunLoopAddSource(CFRunLoopGetCurrent(), s_runLoopRef, kCFRunLoopCommonModes);
		}
	}
}

void onLastObserverUnregistered() noexcept
{
	//Clean up the IOKit code
	if (s_controllerTerminateIterator)
	{
		IOObjectRelease(s_controllerTerminateIterator);
	}

	if (s_controllerMatchIterator)
	{
		IOObjectRelease(s_controllerMatchIterator);
	}

	if (s_notificationPort)
	{
		IONotificationPortDestroy(s_notificationPort);
	}

	// Clean up run loop
	if (s_runLoopRef)
	{
		CFRunLoopSourceInvalidate(s_runLoopRef);
		CFRelease(s_runLoopRef);
		s_runLoopRef = nullptr;
	}
}

} // namespace networkInterface
} // namespace avdecc
} // namespace la
