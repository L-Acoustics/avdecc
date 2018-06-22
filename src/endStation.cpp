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
* @file endStation.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/internals/endStation.hpp"
#include "la/avdecc/internals/controllerEntity.hpp"
#include <memory>
#include <vector>

// Protocol Interface
#ifdef HAVE_PROTOCOL_INTERFACE_PCAP
#include "protocolInterface/protocolInterface_pcap.hpp"
#endif // HAVE_PROTOCOL_INTERFACE_PCAP
#ifdef HAVE_PROTOCOL_INTERFACE_MAC
#include "protocolInterface/protocolInterface_macNative.hpp"
#endif // HAVE_PROTOCOL_INTERFACE_MAC
#ifdef HAVE_PROTOCOL_INTERFACE_PROXY
#error "Not implemented yet"
#include "protocolInterface/protocolInterface_proxy.hpp"
#endif // HAVE_PROTOCOL_INTERFACE_PROXY
#ifdef HAVE_PROTOCOL_INTERFACE_VIRTUAL
#include "protocolInterface/protocolInterface_virtual.hpp"
#endif // HAVE_PROTOCOL_INTERFACE_VIRTUAL

// Entities
#include "entity/controllerEntityImpl.hpp"

namespace la
{
namespace avdecc
{

class EndStationImpl final : public EndStation
{
public:
	EndStationImpl(protocol::ProtocolInterface::UniquePointer&& protocolInterface) noexcept
		: _protocolInterface(std::move(protocolInterface))
	{
	}

	~EndStationImpl() noexcept
	{
		// Remove all entities before shuting down the protocol interface (so they have a chance to send a ENTITY_DEPARTING message)
		_entities.clear();
		// Shutdown protocolInterface now
		_protocolInterface->shutdown();
	}

	// EndStation overrides
	virtual entity::ControllerEntity* addControllerEntity(std::uint16_t const progID, UniqueIdentifier const entityModelID, entity::ControllerEntity::Delegate* const delegate) override
	{
		std::unique_ptr<entity::LocalEntityGuard<entity::ControllerEntityImpl>> controller{ nullptr };
		try
		{
			controller = std::make_unique<entity::LocalEntityGuard<entity::ControllerEntityImpl>>(_protocolInterface.get(), progID, entityModelID, delegate);
		}
		catch (la::avdecc::Exception const& e) // Because entity::ControllerEntityImpl::ControllerEntityImpl might throw if an entityID cannot be generated
		{
			throw Exception(Error::InterfaceInvalid, e.what());
		}

		// Get controller's pointer now, we'll soon move the object
		auto* const controllerPtr = static_cast<entity::ControllerEntity*>(controller.get());

		// Add the entity to our list
		_entities.push_back(std::move(controller));

		// Return the controller to the user
		return controllerPtr;
	}

