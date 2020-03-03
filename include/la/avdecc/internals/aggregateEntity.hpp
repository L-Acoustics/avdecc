/*
* Copyright (C) 2016-2020, L-Acoustics and its contributors

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
	using UniquePointer = std::unique_ptr<AggregateEntity, void (*)(AggregateEntity*)>;

	/**
	* @brief Factory method to create a new AggregateEntity.
	* @details Creates a new AggregateEntity as a unique pointer.
	* @param[in] protocolInterface The protocol interface to bind the entity to.
	* @param[in] commonInformation Common information for this aggregate entity.
	* @param[in] interfacesInformation All interfaces information for this aggregate entity.
	* @param[in] delegate The Delegate to be called whenever a controller related notification occurs.
	* @return A new AggregateEntity as a Entity::UniquePointer.
	* @note Might throw an Exception.
	*/
	static UniquePointer create(protocol::ProtocolInterface* const protocolInterface, CommonInformation const& commonInformation, InterfacesInformation const& interfacesInformation, controller::Delegate* const controllerDelegate)
	{
		auto deleter = [](AggregateEntity* self)
		{
			self->destroy();
		};
		return UniquePointer(createRawAggregateEntity(protocolInterface, commonInformation, interfacesInformation, controllerDelegate), deleter);
	}

	/* Discovery Protocol (ADP) */
	/** Enables entity advertising with available duration included between 2-62 seconds on the specified interfaceIndex if set, otherwise on all interfaces. Returns false if EntityID is already in use on the local computer, true otherwise. */
	using LocalEntity::enableEntityAdvertising;
	/** Disables entity advertising on the specified interfaceIndex if set, otherwise on all interfaces. */
	using LocalEntity::disableEntityAdvertising;

	/* Other methods */
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
	AggregateEntity(CommonInformation const& commonInformation, InterfacesInformation const& interfacesInformation);

	/** Destructor */
	virtual ~AggregateEntity() noexcept = default;

private:
	/** Entry point */
	static LA_AVDECC_API AggregateEntity* LA_AVDECC_CALL_CONVENTION createRawAggregateEntity(protocol::ProtocolInterface* const protocolInterface, CommonInformation const& commonInformation, InterfacesInformation const& interfacesInformation, controller::Delegate* const controllerDelegate);

	/** Destroy method for COM-like interface */
	virtual void destroy() noexcept = 0;
};

} // namespace entity
} // namespace avdecc
} // namespace la
