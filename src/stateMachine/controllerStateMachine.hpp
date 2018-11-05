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
* @file controllerStateMachine.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "la/avdecc/internals/protocolInterface.hpp"
#include "la/avdecc/internals/entity.hpp"
#include <thread>
#include <mutex>
#include <list>
#include <cstdint>
#include <utility>
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
class ControllerStateMachine final
{
public:
	class Delegate
	{
	public:
		/* **** Discovery notifications **** */
		virtual void onLocalEntityOnline(la::avdecc::entity::Entity const& entity) noexcept = 0;
		virtual void onLocalEntityOffline(la::avdecc::UniqueIdentifier const entityID) noexcept = 0;
		virtual void onLocalEntityUpdated(la::avdecc::entity::Entity const& entity) noexcept = 0;
		virtual void onRemoteEntityOnline(la::avdecc::entity::Entity const& entity) noexcept = 0;
		virtual void onRemoteEntityOffline(la::avdecc::UniqueIdentifier const entityID) noexcept = 0;
		virtual void onRemoteEntityUpdated(la::avdecc::entity::Entity const& entity) noexcept = 0;
		/* **** AECP notifications **** */
		virtual void onAecpCommand(la::avdecc::entity::LocalEntity const& entity, la::avdecc::protocol::Aecpdu const& aecpdu) noexcept = 0;
		virtual void onAecpUnsolicitedResponse(la::avdecc::entity::LocalEntity const& entity, la::avdecc::protocol::Aecpdu const& aecpdu) noexcept = 0;
		/* **** ACMP notifications **** */
		virtual void onAcmpSniffedCommand(la::avdecc::entity::LocalEntity const& entity, la::avdecc::protocol::Acmpdu const& acmpdu) noexcept = 0;
		virtual void onAcmpSniffedResponse(la::avdecc::entity::LocalEntity const& entity, la::avdecc::protocol::Acmpdu const& acmpdu) noexcept = 0;
		/* **** Delegate methods **** */
		virtual ProtocolInterface::Error sendMessage(la::avdecc::protocol::Adpdu const& adpdu) const noexcept = 0;
		virtual ProtocolInterface::Error sendMessage(la::avdecc::protocol::Aecpdu const& aecpdu) const noexcept = 0;
		virtual ProtocolInterface::Error sendMessage(la::avdecc::protocol::Acmpdu const& acmpdu) const noexcept = 0;
	};

	ControllerStateMachine(ProtocolInterface const* const protocolInterface, Delegate* const delegate, size_t const maxInflightAecpMessages = Aecpdu::DefaultMaxInflightCommands); // Throws Exception if delegate is nullptr
	~ControllerStateMachine() noexcept;

	ProtocolInterface::Error sendAecpCommand(Aecpdu::UniquePointer&& aecpdu, ProtocolInterface::AecpCommandResultHandler const& onResult) noexcept;
	ProtocolInterface::Error sendAcmpCommand(Acmpdu::UniquePointer&& acmpdu, ProtocolInterface::AcmpCommandResultHandler const& onResult) noexcept;
	bool processAdpdu(Adpdu const& adpdu) noexcept; // Returns true if processed
	bool isAEMUnsolicitedResponse(Aecpdu const& aecpdu) const noexcept;
	bool shouldRearmTimer(Aecpdu const& aecpdu) const noexcept;
	bool processAecpdu(Aecpdu const& aecpdu) noexcept; // Returns true if processed
	bool processAcmpdu(Acmpdu const& acmpdu) noexcept; // Returns true if processed
	ProtocolInterface::Error registerLocalEntity(entity::LocalEntity& entity) noexcept;
	ProtocolInterface::Error unregisterLocalEntity(entity::LocalEntity& entity) noexcept;
	ProtocolInterface::Error setEntityNeedsAdvertise(entity::LocalEntity const& entity, std::optional<entity::model::AvbInterfaceIndex> const interfaceIndex) noexcept;
	ProtocolInterface::Error enableEntityAdvertising(entity::LocalEntity const& entity, std::optional<entity::model::AvbInterfaceIndex> const interfaceIndex) noexcept;
	ProtocolInterface::Error disableEntityAdvertising(entity::LocalEntity& entity, std::optional<entity::model::AvbInterfaceIndex> const interfaceIndex) noexcept;
	ProtocolInterface::Error discoverRemoteEntities() noexcept;
	ProtocolInterface::Error discoverRemoteEntity(UniqueIdentifier const entityID) noexcept;

