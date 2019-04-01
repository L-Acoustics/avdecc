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
* @file networkInterfaceHelper_common.cpp
* @author Christophe Calmejane
*/

#include "networkInterfaceHelper_common.hpp"

#include <sstream>
#include <exception>
#include <iomanip> // setfill
#include <ios> // uppercase
#include <algorithm> // remove
#include <string>
#include <mutex>

namespace la
{
namespace avdecc
{
namespace networkInterface
{
template<typename T>
inline auto forceNumeric(T&& t)
{
	// Promote a built-in type to at least (unsigned)int
	return +t;
}

static Interfaces s_NetworkInterfaces{};
static std::recursive_mutex s_Mutex;

void LA_AVDECC_CALL_CONVENTION refreshInterfaces() noexcept
{
	std::lock_guard<decltype(s_Mutex)> const lg(s_Mutex);

	s_NetworkInterfaces.clear();
	refreshInterfaces(s_NetworkInterfaces);
}

void LA_AVDECC_CALL_CONVENTION enumerateInterfaces(EnumerateInterfacesHandler const& onInterface) noexcept
{
	std::lock_guard<decltype(s_Mutex)> const lg(s_Mutex);

	if (onInterface == nullptr)
		return;

	// No interfaces, force a refresh
	if (s_NetworkInterfaces.empty())
		refreshInterfaces();

	// Now enumerate all interfaces
	for (auto const& intfcKV : s_NetworkInterfaces)
	{
		try
		{
			onInterface(intfcKV.second);
		}
		catch (...)
		{
			// Ignore exceptions
		}
	}
}

Interface LA_AVDECC_CALL_CONVENTION getInterfaceByName(std::string const& name)
{
	std::lock_guard<decltype(s_Mutex)> const lg(s_Mutex);

	// No interfaces, force a refresh
	if (s_NetworkInterfaces.empty())
		refreshInterfaces();

	// Search specified interface name in the list
	auto const it = s_NetworkInterfaces.find(name);
	if (it == s_NetworkInterfaces.end())
		throw Exception("getInterfaceByName() error: No interface found with specified name");
	return it->second;
}

std::string LA_AVDECC_CALL_CONVENTION macAddressToString(MacAddress const& macAddress, bool const upperCase) noexcept
{
	try
	{
		bool isFirst = true;
		std::stringstream ss;

		if (upperCase)
			ss << std::uppercase;
		ss << std::hex << std::setfill('0');

		for (auto const v : macAddress)
		{
			if (isFirst)
				isFirst = false;
			else
				ss << ":";
			ss << std::setw(2) << forceNumeric(v); // setw has to be called every time
		}

		return ss.str();
	}
	catch (...)
	{
		return {};
	}
}

MacAddress LA_AVDECC_CALL_CONVENTION stringToMacAddress(std::string const& macAddressAsString)
{
	auto str = macAddressAsString;
	str.erase(std::remove(str.begin(), str.end(), ':'), str.end());

	auto ss = std::stringstream{};
	ss << std::hex << str;

	auto macAsInteger = std::uint64_t{};
	ss >> macAsInteger;
	if (ss.fail())
	{
		throw std::invalid_argument("Invalid MacAddress representation: " + macAddressAsString);
	}

	auto out = MacAddress{};
	out[0] = static_cast<MacAddress::value_type>((macAsInteger >> 40) & 0xFF);
	out[1] = static_cast<MacAddress::value_type>((macAsInteger >> 32) & 0xFF);
	out[2] = static_cast<MacAddress::value_type>((macAsInteger >> 24) & 0xFF);
	out[3] = static_cast<MacAddress::value_type>((macAsInteger >> 16) & 0xFF);
	out[4] = static_cast<MacAddress::value_type>((macAsInteger >> 8) & 0xFF);
	out[5] = static_cast<MacAddress::value_type>((macAsInteger >> 0) & 0xFF);

	return out;
}

bool LA_AVDECC_CALL_CONVENTION isMacAddressValid(MacAddress const& macAddress) noexcept
{
	if (macAddress.size() != 6)
		return false;
	for (auto const v : macAddress)
	{
		if (v != 0)
			return true;
	}
	return false;
}

} // namespace networkInterface
} // namespace avdecc
} // namespace la
