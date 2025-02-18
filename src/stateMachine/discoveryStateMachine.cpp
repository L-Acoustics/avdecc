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
* @file discoveryStateMachine.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/utils.hpp"

#include "discoveryStateMachine.hpp"
#include "stateMachineManager.hpp"
#include "logHelper.hpp"

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
DiscoveryStateMachine::DiscoveryStateMachine(Manager* manager, Delegate* const delegate) noexcept
	: _manager(manager)
	, _delegate(delegate)
{
}

DiscoveryStateMachine::~DiscoveryStateMachine() noexcept {}

void DiscoveryStateMachine::setDiscoveryDelay(std::chrono::milliseconds const delay) noexcept
{
	_discoveryDelay = delay;
	_lastDiscovery = std::chrono::steady_clock::now();
}

void DiscoveryStateMachine::discoverMessageSent() noexcept
{
	_lastDiscovery = std::chrono::steady_clock::now();
}

ProtocolInterface::Error DiscoveryStateMachine::forgetRemoteEntity(UniqueIdentifier const entityID) noexcept
{
	// Lock
	auto const lg = std::lock_guard{ *_manager };

	// Check if we already know this entity
	auto entityIt = _discoveredEntities.find(entityID);

	// Not found it in the list, return now
	if (entityIt == _discoveredEntities.end())
	{
		return ProtocolInterface::Error::UnknownRemoteEntity;
	}

	// Remove from the list
	_discoveredEntities.erase(entityIt);

	// Notify delegate
	utils::invokeProtectedMethod(&Delegate::onRemoteEntityOffline, _delegate, entityID);

	return ProtocolInterface::Error::NoError;
}

void DiscoveryStateMachine::checkRemoteEntitiesTimeoutExpiracy() noexcept
{
	// Lock
	auto const lg = std::lock_guard{ *_manager };

	// Get current time
	auto const now = std::chrono::steady_clock::now();

	// Process all Discovered Entities on the attached Protocol Interface
	for (auto discoveredEntityKV = _discoveredEntities.begin(); discoveredEntityKV != _discoveredEntities.end(); /* Iterate inside the loop */)
	{
		auto& entity = discoveredEntityKV->second;
		auto hasInterfaceTimeout{ false };
		auto shouldRemoveEntity{ false };

		// Check timeout value for all interfaces
		for (auto timeoutKV = entity.timeouts.begin(); timeoutKV != entity.timeouts.end(); /* Iterate inside the loop */)
		{
			auto const timeout = timeoutKV->second;
			if (now > timeout)
			{
				hasInterfaceTimeout = true;
				entity.entity.removeInterfaceInformation(timeoutKV->first);
				timeoutKV = entity.timeouts.erase(timeoutKV);
			}
			else
			{
				++timeoutKV;
			}
		}

		// We had at least one interface timeout
		if (hasInterfaceTimeout)
		{
			// No more interfaces, set the entity offline
			if (entity.entity.getInterfacesInformation().empty())
			{
				// Notify this entity is offline
				utils::invokeProtectedMethod(&Delegate::onRemoteEntityOffline, _delegate, discoveredEntityKV->first);
				shouldRemoveEntity = true;
			}
			// Otherwise just notify an update
			else
			{
				// Notify this entity has been updated
				utils::invokeProtectedMethod(&Delegate::onRemoteEntityUpdated, _delegate, entity.entity);
			}
		}

		// Should we remove the entity from the list of known entities
		if (shouldRemoveEntity)
		{
			discoveredEntityKV = _discoveredEntities.erase(discoveredEntityKV);
		}
		else
		{
			++discoveredEntityKV;
		}
	}
}

