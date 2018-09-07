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
* @file endStationImpl.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/internals/protocolInterface.hpp"
#include "endStationImpl.hpp"

namespace la
{
namespace avdecc
{

EndStationImpl::EndStationImpl(protocol::ProtocolInterface::UniquePointer&& protocolInterface) noexcept
	: _protocolInterface(std::move(protocolInterface))
{
}

EndStationImpl::~EndStationImpl() noexcept
{
	// Remove all entities before shuting down the protocol interface (so they have a chance to send a ENTITY_DEPARTING message)
	_entities.clear();
	// Shutdown protocolInterface now
	_protocolInterface->shutdown();
}

// EndStation overrides
entity::ControllerEntity* EndStationImpl::addControllerEntity(std::uint16_t const progID, UniqueIdentifier const entityModelID, entity::ControllerEntity::Delegate* const delegate)
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
void EndStationImpl::destroy() noexcept
{
	delete this;
}

/** EndStation Entry point */
EndStation* LA_AVDECC_CALL_CONVENTION EndStation::createRawEndStation(protocol::ProtocolInterface::Type const protocolInterfaceType, std::string const& networkInterfaceName)
{
	try
	{
		return new EndStationImpl(protocol::ProtocolInterface::create(protocolInterfaceType, networkInterfaceName));
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
			case protocol::ProtocolInterface::Error::InvalidParameters:
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
