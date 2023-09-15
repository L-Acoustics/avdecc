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
* @file stateMachineManager.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/internals/instrumentationNotifier.hpp"
#include "la/avdecc/watchDog.hpp"

#include "stateMachineManager.hpp"
#include "logHelper.hpp"

// Only enable instrumentation in static library and in debug (for unit testing mainly)
#if defined(DEBUG) && defined(la_avdecc_static_STATICS)
#	define SEND_INSTRUMENTATION_NOTIFICATION(eventName) la::avdecc::InstrumentationNotifier::getInstance().triggerEvent(eventName)
#else // !DEBUG || !la_avdecc_static_STATICS
#	define SEND_INSTRUMENTATION_NOTIFICATION(eventName)
#endif // DEBUG && la_avdecc_static_STATICS

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
Manager::Manager(ProtocolInterface const* const protocolInterface, ProtocolInterfaceDelegate* const protocolInterfaceDelegate, AdvertiseStateMachine::Delegate* const advertiseDelegate, DiscoveryStateMachine::Delegate* const discoveryDelegate, CommandStateMachine::Delegate* const controllerDelegate) noexcept
	: _protocolInterface(protocolInterface)
	, _protocolInterfaceDelegate(protocolInterfaceDelegate)
	, _advertiseDelegate(advertiseDelegate)
	, _discoveryDelegate(discoveryDelegate)
	, _controllerDelegate(controllerDelegate)
{
}

Manager::~Manager() noexcept
{
	// Stop state machines (if they are running)
	stopStateMachines();
}

/* ************************************************************ */
/* Static methods                                               */
/* ************************************************************ */
Adpdu Manager::makeDiscoveryMessage(networkInterface::MacAddress const& sourceMacAddress, UniqueIdentifier const targetEntityID) noexcept
{
	Adpdu frame;
	// Set Ether2 fields
	frame.setSrcAddress(sourceMacAddress);
	frame.setDestAddress(Adpdu::Multicast_Mac_Address);
	// Set ADP fields
	frame.setMessageType(AdpMessageType::EntityDiscover);
	frame.setValidTime(0);
	frame.setEntityID(targetEntityID);
	frame.setEntityModelID(UniqueIdentifier::getNullUniqueIdentifier());
	frame.setEntityCapabilities({});
	frame.setTalkerStreamSources(0);
	frame.setTalkerCapabilities({});
	frame.setListenerStreamSinks(0);
	frame.setListenerCapabilities({});
	frame.setControllerCapabilities({});
	frame.setAvailableIndex(0);
	frame.setGptpGrandmasterID(UniqueIdentifier::getNullUniqueIdentifier());
	frame.setGptpDomainNumber(0);
	frame.setIdentifyControlIndex(0);
	frame.setInterfaceIndex(0);
	frame.setAssociationID(UniqueIdentifier::getNullUniqueIdentifier());

	return frame;
}

