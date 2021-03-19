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
* @file networkInterfaceHelper.hpp
* @author Christophe Calmejane
* @brief OS dependent network interface helper.
*/

#pragma once

#include "internals/exports.hpp"
#include "internals/exception.hpp"

#include "utils.hpp"

#include <vector>
#include <string>
#include <cstdint>
#include <array>
#include <functional>
#include <mutex>

namespace la
{
namespace avdecc
{
namespace networkInterface
{
using MacAddress = std::array<std::uint8_t, 6>;

/* ************************************************************ */
/* IPAddress class declaration                                  */
/* ************************************************************ */
class LA_AVDECC_API IPAddress final
{
public:
	using value_type_v4 = std::array<std::uint8_t, 4>; // "a.b.c.d" -> [0] = a, [1] = b, [2] = c, [3] = d
	using value_type_v6 = std::array<std::uint16_t, 8>; // "aa::bb::cc::dd::ee::ff::gg::hh" -> [0] = aa, [1] = bb, ..., [7] = hh
	using value_type_packed_v4 = std::uint32_t; // Packed version of an IP V4: "a.b.c.d" -> MSB = a, LSB = d

	enum class Type
	{
		None,
		V4,
		V6,
	};

	/** Default constructor. */
	IPAddress() noexcept;

	/** Constructor from value_type_v4. */
	explicit IPAddress(value_type_v4 const ipv4) noexcept;

	/** Constructor from value_type_v6. */
	explicit IPAddress(value_type_v6 const ipv6) noexcept;

	/** Constructor from a value_type_packed_v4. */
	explicit IPAddress(value_type_packed_v4 const ipv4) noexcept;

	/** Constructor from a string. */
	explicit IPAddress(std::string const& ipString);

	/** Destructor. */
	~IPAddress() noexcept;

	/** Setter to change the IP value. */
	void setValue(value_type_v4 const ipv4) noexcept;

	/** Setter to change the IP value. */
	void setValue(value_type_v6 const ipv6) noexcept;

	/** Setter to change the IP value. */
	void setValue(value_type_packed_v4 const ipv4) noexcept;

	/** Getter to retrieve the Type of address. */
	Type getType() const noexcept;

	/** Getter to retrieve the IP value. Throws std::invalid_argument if IPAddress is not a Type::V4. */
	value_type_v4 getIPV4() const;

	/** Getter to retrieve the IP value. Throws std::invalid_argument if IPAddress is not a Type::V6. */
	value_type_v6 getIPV6() const;

	/** Getter to retrieve the IP value in the packed format. Throws std::invalid_argument if IPAddress is not a Type::V6. */
	value_type_packed_v4 getIPV4Packed() const;

	/** True if the IPAddress contains a value, false otherwise. */
	bool isValid() const noexcept;

	/** IPV4 operator (equivalent to getIPV4()). Throws std::invalid_argument if IPAddress is not a Type::V4. */
	explicit operator value_type_v4() const;

	/** IPV6 operator (equivalent to getIPV6()). Throws std::invalid_argument if IPAddress is not a Type::V6. */
	explicit operator value_type_v6() const;

	/** IPV4 operator (equivalent to getIPV4()). Throws std::invalid_argument if IPAddress is not a Type::V4. */
	explicit operator value_type_packed_v4() const;

	/** IPAddress validity bool operator (equivalent to isValid()). */
	explicit operator bool() const noexcept;

	/** std::string convertion operator. */
	explicit operator std::string() const noexcept;

	/** Equality operator. Returns true if the IPAddress values are equal. */
	friend LA_AVDECC_API bool operator==(IPAddress const& lhs, IPAddress const& rhs) noexcept;

	/** Non equality operator. */
	friend LA_AVDECC_API bool operator!=(IPAddress const& lhs, IPAddress const& rhs) noexcept;

	/** Inferiority operator. Throws std::invalid_argument if Type is unsupported. */
	friend LA_AVDECC_API bool operator<(IPAddress const& lhs, IPAddress const& rhs);

	/** Inferiority or equality operator. Throws std::invalid_argument if Type is unsupported. */
	friend LA_AVDECC_API bool operator<=(IPAddress const& lhs, IPAddress const& rhs);

	/** Increment operator. Throws std::invalid_argument if Type is unsupported. Note: Increment value is currently limited to 32bits. */
	friend LA_AVDECC_API IPAddress operator+(IPAddress const& lhs, std::uint32_t const value);

