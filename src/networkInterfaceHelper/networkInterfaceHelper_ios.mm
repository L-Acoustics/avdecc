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
* @file networkInterfaceHelper_ios.mm
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
#include <errno.h>
#include <unordered_map>
#include <memory>
#include <string>
#include <mutex> // once, mutex
#include <cstring> // memcpy
#include <atomic>
#include <thread>
#include <thread>

#import <UIKit/UIKit.h>
#import <SystemConfiguration/SystemConfiguration.h>
#import <Foundation/Foundation.h>
#import <SystemConfiguration/CaptiveNetwork.h>

//#define USE_REACHABILITY
#define USE_POLLING

namespace la
{
namespace avdecc
{
namespace networkInterface
{
#if defined(USE_REACHABILITY)
struct GlobalInformation
{
	SCNetworkReachabilityRef reachabilityRef{ nullptr };
	std::thread thread{};
	CFRunLoopRef threadRunLoopRef{ nullptr };
	std::mutex lock{};
};

static auto s_global = GlobalInformation{};
#endif // USE_REACHABILITY

#if defined(USE_POLLING)
static auto s_shouldTerminate = std::atomic_bool{ false };
static auto s_observerThread = std::thread{};
#endif // USE_POLLING

/** std::string to NSString conversion */
//inline NSString* getNSString(std::string const& cString)
//{
//	return [NSString stringWithCString:cString.c_str() encoding:NSUTF8StringEncoding];
//}

/** NSString to std::string conversion */
inline std::string getStdString(NSString const* const nsString) noexcept
{
	return std::string{ [nsString UTF8String] };
}

#if defined(USE_REACHABILITY)
static void updateReachabilityFromFlags(SCNetworkReachabilityFlags const flags)
{
	auto reachableWithWifi = false;
	auto reachableWithCellular = false;

	// Somehow reachable
	if (flags & kSCNetworkReachabilityFlagsReachable)
	{
		// Doesn't require connection
		if ((flags & kSCNetworkReachabilityFlagsConnectionRequired) == 0)
		{
			reachableWithCellular = !!(flags & kSCNetworkReachabilityFlagsIsWWAN);
			reachableWithWifi = !(flags & kSCNetworkReachabilityFlagsIsWWAN);
		}
	}

	NSLog(@"Reachability changed: Wifi=%s Cellular=%s", reachableWithWifi ? "yes" : "no", reachableWithCellular ? "yes" : "no");
}

static void reachabilityChangedCallback(SCNetworkReachabilityRef target, SCNetworkReachabilityFlags flags, void* ctx)
{
	updateReachabilityFromFlags(flags);
}
#endif // USE_REACHABILITY

Interface::Type getInterfaceType(struct ifaddrs const* const ifa, int const sock)
{
	// Check for AWDL
	if (strncmp(ifa->ifa_name, "awdl", 4) == 0)
		return Interface::Type::AWDL;

	// Check for loopback
	if ((ifa->ifa_flags & IFF_LOOPBACK) != 0)
		return Interface::Type::Loopback;

	// Check for WiFi will be done in another pass

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
	auto pos = std::uint8_t{ 0u }; // Variable used to generate a unique 'fake' mac address for the device
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
			Interface interface;
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
			// Getting iOS device mac address is forbidden since iOS7 so use a fake address
			interface.macAddress = { 0x00, 0x01, 0x02, 0x03, 0x04, pos++ };
			// Add the interface to the list
			interfaces[ifa->ifa_name] = interface;
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

	// Second pass to detect WiFi interfaces
	{
		// This unfortunately only works on a physical device, we have to hardcode something for iPhone simulator
#if TARGET_IPHONE_SIMULATOR
		if (auto const intfcIt = interfaces.find("en0"); intfcIt != interfaces.end())
		{
			intfcIt->second.type = Interface::Type::WiFi;
		}
#else // TARGET_IPHONE_SIMULATOR
		auto const* const ifsRef = CNCopySupportedInterfaces();

		if (ifsRef)
		{
			auto const* const ifs = (__bridge NSArray const*)ifsRef;
			for (NSString* ifname in ifs)
			{
				if (auto const intfcIt = interfaces.find(getStdString(ifname)); intfcIt != interfaces.end())
				{
					intfcIt->second.type = Interface::Type::WiFi;
				}
			}
			CFRelease(ifsRef);
		}
#endif // TARGET_IPHONE_SIMULATOR
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

void onFirstObserverRegistered() noexcept
{
#if defined(USE_REACHABILITY)
	// Register to Reachability
	{
		// Create a 'zero' address
		auto zeroAddress = sockaddr_in{};
		bzero(&zeroAddress, sizeof(zeroAddress));
		zeroAddress.sin_len = sizeof(zeroAddress);
		zeroAddress.sin_family = AF_INET;
		s_global.reachabilityRef = SCNetworkReachabilityCreateWithAddress(kCFAllocatorDefault, reinterpret_cast<sockaddr const*>(&zeroAddress));

		if (s_global.reachabilityRef)
		{
			/* Start monitoring */
			auto ctx = SCNetworkReachabilityContext{ 0, nullptr, nullptr, nullptr, nullptr };
			if (SCNetworkReachabilitySetCallback(s_global.reachabilityRef, reachabilityChangedCallback, &ctx))
			{
				// Create a thread with a runloop to monitor changes
				s_global.thread = std::thread(
					[]()
					{
						// Add Reachability to the thread's loop, so it has something to do
						s_global.threadRunLoopRef = CFRunLoopGetCurrent();
						SCNetworkReachabilityScheduleWithRunLoop(s_global.reachabilityRef, s_global.threadRunLoopRef, kCFRunLoopCommonModes);

						// Run the thread's loop, until stopped
						CFRunLoopRun();

						// Cleanup
						s_global.threadRunLoopRef = nullptr;
					});

				// Immediately update reachability status
				auto flags = SCNetworkReachabilityFlags{ 0u };
				SCNetworkReachabilityGetFlags(s_global.reachabilityRef, &flags);
				updateReachabilityFromFlags(flags);
			}
		}
	}
#endif // USE_REACHABILITY

#if defined(USE_POLLING)
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

					// Algorithm to (try to) prevent deadlocking if onLastObserverUnregistered() is called while are are about to call onNewInterfacesList
					// If this is the case, onLastObserverUnregistered() will own s_Monitor lock and will wait for this thread to terminate through join()
					// At the same time, this thread will wait on s_Monitor when calling onNewInterfacesList(), not being able to complete.
					// So here we try to take s_Monitor, if it fails we check for s_shouldTerminate, and wait a few milliseconds before trying again.
					// !!!!!!! CURRENTLY NOT POSSIBLE TO IMPLEMENT - HAVE TO FIND A BETTER SOLUTION !!!!!!!!
					{
						// Check for change in Interface count
						onNewInterfacesList(std::move(newList));
					}

					// Setup next check time
					nextCheck = now + std::chrono::milliseconds(1000);
				}

				// Wait a little bit so we don't burn the CPU
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		});
#endif // USE_POLLING
}

void onLastObserverUnregistered() noexcept
{
#if defined(USE_REACHABILITY)
	// Stop the thread
	if (s_global.thread.joinable())
	{
		// Unschedule Reachability callbacks from the loop, which will cause the loop to end (non more tasks to run)
		SCNetworkReachabilityUnscheduleFromRunLoop(s_global.reachabilityRef, s_global.threadRunLoopRef, kCFRunLoopCommonModes);

		// Stop the thread's run loop
		CFRunLoopStop(s_global.threadRunLoopRef);

		// Wait for the thread to complete its pending tasks
		s_global.thread.join();
	}

	// Release Reachability
	if (s_global.reachabilityRef)
	{
		CFRelease(s_global.reachabilityRef);
		s_global.reachabilityRef = nullptr;
	}
#endif // USE_REACHABILITY

#if defined(USE_POLLING)
	s_shouldTerminate = true;
	if (s_observerThread.joinable())
	{
		s_observerThread.join();
		s_observerThread = {};
	}
#endif // USE_POLLING
}

} // namespace networkInterface
} // namespace avdecc
} // namespace la
