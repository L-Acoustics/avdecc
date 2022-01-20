/*
* Copyright (C) 2016-2022, L-Acoustics and its contributors

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
* @file discoveryStateMachine.hpp
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

class DiscoveryStateMachine final
{
public:
	static constexpr auto DefaultDiscoverySendDelay = std::chrono::milliseconds{ 10000u }; // Default delay between 2 DISCOVER message broadcast

	class Delegate
	{
	public:
		virtual ~Delegate() noexcept = default;
		virtual void onLocalEntityOnline(la::avdecc::entity::Entity const& entity) noexcept = 0;
		virtual void onLocalEntityOffline(la::avdecc::UniqueIdentifier const entityID) noexcept = 0;
		virtual void onLocalEntityUpdated(la::avdecc::entity::Entity const& entity) noexcept = 0;
		virtual void onRemoteEntityOnline(la::avdecc::entity::Entity const& entity) noexcept = 0;
		virtual void onRemoteEntityOffline(la::avdecc::UniqueIdentifier const entityID) noexcept = 0;
		virtual void onRemoteEntityUpdated(la::avdecc::entity::Entity const& entity) noexcept = 0;
	};

	DiscoveryStateMachine(Manager* manager, Delegate* const delegate) noexcept;
	~DiscoveryStateMachine() noexcept;

	void setDiscoveryDelay(std::chrono::milliseconds const delay = DefaultDiscoverySendDelay) noexcept; // 0 as delay means never send automatic DISCOVER messages
	void discoverMessageSent() noexcept;
	void checkRemoteEntitiesTimeoutExpiracy() noexcept;
	void checkDiscovery() noexcept;
	void handleAdpEntityAvailable(Adpdu const& adpdu) noexcept;
	void handleAdpEntityDeparting(Adpdu const& adpdu) noexcept;
	void notifyDiscoveredRemoteEntities(Delegate& delegate) const noexcept;

private:
	// Private types
	enum class EntityUpdateAction
	{
		NoNotify = 0, /**< Ne need to notify upper layers, the change(s) are only for the DiscoveryStateMachine to interpret */
		NotifyUpdate = 1, /**< Upper layers shall be notified of change(s) in the entity */
		NotifyOfflineOnline = 2, /**< An invalid change in consecutive ADPDUs has been detecter, upper layers will be notified through Offline/Online simulation calls */
	};
	struct DiscoveredEntityInfo
	{
		entity::Entity entity{ {}, {} };
		std::unordered_map<entity::model::AvbInterfaceIndex, std::chrono::time_point<std::chrono::steady_clock>> timeouts{};
	};
	using DiscoveredEntities = std::unordered_map<UniqueIdentifier, DiscoveredEntityInfo, UniqueIdentifier::hash>;

	// Private methods
	entity::Entity makeEntity(Adpdu const& adpdu) const noexcept;
	EntityUpdateAction updateEntity(entity::Entity& entity, entity::Entity&& newEntity) const noexcept;

	// Private members
	Manager* _manager{ nullptr };
	Delegate* _delegate{ nullptr };
	DiscoveredEntities _discoveredEntities{};
	std::chrono::milliseconds _discoveryDelay{};
	std::chrono::time_point<std::chrono::steady_clock> _lastDiscovery{ std::chrono::steady_clock::now() };
};

} // namespace stateMachine
} // namespace protocol
} // namespace avdecc
} // namespace la