	/** Decrement operator. Throws std::invalid_argument if Type is unsupported. Note: Decrement value is currently limited to 32bits. */
	friend LA_AVDECC_API IPAddress operator-(IPAddress const& lhs, std::uint32_t const value);

	/** operator++ Throws std::invalid_argument if Type is unsupported. */
	friend LA_AVDECC_API IPAddress& operator++(IPAddress& lhs);

	/** operator-- Throws std::invalid_argument if Type is unsupported. */
	friend LA_AVDECC_API IPAddress& operator--(IPAddress& lhs);

	/** operator& Throws std::invalid_argument if Type is unsupported. */
	friend LA_AVDECC_API IPAddress operator&(IPAddress const& lhs, IPAddress const& rhs);

	/** operator| Throws std::invalid_argument if Type is unsupported. */
	friend LA_AVDECC_API IPAddress operator|(IPAddress const& lhs, IPAddress const& rhs);

	/** Pack an IP of Type::V4. */
	static value_type_packed_v4 pack(value_type_v4 const ipv4) noexcept;

	/** Unpack an IP of Type::V4. */
	static value_type_v4 unpack(value_type_packed_v4 const ipv4) noexcept;

	/** Hash functor to be used for std::hash */
	struct LA_AVDECC_API hash
	{
		std::size_t operator()(IPAddress const& ip) const;
	};

	// Defaulted compiler auto-generated methods
	IPAddress(IPAddress&&) = default;
	IPAddress(IPAddress const&) = default;
	IPAddress& operator=(IPAddress const&) = default;
	IPAddress& operator=(IPAddress&&) = default;

private:
	// Private methods
	void buildIPString() noexcept;
	// Private defines
	static constexpr size_t IPStringMaxLength = 40u;
	// Private variables
	Type _type{ Type::None };
	value_type_v4 _ipv4{};
	value_type_v6 _ipv6{};
	std::array<std::string::value_type, IPStringMaxLength> _ipString{};
};

/* ************************************************************ */
/* IPAddressInfo declaration                                    */
/* ************************************************************ */
struct IPAddressInfo
{
	IPAddress address{};
	IPAddress netmask{};

	/** Gets the network base IPAddress from specified netmask. Throws std::invalid_argument if either address or netmask is invalid, or if they are not of the same IPAddress::Type */
	LA_AVDECC_API IPAddress getNetworkBaseAddress() const;

	/** Gets the broadcast IPAddress from specified netmask. Throws std::invalid_argument if either address or netmask is invalid, or if they are not of the same IPAddress::Type */
	LA_AVDECC_API IPAddress getBroadcastAddress() const;

	/** Returns true if the IPAddressInfo is in the private network range (see https://en.wikipedia.org/wiki/Private_network). Throws std::invalid_argument if either address or netmask is invalid, or if they are not of the same IPAddress::Type */
	LA_AVDECC_API bool isPrivateNetworkAddress() const;

	/** Equality operator. Returns true if the IPAddressInfo values are equal. */
	friend LA_AVDECC_API bool operator==(IPAddressInfo const& lhs, IPAddressInfo const& rhs) noexcept;

	/** Non equality operator. */
	friend LA_AVDECC_API bool operator!=(IPAddressInfo const& lhs, IPAddressInfo const& rhs) noexcept;

	/** Inferiority operator. Throws std::invalid_argument if IPAddress::Type of either address or netmask is unsupported. */
	friend LA_AVDECC_API bool operator<(IPAddressInfo const& lhs, IPAddressInfo const& rhs);

	/** Inferiority or equality operator. Throws std::invalid_argument if IPAddress::Type of either address or netmask is unsupported. */
	friend LA_AVDECC_API bool operator<=(IPAddressInfo const& lhs, IPAddressInfo const& rhs);
};

/* ************************************************************ */
/* Interface declaration                                        */
/* ************************************************************ */
struct Interface
{
	using IPAddressInfos = std::vector<IPAddressInfo>;
	using Gateways = std::vector<IPAddress>;

	enum class Type
	{
		None = 0, /**< Only used for initialization purpose. Never returned as a real Interface::Type */
		Loopback = 1, /**< Loopback interface */
		Ethernet = 2, /**< Ethernet interface */
		WiFi = 3, /**< 802.11 WiFi interface */
		AWDL = 4, /**< Apple Wireless Direct Link */
	};

