/*
* Copyright (C) 2016-2023, L-Acoustics and its contributors

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
* @file protocolInterface.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/internals/protocolInterface.hpp"
#include "la/avdecc/executor.hpp"

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
#ifdef HAVE_PROTOCOL_INTERFACE_SERIAL
#	include "protocolInterface/protocolInterface_serial.hpp"
#endif // HAVE_PROTOCOL_INTERFACE_SERIAL
#ifdef HAVE_PROTOCOL_INTERFACE_LOCAL
#	include "protocolInterface/protocolInterface_local.hpp"
#endif // HAVE_PROTOCOL_INTERFACE_LOCAL

namespace la
{
namespace avdecc
{
namespace protocol
{
// Throws an Exception if networkInterfaceName is not usable
ProtocolInterface::ProtocolInterface(std::string const& networkInterfaceName, std::string const& executorName)
	: _networkInterfaceName(networkInterfaceName)
	, _executorName{ executorName }
{
	// Check if the executor exists
	if (!ExecutorManager::getInstance().isExecutorRegistered(_executorName))
	{
		throw Exception(Error::ExecutorNotInitialized, "The receive executor '" + std::string{ _executorName } + "' is not registered");
	}

	try
	{
		auto const intfc = networkInterface::NetworkInterfaceHelper::getInstance().getInterfaceByName(networkInterfaceName);

		// Check we have a valid mac address
		if (!networkInterface::NetworkInterfaceHelper::isMacAddressValid(intfc.macAddress))
			throw Exception(Error::InvalidParameters, "Network interface has an invalid mac address");

		_networkInterfaceMacAddress = intfc.macAddress;
	}
	catch (std::invalid_argument const&)
	{
		throw Exception(Error::InterfaceNotFound, "No interface found with specified name");
	}
}

ProtocolInterface::ProtocolInterface(std::string const& networkInterfaceName, networkInterface::MacAddress const& macAddress, std::string const& executorName)
	: _networkInterfaceName(networkInterfaceName)
	, _networkInterfaceMacAddress(macAddress)
	, _executorName{ executorName }
{
	// Check if the executor exists
	if (!ExecutorManager::getInstance().isExecutorRegistered(_executorName))
	{
		throw Exception(Error::ExecutorNotInitialized, "The receive executor '" + std::string{ _executorName } + "' is not registered");
	}

	// Check we have a valid name
	if (networkInterfaceName.empty())
		throw Exception(Error::InvalidParameters, "Network interface name should not be empty");

	// Check we have a valid mac address
	if (!networkInterface::NetworkInterfaceHelper::isMacAddressValid(macAddress))
		throw Exception(Error::InvalidParameters, "Network interface has an invalid mac address");
}

std::string const& LA_AVDECC_CALL_CONVENTION ProtocolInterface::getExecutorName() const noexcept
{
	return _executorName;
}

networkInterface::MacAddress const& LA_AVDECC_CALL_CONVENTION ProtocolInterface::getMacAddress() const noexcept
{
	return _networkInterfaceMacAddress;
}

ProtocolInterface::Error LA_AVDECC_CALL_CONVENTION ProtocolInterface::registerVendorUniqueDelegate(VuAecpdu::ProtocolIdentifier const& protocolIdentifier, VendorUniqueDelegate* const delegate) noexcept
{
	// Lock
	auto const lg = std::lock_guard{ *this };

	_vendorUniqueDelegates[protocolIdentifier] = delegate;

	return Error::NoError;
}

ProtocolInterface::Error LA_AVDECC_CALL_CONVENTION ProtocolInterface::unregisterVendorUniqueDelegate(VuAecpdu::ProtocolIdentifier const& protocolIdentifier) noexcept
{
	// Lock
	auto const lg = std::lock_guard{ *this };

	_vendorUniqueDelegates.erase(protocolIdentifier);

	return Error::NoError;
}

ProtocolInterface::Error LA_AVDECC_CALL_CONVENTION ProtocolInterface::unregisterAllVendorUniqueDelegates() noexcept
{
	// Lock
	auto const lg = std::lock_guard{ *this };

	_vendorUniqueDelegates.clear();

	return Error::NoError;
}

bool ProtocolInterface::isAecpResponseMessageType(AecpMessageType const messageType) noexcept
{
	if (messageType == protocol::AecpMessageType::AemResponse || messageType == protocol::AecpMessageType::AddressAccessResponse || messageType == protocol::AecpMessageType::AvcResponse || messageType == protocol::AecpMessageType::VendorUniqueResponse || messageType == protocol::AecpMessageType::HdcpAemResponse || messageType == protocol::AecpMessageType::ExtendedResponse)
		return true;
	return false;
}

std::uint32_t ProtocolInterface::getVuAecpCommandTimeout(VuAecpdu::ProtocolIdentifier const& protocolIdentifier, VuAecpdu const& aecpdu) const noexcept
{
	auto timeout = std::uint32_t{ 250u };

	// Lock
	auto const lg = std::lock_guard{ *this };

	if (auto const vudIt = _vendorUniqueDelegates.find(protocolIdentifier); vudIt != _vendorUniqueDelegates.end())
	{
		auto* vuDelegate = vudIt->second;

		AVDECC_ASSERT(vuDelegate->areHandledByControllerStateMachine(protocolIdentifier), "getVuAecpCommandTimeout should only be called for VendorUniqueDelegates that let the ControllerStateMachine handle sending commands");
		timeout = vuDelegate->getVuAecpCommandTimeoutMsec(protocolIdentifier, aecpdu);
	}

	return timeout;
}

ProtocolInterface::VendorUniqueDelegate* ProtocolInterface::getVendorUniqueDelegate(VuAecpdu::ProtocolIdentifier const& protocolIdentifier) const noexcept
{
	// Lock
	auto const lg = std::lock_guard{ *this };

	auto const vudIt = _vendorUniqueDelegates.find(protocolIdentifier);
	if (vudIt == _vendorUniqueDelegates.end())
	{
		return nullptr;
	}

	return vudIt->second;
}

ProtocolInterface* LA_AVDECC_CALL_CONVENTION ProtocolInterface::createRawProtocolInterface(Type const protocolInterfaceType, std::string const& networkInterfaceName, std::string const& executorName)
{
	if (!isSupportedProtocolInterfaceType(protocolInterfaceType))
		throw Exception(Error::InterfaceNotSupported, "Selected protocol interface type not supported");

	switch (protocolInterfaceType)
	{
#if defined(HAVE_PROTOCOL_INTERFACE_PCAP)
		case Type::PCap:
			return ProtocolInterfacePcap::createRawProtocolInterfacePcap(networkInterfaceName, executorName);
#endif // HAVE_PROTOCOL_INTERFACE_PCAP
#if defined(HAVE_PROTOCOL_INTERFACE_MAC)
		case Type::MacOSNative:
			return ProtocolInterfaceMacNative::createRawProtocolInterfaceMacNative(networkInterfaceName, executorName);
#endif // HAVE_PROTOCOL_INTERFACE_MAC
#if defined(HAVE_PROTOCOL_INTERFACE_PROXY)
		case Type::Proxy:
			AVDECC_ASSERT(false, "TODO: Proxy protocol interface to create");
			break;
#endif // HAVE_PROTOCOL_INTERFACE_PROXY
#if defined(HAVE_PROTOCOL_INTERFACE_VIRTUAL)
		case Type::Virtual:
			return ProtocolInterfaceVirtual::createRawProtocolInterfaceVirtual(networkInterfaceName, { { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 } }, executorName);
#endif // HAVE_PROTOCOL_INTERFACE_VIRTUAL
#if defined(HAVE_PROTOCOL_INTERFACE_SERIAL)
		case Type::Serial:
			return ProtocolInterfaceSerial::createRawProtocolInterfaceSerial(networkInterfaceName, executorName);
#endif // HAVE_PROTOCOL_INTERFACE_SERIAL
#if defined(HAVE_PROTOCOL_INTERFACE_LOCAL)
		case Type::Local:
			return ProtocolInterfaceLocal::createRawProtocolInterfaceLocal(networkInterfaceName, executorName);
#endif // HAVE_PROTOCOL_INTERFACE_LOCAL
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
		case Type::Serial:
			return "Serial port interface";
		case Type::Local:
			return "Local domain socket interface";
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

		// Serial
#if defined(HAVE_PROTOCOL_INTERFACE_SERIAL)
		if (protocol::ProtocolInterfaceSerial::isSupported())
		{
			s_supportedProtocolInterfaceTypes.set(Type::Serial);
		}
#endif // HAVE_PROTOCOL_INTERFACE_SERIAL

		// Local domain socket
#if defined(HAVE_PROTOCOL_INTERFACE_LOCAL)
		if (protocol::ProtocolInterfaceLocal::isSupported())
		{
			s_supportedProtocolInterfaceTypes.set(Type::Local);
		}
#endif // HAVE_PROTOCOL_INTERFACE_LOCAL
	}

	return s_supportedProtocolInterfaceTypes;
}

} // namespace protocol
} // namespace avdecc
} // namespace la