	/** Destroy method for COM-like interface */
	virtual void destroy() noexcept override;

private:
	protocol::ProtocolInterface::UniquePointer const _protocolInterface{ nullptr };
	std::vector<entity::Entity::UniquePointer> _entities{};
};

/** Destroy method for COM-like interface */
void EndStationImpl::destroy() noexcept
{
	delete this;
}

/** EndStation static methods */
bool LA_AVDECC_CALL_CONVENTION EndStation::isSupportedProtocolInterfaceType(ProtocolInterfaceType const protocolInterfaceType) noexcept
{
	auto const types = getSupportedProtocolInterfaceTypes();

	return std::find_if(types.begin(), types.end(), [protocolInterfaceType](decltype(types)::value_type const type)
	{
		return type == protocolInterfaceType;
	}) != types.end();
}

/** Returns the name of the specified protocol interface type. */
std::string LA_AVDECC_CALL_CONVENTION EndStation::typeToString(ProtocolInterfaceType const protocolInterfaceType) noexcept
{
	switch (protocolInterfaceType)
	{
		case ProtocolInterfaceType::PCap:
			return "Packet capture (PCap)";
		case ProtocolInterfaceType::MacOSNative:
			return "macOS native";
		case ProtocolInterfaceType::Proxy:
			return "IEEE Std 1722.1 proxy";
		case ProtocolInterfaceType::Virtual:
			return "Virtual interface";
		default:
			return "Unknown protocol interface type";
	}
}

EndStation::SupportedProtocolInterfaceTypes LA_AVDECC_CALL_CONVENTION EndStation::getSupportedProtocolInterfaceTypes() noexcept
{
	static SupportedProtocolInterfaceTypes s_supportedProtocolInterfaceTypes{};

	if (s_supportedProtocolInterfaceTypes.empty())
	{
		// PCap
#if defined(HAVE_PROTOCOL_INTERFACE_PCAP)
		if (protocol::ProtocolInterfacePcap::isSupported())
		{
			s_supportedProtocolInterfaceTypes.push_back(ProtocolInterfaceType::PCap);
		}
#endif // HAVE_PROTOCOL_INTERFACE_PCAP

		// MacOSNative (only supported on macOS)
#if defined(HAVE_PROTOCOL_INTERFACE_MAC)
		if (protocol::ProtocolInterfaceMacNative::isSupported())
		{
			s_supportedProtocolInterfaceTypes.push_back(ProtocolInterfaceType::MacOSNative);
		}
#endif // HAVE_PROTOCOL_INTERFACE_MAC

			// Proxy
#if defined(HAVE_PROTOCOL_INTERFACE_PROXY)
		if (protocol::ProtocolInterfaceProxy::isSupported())
		{
			s_supportedProtocolInterfaceTypes.push_back(ProtocolInterfaceType::Proxy);
		}
#endif // HAVE_PROTOCOL_INTERFACE_PROXY

		// Virtual
#if defined(HAVE_PROTOCOL_INTERFACE_VIRTUAL)
		if (protocol::ProtocolInterfaceVirtual::isSupported())
		{
			s_supportedProtocolInterfaceTypes.push_back(ProtocolInterfaceType::Virtual);
		}
#endif // HAVE_PROTOCOL_INTERFACE_VIRTUAL
	}

	return s_supportedProtocolInterfaceTypes;
}

/** EndStation Entry point */
EndStation* LA_AVDECC_CALL_CONVENTION EndStation::createRawEndStation(ProtocolInterfaceType const protocolInterfaceType, std::string const& networkInterfaceName)
{
	if (!isSupportedProtocolInterfaceType(protocolInterfaceType))
		throw Exception(Error::InvalidProtocolInterfaceType, "Selected protocol interface type not supported");

	try
	{
		protocol::ProtocolInterface::UniquePointer pi{ nullptr };

		switch (protocolInterfaceType)
		{
#if defined(HAVE_PROTOCOL_INTERFACE_PCAP)
			case ProtocolInterfaceType::PCap:
				pi = protocol::ProtocolInterfacePcap::create(networkInterfaceName);
				break;
#endif // HAVE_PROTOCOL_INTERFACE_PCAP
#if defined(HAVE_PROTOCOL_INTERFACE_MAC)
			case ProtocolInterfaceType::MacOSNative:
				pi = protocol::ProtocolInterfaceMacNative::create(networkInterfaceName);
				break;
#endif // HAVE_PROTOCOL_INTERFACE_MAC
#if defined(HAVE_PROTOCOL_INTERFACE_PROXY)
			case ProtocolInterfaceType::Proxy:
				AVDECC_ASSERT(false, "TODO: Proxy protocol interface to create");
				break;
#endif // HAVE_PROTOCOL_INTERFACE_PROXY
#if defined(HAVE_PROTOCOL_INTERFACE_VIRTUAL)
			case ProtocolInterfaceType::Virtual:
				pi = protocol::ProtocolInterfaceVirtual::create(networkInterfaceName, { { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 } });
				break;
#endif // HAVE_PROTOCOL_INTERFACE_VIRTUAL
			default:
				throw Exception(Error::InvalidProtocolInterfaceType, "Unknown protocol interface type");
		}

		return new EndStationImpl(std::move(pi));
	}
	catch (protocol::ProtocolInterface::Exception const& e)
	{
		auto const err = e.getError();
		switch (err)
		{
			case protocol::ProtocolInterface::Error::TransportError:
				throw Exception(Error::InterfaceOpenError, e.what());
			case protocol::ProtocolInterface::Error::InterfaceNotFound:
				throw Exception(Error::InterfaceNotFound, e.what());
			case protocol::ProtocolInterface::Error::InterfaceInvalid:
				throw Exception(Error::InterfaceInvalid, e.what());
			case protocol::ProtocolInterface::Error::InterfaceNotSupported:
				throw Exception(Error::InvalidProtocolInterfaceType, e.what());
			default:
				AVDECC_ASSERT(false, "Unhandled exception");
				throw Exception(Error::InternalError, e.what());
		}
	}
}

} // namespace avdecc
} // namespace la
