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
* @file networkInterfaceHelper_common.hpp
* @author Christophe Calmejane
* @brief OS independent network interface types and methods.
*/

#pragma once

#include "la/avdecc/networkInterfaceHelper.hpp"

#include <unordered_map>
#include <string>

namespace la
{
namespace avdecc
{
namespace networkInterface
{
using Interfaces = std::unordered_map<std::string, Interface>;

// Methods to be implemented by eachOS-dependant implementation
/** Block until the first enumeration occured */
void waitForFirstEnumeration() noexcept;
/** When the first observer is registered */
void onFirstObserverRegistered() noexcept;
/** When the last observer is unregistered */
void onLastObserverUnregistered() noexcept;

// Notifications from OS-dependant implementation
/** When the list of interfaces changed */
void onNewInterfacesList(Interfaces&& interfaces) noexcept;
/** When the Enabled state of an interface changed */
void onEnabledStateChanged(std::string const& interfaceName, bool const isEnabled) noexcept;
/** When the Connected state of an interface changed */
void onConnectedStateChanged(std::string const& interfaceName, bool const isConnected) noexcept;
/** When the Alias of an interface changed */
void onAliasChanged(std::string const& interfaceName, std::string&& alias) noexcept;
/** When the IPAddressInfos of an interface changed */
void onIPAddressInfosChanged(std::string const& interfaceName, Interface::IPAddressInfos&& ipAddressInfos) noexcept;
/** When the Gateways of an interface changed */
void onGatewaysChanged(std::string const& interfaceName, Interface::Gateways&& gateways) noexcept;

// Observer notifications
void notifyEnabledStateChanged(Interface const& intfc, bool const isEnabled) noexcept;
/** When the Connected state of an interface changed */
void notifyConnectedStateChanged(Interface const& intfc, bool const isConnected) noexcept;
/** When the Alias of an interface changed */
void notifyAliasChanged(Interface const& intfc, std::string const& alias) noexcept;
/** When the IPAddressInfos of an interface changed */
void notifyIPAddressInfosChanged(Interface const& intfc, Interface::IPAddressInfos const& ipAddressInfos) noexcept;
/** When the Gateways of an interface changed */
void notifyGatewaysChanged(Interface const& intfc, Interface::Gateways const& gateways) noexcept;


constexpr IPAddress::value_type_packed_v4 makePackedMaskV4(std::uint8_t const countBits) noexcept
{
	constexpr auto MaxBits = sizeof(IPAddress::value_type_packed_v4) * 8;
	if (countBits >= MaxBits)
	{
		return ~IPAddress::value_type_packed_v4(0);
	}
	if (countBits == 0)
	{
		return IPAddress::value_type_packed_v4(0);
	}
	return ~IPAddress::value_type_packed_v4(0) << (MaxBits - countBits);
}

constexpr IPAddress::value_type_packed_v4 makePackedMaskV6(std::uint8_t const countBits) noexcept
{
#pragma message("TODO: Use value_type_packed_v6 instead of value_type_packed_v4")
	constexpr auto MaxBits = sizeof(IPAddress::value_type_packed_v4) * 8;
	if (countBits >= MaxBits)
	{
		return ~IPAddress::value_type_packed_v4(0);
	}
	if (countBits == 0)
	{
		return IPAddress::value_type_packed_v4(0);
	}
	return ~IPAddress::value_type_packed_v4(0) << (MaxBits - countBits);
}

static inline void validateNetmaskV4(IPAddress const& netmask)
{
	auto packed = netmask.getIPV4Packed();
	auto maskStarted = false;
	for (auto i = 0u; i < (sizeof(IPAddress::value_type_packed_v4) * 8); ++i)
	{
		auto const isSet = packed & 0x00000001;
		// Bit is not set, check if mask was already started
		if (!isSet)
		{
			// Already started
			if (maskStarted)
			{
				throw std::invalid_argument("netmask is not contiguous");
			}
		}
		// Bit is set, start the mask
		else
		{
			maskStarted = true;
		}
		packed >>= 1;
	}
	// At least one bit must be set
	if (!maskStarted)
	{
		throw std::invalid_argument("netmask cannot be empty");
	}
}

} // namespace networkInterface
} // namespace avdecc
} // namespace la
