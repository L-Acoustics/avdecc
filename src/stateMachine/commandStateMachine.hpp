/*
* Copyright (C) 2016-2019, L-Acoustics and its contributors

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
* @file commandStateMachine.hpp
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

/** State Machine for entities that need to send AECP and ACMP Commands */
class CommandStateMachine final
{
public:
	class Delegate
	{
	public:
		virtual ~Delegate() noexcept = default;
		/* **** AECP notifications **** */
		virtual void onAecpAemUnsolicitedResponse(la::avdecc::protocol::Aecpdu const& aecpdu) noexcept = 0;
	};

	CommandStateMachine(Manager* manager, Delegate* const delegate) noexcept;
	~CommandStateMachine() noexcept;

	void registerLocalEntity(entity::LocalEntity& entity) noexcept;
	void unregisterLocalEntity(entity::LocalEntity& entity) noexcept;
	void checkInflightCommandsTimeoutExpiracy() noexcept;
	void handleAecpResponse(Aecpdu const& aecpdu) noexcept;
	void handleAcmpResponse(Acmpdu const& acmpdu) noexcept;
	ProtocolInterface::Error sendAecpCommand(Aecpdu::UniquePointer&& aecpdu, ProtocolInterface::AecpCommandResultHandler const& onResult) noexcept;
	ProtocolInterface::Error sendAcmpCommand(Acmpdu::UniquePointer&& acmpdu, ProtocolInterface::AcmpCommandResultHandler const& onResult) noexcept;

private:
	// Private types
	struct AecpCommandInfo
	{
		AecpSequenceID sequenceID{ 0 };
		std::chrono::time_point<std::chrono::system_clock> timeout{};
		bool retried{ false };
		Aecpdu::UniquePointer command{ nullptr, nullptr };
		ProtocolInterface::AecpCommandResultHandler resultHandler{};

		AecpCommandInfo() {}
		AecpCommandInfo(AecpSequenceID const sequenceID, Aecpdu::UniquePointer&& command, ProtocolInterface::AecpCommandResultHandler const& resultHandler)
			: sequenceID(sequenceID)
			, command(std::move(command))
			, resultHandler(resultHandler)
		{
		}
	};
	using AecpCommands = std::list<AecpCommandInfo>;
	using InflightAecpCommands = std::unordered_map<UniqueIdentifier, AecpCommands, UniqueIdentifier::hash>;
	using AecpCommandsQueue = std::unordered_map<UniqueIdentifier, AecpCommands, UniqueIdentifier::hash>;

	struct AcmpCommandInfo
	{
		AcmpSequenceID sequenceID{ 0 };
		std::chrono::time_point<std::chrono::system_clock> timeout{};
		bool retried{ false };
		Acmpdu::UniquePointer command{ nullptr, nullptr };
		ProtocolInterface::AcmpCommandResultHandler resultHandler{};

		AcmpCommandInfo() {}
		AcmpCommandInfo(AcmpSequenceID const sequenceID, Acmpdu::UniquePointer&& command, ProtocolInterface::AcmpCommandResultHandler const& resultHandler)
			: sequenceID(sequenceID)
			, command(std::move(command))
			, resultHandler(resultHandler)
		{
		}
	};
	using InflightAcmpCommands = std::unordered_map<AcmpSequenceID, AcmpCommandInfo>;

	using ScheduledAecpErrors = std::list<std::pair<ProtocolInterface::Error, ProtocolInterface::AecpCommandResultHandler>>;

	struct CommandEntityInfo
	{
		entity::LocalEntity& entity;

		// AECP variables
		AecpSequenceID currentAecpSequenceID{ 0 };
		InflightAecpCommands inflightAecpCommands{};
		AecpCommandsQueue commandsQueue{};

		// ACMP variables
		AcmpSequenceID currentAcmpSequenceID{ 0 };
		InflightAcmpCommands inflightAcmpCommands{};

		// Other variables
		ScheduledAecpErrors scheduledAecpErrors{};

		/** Constructor */
		CommandEntityInfo(entity::LocalEntity& entity) noexcept
			: entity(entity)
		{
		}
	};
	using CommandEntities = std::unordered_map<UniqueIdentifier, CommandEntityInfo, UniqueIdentifier::hash>;

	// Private methods
	template<typename T>
	T setCommandInflight(ProtocolInterfaceDelegate* const protocolInterface, CommandEntityInfo& info, AecpCommands& inflight, T const it, AecpCommandInfo&& command)
	{
		// Ask the transport layer to send the packet
		auto const error = protocolInterface->sendMessage(static_cast<Aecpdu const&>(*command.command));
		if (!!error)
		{
			// Schedule the result handler to be called with the returned error from the delegate
			info.scheduledAecpErrors.push_back(std::make_pair(error, command.resultHandler));
			return it;
		}
		else
		{
			// Move the command to inflight queue
			resetAecpCommandTimeoutValue(command);
			return inflight.insert(it, std::move(command));
		}
	}
	template<typename T>
	T checkQueue(ProtocolInterfaceDelegate* const protocolInterface, CommandEntityInfo& info, UniqueIdentifier const entityID, AecpCommands& inflight, T const it)
	{
		// Check if we don't have too many inflight commands for this entity
		if (inflight.size() >= getMaxInflightAecpMessages(entityID))
			return it;

		// Check if queue is not empty for this entity
		auto& queue = info.commandsQueue[entityID];
		if (queue.empty())
			return it;

		// Remove command from queue
		auto command = std::move(queue.front());
		queue.pop_front();

		return setCommandInflight(protocolInterface, info, inflight, it, std::move(command));
	}

	template<typename T>
	T removeInflight(ProtocolInterfaceDelegate* const protocolInterface, CommandEntityInfo& info, UniqueIdentifier const entityID, AecpCommands& inflight, T const it)
	{
		auto retIt = inflight.erase(it);
		return checkQueue(protocolInterface, info, entityID, inflight, retIt);
	}

	bool isAEMUnsolicitedResponse(Aecpdu const& aecpdu) const noexcept;
	bool shouldRearmTimer(Aecpdu const& aecpdu) const noexcept;
	void resetAecpCommandTimeoutValue(AecpCommandInfo& command) const noexcept;
	void resetAcmpCommandTimeoutValue(AcmpCommandInfo& command) const noexcept;
	AecpSequenceID getNextAecpSequenceID(CommandEntityInfo& info) noexcept;
	AcmpSequenceID getNextAcmpSequenceID(CommandEntityInfo& info) noexcept;
	size_t getMaxInflightAecpMessages(UniqueIdentifier const entityID) const noexcept;

	// Private members
	Manager* _manager{ nullptr };
	Delegate* _delegate{ nullptr };
	CommandEntities _commandEntities{};
};

} // namespace stateMachine
} // namespace protocol
} // namespace avdecc
} // namespace la
