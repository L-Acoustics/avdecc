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

#include "la/avdecc/utils.hpp"
#include "la/avdecc/internals/endian.hpp"

#include "networkInterfaceHelper_common.hpp"

#include <sstream>
#include <exception>
#include <iomanip> // setfill
#include <ios> // uppercase
#include <algorithm> // remove / copy
#include <string>
#include <mutex>
#include <vector>
#include <set>

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

class NetworkInterfaceMonitorImpl final : public NetworkInterfaceMonitor
{
public:
private:
	virtual void onFirstObserverRegistered() noexcept override
	{
		networkInterface::onFirstObserverRegistered();
	}
	virtual void onLastObserverUnregistered() noexcept override
	{
		networkInterface::onLastObserverUnregistered();
	}
};

static NetworkInterfaceMonitorImpl s_Monitor{}; // Also serves as lock
static Interfaces s_NetworkInterfaces{};

void LA_AVDECC_CALL_CONVENTION refreshInterfaces() noexcept
{
	auto const lg = std::lock_guard(s_Monitor);

	s_NetworkInterfaces.clear();
	refreshInterfaces(s_NetworkInterfaces);
}

void LA_AVDECC_CALL_CONVENTION enumerateInterfaces(EnumerateInterfacesHandler const& onInterface) noexcept
{
	auto const lg = std::lock_guard(s_Monitor);

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
	auto const lg = std::lock_guard(s_Monitor);

	// No interfaces, force a refresh
	if (s_NetworkInterfaces.empty())
		refreshInterfaces();

	// Search specified interface name in the list
	auto const it = s_NetworkInterfaces.find(name);
	if (it == s_NetworkInterfaces.end())
		throw Exception("getInterfaceByName() error: No interface found with specified name");
	return it->second;
}

std::string LA_AVDECC_CALL_CONVENTION macAddressToString(MacAddress const& macAddress, bool const upperCase, char const separator) noexcept
{
	try
	{
		bool isFirst = true;
		std::stringstream ss;

		if (upperCase)
		{
			ss << std::uppercase;
		}
		ss << std::hex << std::setfill('0');

		for (auto const v : macAddress)
		{
			if (isFirst)
			{
				isFirst = false;
			}
			else
			{
				if (separator != '\0')
				{
					ss << separator;
				}
			}
			ss << std::setw(2) << forceNumeric(v); // setw has to be called every time
		}

		return ss.str();
	}
	catch (...)
	{
		return {};
	}
}