Adpdu Manager::makeEntityAvailableMessage(entity::LocalEntity& entity, entity::model::AvbInterfaceIndex const interfaceIndex)
{
	auto& interfaceInfo = entity.getInterfaceInformation(interfaceIndex);
	auto entityCaps{ entity.getEntityCapabilities() };
	auto identifyControlIndex{ entity::model::ControlIndex{ 0u } };
	auto avbInterfaceIndex{ entity::model::AvbInterfaceIndex{ 0u } };
	auto associationID{ UniqueIdentifier::getNullUniqueIdentifier() };
	auto gptpGrandmasterID{ UniqueIdentifier::getNullUniqueIdentifier() };
	auto gptpDomainNumber{ std::uint8_t{ 0u } };

	if (entity.getIdentifyControlIndex())
	{
		entityCaps.set(entity::EntityCapability::AemIdentifyControlIndexValid);
		identifyControlIndex = *entity.getIdentifyControlIndex();
	}
	else
	{
		// We don't have a valid IdentifyControlIndex, don't set the flag
		entityCaps.reset(entity::EntityCapability::AemIdentifyControlIndexValid);
	}

	if (interfaceIndex != entity::Entity::GlobalAvbInterfaceIndex)
	{
		entityCaps.set(entity::EntityCapability::AemInterfaceIndexValid);
		avbInterfaceIndex = interfaceIndex;
	}
	else
	{
		// We don't have a valid AvbInterfaceIndex, don't set the flag
		entityCaps.reset(entity::EntityCapability::AemInterfaceIndexValid);
	}

	if (entity.getAssociationID())
	{
		entityCaps.set(entity::EntityCapability::AssociationIDValid);
		associationID = *entity.getAssociationID();
	}
	else
	{
		// We don't have a valid AssociationID, don't set the flag
		entityCaps.reset(entity::EntityCapability::AssociationIDValid);
	}

	if (interfaceInfo.gptpGrandmasterID)
	{
		entityCaps.set(entity::EntityCapability::GptpSupported);
		gptpGrandmasterID = *interfaceInfo.gptpGrandmasterID;
		if (AVDECC_ASSERT_WITH_RET(interfaceInfo.gptpDomainNumber, "gptpDomainNumber should be set when gptpGrandmasterID is set"))
		{
			gptpDomainNumber = *interfaceInfo.gptpDomainNumber;
		}
	}
	else
	{
		// We don't have a valid gptpGrandmasterID value, don't set the flag
		entityCaps.reset(entity::EntityCapability::GptpSupported);
	}

	Adpdu frame;
	// Set Ether2 fields
	frame.setSrcAddress(interfaceInfo.macAddress);
	frame.setDestAddress(Adpdu::Multicast_Mac_Address);
	// Set ADP fields
	frame.setMessageType(AdpMessageType::EntityAvailable);
	frame.setValidTime(interfaceInfo.validTime);
	frame.setEntityID(entity.getEntityID());
	frame.setEntityModelID(entity.getEntityModelID());
	frame.setEntityCapabilities(entityCaps);
	frame.setTalkerStreamSources(entity.getTalkerStreamSources());
	frame.setTalkerCapabilities(entity.getTalkerCapabilities());
	frame.setListenerStreamSinks(entity.getListenerStreamSinks());
	frame.setListenerCapabilities(entity.getListenerCapabilities());
	frame.setControllerCapabilities(entity.getControllerCapabilities());
	frame.setAvailableIndex(interfaceInfo.availableIndex++);
	frame.setGptpGrandmasterID(gptpGrandmasterID);
	frame.setGptpDomainNumber(gptpDomainNumber);
	frame.setIdentifyControlIndex(identifyControlIndex);
	frame.setInterfaceIndex(avbInterfaceIndex);
	frame.setAssociationID(associationID);

	return frame;
}