void DiscoveryStateMachine::checkDiscovery() noexcept
{
	if (_discoveryDelay.count() == 0)
	{
		return;
	}

	auto const now = std::chrono::steady_clock::now();

	if (now >= (_lastDiscovery + _discoveryDelay))
	{
		// Update time now so we don't enter the loop again (in case the message is delayed a bit by the ProtocolInterface for any reason)
		// It's up to the ProtocolInterface to call discoverMessageSent() on the manager when the message is actually sent
		_lastDiscovery = now;

		// Ask the ProtocolInterface to process the discover request
		_manager->getProtocolInterface()->discoverRemoteEntities();
	}
}

void DiscoveryStateMachine::handleAdpEntityAvailable(Adpdu const& adpdu) noexcept
{
	// Ignore messages from a local entity
	if (_manager->isLocalEntity(adpdu.getEntityID()))
	{
		return;
	}

	// If entity is not ready
	if (adpdu.getEntityCapabilities().test(entity::EntityCapability::EntityNotReady))
	{
		return;
	}

	auto const entityID = adpdu.getEntityID();
	auto notify = true;
	auto update = false;
	auto simulateOffline = false;
	auto* discoveredInfo = static_cast<DiscoveredEntityInfo*>(nullptr);
	auto entity = makeEntity(adpdu);
	auto const avbInterfaceIndex = entity.getInterfacesInformation().begin()->first;

	// Lock
	auto const lg = std::lock_guard{ *_manager };

	// Check if we already know this entity
	auto entityIt = _discoveredEntities.find(entityID);

	// Found it in the list, check if data are the same
	if (entityIt != _discoveredEntities.end())
	{
		// Merge changes from new entity into current one, and return the action to perform
		auto const action = updateEntity(entityIt->second.entity, std::move(entity));
		discoveredInfo = &entityIt->second;
		notify = action != EntityUpdateAction::NoNotify;
		update = action == EntityUpdateAction::NotifyUpdate;
		simulateOffline = action == EntityUpdateAction::NotifyOfflineOnline;
	}
	// Not found, create a new entity
	else
	{
		// Insert new info and get address of the created Entity so it can be passed to Delegate
		discoveredInfo = &_discoveredEntities.emplace(std::make_pair(entityID, DiscoveredEntityInfo{ std::move(entity) })).first->second;
	}

	// Compute timeout value and always update
	discoveredInfo->timeouts[avbInterfaceIndex] = std::chrono::steady_clock::now() + std::chrono::seconds(2 * adpdu.getValidTime());

	// Notify delegate
	if (notify && _delegate != nullptr)
	{
		if (!AVDECC_ASSERT_WITH_RET(discoveredInfo != nullptr, "discoveredInfo should not be nullptr here"))
		{
			return;
		}
		// Invalid change in entity announcement, simulate offline/online
		if (simulateOffline)
		{
			AVDECC_ASSERT(!update, "When simulateOffline is set, update should not be");
			utils::invokeProtectedMethod(&Delegate::onRemoteEntityOffline, _delegate, entityID);
		}

		if (update)
		{
			utils::invokeProtectedMethod(&Delegate::onRemoteEntityUpdated, _delegate, discoveredInfo->entity);
		}
		else
		{
			utils::invokeProtectedMethod(&Delegate::onRemoteEntityOnline, _delegate, discoveredInfo->entity);
		}
	}
}

void DiscoveryStateMachine::handleAdpEntityDeparting(Adpdu const& adpdu) noexcept
{
	// Ignore messages from a local entity
	if (_manager->isLocalEntity(adpdu.getEntityID()))
		return;

	auto const entityID = adpdu.getEntityID();

	forgetRemoteEntity(entityID);
}

void DiscoveryStateMachine::notifyDiscoveredRemoteEntities(Delegate& delegate) const noexcept
{
	// Lock
	auto const lg = std::lock_guard{ *_manager };

	for (auto const& [entityID, entityInfo] : _discoveredEntities)
	{
		utils::invokeProtectedMethod(&Delegate::onRemoteEntityOnline, &delegate, entityInfo.entity);
	}
}