MacAddress LA_AVDECC_CALL_CONVENTION stringToMacAddress(std::string const& macAddressAsString, char const separator)
{
	auto str = macAddressAsString;
	if (separator != '\0')
	{
		str.erase(std::remove(str.begin(), str.end(), separator), str.end());
	}

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

void LA_AVDECC_CALL_CONVENTION registerObserver(NetworkInterfaceObserver* const observer) noexcept
{
	try
	{
		auto const lg = std::lock_guard(s_Monitor);

		// No interfaces, force a refresh
		if (s_NetworkInterfaces.empty())
		{
			refreshInterfaces();
		}

		// Register observer
		s_Monitor.registerObserver(observer);

		// Now call the observer for all interfaces
		for (auto const& intfcKV : s_NetworkInterfaces)
		{
			try
			{
				observer->onInterfaceAdded(intfcKV.second);
			}
			catch (...)
			{
				// Ignore exceptions
			}
		}
	}
	catch (...)
	{
		// Ignore exceptions
	}
}

void LA_AVDECC_CALL_CONVENTION unregisterObserver(NetworkInterfaceObserver* const observer) noexcept
{
	try
	{
		auto const lg = std::lock_guard(s_Monitor);

		s_Monitor.unregisterObserver(observer);
	}
	catch (...)
	{
		// Ignore exceptions
	}
}

void onNewInterfacesList(Interfaces&& interfaces) noexcept
{
	auto const lg = std::lock_guard(s_Monitor);

	// Compare previous interfaces and new ones
	auto addedInterfaces = std::vector<std::string>{};
	auto removedInterfaces = std::vector<std::string>{};

	// Process all previous interfaces and search if it's still present in the new list
	for (auto const& intfcKV : s_NetworkInterfaces)
	{
		if (interfaces.count(intfcKV.first) == 0)
		{
			s_Monitor.notifyObservers<NetworkInterfaceObserver>(
				[&intfcKV](auto* obs)
				{
					obs->onInterfaceRemoved(intfcKV.second);
				});
		}
	}

	// Process all new interfaces and search if it was present in the previous list
	for (auto const& intfcKV : interfaces)
	{
		if (s_NetworkInterfaces.count(intfcKV.first) == 0)
		{
			s_Monitor.notifyObservers<NetworkInterfaceObserver>(
				[&intfcKV](auto* obs)
				{
					obs->onInterfaceAdded(intfcKV.second);
				});
		}
	}

	// Update the interfaces list
	s_NetworkInterfaces = std::move(interfaces);
}

void onEnabledStateChanged(std::string const& interfaceName, bool const isEnabled) noexcept
{
	auto const lg = std::lock_guard(s_Monitor);

	// Search the interface matching the name
	auto intfcIt = s_NetworkInterfaces.find(interfaceName);
	if (intfcIt != s_NetworkInterfaces.end())
	{
		auto& intfc = intfcIt->second;
		if (intfc.isEnabled != isEnabled)
		{
			intfc.isEnabled = isEnabled;
			s_Monitor.notifyObservers<NetworkInterfaceObserver>(
				[&intfc](auto* obs)
				{
					obs->onInterfaceEnabledStateChanged(intfc, intfc.isEnabled);
				});
		}
	}
}

void onConnectedStateChanged(std::string const& interfaceName, bool const isConnected) noexcept
{
	auto const lg = std::lock_guard(s_Monitor);

	// Search the interface matching the name
	auto intfcIt = s_NetworkInterfaces.find(interfaceName);
	if (intfcIt != s_NetworkInterfaces.end())
	{
		auto& intfc = intfcIt->second;
		if (intfc.isConnected != isConnected)
		{
			intfc.isConnected = isConnected;
			s_Monitor.notifyObservers<NetworkInterfaceObserver>(
				[&intfc](auto* obs)
				{
					obs->onInterfaceConnectedStateChanged(intfc, intfc.isConnected);
				});
		}
	}
}

/* ************************************************************ */
/* IPAddress class definition                                   */
/* ************************************************************ */
IPAddress::IPAddress() noexcept
	: _ipString{ "Invalid IP" }
{
}

IPAddress::IPAddress(value_type_v4 const ipv4) noexcept
{
	setValue(ipv4);
}

IPAddress::IPAddress(value_type_v6 const ipv6) noexcept
{
	setValue(ipv6);
}

IPAddress::IPAddress(value_type_packed_v4 const ipv4) noexcept
{
	setValue(ipv4);
}

IPAddress::IPAddress(std::string const& ipString)
{
	// TODO: Handle IPV6 later: https://tools.ietf.org/html/rfc5952
	auto tokens = la::avdecc::utils::tokenizeString(ipString, '.', false);

	value_type_v4 ip{};
	if (tokens.size() != ip.size())
	{
		throw std::invalid_argument("Invalid IPV4 format");
	}
	for (auto i = 0u; i < ip.size(); ++i)
	{
		// Using std::uint16_t for convertFromString since it's not possible to use 'char' type for this method
		auto const tokenValue = la::avdecc::utils::convertFromString<std::uint16_t>(tokens[i].c_str());
		// Check if parsed value doesn't exceed max value for a value_type_v4 single element
		if (tokenValue > std::integral_constant<decltype(tokenValue), std::numeric_limits<decltype(ip)::value_type>::max()>::value)
		{
			throw std::invalid_argument("Invalid IPV4 value");
		}
		ip[i] = static_cast<decltype(ip)::value_type>(tokenValue);
	}
	setValue(ip);
}

IPAddress::~IPAddress() noexcept {}

void IPAddress::setValue(value_type_v4 const ipv4) noexcept
{
	_type = Type::V4;
	_ipv4 = ipv4;
	_ipv6 = {};

	buildIPString();
}

void IPAddress::setValue(value_type_v6 const ipv6) noexcept
{
	_type = Type::V6;
	_ipv4 = {};
	_ipv6 = ipv6;

	buildIPString();
}

void IPAddress::setValue(value_type_packed_v4 const ipv4) noexcept
{
	setValue(unpack(ipv4));
}

IPAddress::Type IPAddress::getType() const noexcept
{
	return _type;
}

IPAddress::value_type_v4 IPAddress::getIPV4() const
{
	if (_type != Type::V4)
	{
		throw std::invalid_argument("Not an IP V4");
	}
	return _ipv4;
}

IPAddress::value_type_v6 IPAddress::getIPV6() const
{
	if (_type != Type::V6)
	{
		throw std::invalid_argument("Not an IP V6");
	}
	return _ipv6;
}

IPAddress::value_type_packed_v4 IPAddress::getIPV4Packed() const
{
	if (_type != Type::V4)
	{
		throw std::invalid_argument("Not an IP V4");
	}

	return pack(_ipv4);
}

bool IPAddress::isValid() const noexcept
{
	return _type != Type::None;
}

IPAddress::operator value_type_v4() const
{
	return getIPV4();
}

IPAddress::operator value_type_v6() const
{
	return getIPV6();
}

IPAddress::operator value_type_packed_v4() const
{
	return getIPV4Packed();
}

IPAddress::operator bool() const noexcept
{
	return isValid();
}

IPAddress::operator std::string() const noexcept
{
	return std::string(_ipString.data());
}

bool operator==(IPAddress const& lhs, IPAddress const& rhs) noexcept
{
	if (!lhs.isValid() && !rhs.isValid())
	{
		return true;
	}
	if (lhs._type != rhs._type)
	{
		return false;
	}
	switch (lhs._type)
	{
		case IPAddress::Type::V4:
			return lhs._ipv4 == rhs._ipv4;
		case IPAddress::Type::V6:
			return lhs._ipv6 == rhs._ipv6;
		default:
			break;
	}
	return false;
}

bool operator!=(IPAddress const& lhs, IPAddress const& rhs) noexcept
{
	return !operator==(lhs, rhs);
}

bool operator<(IPAddress const& lhs, IPAddress const& rhs)
{
	if (lhs._type != rhs._type)
	{
		return lhs._type < rhs._type;
	}
	switch (lhs._type)
	{
		case IPAddress::Type::V4:
			return lhs._ipv4 < rhs._ipv4;
		case IPAddress::Type::V6:
			return lhs._ipv6 < rhs._ipv6;
		default:
			throw std::invalid_argument("Invalid Type");
	}
}

bool operator<=(IPAddress const& lhs, IPAddress const& rhs)
{
	if (lhs._type != rhs._type)
	{
		return lhs._type < rhs._type;
	}
	switch (lhs._type)
	{
		case IPAddress::Type::V4:
			return lhs._ipv4 <= rhs._ipv4;
		case IPAddress::Type::V6:
			return lhs._ipv6 <= rhs._ipv6;
		default:
			throw std::invalid_argument("Invalid Type");
	}
}

IPAddress operator+(IPAddress const& lhs, std::uint32_t const value)
{
	switch (lhs._type)
	{
		case IPAddress::Type::V4:
		{
			auto v = endianSwap<Endianness::NetworkEndian, Endianness::HostEndian>(lhs.getIPV4Packed());
			v += value;
			return IPAddress{ endianSwap<Endianness::HostEndian, Endianness::NetworkEndian>(v) };
		}
		case IPAddress::Type::V6:
			throw std::invalid_argument("IPV6 not supported yet");
		default:
			throw std::invalid_argument("Invalid Type");
	}
}

IPAddress operator-(IPAddress const& lhs, std::uint32_t const value)
{
	switch (lhs._type)
	{
		case IPAddress::Type::V4:
		{
			auto v = endianSwap<Endianness::NetworkEndian, Endianness::HostEndian>(lhs.getIPV4Packed());
			v -= value;
			return IPAddress{ endianSwap<Endianness::HostEndian, Endianness::NetworkEndian>(v) };
		}
		case IPAddress::Type::V6:
			throw std::invalid_argument("IPV6 not supported yet");
		default:
			throw std::invalid_argument("Invalid Type");
	}
}

IPAddress& operator++(IPAddress& lhs)
{
	switch (lhs._type)
	{
		case IPAddress::Type::V4:
		{
			auto v = endianSwap<Endianness::NetworkEndian, Endianness::HostEndian>(lhs.getIPV4Packed());
			++v;
			lhs.setValue(endianSwap<Endianness::HostEndian, Endianness::NetworkEndian>(v));
			break;
		}
		case IPAddress::Type::V6:
			throw std::invalid_argument("IPV6 not supported yet");
		default:
			throw std::invalid_argument("Invalid Type");
	}

	return lhs;
}

IPAddress& operator--(IPAddress& lhs)
{
	switch (lhs._type)
	{
		case IPAddress::Type::V4:
		{
			auto v = endianSwap<Endianness::NetworkEndian, Endianness::HostEndian>(lhs.getIPV4Packed());
			--v;
			lhs.setValue(endianSwap<Endianness::HostEndian, Endianness::NetworkEndian>(v));
			break;
		}
		case IPAddress::Type::V6:
			throw std::invalid_argument("IPV6 not supported yet");
		default:
			throw std::invalid_argument("Invalid Type");
	}

	return lhs;
}

IPAddress operator&(IPAddress const& lhs, IPAddress const& rhs)
{
	switch (lhs._type)
	{
		case IPAddress::Type::V4:
		{
			return IPAddress{ lhs.getIPV4Packed() & rhs.getIPV4Packed() };
		}
		case IPAddress::Type::V6:
			throw std::invalid_argument("IPV6 not supported yet");
		default:
			throw std::invalid_argument("Invalid Type");
	}
}

IPAddress operator|(IPAddress const& lhs, IPAddress const& rhs)
{
	switch (lhs._type)
	{
		case IPAddress::Type::V4:
		{
			return IPAddress{ lhs.getIPV4Packed() | rhs.getIPV4Packed() };
		}
		case IPAddress::Type::V6:
			throw std::invalid_argument("IPV6 not supported yet");
		default:
			throw std::invalid_argument("Invalid Type");
	}
}

IPAddress::value_type_packed_v4 IPAddress::pack(value_type_v4 const ipv4) noexcept
{
	value_type_packed_v4 ip{ 0u };

	ip |= ipv4[0];
	ip |= ipv4[1] << 8;
	ip |= ipv4[2] << 16;
	ip |= ipv4[3] << 24;

	return ip;
}

IPAddress::value_type_v4 IPAddress::unpack(value_type_packed_v4 const ipv4) noexcept
{
	value_type_v4 ip{};

	ip[0] = ipv4 & 0xFF;
	ip[1] = (ipv4 >> 8) & 0xFF;
	ip[2] = (ipv4 >> 16) & 0xFF;
	ip[3] = (ipv4 >> 24) & 0xFF;

	return ip;
}


std::size_t IPAddress::hash::operator()(IPAddress const& ip) const
{
	std::size_t h{ 0u };
	switch (ip._type)
	{
		case Type::V4:
		{
			for (auto const v : ip._ipv4)
			{
				h = h * 0x100 + v;
			}
		}
		case Type::V6:
		{
			for (auto const v : ip._ipv6)
			{
				h = h * 0x10 + v;
			}
		}
		default:
			break;
	}
	return h;
}

void IPAddress::buildIPString() noexcept
{
	std::string ip;

	switch (_type)
	{
		case Type::V4:
		{
			for (auto const v : _ipv4)
			{
				if (!ip.empty())
				{
					ip.append(".");
				}
				ip.append(std::to_string(v));
			}
			break;
		}
		case Type::V6:
		{
			bool first{ true };
			std::stringstream ss;
			ss << std::hex;

			for (auto const v : _ipv6)
			{
				if (first)
				{
					first = false;
				}
				else
				{
					ss << ":";
				}
				ss << v;
			}

			ip = ss.str();
			break;
		}
		default:
			ip = "Invalid IPAddress";
			break;
	}

	auto len = ip.size();
	if (!AVDECC_ASSERT_WITH_RET(len < _ipString.size(), "String length greater than IPStringMaxLength"))
	{
		len = _ipString.size() - 1;
		ip.resize(len);
	}
	std::copy(ip.begin(), ip.end(), _ipString.begin());
	_ipString[len] = 0;
}

/* ************************************************************ */
/* IPAddressInfo definition                                     */
/* ************************************************************ */
static void checkValidIPAddressInfo(IPAddress const& address, IPAddress const& netmask)
{
	// Check if address and netmask types are identical
	auto const addressType = address.getType();
	if (addressType != netmask.getType())
	{
		throw std::invalid_argument("address and netmask not of the same Type");
	}

	// Check if netmask is contiguous
	switch (addressType)
	{
		case IPAddress::Type::V4:
		{
			auto packed = endianSwap<Endianness::NetworkEndian, Endianness::HostEndian>(netmask.getIPV4Packed());
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
			break;
		}
		case IPAddress::Type::V6:
			throw std::invalid_argument("IPV6 not supported yet");
		default:
			throw std::invalid_argument("Invalid Type");
	}
}

IPAddress IPAddressInfo::getNetworkBaseAddress() const
{
	checkValidIPAddressInfo(address, netmask);

	switch (address.getType())
	{
		case IPAddress::Type::V4:
		{
			return IPAddress{ address.getIPV4Packed() & netmask.getIPV4Packed() };
		}
		case IPAddress::Type::V6:
			throw std::invalid_argument("IPV6 not supported yet");
		default:
			throw std::invalid_argument("Invalid Type");
	}
}

IPAddress IPAddressInfo::getBroadcastAddress() const
{
	checkValidIPAddressInfo(address, netmask);

	switch (address.getType())
	{
		case IPAddress::Type::V4:
		{
			return IPAddress{ address.getIPV4Packed() | ~netmask.getIPV4Packed() };
		}
		case IPAddress::Type::V6:
			throw std::invalid_argument("IPV6 not supported yet");
		default:
			throw std::invalid_argument("Invalid Type");
	}
}

bool IPAddressInfo::isPrivateNetworkAddress() const
{
	checkValidIPAddressInfo(address, netmask);

	switch (address.getType())
	{
		case IPAddress::Type::V4:
		{
			auto const isInRange = [](auto const rangeStart, auto const rangeEnd, auto const rangeMask, auto const adrs, auto const mask)
			{
				return (adrs >= rangeStart) && (adrs <= rangeEnd) && (mask >= rangeMask);
			};

			constexpr auto PrivateClassAStart = IPAddress::value_type_packed_v4{ 0x0A000000 }; // 10.0.0.0
			constexpr auto PrivateClassAEnd = IPAddress::value_type_packed_v4{ 0x0AFFFFFF }; // 10.255.255.255
			constexpr auto PrivateClassAMask = IPAddress::value_type_packed_v4{ 0xFF000000 }; // 255.0.0.0
			constexpr auto PrivateClassBStart = IPAddress::value_type_packed_v4{ 0xAC100000 }; // 172.16.0.0
			constexpr auto PrivateClassBEnd = IPAddress::value_type_packed_v4{ 0xAC1FFFFF }; // 172.31.255.255
			constexpr auto PrivateClassBMask = IPAddress::value_type_packed_v4{ 0xFFF00000 }; // 255.240.0.0
			constexpr auto PrivateClassCStart = IPAddress::value_type_packed_v4{ 0xC0A80000 }; // 192.168.0.0
			constexpr auto PrivateClassCEnd = IPAddress::value_type_packed_v4{ 0xC0A8FFFF }; // 192.168.255.255
			constexpr auto PrivateClassCMask = IPAddress::value_type_packed_v4{ 0xFFFF0000 }; // 255.255.0.0

			// Get the packed address and mask for easy comparison
			auto const adrs = endianSwap<Endianness::NetworkEndian, Endianness::HostEndian>(address.getIPV4Packed());
			auto const mask = endianSwap<Endianness::NetworkEndian, Endianness::HostEndian>(netmask.getIPV4Packed());

			// Check if the address is in any of the ranges
			if (isInRange(PrivateClassAStart, PrivateClassAEnd, PrivateClassAMask, adrs, mask) || isInRange(PrivateClassBStart, PrivateClassBEnd, PrivateClassBMask, adrs, mask) || isInRange(PrivateClassCStart, PrivateClassCEnd, PrivateClassCMask, adrs, mask))
			{
				return true;
			}

			return false;
		}
		case IPAddress::Type::V6:
			throw std::invalid_argument("IPV6 not supported yet");
		default:
			throw std::invalid_argument("Invalid Type");
	}
}

bool operator==(IPAddressInfo const& lhs, IPAddressInfo const& rhs) noexcept
{
	return (lhs.address == rhs.address) && (lhs.netmask == rhs.netmask);
}

bool operator!=(IPAddressInfo const& lhs, IPAddressInfo const& rhs) noexcept
{
	return !operator==(lhs, rhs);
}

bool operator<(IPAddressInfo const& lhs, IPAddressInfo const& rhs)
{
	if (lhs.address != rhs.address)
	{
		return lhs.address < rhs.address;
	}
	return lhs.netmask < rhs.netmask;
}

bool operator<=(IPAddressInfo const& lhs, IPAddressInfo const& rhs)
{
	if (lhs.address != rhs.address)
	{
		return lhs.address < rhs.address;
	}
	return lhs.netmask <= rhs.netmask;
}

} // namespace networkInterface
} // namespace avdecc
} // namespace la