Adpdu Manager::makeEntityDepartingMessage(entity::LocalEntity const& entity, entity::model::AvbInterfaceIndex const interfaceIndex)
{
	auto const& interfaceInfo = entity.getInterfaceInformation(interfaceIndex);
	auto entityCaps = entity::EntityCapabilities{};
	auto avbInterfaceIndex{ entity::model::AvbInterfaceIndex{ 0u } };

	if (interfaceIndex != entity::Entity::GlobalAvbInterfaceIndex)
	{
		entityCaps.set(entity::EntityCapability::AemInterfaceIndexValid);
		avbInterfaceIndex = interfaceIndex;
	}

	Adpdu frame;
	// Set Ether2 fields
	frame.setSrcAddress(interfaceInfo.macAddress);
	frame.setDestAddress(Adpdu::Multicast_Mac_Address);
	// Set ADP fields
	frame.setMessageType(AdpMessageType::EntityDeparting);
	frame.setValidTime(0);
	frame.setEntityID(entity.getEntityID());
	frame.setEntityModelID(UniqueIdentifier::getNullUniqueIdentifier());
	frame.setEntityCapabilities(entityCaps);
	frame.setTalkerStreamSources(0);
	frame.setTalkerCapabilities({});
	frame.setListenerStreamSinks(0);
	frame.setListenerCapabilities({});
	frame.setControllerCapabilities({});
	frame.setAvailableIndex(0);
	frame.setGptpGrandmasterID(UniqueIdentifier::getNullUniqueIdentifier());
	frame.setGptpDomainNumber(0);
	frame.setIdentifyControlIndex(0);
	frame.setInterfaceIndex(avbInterfaceIndex);
	frame.setAssociationID(UniqueIdentifier::getNullUniqueIdentifier());

	return frame;
}
/* ************************************************************ */
/* General entry points                                         */
/* ************************************************************ */
void Manager::startStateMachines() noexcept
{
	// StateMachines are not already started
	if (!_stateMachineThread.joinable())
	{
		// Should no longer terminate
		_shouldTerminate = false;

		// Create the state machine thread
		_stateMachineThread = std::thread(
			[this]
			{
				utils::setCurrentThreadName("avdecc::StateMachine");

				auto watchDogSharedPointer = watchDog::WatchDog::getInstance();
				auto& watchDog = *watchDogSharedPointer;
				watchDog.registerWatch("avdecc::StateMachine", std::chrono::milliseconds{ 1000u }, true);

				while (!_shouldTerminate)
				{
					// Check for local entities announcement
					_advertiseStateMachine.checkLocalEntitiesAnnouncement();

					// Check for discovery time
					_discoveryStateMachine.checkDiscovery();

					// Check for timeout expiracy on all remote entities
					_discoveryStateMachine.checkRemoteEntitiesTimeoutExpiracy();

					// Check for inflight commands expiracy
					_commandStateMachine.checkInflightCommandsTimeoutExpiracy();

					// Try to detect deadlocks
					watchDog.alive("avdecc::StateMachine", true);

					// Wait a little bit so we don't burn the CPU
					std::this_thread::sleep_for(std::chrono::milliseconds(5));
				}
				watchDog.unregisterWatch("avdecc::StateMachine", true);
			});
	}
}

void Manager::stopStateMachines() noexcept
{
	// StateMachines are started
	if (_stateMachineThread.joinable())
	{
		// Notify the thread we are shutting down
		_shouldTerminate = true;

		// Wait for the thread to complete its pending tasks
		_stateMachineThread.join();
	}
}

ProtocolInterface::Error Manager::registerLocalEntity(entity::LocalEntity& entity) noexcept
{
	// Lock
	auto const lg = std::lock_guard{ *this };

	// Check if an entity has already been registered with the same EntityID
	auto const entityID = entity.getEntityID();
	for (auto const& entityKV : _localEntities)
	{
		if (entityID == entityKV.first)
			return ProtocolInterface::Error::DuplicateLocalEntityID;
	}

	// Ok, register the new entity
	_localEntities.insert(std::make_pair(entityID, std::ref(entity)));

	// Any entity should be able to send commands, so register it to the CommandStateMachine
	_commandStateMachine.registerLocalEntity(entity);

	// Notify delegate
	utils::invokeProtectedMethod(&DiscoveryStateMachine::Delegate::onLocalEntityOnline, _discoveryDelegate, entity);

	return ProtocolInterface::Error::NoError;
}

ProtocolInterface::Error Manager::unregisterLocalEntity(entity::LocalEntity& entity) noexcept
{
	// Lock
	auto const lg = std::lock_guard{ *this };

	auto removed = false;

	for (auto it = _localEntities.begin(); it != _localEntities.end(); /* Iterate inside the loop */)
	{
		if (&it->second == &entity)
		{
			// Disable advertising
			disableEntityAdvertising(entity);
			// Remove from the list
			it = _localEntities.erase(it);
			removed = true;
		}
		else
		{
			++it;
		}
	}

	// Not found
	if (!removed)
	{
		return ProtocolInterface::Error::UnknownLocalEntity;
	}

	// Unregister the entity from the CommandStateMachine
	_commandStateMachine.unregisterLocalEntity(entity);

	// Notify delegate
	utils::invokeProtectedMethod(&DiscoveryStateMachine::Delegate::onLocalEntityOffline, _discoveryDelegate, entity.getEntityID());

	return ProtocolInterface::Error::NoError;
}

