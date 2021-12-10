/*
* Copyright (C) 2016-2021, L-Acoustics and its contributors

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
* @file stateMachineManager.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "la/avdecc/internals/entity.hpp"

#include "protocolInterfaceDelegate.hpp"
#include "advertiseStateMachine.hpp"
#include "discoveryStateMachine.hpp"
#include "commandStateMachine.hpp"

#include <chrono>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <cstdint>

namespace la
{
namespace avdecc
{
namespace protocol
{
namespace stateMachine
{
class Manager final
{
public:
	Manager(ProtocolInterface const* const protocolInterface, ProtocolInterfaceDelegate* const protocolInterfaceDelegate, AdvertiseStateMachine::Delegate* const advertiseDelegate, DiscoveryStateMachine::Delegate* const discoveryDelegate, CommandStateMachine::Delegate* const controllerDelegate) noexcept;
	~Manager() noexcept;

	/* ************************************************************ */
	/* Static methods                                               */
	/* ************************************************************ */
	static Adpdu makeDiscoveryMessage(networkInterface::MacAddress const& sourceMacAddress, UniqueIdentifier const targetEntityID) noexcept;
	static Adpdu makeEntityAvailableMessage(entity::LocalEntity& entity, entity::model::AvbInterfaceIndex const interfaceIndex);
	static Adpdu makeEntityDepartingMessage(entity::LocalEntity const& entity, entity::model::AvbInterfaceIndex const interfaceIndex);

	/* ************************************************************ */
	/* General entry points                                         */
	/* ************************************************************ */
	void startStateMachines() noexcept;
	void stopStateMachines() noexcept;
	ProtocolInterface::Error registerLocalEntity(entity::LocalEntity& entity) noexcept;
	ProtocolInterface::Error unregisterLocalEntity(entity::LocalEntity& entity) noexcept;
	void processAdpdu(Adpdu const& adpdu) noexcept;
	void processAecpdu(Aecpdu const& aecpdu) noexcept;
	void processAcmpdu(Acmpdu const& acmpdu) noexcept;

	/** BasicLockable concept 'lock' method for the whole StateMachine */
	void lock() noexcept;
	/** BasicLockable concept 'unlock' method for the whole StateMachine */
	void unlock() noexcept;
	/** Debug method: Returns true if the whole ProtocolInterface is locked by the calling thread */
	bool isSelfLocked() const noexcept;

	ProtocolInterface const* getProtocolInterface() noexcept;
	ProtocolInterfaceDelegate* getProtocolInterfaceDelegate() noexcept;
	std::optional<entity::model::AvbInterfaceIndex> getMatchingInterfaceIndex(entity::LocalEntity const& entity) const noexcept;
	bool isLocalEntity(UniqueIdentifier const entityID) noexcept;
	void notifyDiscoveredEntities(DiscoveryStateMachine::Delegate& delegate) noexcept;

	/* ************************************************************ */
	/* Advertising entry points                                     */
	/* ************************************************************ */
	ProtocolInterface::Error setEntityNeedsAdvertise(entity::LocalEntity const& entity) noexcept;
	ProtocolInterface::Error enableEntityAdvertising(entity::LocalEntity& entity) noexcept;
	ProtocolInterface::Error disableEntityAdvertising(entity::LocalEntity const& entity) noexcept;

	/* ************************************************************ */
	/* Discovery entry points                                       */
	/* ************************************************************ */
	ProtocolInterface::Error discoverRemoteEntities() noexcept;
	ProtocolInterface::Error discoverRemoteEntity(UniqueIdentifier const entityID) noexcept;
	ProtocolInterface::Error setAutomaticDiscoveryDelay(std::chrono::milliseconds const delay) noexcept;

	/* ************************************************************ */
	/* Sending entry points                                         */
	/* ************************************************************ */
	ProtocolInterface::Error sendAecpCommand(Aecpdu::UniquePointer&& aecpdu, ProtocolInterface::AecpCommandResultHandler const& onResult) noexcept;
	ProtocolInterface::Error sendAcmpCommand(Acmpdu::UniquePointer&& acmpdu, ProtocolInterface::AcmpCommandResultHandler const& onResult) noexcept;

private:
	/* ************************************************************ */
	/* Private types                                                */
	/* ************************************************************ */
	using LocalEntities = std::unordered_map<UniqueIdentifier, entity::LocalEntity&, UniqueIdentifier::hash>;

	/* ************************************************************ */
	/* Private methods                                              */
	/* ************************************************************ */

	/* ************************************************************ */
	/* Common members                                               */
	/* ************************************************************ */
	std::recursive_mutex _lock{}; /** Lock to protect the whole class */
	std::uint32_t _lockedCount{ 0u }; // DEBUG status for BasicLockable concept
	std::thread::id _lockingThreadID{}; // DEBUG status for BasicLockable concept
	bool _shouldTerminate{ false };
	ProtocolInterface const* const _protocolInterface{ nullptr };
	std::thread _stateMachineThread{}; // Can safely be declared here, will be joined during destruction
	LocalEntities _localEntities{}; /** Local entities declared by the running program */

	/* ************************************************************ */
	/* Delegate members                                             */
	/* ************************************************************ */
	ProtocolInterfaceDelegate* _protocolInterfaceDelegate{ nullptr };
	AdvertiseStateMachine::Delegate* _advertiseDelegate{ nullptr };
	DiscoveryStateMachine::Delegate* _discoveryDelegate{ nullptr };
	CommandStateMachine::Delegate* _controllerDelegate{ nullptr };

	/* ************************************************************ */
	/* State machine members                                        */
	/* ************************************************************ */
	AdvertiseStateMachine _advertiseStateMachine{ this, _advertiseDelegate };
	DiscoveryStateMachine _discoveryStateMachine{ this, _discoveryDelegate };
	CommandStateMachine _commandStateMachine{ this, _controllerDelegate };
};

} // namespace stateMachine
} // namespace protocol
} // namespace avdecc
} // namespace la
