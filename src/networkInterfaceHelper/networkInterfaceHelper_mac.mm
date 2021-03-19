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
* @file networkInterfaceHelper_mac.mm
* @author Christophe Calmejane
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
#include <mutex> // once, mutex
#include <cstring> // memcpy
#include <thread>

#import <SystemConfiguration/SystemConfiguration.h>
#import <Foundation/Foundation.h>

// https://developer.apple.com/library/archive/documentation/Networking/Conceptual/SystemConfigFrameworks/
// Command line tool: scutil
#define DYNAMIC_STORE_NETWORK_STATE_STRING @"State:/Network/Interface/"
#define DYNAMIC_STORE_NETWORK_SERVICE_STRING @"Setup:/Network/Service/"
#define DYNAMIC_STORE_LINK_STRING @"/Link"
#define DYNAMIC_STORE_IPV4_STRING @"/IPv4"
#define DYNAMIC_STORE_INTERFACE_STRING @"/Interface"

namespace la
{
namespace avdecc
{
namespace networkInterface
{
struct GlobalInformation
{
	IONotificationPortRef notificationPort{ nullptr };
	io_iterator_t controllerMatchIterator{ 0 };
	io_iterator_t controllerTerminateIterator{ 0 };
	SCDynamicStoreRef storeRef{ nullptr };
	std::thread thread{};
	CFRunLoopRef threadRunLoopRef{ nullptr };
	std::mutex lock{};
	std::unordered_map<std::string, std::string> interfaceToServiceMapping{};
};

static auto s_global = GlobalInformation{};

/** std::string to NSString conversion */
static NSString* getNSString(std::string const& cString)
{
	return [NSString stringWithCString:cString.c_str() encoding:NSUTF8StringEncoding];
}

/** NSString to std::string conversion */
static std::string getStdString(NSString const* const nsString) noexcept
{
	return std::string{ [nsString UTF8String] };
}

static void clearInterfaceToServiceMapping()
{
	auto const lg = std::lock_guard{ s_global.lock };

	s_global.interfaceToServiceMapping.clear();
}

static void setInterfaceToServiceMapping(NSString const* const interfaceName, NSString const* const serviceID)
{
	auto const lg = std::lock_guard{ s_global.lock };

	s_global.interfaceToServiceMapping[getStdString(interfaceName)] = getStdString(serviceID);
}

static NSString* getServiceForInterface(NSString const* const interfaceName)
{
	auto const lg = std::lock_guard{ s_global.lock };

	if (auto const serviceIt = s_global.interfaceToServiceMapping.find(getStdString(interfaceName)); serviceIt != s_global.interfaceToServiceMapping.end())
	{
		return getNSString(serviceIt->second);
	}

	return nullptr;
}

static NSString* getInterfaceForService(NSString const* const serviceID)
{
	auto const lg = std::lock_guard{ s_global.lock };

	auto const std_serviceID = getStdString(serviceID);
	for (auto const& [interfaceName, servID] : s_global.interfaceToServiceMapping)
	{
		if (servID == std_serviceID)
		{
			return getNSString(interfaceName);
		}
	}

	return nullptr;
}

static bool getIsVirtualInterface(NSString* deviceName, Interface::Type const type) noexcept
{
	if (type == Interface::Type::Loopback)
	{
		return true;
	}

	if ([deviceName hasPrefix:@"bridge"])
	{
		return true;
	}

	return false;
}

static Interface::Type getInterfaceType(NSString* hardware) noexcept
{
	if ([hardware isEqualToString:@"AirPort"])
	{
		return Interface::Type::WiFi;
	}

	// Check for Ethernet
	if ([hardware isEqualToString:@"Ethernet"])
	{
		return Interface::Type::Ethernet;
	}

	// Not supported interface type
	return Interface::Type::None;
}

static bool isInterfaceConnected(CFStringRef const linkStateKeyRef) noexcept
{
	auto isConnected = false;

	auto const* const valueRef = SCDynamicStoreCopyValue(s_global.storeRef, linkStateKeyRef);
	if (valueRef)
	{
		isConnected = [(NSString*)[(__bridge NSDictionary const*)valueRef valueForKey:@"Active"] boolValue];
		CFRelease(valueRef);
	}

	return isConnected;
}

static Interface::IPAddressInfos getIPAddressInfoFromKey(CFStringRef const ipKeyRef) noexcept
{
	auto ipAddressInfos = Interface::IPAddressInfos{};

	auto const* const valueRef = SCDynamicStoreCopyValue(s_global.storeRef, ipKeyRef);

	// Still have IP addresses
	if (valueRef)
	{
		auto* const addresses = (NSArray*)[(__bridge NSDictionary const*)valueRef valueForKey:@"Addresses"];
		auto* const netmasks = (NSArray*)[(__bridge NSDictionary const*)valueRef valueForKey:@"SubnetMasks"];
		// Only parse if we have the same count of addresses and netmasks, or no masks at all
		// (No netmasks is allowed in DHCP with manual IP)
		if (netmasks == nullptr || [addresses count] == [netmasks count])
		{
			for (auto ipIndex = NSUInteger{ 0u }; ipIndex < [addresses count]; ++ipIndex)
			{
				auto const* const address = (NSString*)[addresses objectAtIndex:ipIndex];
				auto const* netmask = @"255.255.255.255";
				if (netmasks)
				{
					auto const* mask = (NSString*)[netmasks objectAtIndex:ipIndex];
					// Empty netmask is allowed in manual IP
					if (mask.length > 0)
					{
						netmask = mask;
					}
				}
				try
				{
					ipAddressInfos.push_back(IPAddressInfo{ IPAddress{ std::string{ [address UTF8String] } }, IPAddress{ std::string{ [netmask UTF8String] } } });
				}
				catch (...)
				{
				}
			}
		}

		CFRelease(valueRef);
	}

	return ipAddressInfos;
}

static Interface::IPAddressInfos getIPAddressInfo(NSString const* const interfaceName) noexcept
{
	auto ipInfos = Interface::IPAddressInfos{};

	auto const ipKey = [NSString stringWithFormat:DYNAMIC_STORE_NETWORK_STATE_STRING @"%@" DYNAMIC_STORE_IPV4_STRING, interfaceName];
	auto const* const ipKeyRef = static_cast<CFStringRef>(ipKey);
	ipInfos = getIPAddressInfoFromKey(ipKeyRef);

	// In case there is no cable or it was just plugged in we won't have any IPv4 information available in the State keys, so try to get it from the Setup keys
	if (ipInfos.empty())
	{
		if (auto const serviceID = getServiceForInterface(interfaceName); serviceID != nullptr)
		{
			auto const ipKey = [NSString stringWithFormat:DYNAMIC_STORE_NETWORK_SERVICE_STRING @"%@" DYNAMIC_STORE_IPV4_STRING, serviceID];
			auto const* const ipKeyRef = static_cast<CFStringRef>(ipKey);
			ipInfos = getIPAddressInfoFromKey(ipKeyRef);
		}
	}

	return ipInfos;
}

void dynamicStoreChangedCallback(SCDynamicStoreRef store, CFArrayRef changedKeys, void* ctx)
{
	auto const count = CFArrayGetCount(changedKeys);

	for (auto index = CFIndex{ 0 }; index < count; ++index)
	{
		auto const* const keyRef = static_cast<CFStringRef>(CFArrayGetValueAtIndex(changedKeys, index));
		auto const* const key = (__bridge NSString const*)keyRef;

		if ([key hasPrefix:DYNAMIC_STORE_NETWORK_STATE_STRING])
		{
			if ([key hasSuffix:DYNAMIC_STORE_LINK_STRING])
			{
				auto const prefixLength = [DYNAMIC_STORE_NETWORK_STATE_STRING length];
				auto const suffixLength = [DYNAMIC_STORE_LINK_STRING length];
				auto const* const interfaceName = [key substringWithRange:NSMakeRange(prefixLength, [key length] - prefixLength - suffixLength)];

				// Read value
				auto const isConnected = isInterfaceConnected(keyRef);

				// Notify
				onConnectedStateChanged(std::string{ [interfaceName UTF8String] }, isConnected);
			}
			else if ([key hasSuffix:DYNAMIC_STORE_IPV4_STRING])
			{
				auto const prefixLength = [DYNAMIC_STORE_NETWORK_STATE_STRING length];
				auto const suffixLength = [DYNAMIC_STORE_IPV4_STRING length];
				auto const* const interfaceName = [key substringWithRange:NSMakeRange(prefixLength, [key length] - prefixLength - suffixLength)];

				// Read IP addresses info
				auto ipAddressInfos = getIPAddressInfo(interfaceName);

				// Notify
				onIPAddressInfosChanged(std::string{ [interfaceName UTF8String] }, std::move(ipAddressInfos));
			}
		}
		else if ([key hasPrefix:DYNAMIC_STORE_NETWORK_SERVICE_STRING])
		{
			if ([key hasSuffix:DYNAMIC_STORE_IPV4_STRING])
			{
				auto const prefixLength = [DYNAMIC_STORE_NETWORK_SERVICE_STRING length];
				auto const suffixLength = [DYNAMIC_STORE_IPV4_STRING length];
				auto const* const serviceID = [key substringWithRange:NSMakeRange(prefixLength, [key length] - prefixLength - suffixLength)];
				auto const* const interfaceName = getInterfaceForService(serviceID);

				if (interfaceName)
				{
					// Read IP addresses info
					auto ipAddressInfos = getIPAddressInfo(interfaceName);

					// Notify
					onIPAddressInfosChanged(std::string{ [interfaceName UTF8String] }, std::move(ipAddressInfos));
				}
			}
		}
	}
}

static void setOtherFieldsFromIOCTL(Interfaces& interfaces) noexcept
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

