/*
* Copyright (C) 2016-2018, L-Acoustics and its contributors

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

/**
* @file protocolInterface.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/internals/protocolInterface.hpp"

// Protocol Interface
#ifdef HAVE_PROTOCOL_INTERFACE_PCAP
#	include "protocolInterface/protocolInterface_pcap.hpp"
#endif // HAVE_PROTOCOL_INTERFACE_PCAP
#ifdef HAVE_PROTOCOL_INTERFACE_MAC
#	include "protocolInterface/protocolInterface_macNative.hpp"
#endif // HAVE_PROTOCOL_INTERFACE_MAC
#ifdef HAVE_PROTOCOL_INTERFACE_PROXY
#	error "Not implemented yet"
#	include "protocolInterface/protocolInterface_proxy.hpp"
#endif // HAVE_PROTOCOL_INTERFACE_PROXY
#ifdef HAVE_PROTOCOL_INTERFACE_VIRTUAL
#	include "protocolInterface/protocolInterface_virtual.hpp"
#endif // HAVE_PROTOCOL_INTERFACE_VIRTUAL

namespace la
{
namespace avdecc
{
namespace protocol
{
// Throws an Exception if networkInterfaceName is not usable
ProtocolInterface::ProtocolInterface(std::string const& networkInterfaceName)
	: _networkInterfaceName(networkInterfaceName)
{
	try
	{
		auto const interface = la::avdecc::networkInterface::getInterfaceByName(networkInterfaceName);

		// Check we have a valid mac address
		if (!la::avdecc::networkInterface::isMacAddressValid(interface.macAddress))
			throw Exception(Error::InvalidParameters, "Network interface has an invalid mac address");

		_networkInterfaceMacAddress = interface.macAddress;
	}
	catch (la::avdecc::Exception const&)
	{
		throw Exception(Error::InterfaceNotFound, "No interface found with specified name");
	}
}

ProtocolInterface::ProtocolInterface(std::string const& networkInterfaceName, networkInterface::MacAddress const& macAddress)
	: _networkInterfaceName(networkInterfaceName)
	, _networkInterfaceMacAddress(macAddress)
{
	// Check we have a valid name
	if (networkInterfaceName.empty())
		throw Exception(Error::InvalidParameters, "Network interface name should not be empty");

	// Check we have a valid mac address
	if (!la::avdecc::networkInterface::isMacAddressValid(macAddress))
		throw Exception(Error::InvalidParameters, "Network interface has an invalid mac address");
}


la::avdecc::networkInterface::MacAddress const& ProtocolInterface::getMacAddress() const noexcept
{
	return _networkInterfaceMacAddress;
}

ProtocolInterface* LA_AVDECC_CALL_CONVENTION ProtocolInterface::createRawProtocolInterface(Type const protocolInterfaceType, std::string const& networkInterfaceName)
{
	if (!isSupportedProtocolInterfaceType(protocolInterfaceType))
		throw Exception(Error::InterfaceNotSupported, "Selected protocol interface type not supported");

	switch (protocolInterfaceType)
	{
#if defined(HAVE_PROTOCOL_INTERFACE_PCAP)
		case Type::PCap:
			return ProtocolInterfacePcap::createRawProtocolInterfacePcap(networkInterfaceName);
#endif // HAVE_PROTOCOL_INTERFACE_PCAP
#if defined(HAVE_PROTOCOL_INTERFACE_MAC)
		case Type::MacOSNative:
			return ProtocolInterfaceMacNative::createRawProtocolInterfaceMacNative(networkInterfaceName);
#endif // HAVE_PROTOCOL_INTERFACE_MAC
#if defined(HAVE_PROTOCOL_INTERFACE_PROXY)
		case Type::Proxy:
			AVDECC_ASSERT(false, "TODO: Proxy protocol interface to create");
			break;
#endif // HAVE_PROTOCOL_INTERFACE_PROXY
#if defined(HAVE_PROTOCOL_INTERFACE_VIRTUAL)
		case Type::Virtual:
			return ProtocolInterfaceVirtual::createRawProtocolInterfaceVirtual(networkInterfaceName, { { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 } });
#endif // HAVE_PROTOCOL_INTERFACE_VIRTUAL
		default:
			break;
	}

	throw Exception(Error::InterfaceNotSupported, "Unknown protocol interface type");
}

bool LA_AVDECC_CALL_CONVENTION ProtocolInterface::isSupportedProtocolInterfaceType(Type const protocolInterfaceType) noexcept
{
	auto const types = getSupportedProtocolInterfaceTypes();

	return types.test(protocolInterfaceType);
}

std::string LA_AVDECC_CALL_CONVENTION ProtocolInterface::typeToString(Type const protocolInterfaceType) noexcept
{
	switch (protocolInterfaceType)
	{
		case Type::PCap:
			return "Packet capture (PCap)";
		case Type::MacOSNative:
			return "macOS native";
		case Type::Proxy:
			return "IEEE Std 1722.1 proxy";
		case Type::Virtual:
			return "Virtual interface";
		default:
			return "Unknown protocol interface type";
	}
}

ProtocolInterface::SupportedProtocolInterfaceTypes LA_AVDECC_CALL_CONVENTION ProtocolInterface::getSupportedProtocolInterfaceTypes() noexcept
{
	static SupportedProtocolInterfaceTypes s_supportedProtocolInterfaceTypes{};

	if (s_supportedProtocolInterfaceTypes.empty())
	{
		// PCap
#if defined(HAVE_PROTOCOL_INTERFACE_PCAP)
		if (protocol::ProtocolInterfacePcap::isSupported())
		{
			s_supportedProtocolInterfaceTypes.set(Type::PCap);
		}
#endif // HAVE_PROTOCOL_INTERFACE_PCAP

		// MacOSNative (only supported on macOS)
#if defined(HAVE_PROTOCOL_INTERFACE_MAC)
		if (protocol::ProtocolInterfaceMacNative::isSupported())
		{
			s_supportedProtocolInterfaceTypes.set(Type::MacOSNative);
		}
#endif // HAVE_PROTOCOL_INTERFACE_MAC

		// Proxy
#if defined(HAVE_PROTOCOL_INTERFACE_PROXY)
		if (protocol::ProtocolInterfaceProxy::isSupported())
		{
			s_supportedProtocolInterfaceTypes.set(Type::Proxy);
		}
#endif // HAVE_PROTOCOL_INTERFACE_PROXY

		// Virtual
#if defined(HAVE_PROTOCOL_INTERFACE_VIRTUAL)
		if (protocol::ProtocolInterfaceVirtual::isSupported())
		{
			s_supportedProtocolInterfaceTypes.set(Type::Virtual);
		}
#endif // HAVE_PROTOCOL_INTERFACE_VIRTUAL
	}

	return s_supportedProtocolInterfaceTypes;
}

} // namespace protocol
} // namespace avdecc
} // namespace la