	std::string id{}; /** Identifier of the interface (system chosen, unique) (UTF-8) */
	std::string description{}; /** Description of the interface (system chosen) (UTF-8) */
	std::string alias{}; /** Alias of the interface (often user chosen) (UTF-8) */
	MacAddress macAddress{}; /** Mac address */
	IPAddressInfos ipAddressInfos{}; /** List of IPAddressInfo attached to this interface */
	Gateways gateways{}; /** List of Gateways available for this interface */
	Type type{ Type::None }; /** The type of interface */
	bool isEnabled{ false }; /** True if this interface is enabled */
	bool isConnected{ false }; /** True if this interface is connected to a working network (able to send and receive packets) */
	bool isVirtual{ false }; /** True if this interface is emulating a physical adapter (Like BlueTooth, VirtualMachine, or Software Loopback) */
};

/** MacAddress hash functor to be used for std::hash */
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

using NetworkInterfaceMonitor = la::avdecc::utils::TypedSubject<struct NetworkInterfaceMonitorTag, std::recursive_mutex>;
class NetworkInterfaceObserver : public la::avdecc::utils::Observer<NetworkInterfaceMonitor>
{
public:
	virtual ~NetworkInterfaceObserver() noexcept {}

	/** Called when an Interface was added */
	virtual void onInterfaceAdded(la::avdecc::networkInterface::Interface const& intfc) noexcept = 0;
	/** Called when an Interface was removed */
	virtual void onInterfaceRemoved(la::avdecc::networkInterface::Interface const& intfc) noexcept = 0;
	/** Called when the isEnabled field of the specified Interface changed */
	virtual void onInterfaceEnabledStateChanged(la::avdecc::networkInterface::Interface const& intfc, bool const isEnabled) noexcept = 0;
	/** Called when the isConnected field of the specified Interface changed */
	virtual void onInterfaceConnectedStateChanged(la::avdecc::networkInterface::Interface const& intfc, bool const isConnected) noexcept = 0;
	/** Called when the alias field of the specified Interface changed */
	virtual void onInterfaceAliasChanged(la::avdecc::networkInterface::Interface const& intfc, std::string const& alias) noexcept = 0;
	/** Called when the ipAddressInfos field of the specified Interface changed */
	virtual void onInterfaceIPAddressInfosChanged(la::avdecc::networkInterface::Interface const& intfc, la::avdecc::networkInterface::Interface::IPAddressInfos const& ipAddressInfos) noexcept = 0;
	/** Called when the gateways field of the specified Interface changed */
	virtual void onInterfaceGateWaysChanged(la::avdecc::networkInterface::Interface const& intfc, la::avdecc::networkInterface::Interface::Gateways const& gateways) noexcept = 0;
};

using EnumerateInterfacesHandler = std::function<void(la::avdecc::networkInterface::Interface const&)>;

/** Enumerates network interfaces. The specified handler is called for each found interface */
LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION enumerateInterfaces(EnumerateInterfacesHandler const& onInterface) noexcept;
/** Retrieve a copy of an interface from it's name. Throws std::invalid_argument if no interface exists with that name. */
LA_AVDECC_API Interface LA_AVDECC_CALL_CONVENTION getInterfaceByName(std::string const& name);
/** Converts the specified MAC address to string (in the form: xx:xx:xx:xx:xx:xx, or any chosen separator which can be empty if \0 is given) */
LA_AVDECC_API std::string LA_AVDECC_CALL_CONVENTION macAddressToString(MacAddress const& macAddress, bool const upperCase = true, char const separator = ':') noexcept;
/** Converts the string representation of a MAC address to a MacAddress (from the form: xx:xx:xx:xx:xx:xx or XX:XX:XX:XX:XX:XX, or any chosen separator which can be empty if \0 is given) */
LA_AVDECC_API MacAddress LA_AVDECC_CALL_CONVENTION stringToMacAddress(std::string const& macAddressAsString, char const separator = ':'); // Throws std::invalid_argument if the string cannot be parsed
/** Returns true if specified MAC address is valid */
LA_AVDECC_API bool LA_AVDECC_CALL_CONVENTION isMacAddressValid(MacAddress const& macAddress) noexcept;
/** Registers an observer to monitor changes in network interfaces. NetworkInterfaceObserver::onInterfaceAdded will be called before returning from the call, for all already discovered interfaces. */
LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION registerObserver(NetworkInterfaceObserver* const observer) noexcept;
/** Unregisters a previously registered network interfaces change observer */
LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION unregisterObserver(NetworkInterfaceObserver* const observer) noexcept;

} // namespace networkInterface
} // namespace avdecc
} // namespace la
