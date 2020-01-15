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
* @file commandStateMachine.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/utils.hpp"
#include "la/avdecc/internals/protocolAemAecpdu.hpp"

#include "commandStateMachine.hpp"
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

/* ************************************************************ */
/* Public methods                                               */
/* ************************************************************ */
CommandStateMachine::CommandStateMachine(Manager* manager, Delegate* const delegate) noexcept
	: _manager(manager)
	, _delegate(delegate)
{
}

CommandStateMachine::~CommandStateMachine() noexcept {}

void CommandStateMachine::registerLocalEntity(entity::LocalEntity& entity) noexcept
{
	// Lock
	auto const lg = std::lock_guard{ *_manager };

	// Check if entity already registered
	auto const entityID = entity.getEntityID();
	auto const infoIt = _commandEntities.find(entityID);
	if (infoIt == _commandEntities.end())
	{
		// Register LocalEntity for Advertising
		_commandEntities.emplace(std::make_pair(entityID, CommandEntityInfo{ entity }));
	}
}

void CommandStateMachine::unregisterLocalEntity(entity::LocalEntity& entity) noexcept
{
	// Lock
	auto const lg = std::lock_guard{ *_manager };

	// Check if entity already registered
	auto const entityID = entity.getEntityID();
	auto const infoIt = _commandEntities.find(entityID);
	if (infoIt != _commandEntities.end())
	{
		// Unregister LocalEntity
		_commandEntities.erase(infoIt);
	}
}

void CommandStateMachine::checkInflightCommandsTimeoutExpiracy() noexcept
{
	// Lock
	auto const lg = std::lock_guard{ *_manager };

	// Get current time
	auto const now = std::chrono::steady_clock::now();

	auto* const protocolInterface = _manager->getProtocolInterfaceDelegate();

	// Iterate over all locally registered command entities
	for (auto& localEntityInfoKV : _commandEntities)
	{
		auto& localEntityInfo = localEntityInfoKV.second;

		// Check AECP commands
		for (auto& [targetEntityID, inflight] : localEntityInfo.inflightAecpCommands)
		{
			for (auto it = inflight.begin(); it != inflight.end(); /* Iterate inside the loop */)
			{
				auto& command = *it;
				if (now > command.timeoutTime)
				{
					auto error = ProtocolInterface::Error::NoError;
					// Timeout expired, check if we retried yet
					if (!command.retried)
					{
						// Let's retry
						command.retried = true;
						error = protocolInterface->sendMessage(static_cast<Aecpdu const&>(*command.command));
						resetAecpCommandTimeoutValue(command);
						// Statistics
						utils::invokeProtectedMethod(&Delegate::onAecpRetry, _delegate, targetEntityID);
						LOG_CONTROLLER_STATE_MACHINE_DEBUG(targetEntityID, std::string("AECP command with sequenceID ") + std::to_string(command.sequenceID) + " timed out, trying again");
					}
					else
					{
						error = ProtocolInterface::Error::Timeout;
						// Statistics
						utils::invokeProtectedMethod(&Delegate::onAecpTimeout, _delegate, targetEntityID);
						LOG_CONTROLLER_STATE_MACHINE_DEBUG(targetEntityID, std::string("AECP command with sequenceID ") + std::to_string(command.sequenceID) + " timed out 2 times");
					}

					if (!!error)
					{
						// Already retried, the command has been lost
						utils::invokeProtectedHandler(command.resultHandler, nullptr, error);
						it = removeInflight(protocolInterface, localEntityInfo, targetEntityID, inflight, it);
					}
				}
				else
				{
					++it;
				}
			}
		}

		// Check ACMP commands
		for (auto it = localEntityInfo.inflightAcmpCommands.begin(); it != localEntityInfo.inflightAcmpCommands.end(); /* Iterate inside the loop */)
		{
			auto& command = it->second;
			if (now > command.timeoutTime)
			{
				auto error = ProtocolInterface::Error::NoError;
				// Timeout expired, check if we retried yet
				if (!command.retried)
				{
					// Let's retry
					command.retried = true;
					error = protocolInterface->sendMessage(static_cast<Acmpdu const&>(*command.command));
					resetAcmpCommandTimeoutValue(command);
				}
				else
				{
					error = ProtocolInterface::Error::Timeout;
				}

				if (!!error)
				{
					// Already retried, the command has been lost
					utils::invokeProtectedHandler(command.resultHandler, nullptr, error);
					it = localEntityInfo.inflightAcmpCommands.erase(it);
				}
			}
			else
			{
				++it;
			}
		}

		// Notify scheduled errors
		for (auto const& e : localEntityInfo.scheduledAecpErrors)
		{
			utils::invokeProtectedHandler(e.second, nullptr, e.first);
		}
		localEntityInfo.scheduledAecpErrors.clear();
	}
}