	/** BasicLockable concept 'lock' method for the whole ControllerStateMachine */
	void lock() noexcept;
	/** BasicLockable concept 'unlock' method for the whole ControllerStateMachine */
	void unlock() noexcept;

private:
	struct DiscoveredEntityInfo
	{
		entity::Entity entity;
		std::unordered_map<entity::model::AvbInterfaceIndex, std::chrono::time_point<std::chrono::system_clock>> timeouts;
	};
	using DiscoveredEntities = std::unordered_map<UniqueIdentifier, DiscoveredEntityInfo, UniqueIdentifier::hash>;

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
	struct LocalEntityInfo
	{
		// ADP variables
		entity::LocalEntity& entity;
		std::unordered_map<entity::model::AvbInterfaceIndex, std::chrono::time_point<std::chrono::system_clock>> nextAdvertiseTime{};
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
		LocalEntityInfo(entity::LocalEntity& entity) noexcept
			: entity(entity)
		{
		}
	};

	template<typename T>
	T setCommandInflight(LocalEntityInfo& info, AecpCommands& inflight, T const it, AecpCommandInfo&& command)
	{
		// Ask the transport layer to send the packet
		auto const error = _delegate->sendMessage(static_cast<Aecpdu const&>(*command.command));
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
	T checkQueue(LocalEntityInfo& info, la::avdecc::UniqueIdentifier const entityID, AecpCommands& inflight, T const it)
	{
		// Check if we don't have too many inflight commands for this entity
		if (inflight.size() >= _maxInflightAecpMessages)
			return it;

		// Check if queue is not empty for this entity
		auto& queue = info.commandsQueue[entityID];
		if (queue.empty())
			return it;

		// Remove command from queue
		auto command = std::move(queue.front());
		queue.pop_front();

		return setCommandInflight(info, inflight, it, std::move(command));
	}

	template<typename T>
	T removeInflight(LocalEntityInfo& info, la::avdecc::UniqueIdentifier const entityID, AecpCommands& inflight, T const it)
	{
		auto retIt = inflight.erase(it);
		return checkQueue(info, entityID, inflight, retIt);
	}

	// Private methods
	constexpr ControllerStateMachine& getSelf() const noexcept
	{
		return *const_cast<ControllerStateMachine*>(this);
	}
	Adpdu makeDiscoveryMessage(UniqueIdentifier const entityID) const noexcept;
	Adpdu makeEntityAvailableMessage(entity::Entity& entity, entity::model::AvbInterfaceIndex const interfaceIndex) const;
	Adpdu makeEntityDepartingMessage(entity::Entity& entity, entity::model::AvbInterfaceIndex const interfaceIndex) const noexcept;
	void resetAecpCommandTimeoutValue(AecpCommandInfo& command) const noexcept;
	void resetAcmpCommandTimeoutValue(AcmpCommandInfo& command) const noexcept;
	AecpSequenceID getNextAecpSequenceID(LocalEntityInfo& info) noexcept;
	AcmpSequenceID getNextAcmpSequenceID(LocalEntityInfo& info) noexcept;
	std::chrono::milliseconds computeRandomDelay(entity::Entity const& entity, entity::model::AvbInterfaceIndex const interfaceIndex) const noexcept;
	std::chrono::time_point<std::chrono::system_clock> computeNextAdvertiseTime(entity::Entity const& entity, entity::model::AvbInterfaceIndex const interfaceIndex) const;
	std::chrono::time_point<std::chrono::system_clock> computeDelayedAdvertiseTime(entity::Entity const& entity, entity::model::AvbInterfaceIndex const interfaceIndex) const;
	void checkLocalEntitiesAnnouncement() noexcept;
	void checkEntitiesTimeoutExpiracy() noexcept;
	void checkInflightCommandsTimeoutExpiracy() noexcept;
	void handleAdpEntityAvailable(Adpdu const& adpdu) noexcept;
	void handleAdpEntityDeparting(Adpdu const& adpdu) noexcept;
	void handleAdpEntityDiscover(Adpdu const& adpdu) noexcept;
	bool isLocalEntity(UniqueIdentifier const entityID) const noexcept;

	// Common variables
	mutable std::recursive_mutex _lock{}; /** Lock to protect the whole class */
	ProtocolInterface const* const _protocolInterface{ nullptr };
	Delegate* const _delegate{ nullptr };
	size_t _maxInflightAecpMessages{ 0 };
	bool _shouldTerminate{ false };
	DiscoveredEntities _discoveredEntities{};
	std::unordered_map<UniqueIdentifier, LocalEntityInfo, UniqueIdentifier::hash> _localEntities{}; /** Local entities declared by the running program */
	std::thread _stateMachineThread{};
};

} // namespace stateMachine
} // namespace protocol
} // namespace avdecc
} // namespace la
