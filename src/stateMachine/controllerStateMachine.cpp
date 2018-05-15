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

#include "controllerStateMachine.hpp"
#include "protocol/protocolAemAecpdu.hpp"
#include <utility>
#include <stdexcept>
#include <chrono>
#include <algorithm>
#include "la/avdecc/logger.hpp"

namespace la
{
namespace avdecc
{
namespace protocol
{
namespace stateMachine
{

/* Aecp commands timeout - Clause 9.2.1.2.5 */
static constexpr auto AecpCommandTimeoutMsec = 250u;
/* Acmp commands timeout - Clause 8.2.2 */
static constexpr auto AcmpConnectTxCommandTimeoutMsec = 2000u;
static constexpr auto AcmpDisconnectTxCommandTimeoutMsec = 200u;
static constexpr auto AcmpGetTxStateCommandTimeoutMsec = 200u;
static constexpr auto AcmpConnectRxCommandTimeoutMsec = 4500u;
static constexpr auto AcmpDisconnectRxCommandTimeoutMsec = 500u;
static constexpr auto AcmpGetRxStateCommandTimeoutMsec = 200u;
static constexpr auto AcmpGetTxConnectionCommandTimeoutMsec = 200u;

ControllerStateMachine::ControllerStateMachine(ProtocolInterface const* const protocolInterface, Delegate* const delegate, size_t const maxInflightAecpMessages)
	: _protocolInterface(protocolInterface), _delegate(delegate), _maxInflightAecpMessages(maxInflightAecpMessages)
{
	if (_delegate == nullptr)
		throw Exception("ControllerStateMachine's delegate cannot be nullptr");

	// Create the state machine thread
	_stateMachineThread = std::thread([this]
	{
		setCurrentThreadName("avdecc::ControllerStateMachine");
		while (!_shouldTerminate)
		{
			// Check for local entities announcement
			checkLocalEntitiesAnnouncement();

			// Check for timeout expiracy on all entities
			checkEntitiesTimeoutExpiracy();

			// Check for inflight commands expiracy
			checkInflightCommandsTimeoutExpiracy();

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
		{ AdpMessageType::EntityAvailable.getValue(), [](ControllerStateMachine* const stateMachine, Adpdu const& adpdu)
			{
				stateMachine->handleAdpEntityAvailable(adpdu);
			}
		},
		// Entity Departing
		{ AdpMessageType::EntityDeparting.getValue(), [](ControllerStateMachine* const stateMachine, Adpdu const& adpdu)
			{
				stateMachine->handleAdpEntityDeparting(adpdu);
			}
		},
		// Entity Discover
		{ AdpMessageType::EntityDiscover.getValue(), [](ControllerStateMachine* const stateMachine, Adpdu const& adpdu)
			{
				stateMachine->handleAdpEntityDiscover(adpdu);
			}
		},
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
					auto commandIt = std::find_if(inflight.begin(), inflight.end(), [sequenceID](AecpCommandInfo const& command)
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
						Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("AECP command with sequenceID ") + std::to_string(sequenceID) + " unexpected (timed out already?)");
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
			// Disable advertising
			disableEntityAdvertising(entity);
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

ProtocolInterface::Error ControllerStateMachine::enableEntityAdvertising(entity::LocalEntity const& entity) noexcept
{
	// Lock self
	std::lock_guard<ControllerStateMachine> const lg(getSelf());

	// Search our local entity info
	auto const& localEntityIt = _localEntities.find(entity.getEntityID());
	if (localEntityIt == _localEntities.end())
		return ProtocolInterface::Error::UnknownLocalEntity;
	auto& localEntity = localEntityIt->second;

	localEntity.isAdvertising = true;
	// No need to update nextAdvertiseAt value, we want to advertise asap

	return ProtocolInterface::Error::NoError;
}

ProtocolInterface::Error ControllerStateMachine::disableEntityAdvertising(entity::LocalEntity& entity) noexcept
{
	// Lock self
	std::lock_guard<ControllerStateMachine> const lg(getSelf());

	// Search our local entity info
	auto const& localEntityIt = _localEntities.find(entity.getEntityID());
	if (localEntityIt == _localEntities.end())
		return ProtocolInterface::Error::UnknownLocalEntity;
	auto& localEntity = localEntityIt->second;

	// Send a departing message, if advertising was enabled
	if (localEntity.isAdvertising)
	{
		auto frame = makeEntityDepartingMessage(entity);
		_delegate->sendMessage(frame);
	}

	localEntity.isAdvertising = false;
	// No need to reset nextAdvertiseAt value, we don't want to advertise immediately in case we re-enable advertising too quickly

	return ProtocolInterface::Error::NoError;
}

ProtocolInterface::Error ControllerStateMachine::discoverRemoteEntities() noexcept
{
	return discoverRemoteEntity(getNullIdentifier());
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
	frame.setEntityModelID(getNullIdentifier());
	frame.setEntityCapabilities(entity::EntityCapabilities::None);
	frame.setTalkerStreamSources(0);
	frame.setTalkerCapabilities(entity::TalkerCapabilities::None);
	frame.setListenerStreamSinks(0);
	frame.setListenerCapabilities(entity::ListenerCapabilities::None);
	frame.setControllerCapabilities(entity::ControllerCapabilities::None);
	frame.setAvailableIndex(0);
	frame.setGptpGrandmasterID(getNullIdentifier());
	frame.setGptpDomainNumber(0);
	frame.setIdentifyControlIndex(0);
	frame.setInterfaceIndex(0);
	frame.setAssociationID(getNullIdentifier());

	return frame;
}

Adpdu ControllerStateMachine::makeEntityAvailableMessage(entity::Entity& entity) const noexcept
{
	Adpdu frame;

	// Set Ether2 fields
	frame.setSrcAddress(_protocolInterface->getMacAddress());
	frame.setDestAddress(Adpdu::Multicast_Mac_Address);
	// Set ADP fields
	frame.setMessageType(AdpMessageType::EntityAvailable);
	frame.setValidTime(entity.getValidTime());
	frame.setEntityID(entity.getEntityID());
	frame.setEntityModelID(entity.getVendorEntityModelID());
	frame.setEntityCapabilities(entity.getEntityCapabilities());
	frame.setTalkerStreamSources(entity.getTalkerStreamSources());
	frame.setTalkerCapabilities(entity.getTalkerCapabilities());
	frame.setListenerStreamSinks(entity.getListenerStreamSinks());
	frame.setListenerCapabilities(entity.getListenerCapabilities());
	frame.setControllerCapabilities(entity.getControllerCapabilities());
	frame.setAvailableIndex(entity.getNextAvailableIndex());
	frame.setGptpGrandmasterID(entity.getGptpGrandmasterID());
	frame.setGptpDomainNumber(entity.getGptpDomainNumber());
	frame.setIdentifyControlIndex(entity.getIdentifyControlIndex());
	frame.setInterfaceIndex(entity.getInterfaceIndex());
	frame.setAssociationID(entity.getAssociationID());

	return frame;
}

Adpdu ControllerStateMachine::makeEntityDepartingMessage(entity::Entity& entity) const noexcept
{
	Adpdu frame;

	// Set Ether2 fields
	frame.setSrcAddress(_protocolInterface->getMacAddress());
	frame.setDestAddress(Adpdu::Multicast_Mac_Address);
	// Set ADP fields
	frame.setMessageType(AdpMessageType::EntityDeparting);
	frame.setValidTime(0);
	frame.setEntityID(entity.getEntityID());
	frame.setEntityModelID(getNullIdentifier());
	frame.setEntityCapabilities(entity::EntityCapabilities::None);
	frame.setTalkerStreamSources(0);
	frame.setTalkerCapabilities(entity::TalkerCapabilities::None);
	frame.setListenerStreamSinks(0);
	frame.setListenerCapabilities(entity::ListenerCapabilities::None);
	frame.setControllerCapabilities(entity::ControllerCapabilities::None);
	frame.setAvailableIndex(0);
	frame.setGptpGrandmasterID(getNullIdentifier());
	frame.setGptpDomainNumber(0);
	frame.setIdentifyControlIndex(0);
	frame.setInterfaceIndex(0);
	frame.setAssociationID(getNullIdentifier());

	return frame;
}

void ControllerStateMachine::resetAecpCommandTimeoutValue(AecpCommandInfo& command) const noexcept
{
	command.timeout = std::chrono::system_clock::now() + std::chrono::milliseconds(AecpCommandTimeoutMsec);
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
		timeout = it->second;

	command.timeout = std::chrono::system_clock::now() + std::chrono::milliseconds(timeout);
}

AecpSequenceID ControllerStateMachine::getNextAecpSequenceID(LocalEntityInfo& info) noexcept
{
	info.currentAecpSequenceID++;
	if (info.currentAecpSequenceID == 0) // Don't allow 0 as a valid sequenceID value
		info.currentAecpSequenceID++;
	return info.currentAecpSequenceID;
}

AcmpSequenceID ControllerStateMachine::getNextAcmpSequenceID(LocalEntityInfo& info) noexcept
{
	info.currentAcmpSequenceID++;
	if (info.currentAcmpSequenceID == 0) // Don't allow 0 as a valid sequenceID value
		info.currentAcmpSequenceID++;
	return info.currentAcmpSequenceID;
}

std::chrono::time_point<std::chrono::system_clock> ControllerStateMachine::computeNextAdvertiseTime(entity::Entity const& entity) const noexcept
{
#pragma message("TODO: Compute a random delay. See IEEE-P1722.1-cor1-D8.pdf clause 6.2.4.2.2")
	auto const randomDelay{ 0u };
	return std::chrono::system_clock::now() + std::chrono::milliseconds(std::max(1000u, entity.getValidTime() * 1000 / 2 + randomDelay));
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

		// Continue to next entity if advertising is disabled for this one
		if (!entityInfo.isAdvertising)
			continue;

		// Lock the whole entity while checking dirty state and building the EntityAvailable message so that nobody alters discovery fields at the same time
		std::lock_guard<entity::LocalEntity> const elg(entity);

		// Send an EntityAvailable message if entity is dirty or if the advertise timeout expired
		if (entity.isDirty() || now >= entityInfo.nextAdvertiseAt)
		{
			// Build the EntityAvailable message
			auto frame = makeEntityAvailableMessage(entity);
			// Send it
			_delegate->sendMessage(frame);
			// Update the time for next advertise
			entityInfo.nextAdvertiseAt = computeNextAdvertiseTime(entity);
		}
	}
}

void ControllerStateMachine::checkEntitiesTimeoutExpiracy() noexcept
{
	std::chrono::time_point<std::chrono::system_clock> currentTime = std::chrono::system_clock::now();

	// Lock self
	std::lock_guard<ControllerStateMachine> const lg(getSelf());

	for (auto it = _discoveredEntities.begin(); it != _discoveredEntities.end(); /* Iterate inside the loop */)
	{
		auto const& entity = it->second;
		if (currentTime > entity.timeout)
		{
			// Notify this entity is offline
			invokeProtectedMethod(&Delegate::onRemoteEntityOffline, _delegate, it->first);
			// Remove it from the list of known entities
			it = _discoveredEntities.erase(it);
		}
		else
			++it;
	}
}

void ControllerStateMachine::checkInflightCommandsTimeoutExpiracy() noexcept
{
	std::chrono::time_point<std::chrono::system_clock> currentTime = std::chrono::system_clock::now();

	// Lock self
	std::lock_guard<ControllerStateMachine> const lg(getSelf());

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
						Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("AECP command with sequenceID ") + std::to_string(command.sequenceID) + " timed out, trying again");
					}
					else
					{
						error = ProtocolInterface::Error::Timeout;
						Logger::getInstance().log(Logger::Layer::Protocol, Logger::Level::Debug, std::string("AECP command with sequenceID ") + std::to_string(command.sequenceID) + " timed out 2 times");
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
			return;

	// If entity is not ready
	if (hasFlag(adpdu.getEntityCapabilities(), entity::EntityCapabilities::EntityNotReady))
		return;

	auto const entityID = adpdu.getEntityID();
	bool notify = true;
	bool update = false;
	bool notAllowedUpdate = false;
	// Compute timeout value
	std::chrono::time_point<std::chrono::system_clock> timeout = std::chrono::system_clock::now() + std::chrono::seconds(2 * adpdu.getValidTime());
	DiscoveredEntityInfo info{ timeout, adpdu };
	Adpdu previousAdpdu;

	// Lock self
	std::lock_guard<ControllerStateMachine> const lg(getSelf());

	// Check if we already know this entity
	auto entityIt = _discoveredEntities.find(entityID);

	// Found it in the list, check if data are the same
	if (entityIt != _discoveredEntities.end())
	{
		previousAdpdu = std::move(entityIt->second.adpdu);
		// Get adpdu difference
		auto const diff = getAdpdusDiff(previousAdpdu, adpdu);
		if (diff == AdpduDiff::Same)
			notify = false;
		// Always update info
		entityIt->second = info;
		notAllowedUpdate = diff == AdpduDiff::DiffNotAllowed;
		update = !notAllowedUpdate;
	}
	// Not found, create a new entity
	else
	{
		_discoveredEntities[entityID] = info;
	}

	// Notify delegate
	if (notify && _delegate != nullptr)
	{
		auto entity = makeEntity(adpdu);
		// Adpdu diff is not allowed, simulate entity offline/online
		if (notAllowedUpdate)
			invokeProtectedMethod(&Delegate::onRemoteEntityOffline, _delegate, entityID);

		if (update)
			invokeProtectedMethod(&Delegate::onRemoteEntityUpdated, _delegate, entity);
		else
			invokeProtectedMethod(&Delegate::onRemoteEntityOnline, _delegate, entity);
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
		auto& entity = entityInfo.entity;
		// Only reply to global (entityID == 0) discovery messages (only if advertising is active) and to targeted ones
		if ((!isValidUniqueIdentifier(entityID) && entityInfo.isAdvertising) || entityID == entity.getEntityID())
		{
			// Build the EntityAvailable message
			auto frame = makeEntityAvailableMessage(entity);
			// Send it
			_delegate->sendMessage(frame);
			// Update the time for next advertise (only if isAdvertising is true, meaning it's not a targeted message)
			if (entityInfo.isAdvertising)
			{
				entityInfo.nextAdvertiseAt = computeNextAdvertiseTime(entity);
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

ControllerStateMachine::AdpduDiff ControllerStateMachine::getAdpdusDiff(Adpdu const& lhs, Adpdu const& rhs) const noexcept
{
	// First check not allowed changed fields
	if (lhs.getEntityID() != rhs.getEntityID())
		return AdpduDiff::DiffNotAllowed;
	if (lhs.getSrcAddress() != rhs.getSrcAddress())
		return AdpduDiff::DiffNotAllowed;
	if (lhs.getEntityModelID() != rhs.getEntityModelID())
		return AdpduDiff::DiffNotAllowed;
	if (lhs.getTalkerStreamSources() != rhs.getTalkerStreamSources())
		return AdpduDiff::DiffNotAllowed;
	if (lhs.getTalkerCapabilities() != rhs.getTalkerCapabilities())
		return AdpduDiff::DiffNotAllowed;
	if (lhs.getListenerStreamSinks() != rhs.getListenerStreamSinks())
		return AdpduDiff::DiffNotAllowed;
	if (lhs.getListenerCapabilities() != rhs.getListenerCapabilities())
		return AdpduDiff::DiffNotAllowed;
	if (lhs.getControllerCapabilities() != rhs.getControllerCapabilities())
		return AdpduDiff::DiffNotAllowed;
	if (lhs.getIdentifyControlIndex() != rhs.getIdentifyControlIndex())
		return AdpduDiff::DiffNotAllowed;
	if (lhs.getInterfaceIndex() != rhs.getInterfaceIndex())
		return AdpduDiff::DiffNotAllowed;
	if (lhs.getAssociationID() != rhs.getAssociationID())
		return AdpduDiff::DiffNotAllowed;

	// Special case for AvailableIndex which always be increasing
	if (lhs.getAvailableIndex() >= rhs.getAvailableIndex())
		return AdpduDiff::DiffNotAllowed;

	// Then check allowed changed fields
	if (lhs.getEntityCapabilities() != rhs.getEntityCapabilities())
		return AdpduDiff::DiffAllowed;
	if (lhs.getGptpGrandmasterID() != rhs.getGptpGrandmasterID())
		return AdpduDiff::DiffAllowed;
	if (lhs.getGptpDomainNumber() != rhs.getGptpDomainNumber())
		return AdpduDiff::DiffAllowed;

	// All other changes are not considered as a diff (ValidTime and AvailableIndex)
	return AdpduDiff::Same;
}

entity::DiscoveredEntity ControllerStateMachine::makeEntity(Adpdu const& adpdu) const noexcept
{
	return entity::DiscoveredEntity{ adpdu.getEntityID(), adpdu.getSrcAddress(), adpdu.getValidTime(), adpdu.getEntityModelID(), adpdu.getEntityCapabilities(),
		adpdu.getTalkerStreamSources(), adpdu.getTalkerCapabilities(),
		adpdu.getListenerStreamSinks(), adpdu.getListenerCapabilities(),
		adpdu.getControllerCapabilities(),
		adpdu.getAvailableIndex(), adpdu.getGptpGrandmasterID(), adpdu.getGptpDomainNumber(),
		adpdu.getIdentifyControlIndex(), adpdu.getInterfaceIndex(), adpdu.getAssociationID()
	};
}

} // namespace stateMachine
} // namespace protocol
} // namespace avdecc
} // namespace la