void CommandStateMachine::handleAecpResponse(Aecpdu const& aecpdu) noexcept
{
	// Lock
	auto const lg = std::lock_guard{ *_manager };

	// Get current time
	auto const now = std::chrono::steady_clock::now();

	auto* const protocolInterface = _manager->getProtocolInterfaceDelegate();
	auto const controllerID = aecpdu.getControllerEntityID();

	// First check if we received a multicast IdentifyNotification
	if (controllerID == AemAecpdu::Identify_ControllerEntityID)
	{
		// Check if it's an AEM unsolicited response
		if (isAEMUnsolicitedResponse(aecpdu))
		{
			utils::invokeProtectedMethod(&Delegate::onAecpAemIdentifyNotification, _delegate, static_cast<AemAecpdu const&>(aecpdu));
		}
		else
		{
			LOG_PROTOCOL_INTERFACE_WARN(aecpdu.getSrcAddress(), aecpdu.getDestAddress(), std::string("Received an AECP response message with controller_entity_id set to the IDENTIFY ControllerID, but the message is not an unsolicited AEM response"));
		}
	}

	// Only process it if it's targeted to a registered local command entity (which is set in the ControllerID field)
	auto const commandEntityIt = _commandEntities.find(controllerID);
	if (commandEntityIt != _commandEntities.end())
	{
		// Check if it's an AEM unsolicited response
		if (isAEMUnsolicitedResponse(aecpdu))
		{
			utils::invokeProtectedMethod(&Delegate::onAecpAemUnsolicitedResponse, _delegate, static_cast<AemAecpdu const&>(aecpdu));
		}
		else
		{
			auto& commandEntityInfo = commandEntityIt->second;
			auto const targetID = aecpdu.getTargetEntityID();

			auto inflightIt = commandEntityInfo.inflightAecpCommands.find(targetID);
			if (inflightIt != commandEntityInfo.inflightAecpCommands.end())
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
						return;
					}

					// Move the query (it will be deleted)
					AecpCommandInfo aecpQuery = std::move(info);

					// Remove the command from inflight list
					removeInflight(protocolInterface, commandEntityInfo, targetID, inflight, commandIt);

					// Call completion handler
					utils::invokeProtectedHandler(aecpQuery.resultHandler, &aecpdu, ProtocolInterface::Error::NoError);

					// Statistics
					utils::invokeProtectedMethod(&Delegate::onAecpResponseTime, _delegate, targetID, std::chrono::duration_cast<std::chrono::milliseconds>(now - aecpQuery.sendTime));
				}
				else
				{
					// Statistics
					utils::invokeProtectedMethod(&Delegate::onAecpUnexpectedResponse, _delegate, targetID);
					LOG_CONTROLLER_STATE_MACHINE_DEBUG(targetID, std::string("AECP command with sequenceID ") + std::to_string(sequenceID) + " unexpected (timed out already?)");
				}
			}
		}
	}
}

void CommandStateMachine::handleAcmpResponse(Acmpdu const& acmpdu) noexcept
{
	// Lock
	auto const lg = std::lock_guard{ *_manager };

	// Only process it if it's targeted to a registered local command entity
#pragma message("TODO: This only work for CONTROLLER messages, not for LISTENER-TALKER communication. Will probably have to check command type")
	auto const controllerID = acmpdu.getControllerEntityID();
	auto const commandEntityIt = _commandEntities.find(controllerID);
	if (commandEntityIt != _commandEntities.end())
	{
		auto& commandEntityInfo = commandEntityIt->second;
		auto const sequenceID = acmpdu.getSequenceID();
		auto commandIt = commandEntityInfo.inflightAcmpCommands.find(sequenceID);
		// If the sequenceID is not found, it either means the response already timed out (arriving too late), or it's a communication btw talker and listener (requested by us) and they did not use our sequenceID
		if (commandIt != commandEntityInfo.inflightAcmpCommands.end())
		{
			auto& info = commandIt->second;

			// Check if it's an expected response (since the communication btw listener and talkers uses our controllerID and might use our sequenceID, we don't want to detect talker's response as ours)
			auto const messageType = acmpdu.getMessageType().getValue();
			auto const expectedResponseType = info.command->getMessageType().getValue() + 1; // Based on Clause 8.2.1.5, responses are always Command + 1
			if (messageType == expectedResponseType)
			{
				// Move the query (it will be deleted)
				AcmpCommandInfo acmpQuery = std::move(info);

				// Remove the command from inflight list
				commandEntityInfo.inflightAcmpCommands.erase(sequenceID);

				// Call completion handler
				utils::invokeProtectedHandler(acmpQuery.resultHandler, &acmpdu, ProtocolInterface::Error::NoError);
			}
		}
	}
}