		int family = ifa->ifa_addr->sa_family;

		/* For AF_LINK, get the mac and setup the interface struct */
		if (family == AF_LINK)
		{
			// Only process interfaces that have already been recorded
			auto const intfcIt = interfaces.find(ifa->ifa_name);
			if (intfcIt == interfaces.end())
			{
				continue;
			}
			auto& interface = intfcIt->second;

			// Get media information
			struct ifmediareq ifmr;
			memset(&ifmr, 0, sizeof(ifmr));
			strncpy(ifmr.ifm_name, ifa->ifa_name, sizeof(ifmr.ifm_name));
			if (ioctl(sck, SIOCGIFMEDIA, &ifmr) != -1)
			{
				// Get the mac address contained in the AF_LINK specific data
				auto sdl = reinterpret_cast<struct sockaddr_dl*>(ifa->ifa_addr);
				if (sdl->sdl_alen == 6)
				{
					auto ptr = reinterpret_cast<unsigned char*>(LLADDR(sdl));
					std::memcpy(interface.macAddress.data(), ptr, 6);
				}
			}
		}
	}

	// Release the socket
	close(sck);
}

void refreshInterfaces(Interfaces& interfaces) noexcept
{
	clearInterfaceToServiceMapping();

	auto const serviceKeys = DYNAMIC_STORE_NETWORK_SERVICE_STRING @"[^/]+" DYNAMIC_STORE_INTERFACE_STRING;
	auto const* const serviceKeysRef = static_cast<CFStringRef>(serviceKeys);
	auto servicesArray = SCDynamicStoreCopyKeyList(s_global.storeRef, serviceKeysRef);
	if (servicesArray)
	{
		auto const count = CFArrayGetCount(servicesArray);

		for (auto index = CFIndex{ 0 }; index < count; ++index)
		{
			auto const* const keyRef = static_cast<CFStringRef>(CFArrayGetValueAtIndex(servicesArray, index));
			auto const* const valueRef = SCDynamicStoreCopyValue(s_global.storeRef, keyRef);
			if (valueRef)
			{
				auto const* const valueDictRef = static_cast<CFDictionaryRef>(valueRef);
				auto const* const valueDict = (__bridge NSDictionary const*)valueDictRef;

				NSString* const deviceName = [valueDict objectForKey:@"DeviceName"];
				NSString* const hardware = [valueDict objectForKey:@"Hardware"];
				NSString* const description = [valueDict objectForKey:@"UserDefinedName"];

				if (deviceName && hardware)
				{
					auto const* const key = (__bridge NSString const*)keyRef;
					auto const prefixLength = [DYNAMIC_STORE_NETWORK_SERVICE_STRING length];
					auto const suffixLength = [DYNAMIC_STORE_INTERFACE_STRING length];
					auto const* const serviceID = [key substringWithRange:NSMakeRange(prefixLength, [key length] - prefixLength - suffixLength)];
					setInterfaceToServiceMapping(deviceName, serviceID);

					auto interface = Interface{};
					auto const interfaceID = getStdString(deviceName);
					interface.id = interfaceID;
					interface.type = getInterfaceType(hardware);
					interface.description = getStdString(description);

					// TODO: Understand how to get the real user defined name (not sure why, as Settings is able to display it, the UserDefinedName key is present in /Library/Preferences/SystemConfiguration/preferences.plist under /Network/Service/<serverID>/UserDefinedName but not in DynamicStore)
					interface.alias = interface.description + " (" + interface.id + ")";

					// Declared macOS services are always enabled through DynamicStore (not sure why, as Settings is able to display disabled interfaces and the __INACTIVE__ value is set in /Library/Preferences/SystemConfiguration/preferences.plist)
					interface.isEnabled = true;

					// Check if interface is connected
					{
						auto const linkStateKey = [NSString stringWithFormat:DYNAMIC_STORE_NETWORK_STATE_STRING @"%@" DYNAMIC_STORE_LINK_STRING, deviceName];
						auto const* const linkStateKeyRef = static_cast<CFStringRef>(linkStateKey);
						interface.isConnected = isInterfaceConnected(linkStateKeyRef);
					}

					// Get IP Addresses info
					interface.ipAddressInfos = getIPAddressInfo(deviceName);

					// Is interface Virtual
					interface.isVirtual = getIsVirtualInterface(deviceName, interface.type);

					// Add the interface to the list
					interfaces[interfaceID] = std::move(interface);
				}
				CFRelease(valueRef);
			}
		}
		CFRelease(servicesArray);
	}

	// Set other fields we couldn't retrieve
	setOtherFieldsFromIOCTL(interfaces);

	// Remove interfaces that are not complete
	for (auto it = interfaces.begin(); it != interfaces.end(); /* Iterate inside the loop */)
	{
		auto& intfc = it->second;

		auto isValidMacAddress = false;
		for (auto const v : intfc.macAddress)
		{
			if (v != 0)
			{
				isValidMacAddress = true;
				break;
			}
		}
		if (intfc.type == Interface::Type::None || !isValidMacAddress)
		{
			// Remove from the list
			it = interfaces.erase(it);
		}
		else
		{
			++it;
		}
	}
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
	// Register for Added/Removed interfaces notification (kernel events)
	{
		mach_port_t masterPort = 0;
		IOMasterPort(mach_task_self(), &masterPort);
		s_global.notificationPort = IONotificationPortCreate(masterPort);
		IONotificationPortSetDispatchQueue(s_global.notificationPort, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0));

		IOServiceAddMatchingNotification(s_global.notificationPort, kIOMatchedNotification, IOServiceMatching("IONetworkController"), onIONetworkControllerListChanged, nullptr, &s_global.controllerMatchIterator);
		IOServiceAddMatchingNotification(s_global.notificationPort, kIOTerminatedNotification, IOServiceMatching("IONetworkController"), onIONetworkControllerListChanged, nullptr, &s_global.controllerTerminateIterator);

		// Clear the iterators discarding already discovered adapters, we'll manually list them anyway
		clearIterator(s_global.controllerMatchIterator);
		clearIterator(s_global.controllerTerminateIterator);
	}

