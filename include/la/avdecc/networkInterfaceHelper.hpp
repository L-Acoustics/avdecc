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

/**
* @file networkInterfaceHelper.hpp
* @author Christophe Calmejane
* @brief OS dependent network interface helper.
*/

#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <array>
#include <functional>
#include "internals/exports.hpp"
#include "internals/exception.hpp"

namespace la
{
namespace avdecc
{
namespace networkInterface
{
using MacAddress = std::array<std::uint8_t, 6>;

struct Interface
{
	enum class Type
	{
		None = 0, /**< Only used for initialization purpose. Never returned as a real Interface::Type */
		Loopback = 1, /**< Loopback interface */
		Ethernet = 2, /**< Ethernet interface */
		WiFi = 3, /**< 802.11 WiFi interface */
	};

	std::string name{}; /** Name of the interface (system chosen) */
	std::string description{}; /** Description of the interface (system chosen) */
	std::string alias{}; /** Alias of the interface (user chosen) */
	MacAddress macAddress{}; /** Mac address */
	std::vector<std::string> ipAddresses{}; /** List of IP addresses attached to this interface */
	std::vector<std::string> gateways{}; /** List of Gateways available for this interface */
	Type type{ Type::None }; /** The type of interface */
	bool isActive{ false }; /** True if this interface is active */
};

struct MacAddressHash
{
	size_t operator()(MacAddress const& mac) const
	{
		size_t h = 0;
		for (auto const c : mac)
			h = h * 31 + c;
		return h;
	}
};

using EnumerateInterfacesHandler = std::function<void(la::avdecc::networkInterface::Interface const&)>;

/** Refresh the list of network interfaces (automatically done at startup) */
LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION refreshInterfaces() noexcept;
/** Enumerates network interfaces. The specified handler is called for each found interface */
LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION enumerateInterfaces(EnumerateInterfacesHandler const& onInterface) noexcept;
/** Retrieve a copy of an interface from it's name. Throws Exception if no interface exists with that name. */
LA_AVDECC_API Interface LA_AVDECC_CALL_CONVENTION getInterfaceByName(std::string const& name);
/** Converts the specified MAC address to string (in the form: xx:xx:xx:xx:xx:xx) */
LA_AVDECC_API std::string LA_AVDECC_CALL_CONVENTION macAddressToString(MacAddress const& macAddress, bool const upperCase = true) noexcept;
/** Returns true if specified MAC address is valid */
LA_AVDECC_API bool LA_AVDECC_CALL_CONVENTION isMacAddressValid(MacAddress const& macAddress) noexcept;

} // namespace networkInterface
} // namespace avdecc
} // namespace la
