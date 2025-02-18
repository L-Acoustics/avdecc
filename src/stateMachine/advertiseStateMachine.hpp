/*
* Copyright (C) 2016-2025, L-Acoustics and its contributors

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
* @file advertiseStateMachine.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "la/avdecc/internals/entity.hpp"

#include "protocolInterfaceDelegate.hpp"

#include <chrono>
#include <unordered_map>

namespace la
{
namespace avdecc
{
namespace protocol
{
namespace stateMachine
{
class Manager;

class AdvertiseStateMachine final
{
public:
	class Delegate
	{
	public:
		virtual ~Delegate() noexcept = default;
	};

	AdvertiseStateMachine(Manager* manager, Delegate* const delegate) noexcept;
	~AdvertiseStateMachine() noexcept;

	void checkLocalEntitiesAnnouncement() noexcept;
	void setEntityNeedsAdvertise(entity::LocalEntity const& entity) noexcept;
	void enableEntityAdvertising(entity::LocalEntity& entity) noexcept;
	void disableEntityAdvertising(entity::LocalEntity const& entity) noexcept;
	void handleAdpEntityDiscover(Adpdu const& adpdu) noexcept;

private:
	// Private types
	struct AdvertiseEntityInfo
	{
		entity::LocalEntity& entity;
		entity::model::AvbInterfaceIndex interfaceIndex{ 0u };
		std::chrono::time_point<std::chrono::system_clock> nextAdvertiseTime{};

		/** Constructor */
		AdvertiseEntityInfo(entity::LocalEntity& entity, entity::model::AvbInterfaceIndex const interfaceIndex) noexcept
			: entity(entity)
			, interfaceIndex(interfaceIndex)
		{
		}
	};
	using AdvertisedEntities = std::unordered_map<UniqueIdentifier, AdvertiseEntityInfo, UniqueIdentifier::hash>;

	// Private methods
	std::chrono::milliseconds computeRandomDelay(entity::Entity const& entity, entity::model::AvbInterfaceIndex const interfaceIndex) const noexcept;
	std::chrono::time_point<std::chrono::system_clock> computeNextAdvertiseTime(entity::Entity const& entity, entity::model::AvbInterfaceIndex const interfaceIndex) const;
	std::chrono::time_point<std::chrono::system_clock> computeDelayedAdvertiseTime(entity::Entity const& entity, entity::model::AvbInterfaceIndex const interfaceIndex) const;

	// Private members
	Manager* _manager{ nullptr };
	Delegate* _delegate{ nullptr };
	AdvertisedEntities _advertisedEntities{};
};

} // namespace stateMachine
} // namespace protocol
} // namespace avdecc
} // namespace la