	// Register for State change notification on all interfaces (Dynamic Store events)
	{
		auto* scKeys = [[NSMutableArray alloc] init];
#if !__has_feature(objc_arc)
		[scKeys autorelease];
#endif
		[scKeys addObject:DYNAMIC_STORE_NETWORK_STATE_STRING @"[^/]+" DYNAMIC_STORE_LINK_STRING]; // Monitor changes in the Link State
		[scKeys addObject:DYNAMIC_STORE_NETWORK_STATE_STRING @"[^/]+" DYNAMIC_STORE_IPV4_STRING]; // Monitor changes in the IPv4 State
		[scKeys addObject:DYNAMIC_STORE_NETWORK_SERVICE_STRING @"[^/]+" DYNAMIC_STORE_IPV4_STRING]; // Monitor changes in the IPv4 Setup

		/* Connect to the dynamic store */
		auto ctx = SCDynamicStoreContext{ 0, NULL, NULL, NULL, NULL };
		s_global.storeRef = SCDynamicStoreCreate(nullptr, CFSTR("networkInterfaceHelper"), dynamicStoreChangedCallback, &ctx);

		/* Start monitoring */
		if (SCDynamicStoreSetNotificationKeys(s_global.storeRef, nullptr, (__bridge CFArrayRef)scKeys))
		{
			s_global.thread = std::thread(
				[]()
				{
					// Create a source for the thread's loop
					auto const runLoopSourceRef = SCDynamicStoreCreateRunLoopSource(kCFAllocatorDefault, s_global.storeRef, 0);

					// Add a source to the thread's loop, so it has something to do
					s_global.threadRunLoopRef = CFRunLoopGetCurrent();
					CFRunLoopAddSource(s_global.threadRunLoopRef, runLoopSourceRef, kCFRunLoopCommonModes);

					// Run the thread's loop, until stopped
					CFRunLoopRun();

					// Cleanup
					CFRelease(runLoopSourceRef);
					s_global.threadRunLoopRef = nullptr;
				});
		}
	}
}

void onLastObserverUnregistered() noexcept
{
	//Clean up the IOKit code
	if (s_global.controllerTerminateIterator)
	{
		IOObjectRelease(s_global.controllerTerminateIterator);
	}

	if (s_global.controllerMatchIterator)
	{
		IOObjectRelease(s_global.controllerMatchIterator);
	}

	if (s_global.notificationPort)
	{
		IONotificationPortDestroy(s_global.notificationPort);
	}

	// Stop the thread
	if (s_global.thread.joinable())
	{
		// Stop the thread's run loop
		CFRunLoopStop(s_global.threadRunLoopRef);

		// Wait for the thread to complete its pending tasks
		s_global.thread.join();
	}

	// Release the dynamic store
	if (s_global.storeRef)
	{
		CFRelease(s_global.storeRef);
		s_global.storeRef = nullptr;
	}
}

} // namespace networkInterface
} // namespace avdecc
} // namespace la