/* ************************************************************ */
/* Private methods                                              */
/* ************************************************************ */
entity::Entity DiscoveryStateMachine::makeEntity(Adpdu const& adpdu) const noexcept
{
	auto const entityCaps = adpdu.getEntityCapabilities();
	auto controlIndex{ std::optional<entity::model::ControlIndex>{} };
	auto associationID{ std::optional<UniqueIdentifier>{} };
	auto avbInterfaceIndex{ entity::Entity::GlobalAvbInterfaceIndex };
	auto gptpGrandmasterID{ std::optional<UniqueIdentifier>{} };
	auto gptpDomainNumber{ std::optional<std::uint8_t>{} };

	if (entityCaps.test(entity::EntityCapability::AemIdentifyControlIndexValid))
	{
		controlIndex = adpdu.getIdentifyControlIndex();
	}
	if (entityCaps.test(entity::EntityCapability::AssociationIDValid))
	{
		associationID = adpdu.getAssociationID();
	}
	if (entityCaps.test(entity::EntityCapability::AemInterfaceIndexValid))
	{
		avbInterfaceIndex = adpdu.getInterfaceIndex();
	}
	if (entityCaps.test(entity::EntityCapability::GptpSupported))
	{
		gptpGrandmasterID = adpdu.getGptpGrandmasterID();
		gptpDomainNumber = adpdu.getGptpDomainNumber();
	}

	auto const commonInfo{ entity::Entity::CommonInformation{ adpdu.getEntityID(), adpdu.getEntityModelID(), entityCaps, adpdu.getTalkerStreamSources(), adpdu.getTalkerCapabilities(), adpdu.getListenerStreamSinks(), adpdu.getListenerCapabilities(), adpdu.getControllerCapabilities(), controlIndex, associationID } };
	auto const interfaceInfo{ entity::Entity::InterfaceInformation{ adpdu.getSrcAddress(), adpdu.getValidTime(), adpdu.getAvailableIndex(), gptpGrandmasterID, gptpDomainNumber } };

	return entity::Entity{ commonInfo, { { avbInterfaceIndex, interfaceInfo } } };
}