void Manager::processAdpdu(Adpdu const& adpdu) noexcept
{
	// Dispatching and handling of ADP messages is done on this layer

	static auto const s_Dispatch = std::unordered_map<AdpMessageType::value_type, std::function<void(Manager* const manager, Adpdu const& adpdu)>>{
		// Entity Available
		{ AdpMessageType::EntityAvailable.getValue(),
			[](Manager* const manager, Adpdu const& adpdu)
			{
				manager->_discoveryStateMachine.handleAdpEntityAvailable(adpdu);
			} },
		// Entity Departing
		{ AdpMessageType::EntityDeparting.getValue(),
			[](Manager* const manager, Adpdu const& adpdu)
			{
				manager->_discoveryStateMachine.handleAdpEntityDeparting(adpdu);
			} },
		// Entity Discover
		{ AdpMessageType::EntityDiscover.getValue(),
			[](Manager* const manager, Adpdu const& adpdu)
			{
				manager->_advertiseStateMachine.handleAdpEntityDiscover(adpdu);
			} },
	};

	auto const messageType = adpdu.getMessageType().getValue();
	auto const& it = s_Dispatch.find(messageType);
	if (it != s_Dispatch.end())
	{
		utils::invokeProtectedHandler(it->second, this, adpdu);
	}
}

void Manager::processAecpdu(Aecpdu const& aecpdu) noexcept
{
	auto const messageType = aecpdu.getMessageType();
	auto const isResponse = (messageType.getValue() % 2) == 1; // Odd numbers are responses (see IEEE1722.1-2013 Clause 9.2.1.1.5)

	// If the message is a RESPONSE
	if (isResponse)
	{
		// Forward to the CommandStateMachine
		_commandStateMachine.handleAecpResponse(aecpdu);
	}
	// If the message is a COMMAND
	else
	{
		// Lock
		auto const lg = std::lock_guard{ *this };

		// Only process it if it's targeted to a registered LocalEntity
		auto const targetID = aecpdu.getTargetEntityID();
		if (_localEntities.count(targetID) != 0)
		{
			// Notify the delegate
			utils::invokeProtectedMethod(&ProtocolInterfaceDelegate::onAecpCommand, _protocolInterfaceDelegate, aecpdu);
		}
	}
}

void Manager::processAcmpdu(Acmpdu const& acmpdu) noexcept
{
	auto const messageType = acmpdu.getMessageType().getValue();
	auto const isResponse = (messageType % 2) == 1; // Odd numbers are responses (see IEEE1722.1-2013 Clause 8.2.1.5)

	// Lock
	auto const lg = std::lock_guard{ *this };

	// If the message is a RESPONSE
	if (isResponse)
	{
		// Forward to the CommandStateMachine
		_commandStateMachine.handleAcmpResponse(acmpdu);

		// Notify the delegate
		utils::invokeProtectedMethod(&ProtocolInterfaceDelegate::onAcmpResponse, _protocolInterfaceDelegate, acmpdu);
	}
	// If the message is a COMMAND
	else
	{
		// Notify the delegate
		utils::invokeProtectedMethod(&ProtocolInterfaceDelegate::onAcmpCommand, _protocolInterfaceDelegate, acmpdu);
	}
}

void Manager::lock() noexcept
{
	SEND_INSTRUMENTATION_NOTIFICATION("StateMachineManager::lock::PreLock");
	_lock.lock();
	if (_lockedCount == 0)
	{
		_lockingThreadID = std::this_thread::get_id();
	}
	++_lockedCount;
	SEND_INSTRUMENTATION_NOTIFICATION("StateMachineManager::lock::PostLock");
}

void Manager::unlock() noexcept
{
	SEND_INSTRUMENTATION_NOTIFICATION("StateMachineManager::unlock::PreUnlock");
	--_lockedCount;
	if (_lockedCount == 0)
	{
		_lockingThreadID = {};
	}
	_lock.unlock();
	SEND_INSTRUMENTATION_NOTIFICATION("StateMachineManager::unlock::PostUnlock");
}

bool Manager::isSelfLocked() const noexcept
{
	return _lockingThreadID == std::this_thread::get_id();
}

ProtocolInterface const* Manager::getProtocolInterface() noexcept
{
	return _protocolInterface;
}

ProtocolInterfaceDelegate* Manager::getProtocolInterfaceDelegate() noexcept
{
	return _protocolInterfaceDelegate;
}

