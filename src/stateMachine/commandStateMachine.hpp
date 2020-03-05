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
		virtual void onAecpAemUnsolicitedResponse(la::avdecc::protocol::AemAecpdu const& aecpdu) noexcept = 0;
		virtual void onAecpAemIdentifyNotification(la::avdecc::protocol::AemAecpdu const& aecpdu) noexcept = 0;
		/* **** Statistics **** */
		virtual void onAecpRetry(la::avdecc::UniqueIdentifier const& entityID) noexcept = 0;
		virtual void onAecpTimeout(la::avdecc::UniqueIdentifier const& entityID) noexcept = 0;
		virtual void onAecpUnexpectedResponse(la::avdecc::UniqueIdentifier const& entityID) noexcept = 0;
		virtual void onAecpResponseTime(la::avdecc::UniqueIdentifier const& entityID, std::chrono::milliseconds const& responseTime) noexcept = 0;
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
		std::chrono::time_point<std::chrono::steady_clock> sendTime{};
		std::chrono::time_point<std::chrono::steady_clock> timeoutTime{};
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
	struct InflightAecpInfo
	{
		std::chrono::time_point<std::chrono::steady_clock> lastSendTime{};
		std::list<AecpCommandInfo> inflightCommands{};
	};
	struct QueuedAecpInfo
	{
		std::list<AecpCommandInfo> queuedCommands{};
	};
	using InflightAecpCommands = std::unordered_map<UniqueIdentifier, InflightAecpInfo, UniqueIdentifier::hash>;
	using AecpCommandsQueue = std::unordered_map<UniqueIdentifier, QueuedAecpInfo, UniqueIdentifier::hash>;

	struct AcmpCommandInfo
	{
		AcmpSequenceID sequenceID{ 0 };
		std::chrono::time_point<std::chrono::steady_clock> sendTime{};
		std::chrono::time_point<std::chrono::steady_clock> timeoutTime{};
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
	struct InflightAcmpInfo
	{
		std::chrono::time_point<std::chrono::steady_clock> lastSendTime{};
		std::list<AcmpCommandInfo> inflightCommands{};
	};
	struct QueuedAcmpInfo
	{
		std::list<AcmpCommandInfo> queuedCommands{};
	};
	using InflightAcmpCommands = std::unordered_map<networkInterface::MacAddress, InflightAcmpInfo, networkInterface::MacAddressHash>;
	using AcmpCommandsQueue = std::unordered_map<networkInterface::MacAddress, QueuedAcmpInfo, networkInterface::MacAddressHash>;

	using ScheduledAecpErrors = std::list<std::pair<ProtocolInterface::Error, ProtocolInterface::AecpCommandResultHandler>>;
	using ScheduledAcmpErrors = std::list<std::pair<ProtocolInterface::Error, ProtocolInterface::AcmpCommandResultHandler>>;

	struct CommandEntityInfo
	{
		entity::LocalEntity& entity;

		// AECP variables
		AecpSequenceID currentAecpSequenceID{ 0 };
		InflightAecpCommands inflightAecpCommands{};
		AecpCommandsQueue aecpCommandsQueue{};

		// ACMP variables
		AcmpSequenceID currentAcmpSequenceID{ 0 };
		InflightAcmpCommands inflightAcmpCommands{};
		AcmpCommandsQueue acmpCommandsQueue{};

		// Other variables
		ScheduledAecpErrors scheduledAecpErrors{};
		ScheduledAcmpErrors scheduledAcmpErrors{};

		/** Constructor */
		CommandEntityInfo(entity::LocalEntity& entity) noexcept
			: entity(entity)
		{
		}
	};
	using CommandEntities = std::unordered_map<UniqueIdentifier, CommandEntityInfo, UniqueIdentifier::hash>;

	// Private methods
	template<class TimeInterval>
	constexpr bool hasExpired(std::chrono::time_point<std::chrono::steady_clock> const& currentTime, std::chrono::time_point<std::chrono::steady_clock> const& lastInterval, TimeInterval const& delay)
	{
		return (lastInterval + delay) < currentTime;
	}
	template<typename T>
	T setCommandInflight(ProtocolInterfaceDelegate* const protocolInterface, CommandEntityInfo& info, InflightAecpInfo& inflight, T const it, AecpCommandInfo&& command)
	{
		// Update last send time
		inflight.lastSendTime = std::chrono::steady_clock::now();

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
			return inflight.inflightCommands.insert(it, std::move(command));
		}
	}
	template<typename T>
	T checkQueue(ProtocolInterfaceDelegate* const protocolInterface, CommandEntityInfo& info, UniqueIdentifier const& entityID, InflightAecpInfo& inflight, T const it)
	{
		// Get current time
		auto const now = std::chrono::steady_clock::now();

		// Check if we don't have too many inflight commands or sending too fast for this destination macAddress
		if (inflight.inflightCommands.size() >= getMaxInflightAecpMessages(entityID) || !hasExpired(now, inflight.lastSendTime, getAecpSendInterval(entityID)))
		{
			return it;
		}

		// Check if queue is not empty for this entity
		auto& queue = info.aecpCommandsQueue[entityID].queuedCommands;
		if (queue.empty())
		{
			return it;
		}

		// Remove command from queue
		auto command = std::move(queue.front());
		queue.pop_front();

		return setCommandInflight(protocolInterface, info, inflight, it, std::move(command));
	}
	template<typename T>
	T removeInflight(ProtocolInterfaceDelegate* const protocolInterface, CommandEntityInfo& info, UniqueIdentifier const& entityID, InflightAecpInfo& inflight, T const it)
	{
		auto retIt = inflight.inflightCommands.erase(it);
		return checkQueue(protocolInterface, info, entityID, inflight, retIt);
	}

	template<typename T>
	T setCommandInflight(ProtocolInterfaceDelegate* const protocolInterface, CommandEntityInfo& info, InflightAcmpInfo& inflight, T const it, AcmpCommandInfo&& command)
	{
		// Update last send time
		inflight.lastSendTime = std::chrono::steady_clock::now();

		// Ask the transport layer to send the packet
		auto const error = protocolInterface->sendMessage(static_cast<Acmpdu const&>(*command.command));
		if (!!error)
		{
			// Schedule the result handler to be called with the returned error from the delegate
			info.scheduledAcmpErrors.push_back(std::make_pair(error, command.resultHandler));
			return it;
		}
		else
		{
			// Move the command to inflight queue
			resetAcmpCommandTimeoutValue(command);
			return inflight.inflightCommands.insert(it, std::move(command));
		}
	}
	template<typename T>
	T checkQueue(ProtocolInterfaceDelegate* const protocolInterface, CommandEntityInfo& info, networkInterface::MacAddress const& targetMacAddress, InflightAcmpInfo& inflight, T const it)
	{
		// Get current time
		auto const now = std::chrono::steady_clock::now();

		// Check if we don't have too many inflight commands or sending too fast for this destination macAddress
		if (inflight.inflightCommands.size() >= getMaxInflightAcmpMessages(targetMacAddress) || !hasExpired(now, inflight.lastSendTime, getAcmpSendInterval(targetMacAddress)))
		{
			return it;
		}

		// Check if queue is not empty for this entity
		auto& queue = info.acmpCommandsQueue[targetMacAddress].queuedCommands;
		if (queue.empty())
		{
			return it;
		}

		// Remove command from queue
		auto command = std::move(queue.front());
		queue.pop_front();

		return setCommandInflight(protocolInterface, info, inflight, it, std::move(command));
	}
	template<typename T>
	T removeInflight(ProtocolInterfaceDelegate* const protocolInterface, CommandEntityInfo& info, networkInterface::MacAddress const& macAddress, InflightAcmpInfo& inflight, T const it)
	{
		auto retIt = inflight.inflightCommands.erase(it);
		return checkQueue(protocolInterface, info, macAddress, inflight, retIt);
	}

	bool isAEMUnsolicitedResponse(Aecpdu const& aecpdu) const noexcept;
	bool shouldRearmTimer(Aecpdu const& aecpdu) const noexcept;
	void resetAecpCommandTimeoutValue(AecpCommandInfo& command) const noexcept;
	void resetAcmpCommandTimeoutValue(AcmpCommandInfo& command) const noexcept;
	AecpSequenceID getNextAecpSequenceID(CommandEntityInfo& info) noexcept;
	AcmpSequenceID getNextAcmpSequenceID(CommandEntityInfo& info) noexcept;
	size_t getMaxInflightAecpMessages(UniqueIdentifier const& entityID) const noexcept;
	std::chrono::milliseconds getAecpSendInterval(UniqueIdentifier const& entityID) const noexcept;
	size_t getMaxInflightAcmpMessages(networkInterface::MacAddress const& macAddress) const noexcept;
	std::chrono::milliseconds getAcmpSendInterval(networkInterface::MacAddress const& macAddress) const noexcept;

	// Private members
	Manager* _manager{ nullptr };
	Delegate* _delegate{ nullptr };
	CommandEntities _commandEntities{};
};

} // namespace stateMachine
} // namespace protocol
} // namespace avdecc
} // namespace la
