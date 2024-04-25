/*
* Copyright (C) 2016-2024, L-Acoustics and its contributors

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
* @file advertiseStateMachine.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/utils.hpp"

#include "advertiseStateMachine.hpp"
#include "stateMachineManager.hpp"

#include <utility>
#include <optional>

namespace la
{
namespace avdecc
{
namespace protocol
{
namespace stateMachine
{
/* ************************************************************ */
/* Public methods                                               */
/* ************************************************************ */
AdvertiseStateMachine::AdvertiseStateMachine(Manager* manager, Delegate* const delegate) noexcept
	: _manager(manager)
	, _delegate(delegate)
{
	(void)_delegate; // Silence warning
}

AdvertiseStateMachine::~AdvertiseStateMachine() noexcept {}

void AdvertiseStateMachine::checkLocalEntitiesAnnouncement() noexcept
{
	// Lock
	auto const lg = std::lock_guard{ *_manager };

	auto* const protocolInterface = _manager->getProtocolInterfaceDelegate();

	// Get current time
	auto const now = std::chrono::system_clock::now();

	// Process all Advertised Entities on the attached Protocol Interface
	for (auto& entityKV : _advertisedEntities)
	{
		auto& entityInfo = entityKV.second;
		auto& entity = entityInfo.entity;

		// Lock the whole entity while checking dirty state and building the EntityAvailable message so that nobody alters discovery fields at the same time
		std::lock_guard<entity::LocalEntity> const elg(entity);

		// Send an EntityAvailable message if the advertise timeout expired
		if (now >= entityInfo.nextAdvertiseTime)
		{
			try
			{
				// Build the EntityAvailable message
				auto const frame = Manager::makeEntityAvailableMessage(entity, entityInfo.interfaceIndex);
				// Send it
				protocolInterface->sendMessage(frame);
				// Update the time for next advertise
				entityInfo.nextAdvertiseTime = computeNextAdvertiseTime(entity, entityInfo.interfaceIndex);
			}
			catch (...)
			{
				AVDECC_ASSERT(false, "Should not happen");
			}
		}
	}
}

void AdvertiseStateMachine::setEntityNeedsAdvertise(entity::LocalEntity const& entity) noexcept
{
	// Lock
	auto const lg = std::lock_guard{ *_manager };

	auto const interfaceIndex = _manager->getMatchingInterfaceIndex(entity);
	if (!AVDECC_ASSERT_WITH_RET(interfaceIndex, "Should always have a matching AvbInterfaceIndex when this method is called"))
	{
		return;
	}

	// Check if entity already advertising
	auto const entityID = entity.getEntityID();
	auto const infoIt = _advertisedEntities.find(entityID);
	if (infoIt != _advertisedEntities.end())
	{
		// Schedule EntityAvailable message
		infoIt->second.nextAdvertiseTime = computeDelayedAdvertiseTime(entity, *interfaceIndex);
	}
}

void AdvertiseStateMachine::enableEntityAdvertising(entity::LocalEntity& entity) noexcept
{
	// Lock
	auto const lg = std::lock_guard{ *_manager };

	auto const interfaceIndex = _manager->getMatchingInterfaceIndex(entity);
	if (!AVDECC_ASSERT_WITH_RET(interfaceIndex, "Should always have a matching AvbInterfaceIndex when this method is called"))
	{
		return;
	}

	// Check if entity already advertising
	auto const entityID = entity.getEntityID();
	auto const infoIt = _advertisedEntities.find(entityID);
	if (infoIt == _advertisedEntities.end())
	{
		// Register LocalEntity for Advertising
		_advertisedEntities.emplace(std::make_pair(entityID, AdvertiseEntityInfo{ entity, *interfaceIndex }));
	}
}

void AdvertiseStateMachine::disableEntityAdvertising(entity::LocalEntity const& entity) noexcept
{
	// Lock
	auto const lg = std::lock_guard{ *_manager };

	auto const interfaceIndex = _manager->getMatchingInterfaceIndex(entity);
	if (!AVDECC_ASSERT_WITH_RET(interfaceIndex, "Should always have a matching AvbInterfaceIndex when this method is called"))
	{
		return;
	}

	// Check if entity already advertising
	auto const entityID = entity.getEntityID();
	auto const infoIt = _advertisedEntities.find(entityID);
	if (infoIt != _advertisedEntities.end())
	{
		try
		{
			// Send a departing message
			auto const frame = Manager::makeEntityDepartingMessage(entity, *interfaceIndex);
			_manager->getProtocolInterfaceDelegate()->sendMessage(frame);

			// Unregister LocalEntity from Advertising
			_advertisedEntities.erase(infoIt);
		}
		catch (...)
		{
			AVDECC_ASSERT(false, "Should not happen");
		}
	}
}

void AdvertiseStateMachine::handleAdpEntityDiscover(Adpdu const& adpdu) noexcept
{
	auto const entityID = adpdu.getEntityID();

	// Don't ignore requests coming from the same computer, we might have another controller running on it

	// Check if one (or many) of our local entities is targetted by the discover request
	// Lock
	auto const lg = std::lock_guard{ *_manager };

	for (auto& entityKV : _advertisedEntities)
	{
		auto& entityInfo = entityKV.second;
		auto const& entity = entityInfo.entity;

		// Only reply to global (entityID == 0) discovery messages and to targeted ones
		if (!entityID || entityID == entity.getEntityID())
		{
			// Schedule EntityAvailable message
			entityInfo.nextAdvertiseTime = computeDelayedAdvertiseTime(entity, entityInfo.interfaceIndex);
		}
	}
}

/* ************************************************************ */
/* Private methods                                              */
/* ************************************************************ */
std::chrono::milliseconds AdvertiseStateMachine::computeRandomDelay(entity::Entity const& entity, entity::model::AvbInterfaceIndex const interfaceIndex) const noexcept
{
	auto const& interfaceInfo = entity.getInterfaceInformation(interfaceIndex);
	auto const maxRandValue{ interfaceInfo.validTime * 1000u * 2u / 5u }; // Maximum value for rand range is 1/5 of the "valid time period" (which is twice the validTime field)
	auto const randomValue{ std::rand() % maxRandValue };
	return std::chrono::milliseconds(randomValue);
}

std::chrono::time_point<std::chrono::system_clock> AdvertiseStateMachine::computeNextAdvertiseTime(entity::Entity const& entity, entity::model::AvbInterfaceIndex const interfaceIndex) const
{
	auto const& interfaceInfo = entity.getInterfaceInformation(interfaceIndex);
	return std::chrono::system_clock::now() + std::chrono::milliseconds(std::max(1000u, interfaceInfo.validTime * 1000u / 2u)) + computeRandomDelay(entity, interfaceIndex);
}

std::chrono::time_point<std::chrono::system_clock> AdvertiseStateMachine::computeDelayedAdvertiseTime(entity::Entity const& entity, entity::model::AvbInterfaceIndex const interfaceIndex) const
{
	return std::chrono::system_clock::now() + computeRandomDelay(entity, interfaceIndex);
}


} // namespace stateMachine
} // namespace protocol
} // namespace avdecc
} // namespace la
