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
* @file endStationImpl.hpp
* @author Christophe Calmejane
*/

#pragma once

#include <memory>
#include <vector>

// End Station
#include "la/avdecc/internals/endStation.hpp"

// Protocol Interface
#include "protocolInterface/protocolInterface.hpp"

// Entities
#include "entity/controllerEntityImpl.hpp"

namespace la
{
namespace avdecc
{

class EndStationImpl final : public EndStation
{
public:
	EndStationImpl(protocol::ProtocolInterface::UniquePointer&& protocolInterface) noexcept;
	~EndStationImpl() noexcept;

	/** ProtocolInterface creation helper method. Might throw a EndStation::Exception */
	static protocol::ProtocolInterface::UniquePointer createProtocolInterface(ProtocolInterfaceType const protocolInterfaceType, std::string const& networkInterfaceName);

	// EndStation overrides
	virtual entity::ControllerEntity* addControllerEntity(std::uint16_t const progID, UniqueIdentifier const entityModelID, entity::ControllerEntity::Delegate* const delegate) override;

	/** Destroy method for COM-like interface */
	virtual void destroy() noexcept override;

private:
	protocol::ProtocolInterface::UniquePointer const _protocolInterface{ nullptr };
	std::vector<entity::Entity::UniquePointer> _entities{};
};

} // namespace avdecc
} // namespace la