DiscoveryStateMachine::EntityUpdateAction DiscoveryStateMachine::updateEntity(entity::Entity& entity, entity::Entity&& newEntity) const noexcept
{
	// First check common fields that are not allowed to change from an ADPDU to another
	auto& commonInfo = entity.getCommonInformation();
	auto const& newCommonInfo = newEntity.getCommonInformation();
	auto const& entityID = entity.getEntityID();

	// Check if immutable values changed in CommonInformation
	if (commonInfo.entityModelID != newCommonInfo.entityModelID || commonInfo.talkerCapabilities != newCommonInfo.talkerCapabilities || commonInfo.talkerStreamSources != newCommonInfo.talkerStreamSources || commonInfo.listenerCapabilities != newCommonInfo.listenerCapabilities || commonInfo.listenerStreamSinks != newCommonInfo.listenerStreamSinks || commonInfo.controllerCapabilities != newCommonInfo.controllerCapabilities || commonInfo.identifyControlIndex != newCommonInfo.identifyControlIndex)
	{
		// Log
		{
			auto fieldName = std::string{};
			if (commonInfo.entityModelID != newCommonInfo.entityModelID)
				fieldName = "entity_model_id";
			else if (commonInfo.talkerCapabilities != newCommonInfo.talkerCapabilities)
				fieldName = "talker_capabilities";
			else if (commonInfo.talkerStreamSources != newCommonInfo.talkerStreamSources)
				fieldName = "talker_stream_sources";
			else if (commonInfo.listenerCapabilities != newCommonInfo.listenerCapabilities)
				fieldName = "listener_capabilities";
			else if (commonInfo.listenerStreamSinks != newCommonInfo.listenerStreamSinks)
				fieldName = "listener_stream_sinks";
			else if (commonInfo.controllerCapabilities != newCommonInfo.controllerCapabilities)
				fieldName = "controller_capabilities";
			else if (commonInfo.identifyControlIndex != newCommonInfo.identifyControlIndex)
				fieldName = "identify_control_index";
			LOG_CONTROLLER_STATE_MACHINE_INFO(entityID, "Entity immutable ADP field changed ({}). Consider it a different entity by simulating offline/online", fieldName);
		}
		// Replace current entity with new one
		entity = std::move(newEntity);
		return EntityUpdateAction::NotifyOfflineOnline;
	}

	// Then search if this is an interface we already know
	auto& interfacesInfo = entity.getInterfacesInformation();
	AVDECC_ASSERT(newEntity.getInterfacesInformation().size() == 1, "NewEntity should have exactly one InterfaceInformation");
	auto const newInterfaceInfoIt = newEntity.getInterfacesInformation().begin();
	auto const avbInterfaceIndex = newInterfaceInfoIt->first;
	auto& newInterfaceInfo = newInterfaceInfoIt->second;
	auto result = EntityUpdateAction::NoNotify;

	auto interfaceInfoIt = interfacesInfo.find(avbInterfaceIndex);
	// This interface already exists, check fields that are not allowed to change from an ADPDU to another
	if (interfaceInfoIt != interfacesInfo.end())
	{
		auto& interfaceInfo = interfaceInfoIt->second;

		// macAddress should not change, and availableIndex should always increment
		if (interfaceInfo.macAddress != newInterfaceInfo.macAddress || interfaceInfo.availableIndex >= newInterfaceInfo.availableIndex)
		{
			// Log
			if (interfaceInfo.macAddress != newInterfaceInfo.macAddress)
			{
				LOG_CONTROLLER_STATE_MACHINE_INFO(entityID, "Entity immutable ADP field changed (mac_address). Consider it a different entity by simulating offline/online");
			}
			else if (interfaceInfo.availableIndex >= newInterfaceInfo.availableIndex)
			{
				LOG_CONTROLLER_STATE_MACHINE_INFO(entityID, "Entity ADP field incoherently changed (available_index should always increment). Consider it a different entity by simulating offline/online");
			}
			// Replace current entity with new one
			entity = std::move(newEntity);
			return EntityUpdateAction::NotifyOfflineOnline;
		}
		// Check for changes in fields that are allowed to change and should trigger an update notification to upper layers
		if (interfaceInfo.gptpGrandmasterID != newInterfaceInfo.gptpGrandmasterID || interfaceInfo.gptpDomainNumber != newInterfaceInfo.gptpDomainNumber)
		{
			// Update the changed fields
			interfaceInfo.gptpGrandmasterID = newInterfaceInfo.gptpGrandmasterID;
			interfaceInfo.gptpDomainNumber = newInterfaceInfo.gptpDomainNumber;
			result = EntityUpdateAction::NotifyUpdate;
		}
		// Update the other fields
		interfaceInfo.availableIndex = newInterfaceInfo.availableIndex;
		interfaceInfo.validTime = newInterfaceInfo.validTime;
	}
	// This is a new interface, add it to the entity
	else
	{
		interfacesInfo.emplace(std::make_pair(avbInterfaceIndex, std::move(newInterfaceInfo)));
		result = EntityUpdateAction::NotifyUpdate;
	}

	// Lastly check for changes in common fields that are allowed to change and should trigger an update notification to upper layers
	if (commonInfo.entityCapabilities != newCommonInfo.entityCapabilities || commonInfo.associationID != newCommonInfo.associationID)
	{
		// Update the changed fields
		commonInfo.entityCapabilities = newCommonInfo.entityCapabilities;
		commonInfo.associationID = newCommonInfo.associationID;
		result = EntityUpdateAction::NotifyUpdate;
	}

	return result;
}

} // namespace stateMachine
} // namespace protocol
} // namespace avdecc
} // namespace la