std::optional<entity::model::AvbInterfaceIndex> Manager::getMatchingInterfaceIndex(entity::LocalEntity const& entity) const noexcept
{
	auto avbInterfaceIndex = std::optional<entity::model::AvbInterfaceIndex>{ std::nullopt };
	auto const& macAddress = _protocolInterface->getMacAddress();

	for (auto const& [interfaceIndex, interfaceInfo] : entity.getInterfacesInformation())
	{
		if (interfaceInfo.macAddress == macAddress)
		{
			avbInterfaceIndex = interfaceIndex;
			break;
		}
	}

	return avbInterfaceIndex;
}

bool Manager::isLocalEntity(UniqueIdentifier const entityID) noexcept
{
	// Lock
	auto const lg = std::lock_guard{ *this };

	return _localEntities.find(entityID) != _localEntities.end();
}

void Manager::notifyDiscoveredEntities(DiscoveryStateMachine::Delegate& delegate) noexcept
{
	// Notify local entities
	{
		auto const lg = std::lock_guard{ *this };

		for (auto const& [entityID, entity] : _localEntities)
		{
			// Notify delegate
			utils::invokeProtectedMethod(&DiscoveryStateMachine::Delegate::onLocalEntityOnline, &delegate, entity);
		}
	}

	// Notify remote entities
	_discoveryStateMachine.notifyDiscoveredRemoteEntities(delegate);
}


/* ************************************************************ */
/* Notifications                                                */
/* ************************************************************ */
void Manager::onRemoteEntityOffline(UniqueIdentifier const entityID) noexcept
{
	// Discard messages related to this entity
	_commandStateMachine.discardEntityMessages(entityID);
}


/* ************************************************************ */
/* Advertising entry points                                     */
/* ************************************************************ */
ProtocolInterface::Error Manager::setEntityNeedsAdvertise(entity::LocalEntity const& entity) noexcept
{
	_advertiseStateMachine.setEntityNeedsAdvertise(entity);
	return ProtocolInterface::Error::NoError;
}

ProtocolInterface::Error Manager::enableEntityAdvertising(entity::LocalEntity& entity) noexcept
{
	_advertiseStateMachine.enableEntityAdvertising(entity);
	return ProtocolInterface::Error::NoError;
}

ProtocolInterface::Error Manager::disableEntityAdvertising(entity::LocalEntity const& entity) noexcept
{
	_advertiseStateMachine.disableEntityAdvertising(entity);
	return ProtocolInterface::Error::NoError;
}

/* ************************************************************ */
/* Discovery entry points                                       */
/* ************************************************************ */
ProtocolInterface::Error Manager::setAutomaticDiscoveryDelay(std::chrono::milliseconds const delay) noexcept
{
	_discoveryStateMachine.setDiscoveryDelay(delay);
	return ProtocolInterface::Error::NoError;
}

void Manager::discoverMessageSent() noexcept
{
	_discoveryStateMachine.discoverMessageSent();
}

/* ************************************************************ */
/* Sending entry points                                         */
/* ************************************************************ */
ProtocolInterface::Error Manager::sendAecpCommand(Aecpdu::UniquePointer&& aecpdu, ProtocolInterface::AecpCommandResultHandler const& onResult) noexcept
{
#pragma message("TODO: If TargetEntity is a LocalEntity, then bypass the CommandStateMachine, directly dispatch the message (and asynchroneously trigger onResult)")
	return _commandStateMachine.sendAecpCommand(std::move(aecpdu), onResult);
}

ProtocolInterface::Error Manager::sendAcmpCommand(Acmpdu::UniquePointer&& acmpdu, ProtocolInterface::AcmpCommandResultHandler const& onResult) noexcept
{
#pragma message("TODO: If TargetEntity is a LocalEntity, then bypass the CommandStateMachine, directly dispatch the message (and asynchroneously trigger onResult)")
	return _commandStateMachine.sendAcmpCommand(std::move(acmpdu), onResult);
}

/* ************************************************************ */
/* Private methods                                              */
/* ************************************************************ */

} // namespace stateMachine
} // namespace protocol
} // namespace avdecc
} // namespace la
