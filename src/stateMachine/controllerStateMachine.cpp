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
* @file controllerStateMachine.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/internals/protocolAemAecpdu.hpp"
#include "controllerStateMachine.hpp"
#include "logHelper.hpp"
#include <utility>
#include <stdexcept>
#include <chrono>
#include <algorithm>
#include <cstdlib>

namespace la
{
namespace avdecc
{
namespace protocol
{
namespace stateMachine
{
/* Aecp commands timeout - Clause 9.2.1 */
static constexpr auto AecpAemCommandTimeoutMsec = 250u;
static constexpr auto AecpAaCommandTimeoutMsec = 250u;
static constexpr auto AecpVuCommandTimeoutMsec = 250u; // This is actually not true, it may be different for each Vendor Unique
/* Acmp commands timeout - Clause 8.2.2 */
static constexpr auto AcmpConnectTxCommandTimeoutMsec = 2000u;
static constexpr auto AcmpDisconnectTxCommandTimeoutMsec = 200u;
static constexpr auto AcmpGetTxStateCommandTimeoutMsec = 200u;
static constexpr auto AcmpConnectRxCommandTimeoutMsec = 4500u;
static constexpr auto AcmpDisconnectRxCommandTimeoutMsec = 500u;
static constexpr auto AcmpGetRxStateCommandTimeoutMsec = 200u;
static constexpr auto AcmpGetTxConnectionCommandTimeoutMsec = 200u;

enum class EntityUpdateAction
{
	NoNotify = 0, /**< Ne need to notify upper layers, the change(s) are only for the ControllerStateMachine to interpret */
	NotifyUpdate = 1, /**< Upper layers shall be notified of change(s) in the entity */
	NotifyOfflineOnline = 2, /**< An invalid change in consecutive ADPDUs has been detecter, upper layers will be notified through Offline/Online simulation calls */
};

static EntityUpdateAction updateEntity(entity::Entity& entity, entity::Entity&& newEntity) noexcept
{
	// First check common fields that are not allowed to change from an ADPDU to another
	auto& commonInfo = entity.getCommonInformation();
	auto const& newCommonInfo = newEntity.getCommonInformation();

	if (commonInfo.entityModelID != newCommonInfo.entityModelID || commonInfo.talkerCapabilities != newCommonInfo.talkerCapabilities || commonInfo.talkerStreamSources != newCommonInfo.talkerStreamSources || commonInfo.listenerCapabilities != newCommonInfo.listenerCapabilities || commonInfo.listenerStreamSinks != newCommonInfo.listenerStreamSinks || commonInfo.controllerCapabilities != newCommonInfo.controllerCapabilities || commonInfo.identifyControlIndex != newCommonInfo.identifyControlIndex)
	{
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
	auto result{ EntityUpdateAction::NoNotify };

	auto interfaceInfoIt = interfacesInfo.find(avbInterfaceIndex);
	// This interface already exists, check fields that are not allowed to change from an ADPDU to another
	if (interfaceInfoIt != interfacesInfo.end())
	{
		auto& interfaceInfo = interfaceInfoIt->second;

		// macAddress should not change, and availableIndex should always increment
		if (interfaceInfo.macAddress != newInterfaceInfo.macAddress || interfaceInfo.availableIndex >= newInterfaceInfo.availableIndex)
		{
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

/** Constructs a la::avdecc::entity::Entity from a la::avdecc:protocol::Adpdu */
static entity::Entity makeEntity(Adpdu const& adpdu) noexcept
{
	auto const entityCaps = adpdu.getEntityCapabilities();
	auto controlIndex{ std::optional<entity::model::ControlIndex>{} };
	auto associationID{ std::optional<UniqueIdentifier>{} };
	auto avbInterfaceIndex{ entity::Entity::GlobalAvbInterfaceIndex };
	auto gptpGrandmasterID{ std::optional<UniqueIdentifier>{} };
	auto gptpDomainNumber{ std::optional<std::uint8_t>{} };

	if (hasFlag(entityCaps, entity::EntityCapabilities::AemIdentifyControlIndexValid))
	{
		controlIndex = adpdu.getIdentifyControlIndex();
	}
	if (hasFlag(entityCaps, entity::EntityCapabilities::AssociationIDValid))
	{
		associationID = adpdu.getAssociationID();
	}
	if (hasFlag(entityCaps, entity::EntityCapabilities::AemInterfaceIndexValid))
	{
		avbInterfaceIndex = adpdu.getInterfaceIndex();
	}
	if (hasFlag(entityCaps, entity::EntityCapabilities::GptpSupported))
	{
		gptpGrandmasterID = adpdu.getGptpGrandmasterID();
		gptpDomainNumber = adpdu.getGptpDomainNumber();
	}

	auto const commonInfo{ entity::Entity::CommonInformation{ adpdu.getEntityID(), adpdu.getEntityModelID(), entityCaps, adpdu.getTalkerStreamSources(), adpdu.getTalkerCapabilities(), adpdu.getListenerStreamSinks(), adpdu.getListenerCapabilities(), adpdu.getControllerCapabilities(), controlIndex, associationID } };
	auto const interfaceInfo{ entity::Entity::InterfaceInformation{ adpdu.getSrcAddress(), adpdu.getValidTime(), adpdu.getAvailableIndex(), gptpGrandmasterID, gptpDomainNumber } };

	return entity::Entity{ commonInfo, { { avbInterfaceIndex, interfaceInfo } } };
}

ControllerStateMachine::ControllerStateMachine(ProtocolInterface const* const protocolInterface, Delegate* const delegate, size_t const maxInflightAecpMessages)
	: _protocolInterface(protocolInterface)
	, _delegate(delegate)
	, _maxInflightAecpMessages(maxInflightAecpMessages)
{
	if (_delegate == nullptr)
		throw Exception("ControllerStateMachine's delegate cannot be nullptr");

	// Create the state machine thread
	_stateMachineThread = std::thread(
		[this]
		{
			setCurrentThreadName("avdecc::ControllerStateMachine");
			while (!_shouldTerminate)
			{
				// Try to detect possible deadlock
				{
					auto const startTime = std::chrono::system_clock::now();

					// Check for local entities announcement
					checkLocalEntitiesAnnouncement();

					// Check for timeout expiracy on all entities
					checkEntitiesTimeoutExpiracy();

					// Check for inflight commands expiracy
					checkInflightCommandsTimeoutExpiracy();

					auto const endTime = std::chrono::system_clock::now();
					if (!AVDECC_ASSERT_WITH_RET(std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count() < 1000, "avdecc::ControllerStateMachine thread loop took too much time. Deadlock?"))
					{
						LOG_CONTROLLER_STATE_MACHINE_ERROR(UniqueIdentifier::getNullUniqueIdentifier(), "avdecc::ControllerStateMachine thread loop took too much time. Deadlock?");
					}
				}

				// Wait a little bit so we don't burn the CPU
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		});
}

ControllerStateMachine::~ControllerStateMachine() noexcept
{
	// Notify the thread we are shutting down
	_shouldTerminate = true;

	// Wait for the thread to complete its pending tasks
	if (_stateMachineThread.joinable())
		_stateMachineThread.join();
}

ProtocolInterface::Error ControllerStateMachine::sendAecpCommand(Aecpdu::UniquePointer&& aecpdu, ProtocolInterface::AecpCommandResultHandler const& onResult) noexcept
{
	auto* aecp = static_cast<Aecpdu*>(aecpdu.get());
	auto const targetEntityID = aecp->getTargetEntityID();

	// Lock self
	std::lock_guard<ControllerStateMachine> const lg(getSelf());

	// Search our local entity info
	auto const& localEntityIt = _localEntities.find(aecp->getControllerEntityID());
	if (localEntityIt == _localEntities.end())
		return ProtocolInterface::Error::UnknownLocalEntity;
	auto& localEntity = localEntityIt->second;

	// Check entity has controller capabilities
	if (!hasFlag(localEntity.entity.getControllerCapabilities(), entity::ControllerCapabilities::Implemented))
		return ProtocolInterface::Error::InvalidEntityType;

	// Get next available sequenceID and update the aecpdu with it
	auto const sequenceID = getNextAecpSequenceID(localEntity);
	aecpdu->setSequenceID(sequenceID);

	try
	{
#pragma message("TODO: If Entity is a LocalEntity, then bypass networking and inflight stuff, and directly call processAecpdu()")
		// Record the query for when we get a response (so we can send it again if it timed out)
		AecpCommandInfo command{ sequenceID, std::move(aecpdu), onResult };
		{
			auto& inflight = localEntity.inflightAecpCommands[targetEntityID];
			// Check if we don't have too many inflight commands for this entity
			if (inflight.size() < _maxInflightAecpMessages)
			{
				// Send the command
				setCommandInflight(localEntity, inflight, inflight.end(), std::move(command));
			}
			else // Too many inflight commands, queue it
			{
				auto& queue = localEntity.commandsQueue[targetEntityID];
				queue.push_back(std::move(command));
			}
		}
	}
	catch (...)
	{
		return ProtocolInterface::Error::InternalError;
	}
	return ProtocolInterface::Error::NoError;
}

ProtocolInterface::Error ControllerStateMachine::sendAcmpCommand(Acmpdu::UniquePointer&& acmpdu, ProtocolInterface::AcmpCommandResultHandler const& onResult) noexcept
{
	auto* acmp = static_cast<Acmpdu*>(acmpdu.get());

	// Lock self
	std::lock_guard<ControllerStateMachine> const lg(getSelf());

	// Search our local entity info
	auto const& localEntityIt = _localEntities.find(acmp->getControllerEntityID());
	if (localEntityIt == _localEntities.end())
		return ProtocolInterface::Error::UnknownLocalEntity;
	auto& localEntity = localEntityIt->second;

	// Check entity has controller capabilities
	if (!hasFlag(localEntity.entity.getControllerCapabilities(), entity::ControllerCapabilities::Implemented))
		return ProtocolInterface::Error::InvalidEntityType;

	// Get next available sequenceID and update the acmpdu with it
	auto const sequenceID = getNextAcmpSequenceID(localEntity);
	acmpdu->setSequenceID(sequenceID);

	auto error{ ProtocolInterface::Error::NoError };
	try
	{
#pragma message("TODO: If Entity is a LocalEntity, then bypass networking and inflight stuff, and directly call processAcmpdu()")
		// Record the query for when we get a response (so we can send it again if it timed out)
		AcmpCommandInfo command{ sequenceID, std::move(acmpdu), onResult };

		error = _delegate->sendMessage(static_cast<Acmpdu const&>(*command.command));
		if (!error)
		{
			resetAcmpCommandTimeoutValue(command);
			localEntity.inflightAcmpCommands[sequenceID] = std::move(command);
		}
	}
	catch (...)
	{
		error = ProtocolInterface::Error::InternalError;
	}
	return error;
}

bool ControllerStateMachine::processAdpdu(Adpdu const& adpdu) noexcept
{
	// Dispatching and handling of ADP messages is done on this layer

	static std::unordered_map<AdpMessageType::value_type, std::function<void(ControllerStateMachine* const stateMachine, Adpdu const& adpdu)>> s_Dispatch{
		// Entity Available
		{ AdpMessageType::EntityAvailable.getValue(),
			[](ControllerStateMachine* const stateMachine, Adpdu const& adpdu)
			{
				stateMachine->handleAdpEntityAvailable(adpdu);
			} },
		// Entity Departing
		{ AdpMessageType::EntityDeparting.getValue(),
			[](ControllerStateMachine* const stateMachine, Adpdu const& adpdu)
			{
				stateMachine->handleAdpEntityDeparting(adpdu);
			} },
		// Entity Discover
		{ AdpMessageType::EntityDiscover.getValue(),
			[](ControllerStateMachine* const stateMachine, Adpdu const& adpdu)
			{
				stateMachine->handleAdpEntityDiscover(adpdu);
			} },
	};

	auto const messageType = adpdu.getMessageType().getValue();
	auto const& it = s_Dispatch.find(messageType);
	if (it != s_Dispatch.end())
	{
		invokeProtectedHandler(it->second, this, adpdu);
		return true;
	}
	return false;
}

bool ControllerStateMachine::isAEMUnsolicitedResponse(Aecpdu const& aecpdu) const noexcept
{
	auto const messageType = aecpdu.getMessageType();

	// Special case for AEM messages (see Clause 9.2.2.3.1.2.4)
	if (messageType == AecpMessageType::AemResponse)
	{
		auto const& aem = static_cast<AemAecpdu const&>(aecpdu);
		// Do not check for expected response if it's an unsolicited notification
		return aem.getUnsolicited();
	}
	return false;
}

bool ControllerStateMachine::shouldRearmTimer(Aecpdu const& aecpdu) const noexcept
{
	auto const messageType = aecpdu.getMessageType();

	// Special case for AEM messages (see Clause 9.2.1.2.5)
	if (messageType == AecpMessageType::AemResponse)
	{
		auto const status = aecpdu.getStatus();
		// Check if we received a InProgress code, in which case we must re-arm the timeout
		if (status == AemAecpStatus::InProgress)
		{
			return true;
		}
	}
	return false;
}

bool ControllerStateMachine::processAecpdu(Aecpdu const& aecpdu) noexcept
{
	auto const messageType = aecpdu.getMessageType();
	auto const isResponse = (messageType.getValue() % 2) == 1; // Odd numbers are responses (see Clause 9.2.1.1.5)
	auto const controllerID = aecpdu.getControllerEntityID();
	auto const targetID = aecpdu.getTargetEntityID();
	bool processedBySomeone{ false };

	// Check if the message is for one of our registered controllers
	// Lock self
	std::lock_guard<ControllerStateMachine> const lg(getSelf());

	for (auto& localEntityKV : _localEntities)
	{
		auto& localEntity = localEntityKV.second;
		auto const& entity = localEntity.entity;
		auto const entityID = entity.getEntityID();

		// It's a command for us
		if (!isResponse && entityID == targetID)
		{
			processedBySomeone = true;
			// Notify the delegate
			invokeProtectedMethod(&Delegate::onAecpCommand, _delegate, entity, aecpdu);
			break;
		}
		// It's a response for us
		if (isResponse && entityID == controllerID)
		{
			// Check if it's an AEM unsolicited response
			if (isAEMUnsolicitedResponse(aecpdu))
			{
				invokeProtectedMethod(&Delegate::onAecpUnsolicitedResponse, _delegate, entity, aecpdu);
			}
			else
			{
				auto inflightIt = localEntity.inflightAecpCommands.find(targetID);
				if (inflightIt != localEntity.inflightAecpCommands.end())
				{
					auto& inflight = inflightIt->second;
					auto const sequenceID = aecpdu.getSequenceID();
					auto commandIt = std::find_if(inflight.begin(), inflight.end(),
						[sequenceID](AecpCommandInfo const& command)
						{
							return command.sequenceID == sequenceID;
						});
					// If the sequenceID is not found, it means the response already timed out (arriving too late)
					if (commandIt != inflight.end())
					{
						auto& info = *commandIt;

						// Check for special cases where we should re-arm the timer
						if (shouldRearmTimer(aecpdu))
						{
							resetAecpCommandTimeoutValue(info);
							break;
						}

						// Move the query (it will be deleted)
						AecpCommandInfo aecpQuery = std::move(info);

						// Remove the command from inflight list
						removeInflight(localEntity, targetID, inflight, commandIt);

						// Call completion handler
						invokeProtectedHandler(aecpQuery.resultHandler, &aecpdu, ProtocolInterface::Error::NoError);
					}
					else
						LOG_CONTROLLER_STATE_MACHINE_DEBUG(targetID, std::string("AECP command with sequenceID ") + std::to_string(sequenceID) + " unexpected (timed out already?)");
				}
			}

			processedBySomeone = true;
			break; // No need to search other controllers, we were targeted
		}
	}

	return processedBySomeone;
}

bool ControllerStateMachine::processAcmpdu(Acmpdu const& acmpdu) noexcept
{
	auto const messageType = acmpdu.getMessageType().getValue();
	auto const isResponse = (messageType % 2) == 1; // Odd numbers are responses (see Clause 8.2.1.5)
	auto const controllerID = acmpdu.getControllerEntityID();
	bool processedBySomeone{ false };

	// Check if the message is for one of our registered controllers
	// Lock self
	std::lock_guard<ControllerStateMachine> const lg(getSelf());

	for (auto& localEntityKV : _localEntities)
	{
		auto& localEntity = localEntityKV.second;
		auto const& entity = localEntity.entity;
		auto const entityID = entity.getEntityID();

		// It's a response
		if (isResponse)
		{
			bool processed{ false };
			// Using our controllerID
			if (entityID == controllerID)
			{
				auto const sequenceID = acmpdu.getSequenceID();
				auto commandIt = localEntity.inflightAcmpCommands.find(sequenceID);
				// If the sequenceID is not found, it either means the response already timed out (arriving too late), or it's a communication btw talker and listener (requested by us) and they did not use our sequenceID
				if (commandIt != localEntity.inflightAcmpCommands.end())
				{
					auto& info = commandIt->second;

					// Check if it's an expected response (since the communication btw listener and talkers uses our controllerID and might use our sequenceID, we don't want to detect talker's response as ours)
					auto const expectedResponseType = info.command->getMessageType().getValue() + 1; // Based on Clause 8.2.1.5, responses are always Command + 1
					if (messageType == expectedResponseType)
					{
						processed = true;

						// Move the query (it will be deleted)
						AcmpCommandInfo acmpQuery = std::move(info);

						// Remove the command from inflight list
						localEntity.inflightAcmpCommands.erase(sequenceID);

						// Call completion handler
						invokeProtectedHandler(acmpQuery.resultHandler, &acmpdu, ProtocolInterface::Error::NoError);
					}
				}
			}
			if (!processed)
			{
				// Notify delegate of a sniffed message
				invokeProtectedMethod(&Delegate::onAcmpSniffedResponse, _delegate, entity, acmpdu);
			}
			processedBySomeone |= processed;
		}
		// It's a command
		else
		{
			// Notify delegate of a sniffed message
			invokeProtectedMethod(&Delegate::onAcmpSniffedCommand, _delegate, entity, acmpdu);
		}
	}

	return processedBySomeone;
}

ProtocolInterface::Error ControllerStateMachine::registerLocalEntity(entity::LocalEntity& entity) noexcept
{
	// Lock self
	std::lock_guard<ControllerStateMachine> const lg(getSelf());

	// Check if an entity has already been registered with the same EntityID
	auto const entityID = entity.getEntityID();
	for (auto const& entityKV : _localEntities)
	{
		if (entityID == entityKV.first)
			return ProtocolInterface::Error::DuplicateLocalEntityID;
	}

	// Ok, register the new entity
	_localEntities.insert(std::make_pair(entityID, LocalEntityInfo(entity)));

	// Notify delegate
	invokeProtectedMethod(&Delegate::onLocalEntityOnline, _delegate, entity);

	return ProtocolInterface::Error::NoError;
}

ProtocolInterface::Error ControllerStateMachine::unregisterLocalEntity(entity::LocalEntity& entity) noexcept
{
	// Lock self
	std::lock_guard<ControllerStateMachine> const lg(getSelf());

	for (auto it = std::begin(_localEntities); it != std::end(_localEntities); /* Iterate inside the loop */)
	{
		auto& entityInfo = it->second;
		if (&entityInfo.entity == &entity)
		{
			// Disable advertising for all interfaces
			disableEntityAdvertising(entity, std::nullopt);
			// Remove from the list
			it = _localEntities.erase(it);
		}
		else
			++it;
	}

	// Notify delegate
	invokeProtectedMethod(&Delegate::onLocalEntityOffline, _delegate, entity.getEntityID());

	return ProtocolInterface::Error::NoError;
}

ProtocolInterface::Error ControllerStateMachine::setEntityNeedsAdvertise(entity::LocalEntity const& entity, std::optional<entity::model::AvbInterfaceIndex> const interfaceIndex) noexcept
{
	// Lock self
	std::lock_guard<ControllerStateMachine> const lg(getSelf());

	// Search our local entity info
	auto const& localEntityIt = _localEntities.find(entity.getEntityID());
	if (localEntityIt == _localEntities.end())
		return ProtocolInterface::Error::UnknownLocalEntity;

	auto& localEntity = localEntityIt->second;

	try
	{
		// If interfaceIndex is specified
		if (interfaceIndex)
		{
			// Only if advertising is enabled
			auto advertiseIt = localEntity.nextAdvertiseTime.find(*interfaceIndex);
			if (advertiseIt != localEntity.nextAdvertiseTime.end())
			{
				// Schedule EntityAvailable message
				advertiseIt->second = computeDelayedAdvertiseTime(entity, *interfaceIndex);
			}
		}
		else
		{
			// Otherwise schedule for all advertising interfaces
			for (auto& advertiseKV : localEntity.nextAdvertiseTime)
			{
				auto const avbInterfaceIndex = advertiseKV.first;

				// Schedule EntityAvailable message
				advertiseKV.second = computeDelayedAdvertiseTime(entity, avbInterfaceIndex);
			}
		}
	}
	catch (Exception const&)
	{
		AVDECC_ASSERT(false, "avbInterfaceIndex might not be valid");
		return ProtocolInterface::Error::InternalError;
	}

	return ProtocolInterface::Error::NoError;
}

ProtocolInterface::Error ControllerStateMachine::enableEntityAdvertising(entity::LocalEntity const& entity, std::optional<entity::model::AvbInterfaceIndex> const interfaceIndex) noexcept
{
	// Lock self
	std::lock_guard<ControllerStateMachine> const lg(getSelf());

	// Search our local entity info
	auto const& localEntityIt = _localEntities.find(entity.getEntityID());
	if (localEntityIt == _localEntities.end())
		return ProtocolInterface::Error::UnknownLocalEntity;

	auto& localEntity = localEntityIt->second;

	// If interfaceIndex is specified, only enable advertising for this interface
	if (interfaceIndex)
	{
		localEntity.nextAdvertiseTime[*interfaceIndex] = {}; // Set next advertise time to minimum value so we advertise ASAP
	}
	else
	{
		// Otherwise enable advertising for all interfaces on the entity
		for (auto const& infoKV : entity.getInterfacesInformation())
		{
			auto const avbInterfaceIndex = infoKV.first;
			localEntity.nextAdvertiseTime[avbInterfaceIndex] = {}; // Set next advertise time to minimum value so we advertise ASAP
		}
	}

	return ProtocolInterface::Error::NoError;
}

ProtocolInterface::Error ControllerStateMachine::disableEntityAdvertising(entity::LocalEntity& entity, std::optional<entity::model::AvbInterfaceIndex> const interfaceIndex) noexcept
{
	// Lock self
	std::lock_guard<ControllerStateMachine> const lg(getSelf());

	// Search our local entity info
	auto const& localEntityIt = _localEntities.find(entity.getEntityID());
	if (localEntityIt == _localEntities.end())
		return ProtocolInterface::Error::UnknownLocalEntity;

	auto& localEntity = localEntityIt->second;

	// If interfaceIndex is specified
	if (interfaceIndex)
	{
		// Send a departing message, if advertising was enabled
		if (localEntity.nextAdvertiseTime.erase(*interfaceIndex) != 0)
		{
			auto frame = makeEntityDepartingMessage(entity, *interfaceIndex);
			_delegate->sendMessage(frame);
		}
	}
	else
	{
		// Otherwise disable advertising for all advertising interfaces
		for (auto const& advertiseKV : localEntity.nextAdvertiseTime)
		{
			auto const avbInterfaceIndex = advertiseKV.first;

			auto frame = makeEntityDepartingMessage(entity, avbInterfaceIndex);
			_delegate->sendMessage(frame);
		}
	}

	return ProtocolInterface::Error::NoError;
}

ProtocolInterface::Error ControllerStateMachine::discoverRemoteEntities() noexcept
{
	return discoverRemoteEntity(UniqueIdentifier::getNullUniqueIdentifier());
}

ProtocolInterface::Error ControllerStateMachine::discoverRemoteEntity(UniqueIdentifier const entityID) noexcept
{
	auto frame = makeDiscoveryMessage(entityID);
	return _delegate->sendMessage(frame);
}

void ControllerStateMachine::lock() noexcept
{
	_lock.lock();
}

void ControllerStateMachine::unlock() noexcept
{
	_lock.unlock();
}

Adpdu ControllerStateMachine::makeDiscoveryMessage(UniqueIdentifier const entityID) const noexcept
{
	Adpdu frame;

	// Set Ether2 fields
	frame.setSrcAddress(_protocolInterface->getMacAddress());
	frame.setDestAddress(Adpdu::Multicast_Mac_Address);
	// Set ADP fields
	frame.setMessageType(AdpMessageType::EntityDiscover);
	frame.setValidTime(0);
	frame.setEntityID(entityID);
	frame.setEntityModelID(UniqueIdentifier::getNullUniqueIdentifier());
	frame.setEntityCapabilities(entity::EntityCapabilities::None);
	frame.setTalkerStreamSources(0);
	frame.setTalkerCapabilities(entity::TalkerCapabilities::None);
	frame.setListenerStreamSinks(0);
	frame.setListenerCapabilities(entity::ListenerCapabilities::None);
	frame.setControllerCapabilities(entity::ControllerCapabilities::None);
	frame.setAvailableIndex(0);
	frame.setGptpGrandmasterID(UniqueIdentifier::getNullUniqueIdentifier());
	frame.setGptpDomainNumber(0);
	frame.setIdentifyControlIndex(0);
	frame.setInterfaceIndex(0);
	frame.setAssociationID(UniqueIdentifier::getNullUniqueIdentifier());

	return frame;
}

Adpdu ControllerStateMachine::makeEntityAvailableMessage(entity::Entity& entity, entity::model::AvbInterfaceIndex const interfaceIndex) const
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
		addFlag(entityCaps, entity::EntityCapabilities::AemIdentifyControlIndexValid);
		identifyControlIndex = *entity.getIdentifyControlIndex();
	}
	else
	{
		// We don't have a valid IdentifyControlIndex, don't set the flag
		clearFlag(entityCaps, entity::EntityCapabilities::AemIdentifyControlIndexValid);
	}

	if (interfaceIndex != entity::Entity::GlobalAvbInterfaceIndex)
	{
		addFlag(entityCaps, entity::EntityCapabilities::AemInterfaceIndexValid);
		avbInterfaceIndex = interfaceIndex;
	}
	else
	{
		// We don't have a valid AvbInterfaceIndex, don't set the flag
		clearFlag(entityCaps, entity::EntityCapabilities::AemInterfaceIndexValid);
	}

	if (entity.getAssociationID())
	{
		addFlag(entityCaps, entity::EntityCapabilities::AssociationIDValid);
		associationID = *entity.getAssociationID();
	}
	else
	{
		// We don't have a valid AssociationID, don't set the flag
		clearFlag(entityCaps, entity::EntityCapabilities::AssociationIDValid);
	}

	if (interfaceInfo.gptpGrandmasterID)
	{
		addFlag(entityCaps, entity::EntityCapabilities::GptpSupported);
		gptpGrandmasterID = *interfaceInfo.gptpGrandmasterID;
		if (AVDECC_ASSERT_WITH_RET(interfaceInfo.gptpDomainNumber.has_value(), "gptpDomainNumber should be set when gptpGrandmasterID is set"))
		{
			gptpDomainNumber = *interfaceInfo.gptpDomainNumber;
		}
	}
	else
	{
		// We don't have a valid gptpGrandmasterID value, don't set the flag
		clearFlag(entityCaps, entity::EntityCapabilities::GptpSupported);
	}

	Adpdu frame;
	// Set Ether2 fields
	frame.setSrcAddress(_protocolInterface->getMacAddress());
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

Adpdu ControllerStateMachine::makeEntityDepartingMessage(entity::Entity& entity, entity::model::AvbInterfaceIndex const interfaceIndex) const noexcept
{
	Adpdu frame;
	auto entityCaps{ entity::EntityCapabilities::None };
	auto avbInterfaceIndex{ entity::model::AvbInterfaceIndex{ 0u } };

	if (interfaceIndex != entity::Entity::GlobalAvbInterfaceIndex)
	{
		addFlag(entityCaps, entity::EntityCapabilities::AemInterfaceIndexValid);
		avbInterfaceIndex = interfaceIndex;
	}

	// Set Ether2 fields
	frame.setSrcAddress(_protocolInterface->getMacAddress());
	frame.setDestAddress(Adpdu::Multicast_Mac_Address);
	// Set ADP fields
	frame.setMessageType(AdpMessageType::EntityDeparting);
	frame.setValidTime(0);
	frame.setEntityID(entity.getEntityID());
	frame.setEntityModelID(UniqueIdentifier::getNullUniqueIdentifier());
	frame.setEntityCapabilities(entityCaps);
	frame.setTalkerStreamSources(0);
	frame.setTalkerCapabilities(entity::TalkerCapabilities::None);
	frame.setListenerStreamSinks(0);
	frame.setListenerCapabilities(entity::ListenerCapabilities::None);
	frame.setControllerCapabilities(entity::ControllerCapabilities::None);
	frame.setAvailableIndex(0);
	frame.setGptpGrandmasterID(UniqueIdentifier::getNullUniqueIdentifier());
	frame.setGptpDomainNumber(0);
	frame.setIdentifyControlIndex(0);
	frame.setInterfaceIndex(avbInterfaceIndex);
	frame.setAssociationID(UniqueIdentifier::getNullUniqueIdentifier());

	return frame;
}

void ControllerStateMachine::resetAecpCommandTimeoutValue(AecpCommandInfo& command) const noexcept
{
	static std::unordered_map<AecpMessageType, std::uint32_t, AecpMessageType::Hash> s_AecpCommandTimeoutMap{
		{ AecpMessageType::AemCommand, AecpAemCommandTimeoutMsec },
		{ AecpMessageType::AddressAccessCommand, AecpAaCommandTimeoutMsec },
		{ AecpMessageType::VendorUniqueCommand, AecpVuCommandTimeoutMsec },
	};

	std::uint32_t timeout{ 250 };
	auto const it = s_AecpCommandTimeoutMap.find(command.command->getMessageType());
	if (AVDECC_ASSERT_WITH_RET(it != s_AecpCommandTimeoutMap.end(), "Timeout for AECP message not defined!"))
	{
		timeout = it->second;
	}

	command.timeout = std::chrono::system_clock::now() + std::chrono::milliseconds(timeout);
}

void ControllerStateMachine::resetAcmpCommandTimeoutValue(AcmpCommandInfo& command) const noexcept
{
	static std::unordered_map<AcmpMessageType, std::uint32_t, AcmpMessageType::Hash> s_AcmpCommandTimeoutMap{
		{ AcmpMessageType::ConnectTxCommand, AcmpConnectTxCommandTimeoutMsec },
		{ AcmpMessageType::DisconnectTxCommand, AcmpDisconnectTxCommandTimeoutMsec },
		{ AcmpMessageType::GetTxStateCommand, AcmpGetTxStateCommandTimeoutMsec },
		{ AcmpMessageType::ConnectRxCommand, AcmpConnectRxCommandTimeoutMsec },
		{ AcmpMessageType::DisconnectRxCommand, AcmpDisconnectRxCommandTimeoutMsec },
		{ AcmpMessageType::GetRxStateCommand, AcmpGetRxStateCommandTimeoutMsec },
		{ AcmpMessageType::GetTxConnectionCommand, AcmpGetTxConnectionCommandTimeoutMsec },
	};

	std::uint32_t timeout{ 250 };
	auto const it = s_AcmpCommandTimeoutMap.find(command.command->getMessageType());
	if (AVDECC_ASSERT_WITH_RET(it != s_AcmpCommandTimeoutMap.end(), "Timeout for ACMP message not defined!"))
	{
		timeout = it->second;
	}

	command.timeout = std::chrono::system_clock::now() + std::chrono::milliseconds(timeout);
}

AecpSequenceID ControllerStateMachine::getNextAecpSequenceID(LocalEntityInfo& info) noexcept
{
	auto const nextID = info.currentAecpSequenceID;
	++info.currentAecpSequenceID;
	return nextID;
}

AcmpSequenceID ControllerStateMachine::getNextAcmpSequenceID(LocalEntityInfo& info) noexcept
{
	auto const nextID = info.currentAcmpSequenceID;
	++info.currentAcmpSequenceID;
	return nextID;
}

std::chrono::milliseconds ControllerStateMachine::computeRandomDelay(entity::Entity const& entity, entity::model::AvbInterfaceIndex const interfaceIndex) const noexcept
{
	auto const& interfaceInfo = entity.getInterfaceInformation(interfaceIndex);
	auto const maxRandValue{ interfaceInfo.validTime * 1000u * 2u / 5u }; // Maximum value for rand range is 1/5 of the "valid time period" (which is twice the validTime field)
	auto const randomValue{ std::rand() % maxRandValue };
	return std::chrono::milliseconds(randomValue);
}

std::chrono::time_point<std::chrono::system_clock> ControllerStateMachine::computeNextAdvertiseTime(entity::Entity const& entity, entity::model::AvbInterfaceIndex const interfaceIndex) const
{
	auto const& interfaceInfo = entity.getInterfaceInformation(interfaceIndex);
	return std::chrono::system_clock::now() + std::chrono::milliseconds(std::max(1000u, interfaceInfo.validTime * 1000u / 2u)) + computeRandomDelay(entity, interfaceIndex);
}

std::chrono::time_point<std::chrono::system_clock> ControllerStateMachine::computeDelayedAdvertiseTime(entity::Entity const& entity, entity::model::AvbInterfaceIndex const interfaceIndex) const
{
	return std::chrono::system_clock::now() + computeRandomDelay(entity, interfaceIndex);
}

void ControllerStateMachine::checkLocalEntitiesAnnouncement() noexcept
{
	// Lock self
	std::lock_guard<ControllerStateMachine> const lg(getSelf());

	auto const now = std::chrono::system_clock::now();

	for (auto& entityKV : _localEntities)
	{
		auto& entityInfo = entityKV.second;
		auto& entity = entityInfo.entity;

		// Lock the whole entity while checking dirty state and building the EntityAvailable message so that nobody alters discovery fields at the same time
		std::lock_guard<entity::LocalEntity> const elg(entity);

		// Continue to next entity if advertising is disabled for this one
		if (entityInfo.nextAdvertiseTime.empty())
			continue;

		// Send an EntityAvailable message if the advertise timeout expired
		for (auto& advertiseKV : entityInfo.nextAdvertiseTime)
		{
			auto const interfaceIndex = advertiseKV.first;
			auto& advertiseTime = advertiseKV.second;

			if (now >= advertiseTime)
			{
				try
				{
					// Build the EntityAvailable message
					auto frame = makeEntityAvailableMessage(entity, interfaceIndex);
					// Send it
					_delegate->sendMessage(frame);
					// Update the time for next advertise
					advertiseTime = computeNextAdvertiseTime(entity, interfaceIndex);
				}
				catch (...)
				{
					AVDECC_ASSERT(false, "Should not happen");
				}
			}
		}
	}
}

void ControllerStateMachine::checkEntitiesTimeoutExpiracy() noexcept
{
	// Lock self
	std::lock_guard<ControllerStateMachine> const lg(getSelf());

	// Get current time
	std::chrono::time_point<std::chrono::system_clock> currentTime = std::chrono::system_clock::now();

	for (auto discoveredEntityKV = _discoveredEntities.begin(); discoveredEntityKV != _discoveredEntities.end(); /* Iterate inside the loop */)
	{
		auto& entity = discoveredEntityKV->second;
		auto hasInterfaceTimeout{ false };
		auto shouldRemoveEntity{ false };

		// Check timeout value for all interfaces
		for (auto timeoutKV = entity.timeouts.begin(); timeoutKV != entity.timeouts.end(); /* Iterate inside the loop */)
		{
			auto const timeout = timeoutKV->second;
			if (currentTime > timeout)
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
				invokeProtectedMethod(&Delegate::onRemoteEntityOffline, _delegate, discoveredEntityKV->first);
				shouldRemoveEntity = true;
			}
			// Otherwise just notify an update
			else
			{
				// Notify this entity has been updated
				invokeProtectedMethod(&Delegate::onRemoteEntityUpdated, _delegate, entity.entity);
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

void ControllerStateMachine::checkInflightCommandsTimeoutExpiracy() noexcept
{
	// Lock self
	std::lock_guard<ControllerStateMachine> const lg(getSelf());

	// Get current time
	std::chrono::time_point<std::chrono::system_clock> currentTime = std::chrono::system_clock::now();

	for (auto& localEntityKV : _localEntities)
	{
		auto& localEntity = localEntityKV.second;

		// Check AECP commands
		for (auto& inflightKV : localEntity.inflightAecpCommands)
		{
			auto& entityID = inflightKV.first;
			auto& inflight = inflightKV.second;
			for (auto it = inflight.begin(); it != inflight.end(); /* Iterate inside the loop */)
			{
				auto& command = *it;
				if (currentTime > command.timeout)
				{
					auto error = ProtocolInterface::Error::NoError;
					// Timeout expired, check if we retried yet
					if (!command.retried)
					{
						// Let's retry
						command.retried = true;
						error = _delegate->sendMessage(static_cast<Aecpdu const&>(*command.command));
						resetAecpCommandTimeoutValue(command);
						LOG_CONTROLLER_STATE_MACHINE_DEBUG(entityID, std::string("AECP command with sequenceID ") + std::to_string(command.sequenceID) + " timed out, trying again");
					}
					else
					{
						error = ProtocolInterface::Error::Timeout;
						LOG_CONTROLLER_STATE_MACHINE_DEBUG(entityID, std::string("AECP command with sequenceID ") + std::to_string(command.sequenceID) + " timed out 2 times");
					}

					if (!!error)
					{
						// Already retried, the command has been lost
						invokeProtectedHandler(command.resultHandler, nullptr, error);
						it = removeInflight(localEntity, entityID, inflight, it);
					}
				}
				else
					++it;
			}
		}
		// Check ACMP commands
		for (auto it = localEntity.inflightAcmpCommands.begin(); it != localEntity.inflightAcmpCommands.end(); /* Iterate inside the loop */)
		{
			auto& command = it->second;
			if (currentTime > command.timeout)
			{
				auto error = ProtocolInterface::Error::NoError;
				// Timeout expired, check if we retried yet
				if (!command.retried)
				{
					// Let's retry
					command.retried = true;
					error = _delegate->sendMessage(static_cast<Acmpdu const&>(*command.command));
					resetAcmpCommandTimeoutValue(command);
				}
				else
				{
					error = ProtocolInterface::Error::Timeout;
				}

				if (!!error)
				{
					// Already retried, the command has been lost
					invokeProtectedHandler(command.resultHandler, nullptr, error);
					it = localEntity.inflightAcmpCommands.erase(it);
				}
			}
			else
				++it;
		}
		// Notify scheduled errors
		for (auto const& e : localEntity.scheduledAecpErrors)
		{
			invokeProtectedHandler(e.second, nullptr, e.first);
		}
		localEntity.scheduledAecpErrors.clear();
	}
}

void ControllerStateMachine::handleAdpEntityAvailable(Adpdu const& adpdu) noexcept
{
	// Ignore messages from a local entity
	if (isLocalEntity(adpdu.getEntityID()))
	{
		return;
	}

	// If entity is not ready
	if (hasFlag(adpdu.getEntityCapabilities(), entity::EntityCapabilities::EntityNotReady))
	{
		return;
	}

	auto const entityID = adpdu.getEntityID();
	auto notify{ true };
	auto update{ false };
	auto simulateOffline{ false };
	DiscoveredEntityInfo* discoveredInfo{ nullptr };
	auto entity{ makeEntity(adpdu) };
	auto const avbInterfaceIndex{ entity.getInterfacesInformation().begin()->first };

	// Lock self
	std::lock_guard<ControllerStateMachine> const lg(getSelf());

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
	discoveredInfo->timeouts[avbInterfaceIndex] = std::chrono::system_clock::now() + std::chrono::seconds(2 * adpdu.getValidTime());

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
			invokeProtectedMethod(&Delegate::onRemoteEntityOffline, _delegate, entityID);
		}

		if (update)
		{
			invokeProtectedMethod(&Delegate::onRemoteEntityUpdated, _delegate, discoveredInfo->entity);
		}
		else
		{
			invokeProtectedMethod(&Delegate::onRemoteEntityOnline, _delegate, discoveredInfo->entity);
		}
	}
}

void ControllerStateMachine::handleAdpEntityDeparting(Adpdu const& adpdu) noexcept
{
	// Ignore messages from a local entity
	if (isLocalEntity(adpdu.getEntityID()))
		return;

	auto const entityID = adpdu.getEntityID();

	// Lock self
	std::lock_guard<ControllerStateMachine> const lg(getSelf());

	// Check if we already know this entity
	auto entityIt = _discoveredEntities.find(entityID);

	// Not found it in the list, ignore it
	if (entityIt == _discoveredEntities.end())
		return;

	// Remove from the list
	_discoveredEntities.erase(entityIt);

	// Notify delegate
	invokeProtectedMethod(&Delegate::onRemoteEntityOffline, _delegate, entityID);
}

void ControllerStateMachine::handleAdpEntityDiscover(Adpdu const& adpdu) noexcept
{
	auto const entityID = adpdu.getEntityID();

	// Don't ignore requests coming from the same computer, we might have another controller running on it

	// Check if one (or many) of our local entities is targetted by the discover request
	// Lock self
	std::lock_guard<ControllerStateMachine> const lg(getSelf());

	for (auto& entityKV : _localEntities)
	{
		auto& entityInfo = entityKV.second;
		auto const& entity = entityInfo.entity;

		// Check on which interface we received the message so we only respond to this one (if not found then advertise is not active on this interface)
		auto const& destAddress = adpdu.getDestAddress();
		for (auto& advertiseKV : entityInfo.nextAdvertiseTime)
		{
			auto const interfaceIndex = advertiseKV.first;
			auto& advertiseTime = advertiseKV.second;
			try
			{
				auto const& interfaceInfo = entity.getInterfaceInformation(interfaceIndex);
				if (destAddress == interfaceInfo.macAddress)
				{
					// Only reply to global (entityID == 0) discovery messages and to targeted ones
					if (!entityID || entityID == entity.getEntityID())
					{
						// Schedule EntityAvailable message
						advertiseTime = computeDelayedAdvertiseTime(entity, interfaceIndex);
					}
				}
			}
			catch (Exception const&)
			{
				AVDECC_ASSERT(false, "avbInterfaceIndex might not be valid");
			}
		}
	}
}

bool ControllerStateMachine::isLocalEntity(UniqueIdentifier const entityID) const noexcept
{
	// Lock self
	std::lock_guard<ControllerStateMachine> const lg(getSelf());

	return _localEntities.find(entityID) != _localEntities.end();
}

} // namespace stateMachine
} // namespace protocol
} // namespace avdecc
} // namespace la