ProtocolInterface::Error CommandStateMachine::sendAecpCommand(Aecpdu::UniquePointer&& aecpdu, ProtocolInterface::AecpCommandResultHandler const& onResult) noexcept
{
	auto* aecp = static_cast<Aecpdu*>(aecpdu.get());
	auto const targetEntityID = aecp->getTargetEntityID();

	// Lock
	auto const lg = std::lock_guard{ *_manager };

	// Get CommandEntityInfo matching ControllerEntityID
	auto const& commandEntityIt = _commandEntities.find(aecp->getControllerEntityID());
	if (commandEntityIt == _commandEntities.end())
		return ProtocolInterface::Error::InvalidEntityType;

	auto& commandEntityInfo = commandEntityIt->second;
	auto* const protocolInterface = _manager->getProtocolInterfaceDelegate();

	// Get next available sequenceID and update the aecpdu with it
	auto const sequenceID = getNextAecpSequenceID(commandEntityInfo);
	aecpdu->setSequenceID(sequenceID);

	try
	{
		// Record the query for when we get a response (so we can send it again if it timed out)
		AecpCommandInfo command{ sequenceID, std::move(aecpdu), onResult };
		{
			auto& inflight = commandEntityInfo.inflightAecpCommands[targetEntityID];
			// Check if we don't have too many inflight commands for this entity
			if (inflight.size() < getMaxInflightAecpMessages(targetEntityID))
			{
				// Send the command
				setCommandInflight(protocolInterface, commandEntityInfo, inflight, inflight.end(), std::move(command));
			}
			else // Too many inflight commands, queue it
			{
				auto& queue = commandEntityInfo.commandsQueue[targetEntityID];
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

ProtocolInterface::Error CommandStateMachine::sendAcmpCommand(Acmpdu::UniquePointer&& acmpdu, ProtocolInterface::AcmpCommandResultHandler const& onResult) noexcept
{
	auto* acmp = static_cast<Acmpdu*>(acmpdu.get());

	// Lock
	auto const lg = std::lock_guard{ *_manager };

	// Get CommandEntityInfo matching ControllerEntityID
	auto const& commandEntityIt = _commandEntities.find(acmp->getControllerEntityID());
	if (commandEntityIt == _commandEntities.end())
		return ProtocolInterface::Error::InvalidEntityType;

	auto& commandEntityInfo = commandEntityIt->second;
	auto* const protocolInterface = _manager->getProtocolInterfaceDelegate();

	// Get next available sequenceID and update the acmpdu with it
	auto const sequenceID = getNextAcmpSequenceID(commandEntityInfo);
	acmpdu->setSequenceID(sequenceID);

	auto error{ ProtocolInterface::Error::NoError };
	try
	{
		// Record the query for when we get a response (so we can send it again if it timed out)
		AcmpCommandInfo command{ sequenceID, std::move(acmpdu), onResult };

		error = protocolInterface->sendMessage(static_cast<Acmpdu const&>(*command.command));
		if (!error)
		{
			resetAcmpCommandTimeoutValue(command);
			commandEntityInfo.inflightAcmpCommands[sequenceID] = std::move(command);
		}
	}
	catch (...)
	{
		error = ProtocolInterface::Error::InternalError;
	}
	return error;
}

/* ************************************************************ */
/* Private methods                                              */
/* ************************************************************ */
bool CommandStateMachine::isAEMUnsolicitedResponse(Aecpdu const& aecpdu) const noexcept
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

bool CommandStateMachine::shouldRearmTimer(Aecpdu const& aecpdu) const noexcept
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

void CommandStateMachine::resetAecpCommandTimeoutValue(AecpCommandInfo& command) const noexcept
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

	command.sendTime = std::chrono::steady_clock::now();
	command.timeoutTime = command.sendTime + std::chrono::milliseconds(timeout);
}

void CommandStateMachine::resetAcmpCommandTimeoutValue(AcmpCommandInfo& command) const noexcept
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

	command.sendTime = std::chrono::steady_clock::now();
	command.timeoutTime = command.sendTime + std::chrono::milliseconds(timeout);
}

AecpSequenceID CommandStateMachine::getNextAecpSequenceID(CommandEntityInfo& info) noexcept
{
	auto const nextID = info.currentAecpSequenceID;
	++info.currentAecpSequenceID;
	return nextID;
}

AcmpSequenceID CommandStateMachine::getNextAcmpSequenceID(CommandEntityInfo& info) noexcept
{
	auto const nextID = info.currentAcmpSequenceID;
	++info.currentAcmpSequenceID;
	return nextID;
}

size_t CommandStateMachine::getMaxInflightAecpMessages(UniqueIdentifier const /*entityID*/) const noexcept
{
	return Aecpdu::DefaultMaxInflightCommands;
}


} // namespace stateMachine
} // namespace protocol
} // namespace avdecc
} // namespace la
