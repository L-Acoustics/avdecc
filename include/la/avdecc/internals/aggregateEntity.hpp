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
* @file aggregateEntity.hpp
* @author Christophe Calmejane
* @brief Avdecc aggregate entity (supporting multiple types for the same EntityID).
*/

#pragma once

#include "la/avdecc/memoryBuffer.hpp"
#include "entity.hpp"
#include "entityModel.hpp"
#include "entityAddressAccessTypes.hpp"
#include "controllerEntity.hpp"
#include "exports.hpp"
#include <thread>
#include <unordered_map>
#include <string>
#include <vector>
#include <functional>

namespace la
{
namespace avdecc
{
namespace entity
{
class AggregateEntity : public LocalEntity, public controller::Interface
{
public:
	/* Discovery Protocol (ADP) */
	/** Enables entity advertising with available duration included between 2-62 seconds, defaulting to 62. */
	using LocalEntity::enableEntityAdvertising;
	/** Disables entity advertising. */
	using LocalEntity::disableEntityAdvertising;

	virtual void setControllerDelegate(controller::Delegate* const delegate) noexcept = 0;
	//virtual void setListenerDelegate(listener::Delegate* const delegate) noexcept = 0;
	//virtual void setTalkerDelegate(talker::Delegate* const delegate) noexcept = 0;

	// Deleted compiler auto-generated methods
	AggregateEntity(AggregateEntity&&) = delete;
	AggregateEntity(AggregateEntity const&) = delete;
	AggregateEntity& operator=(AggregateEntity const&) = delete;
	AggregateEntity& operator=(AggregateEntity&&) = delete;

protected:
	/** Constructor */
	AggregateEntity(UniqueIdentifier const entityID, networkInterface::MacAddress const& macAddress, UniqueIdentifier const entityModelID, EntityCapabilities const entityCapabilities, std::uint16_t const talkerStreamSources, TalkerCapabilities const talkerCapabilities, std::uint16_t const listenerStreamSinks, ListenerCapabilities const listenerCapabilities, ControllerCapabilities const controllerCapabilities, std::uint16_t const identifyControlIndex, std::uint16_t const interfaceIndex, UniqueIdentifier const associationID) noexcept;

	/** Destructor */
	virtual ~AggregateEntity() noexcept = default;
};

} // namespace entity
} // namespace avdecc
} // namespace la
