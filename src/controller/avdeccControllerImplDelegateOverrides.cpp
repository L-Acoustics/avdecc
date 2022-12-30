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
* @file avdeccControllerImplDelegateOverrides.cpp
* @author Christophe Calmejane
*/

#include "avdeccControllerImpl.hpp"
#include "avdeccControllerLogHelper.hpp"

namespace la
{
namespace avdecc
{
namespace controller
{
/* ************************************************************ */
/* entity::ControllerEntity::Delegate overrides                 */
/* ************************************************************ */
/* Global notifications */
void ControllerImpl::onTransportError(entity::controller::Interface const* const /*controller*/) noexcept
{
	notifyObserversMethod<Controller::Observer>(&Controller::Observer::onTransportError, this);
}

/* Discovery Protocol (ADP) delegate */
void ControllerImpl::onEntityOnline(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::Entity const& entity) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onEntityOnline");

	auto const caps = entity.getEntityCapabilities();
	if (caps.test(entity::EntityCapability::EntityNotReady))
	{
		LOG_CONTROLLER_TRACE(entityID, "Entity is declared as 'Not Ready', ignoring it right now");
		return;
	}
	if (caps.test(entity::EntityCapability::GeneralControllerIgnore))
	{
		LOG_CONTROLLER_TRACE(entityID, "Entity is declared as 'General Controller Ignore', ignoring it");
		return;
	}

	SharedControlledEntityImpl controlledEntity{};

	// Create and add the entity
	{
		// Lock to protect _controlledEntities
		std::lock_guard<decltype(_lock)> const lg(_lock);

#ifdef DEBUG
		// TODO: This happens if an Entity has 2 interfaces on the same network (and there is a loop in the network). We should handle this case and report a message to the user
		AVDECC_ASSERT(_controlledEntities.find(entityID) == _controlledEntities.end(), "Entity already online");
#endif

		auto entityIt = _controlledEntities.find(entityID);
		if (entityIt == _controlledEntities.end())
		{
			controlledEntity = _controlledEntities.insert(std::make_pair(entityID, std::make_shared<ControlledEntityImpl>(entity, _entitiesSharedLockInformation, false))).first->second;
		}
	}

	if (controlledEntity)
	{
		// New entity get everything we can from it
		auto steps = ControlledEntityImpl::EnumerationSteps{};

		// The entity supports AEM, also get information related to AEM
		if (caps.test(entity::EntityCapability::AemSupported))
		{
			// Only get MilanInfo if the Entity supports VendorUnique
			if (caps.test(entity::EntityCapability::VendorUniqueSupported))
			{
				steps.set(ControlledEntityImpl::EnumerationStep::GetMilanInfo);
			}
			steps.set(ControlledEntityImpl::EnumerationStep::RegisterUnsol);
			steps.set(ControlledEntityImpl::EnumerationStep::GetStaticModel);
			steps.set(ControlledEntityImpl::EnumerationStep::GetDynamicInfo);
		}

		// Currently, we have nothing more to get if the entity does not support AEM

		// Set Steps
		controlledEntity->setEnumerationSteps(steps);

		// Save the time we start enumeration
		controlledEntity->setStartEnumerationTime(std::chrono::steady_clock::now());

		// Check first enumeration step
		checkEnumerationSteps(controlledEntity.get());
	}
	else
	{
		LOG_CONTROLLER_DEBUG(entityID, "onEntityOnline: Entity already registered, updating it");
		// This should not happen, but just in case... update it
		onEntityUpdate(controller, entityID, entity);
	}
}

void ControllerImpl::onEntityUpdate(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::Entity const& entity) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onEntityUpdate");

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		updateEntity(*controlledEntity, entity);
	}
	else
	{
		// In case the entity was not ready when it was first discovered, maybe now is the time
		onEntityOnline(controller, entityID, entity);
	}
}

void ControllerImpl::onEntityOffline(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onEntityOffline");

	auto controlledEntity = SharedControlledEntityImpl{};

	// Cleanup and remove the entity
	{
		// Lock to protect _controlledEntities
		std::lock_guard<decltype(_lock)> const lg(_lock);

		auto entityIt = _controlledEntities.find(entityID);
		if (entityIt != _controlledEntities.end())
		{
			// Get a reference on the entity while locked, before removing it from the list
			controlledEntity = entityIt->second;
			_controlledEntities.erase(entityIt);
		}
	}

	if (controlledEntity)
	{
		// Entity was advertised to the user, notify observers
		if (controlledEntity->wasAdvertised())
		{
			// Do some final steps before unadvertising entity
			onPreUnadvertiseEntity(*controlledEntity);

			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityOffline, this, controlledEntity.get());
			controlledEntity->setAdvertised(false);
		}
	}
}

/* Connection Management Protocol sniffed messages (ACMP) */
void ControllerImpl::onControllerConnectResponseSniffed(entity::controller::Interface const* const /*controller*/, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, std::uint16_t const /*connectionCount*/, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept
{
	if (!!status)
	{
		// Do not trust the connectionCount value to determine if the listener is connected, but rather use the fact there was no error in the command
		handleListenerStreamStateNotification(talkerStream, listenerStream, true, flags, true);
	}
	// We don't care about sniffed errors
}

void ControllerImpl::onControllerDisconnectResponseSniffed(entity::controller::Interface const* const /*controller*/, entity::model::StreamIdentification const& /*talkerStream*/, entity::model::StreamIdentification const& listenerStream, std::uint16_t const /*connectionCount*/, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept
{
	if (!!status || status == entity::ControllerEntity::ControlStatus::NotConnected)
	{
		// Do not trust the connectionCount value to determine if the listener is disconnected, but rather use the fact there was no error (NOT_CONNECTED is actually not an error) in the command
		handleListenerStreamStateNotification({}, listenerStream, false, flags, true);
	}
	// We don't care about sniffed errors
}

void ControllerImpl::onListenerConnectResponseSniffed(entity::controller::Interface const* const /*controller*/, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, std::uint16_t const /*connectionCount*/, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept
{
	if (!!status)
	{
		// Do not trust the connectionCount value to determine if the listener is connected, but rather use the fact there was no error in the command
		handleTalkerStreamStateNotification(talkerStream, listenerStream, true, flags, true);
	}
	// We don't care about sniffed errors
}

void ControllerImpl::onListenerDisconnectResponseSniffed(entity::controller::Interface const* const /*controller*/, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, std::uint16_t const /*connectionCount*/, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept
{
	if (!!status)
	{
		// Do not trust the connectionCount value to determine if the listener is connected, but rather use the fact there was no error in the command
		handleTalkerStreamStateNotification(talkerStream, listenerStream, false, flags, true);
	}
	// We don't care about sniffed errors
}

void ControllerImpl::onGetListenerStreamStateResponseSniffed(entity::controller::Interface const* const /*controller*/, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept
{
	if (!!status)
	{
		// In a GET_RX_STATE_RESPONSE message, the connectionCount is set to 1 if the stream is connected and 0 if not connected (See Marc Illouz clarification document, and hopefully someday as a corrigendum)
		handleListenerStreamStateNotification(talkerStream, listenerStream, connectionCount != 0, flags, true);
	}
	// We don't care about sniffed errors
}

/* Unsolicited notifications (not triggered for our own commands, the command's 'result' method will be called in that case) and only if command has no error */
void ControllerImpl::onDeregisteredFromUnsolicitedNotifications(entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;
		updateUnsolicitedNotificationsSubscription(entity, false);
	}
}

void ControllerImpl::onEntityAcquired(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, UniqueIdentifier const owningEntity, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const /*descriptorIndex*/) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;
		if (descriptorType == entity::model::DescriptorType::Entity)
		{
			updateAcquiredState(entity, owningEntity ? (owningEntity == getControllerEID() ? model::AcquireState::Acquired : model::AcquireState::AcquiredByOther) : model::AcquireState::NotAcquired, owningEntity);
		}
	}
}

void ControllerImpl::onEntityReleased(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, UniqueIdentifier const owningEntity, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const /*descriptorIndex*/) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;
		if (descriptorType == entity::model::DescriptorType::Entity)
		{
			updateAcquiredState(entity, owningEntity ? (owningEntity == getControllerEID() ? model::AcquireState::Acquired : model::AcquireState::AcquiredByOther) : model::AcquireState::NotAcquired, owningEntity);
		}
	}
}

void ControllerImpl::onEntityLocked(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, UniqueIdentifier const lockingEntity, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const /*descriptorIndex*/) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;
		if (descriptorType == entity::model::DescriptorType::Entity)
		{
			updateLockedState(entity, lockingEntity ? (lockingEntity == getControllerEID() ? model::LockState::Locked : model::LockState::LockedByOther) : model::LockState::NotLocked, lockingEntity);
		}
	}
}

void ControllerImpl::onEntityUnlocked(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, UniqueIdentifier const lockingEntity, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const /*descriptorIndex*/) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;
		if (descriptorType == entity::model::DescriptorType::Entity)
		{
			updateLockedState(entity, lockingEntity ? (lockingEntity == getControllerEID() ? model::LockState::Locked : model::LockState::LockedByOther) : model::LockState::NotLocked, lockingEntity);
		}
	}
}

void ControllerImpl::onConfigurationChanged(entity::controller::Interface const* const controller, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;
		updateConfiguration(controller, entity, configurationIndex);
	}
}

void ControllerImpl::onStreamInputFormatChanged(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		// If we need to update based on a notification, only do it if we already enumerated associated descriptor
		if (!entity.hasAnyConfiguration() || !entity.hasTreeModel(entity.getCurrentConfigurationIndex(), streamIndex, &entity::model::ConfigurationTree::streamInputModels))
		{
			LOG_CONTROLLER_DEBUG(entityID, "Ignoring StreamInputFormat update from unsolicited notification because STREAM {} not enumerated yet", streamIndex);
			return;
		}

		updateStreamInputFormat(entity, streamIndex, streamFormat);
	}
}

void ControllerImpl::onStreamOutputFormatChanged(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		// If we need to update based on a notification, only do it if we already enumerated associated descriptor
		if (!entity.hasAnyConfiguration() || !entity.hasTreeModel(entity.getCurrentConfigurationIndex(), streamIndex, &entity::model::ConfigurationTree::streamOutputModels))
		{
			LOG_CONTROLLER_DEBUG(entityID, "Ignoring StreamOutputFormat update from unsolicited notification because STREAM {} not enumerated yet", streamIndex);
			return;
		}

		updateStreamOutputFormat(entity, streamIndex, streamFormat);
	}
}

void ControllerImpl::onStreamPortInputAudioMappingsChanged(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, entity::model::AudioMappings const& mappings) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		// If we need to update based on a notification, only do it if we already enumerated associated descriptor
		if (!entity.hasAnyConfiguration() || !entity.hasTreeModel(entity.getCurrentConfigurationIndex(), streamPortIndex, &entity::model::ConfigurationTree::streamPortInputModels))
		{
			LOG_CONTROLLER_DEBUG(entityID, "Ignoring StreamPortInputAudioMappings update from unsolicited notification because STREAM_PORT {} not enumerated yet", streamPortIndex);
			return;
		}

		// Only support the case where numberOfMaps == 1
		if (numberOfMaps != 1 || mapIndex != 0)
			return;

		controlledEntity->clearStreamPortInputAudioMappings(streamPortIndex);
		updateStreamPortInputAudioMappingsAdded(entity, streamPortIndex, mappings);
	}
}

void ControllerImpl::onStreamPortOutputAudioMappingsChanged(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, entity::model::AudioMappings const& mappings) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		// If we need to update based on a notification, only do it if we already enumerated associated descriptor
		if (!entity.hasAnyConfiguration() || !entity.hasTreeModel(entity.getCurrentConfigurationIndex(), streamPortIndex, &entity::model::ConfigurationTree::streamPortOutputModels))
		{
			LOG_CONTROLLER_DEBUG(entityID, "Ignoring StreamPortOutputAudioMappings update from unsolicited notification because STREAM_PORT {} not enumerated yet", streamPortIndex);
			return;
		}

		// Only support the case where numberOfMaps == 1
		if (numberOfMaps != 1 || mapIndex != 0)
			return;

		controlledEntity->clearStreamPortOutputAudioMappings(streamPortIndex);
		updateStreamPortOutputAudioMappingsAdded(entity, streamPortIndex, mappings);
	}
}

void ControllerImpl::onStreamInputInfoChanged(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info, bool const fromGetStreamInfoResponse) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		// If we need to update based on a notification, only do it if we already enumerated associated descriptor
		if (!entity.hasAnyConfiguration() || !entity.hasTreeModel(entity.getCurrentConfigurationIndex(), streamIndex, &entity::model::ConfigurationTree::streamInputModels))
		{
			LOG_CONTROLLER_DEBUG(entityID, "Ignoring StreamInputInfo update from unsolicited notification because STREAM {} not enumerated yet", streamIndex);
			return;
		}

		updateStreamInputInfo(entity, streamIndex, info, fromGetStreamInfoResponse, fromGetStreamInfoResponse);
	}
}

void ControllerImpl::onStreamOutputInfoChanged(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info, bool const fromGetStreamInfoResponse) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		// If we need to update based on a notification, only do it if we already enumerated associated descriptor
		if (!entity.hasAnyConfiguration() || !entity.hasTreeModel(entity.getCurrentConfigurationIndex(), streamIndex, &entity::model::ConfigurationTree::streamOutputModels))
		{
			LOG_CONTROLLER_DEBUG(entityID, "Ignoring StreamOutputInfo update from unsolicited notification because STREAM {} not enumerated yet", streamIndex);
			return;
		}

		updateStreamOutputInfo(entity, streamIndex, info, fromGetStreamInfoResponse, fromGetStreamInfoResponse);
	}
}

void ControllerImpl::onEntityNameChanged(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::model::AvdeccFixedString const& entityName) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;
		updateEntityName(entity, entityName);
	}
}

void ControllerImpl::onEntityGroupNameChanged(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::model::AvdeccFixedString const& entityGroupName) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;
		updateEntityGroupName(entity, entityGroupName);
	}
}

void ControllerImpl::onConfigurationNameChanged(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& configurationName) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		// If we need to update based on a notification, only do it if we already enumerated associated descriptor
		if (!entity.hasConfigurationTree(configurationIndex))
		{
			LOG_CONTROLLER_DEBUG(entityID, "Ignoring ConfigurationName update from unsolicited notification because STREAM {} not enumerated yet", configurationIndex);
			return;
		}

		updateConfigurationName(entity, configurationIndex, configurationName);
	}
}

void ControllerImpl::onAudioUnitNameChanged(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::AvdeccFixedString const& audioUnitName) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		// If we need to update based on a notification, only do it if we already enumerated associated descriptor
		if (!entity.hasConfigurationTree(configurationIndex) || !entity.hasTreeModel(entity.getCurrentConfigurationIndex(), audioUnitIndex, &entity::model::ConfigurationTree::audioUnitModels))
		{
			LOG_CONTROLLER_DEBUG(entityID, "Ignoring AudioUnitName update from unsolicited notification because AUDIO_UNIT {} not enumerated yet", audioUnitIndex);
			return;
		}

		updateAudioUnitName(entity, configurationIndex, audioUnitIndex, audioUnitName);
	}
}

void ControllerImpl::onStreamInputNameChanged(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamName) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		// If we need to update based on a notification, only do it if we already enumerated associated descriptor
		if (!entity.hasConfigurationTree(configurationIndex) || !entity.hasTreeModel(entity.getCurrentConfigurationIndex(), streamIndex, &entity::model::ConfigurationTree::streamInputModels))
		{
			LOG_CONTROLLER_DEBUG(entityID, "Ignoring StreamInputName update from unsolicited notification because STREAM {} not enumerated yet", streamIndex);
			return;
		}

		updateStreamInputName(entity, configurationIndex, streamIndex, streamName);
	}
}

void ControllerImpl::onStreamOutputNameChanged(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamName) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		// If we need to update based on a notification, only do it if we already enumerated associated descriptor
		if (!entity.hasConfigurationTree(configurationIndex) || !entity.hasTreeModel(entity.getCurrentConfigurationIndex(), streamIndex, &entity::model::ConfigurationTree::streamOutputModels))
		{
			LOG_CONTROLLER_DEBUG(entityID, "Ignoring StreamOutputName update from unsolicited notification because STREAM {} not enumerated yet", streamIndex);
			return;
		}

		updateStreamOutputName(entity, configurationIndex, streamIndex, streamName);
	}
}

void ControllerImpl::onAvbInterfaceNameChanged(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvdeccFixedString const& avbInterfaceName) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		// If we need to update based on a notification, only do it if we already enumerated associated descriptor
		if (!entity.hasConfigurationTree(configurationIndex) || !entity.hasTreeModel(entity.getCurrentConfigurationIndex(), avbInterfaceIndex, &entity::model::ConfigurationTree::avbInterfaceModels))
		{
			LOG_CONTROLLER_DEBUG(entityID, "Ignoring AvbInterfaceName update from unsolicited notification because AVB_INTERFACE {} not enumerated yet", avbInterfaceIndex);
			return;
		}

		updateAvbInterfaceName(entity, configurationIndex, avbInterfaceIndex, avbInterfaceName);
	}
}

void ControllerImpl::onClockSourceNameChanged(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockSourceIndex, entity::model::AvdeccFixedString const& clockSourceName) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		// If we need to update based on a notification, only do it if we already enumerated associated descriptor
		if (!entity.hasConfigurationTree(configurationIndex) || !entity.hasTreeModel(entity.getCurrentConfigurationIndex(), clockSourceIndex, &entity::model::ConfigurationTree::clockSourceModels))
		{
			LOG_CONTROLLER_DEBUG(entityID, "Ignoring ClockSourceName update from unsolicited notification because CLOCK_SOURCE {} not enumerated yet", clockSourceIndex);
			return;
		}

		updateClockSourceName(entity, configurationIndex, clockSourceIndex, clockSourceName);
	}
}

void ControllerImpl::onMemoryObjectNameChanged(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, entity::model::AvdeccFixedString const& memoryObjectName) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		// If we need to update based on a notification, only do it if we already enumerated associated descriptor
		if (!entity.hasConfigurationTree(configurationIndex) || !entity.hasTreeModel(entity.getCurrentConfigurationIndex(), memoryObjectIndex, &entity::model::ConfigurationTree::memoryObjectModels))
		{
			LOG_CONTROLLER_DEBUG(entityID, "Ignoring MemoryObjectName update from unsolicited notification because MEMORY_OBJECT {} not enumerated yet", memoryObjectIndex);
			return;
		}

		updateMemoryObjectName(entity, configurationIndex, memoryObjectIndex, memoryObjectName);
	}
}

void ControllerImpl::onAudioClusterNameChanged(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const audioClusterIndex, entity::model::AvdeccFixedString const& audioClusterName) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		// If we need to update based on a notification, only do it if we already enumerated associated descriptor
		if (!entity.hasConfigurationTree(configurationIndex) || !entity.hasTreeModel(entity.getCurrentConfigurationIndex(), audioClusterIndex, &entity::model::ConfigurationTree::audioClusterModels))
		{
			LOG_CONTROLLER_DEBUG(entityID, "Ignoring AudioClusterName update from unsolicited notification because AUDIO_CLUSTER {} not enumerated yet", audioClusterIndex);
			return;
		}

		updateAudioClusterName(entity, configurationIndex, audioClusterIndex, audioClusterName);
	}
}

void ControllerImpl::onControlNameChanged(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ControlIndex const controlIndex, entity::model::AvdeccFixedString const& controlName) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		// If we need to update based on a notification, only do it if we already enumerated associated descriptor
		if (!entity.hasConfigurationTree(configurationIndex) || !entity.hasTreeModel(entity.getCurrentConfigurationIndex(), controlIndex, &entity::model::ConfigurationTree::controlModels))
		{
			LOG_CONTROLLER_DEBUG(entityID, "Ignoring ControlName update from unsolicited notification because CONTROL {} not enumerated yet", controlIndex);
			return;
		}

		updateControlName(entity, configurationIndex, controlIndex, controlName);
	}
}

void ControllerImpl::onClockDomainNameChanged(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::AvdeccFixedString const& clockDomainName) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		// If we need to update based on a notification, only do it if we already enumerated associated descriptor
		if (!entity.hasConfigurationTree(configurationIndex) || !entity.hasTreeModel(entity.getCurrentConfigurationIndex(), clockDomainIndex, &entity::model::ConfigurationTree::clockDomainModels))
		{
			LOG_CONTROLLER_DEBUG(entityID, "Ignoring ClockDomainName update from unsolicited notification because CLOCK_DOMAIN {} not enumerated yet", clockDomainIndex);
			return;
		}

		updateClockDomainName(entity, configurationIndex, clockDomainIndex, clockDomainName);
	}
}

void ControllerImpl::onAssociationIDChanged(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, UniqueIdentifier const associationID) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;
		updateAssociationID(entity, associationID);
	}
}

void ControllerImpl::onAudioUnitSamplingRateChanged(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::SamplingRate const samplingRate) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		// If we need to update based on a notification, only do it if we already enumerated associated descriptor
		if (!entity.hasAnyConfiguration() || !entity.hasTreeModel(entity.getCurrentConfigurationIndex(), audioUnitIndex, &entity::model::ConfigurationTree::audioUnitModels))
		{
			LOG_CONTROLLER_DEBUG(entityID, "Ignoring AudioUnitSamplingRate update from unsolicited notification because AUDIO_UNIT {} not enumerated yet", audioUnitIndex);
			return;
		}

		updateAudioUnitSamplingRate(entity, audioUnitIndex, samplingRate);
	}
}

void ControllerImpl::onClockSourceChanged(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockSourceIndex const clockSourceIndex) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		// If we need to update based on a notification, only do it if we already enumerated associated descriptor
		if (!entity.hasAnyConfiguration() || !entity.hasTreeModel(entity.getCurrentConfigurationIndex(), clockDomainIndex, &entity::model::ConfigurationTree::clockDomainModels))
		{
			LOG_CONTROLLER_DEBUG(entityID, "Ignoring ClockSource update from unsolicited notification because CLOCK_DOMAIN {} not enumerated yet", clockDomainIndex);
			return;
		}

		updateClockSource(entity, clockDomainIndex, clockSourceIndex);
	}
}

void ControllerImpl::onControlValuesChanged(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::model::ControlIndex const controlIndex, MemoryBuffer const& packedControlValues) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		// If we need to update based on a notification, only do it if we already enumerated associated descriptor
		if (!entity.hasAnyConfiguration() || !entity.hasTreeModel(entity.getCurrentConfigurationIndex(), controlIndex, &entity::model::ConfigurationTree::controlModels))
		{
			LOG_CONTROLLER_DEBUG(entityID, "Ignoring ControlValues update from unsolicited notification because CONTROL {} not enumerated yet", controlIndex);
			return;
		}

		updateControlValues(entity, controlIndex, packedControlValues);
	}
}

void ControllerImpl::onStreamInputStarted(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		// If we need to update based on a notification, only do it if we already enumerated associated descriptor
		if (!entity.hasAnyConfiguration() || !entity.hasTreeModel(entity.getCurrentConfigurationIndex(), streamIndex, &entity::model::ConfigurationTree::streamInputModels))
		{
			LOG_CONTROLLER_DEBUG(entityID, "Ignoring StreamInputRunning update from unsolicited notification because STREAM {} not enumerated yet", streamIndex);
			return;
		}

		updateStreamInputRunningStatus(entity, streamIndex, true);
	}
}

void ControllerImpl::onStreamOutputStarted(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		// If we need to update based on a notification, only do it if we already enumerated associated descriptor
		if (!entity.hasAnyConfiguration() || !entity.hasTreeModel(entity.getCurrentConfigurationIndex(), streamIndex, &entity::model::ConfigurationTree::streamOutputModels))
		{
			LOG_CONTROLLER_DEBUG(entityID, "Ignoring StreamOutputRunning update from unsolicited notification because STREAM {} not enumerated yet", streamIndex);
			return;
		}

		updateStreamOutputRunningStatus(entity, streamIndex, true);
	}
}

void ControllerImpl::onStreamInputStopped(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		// If we need to update based on a notification, only do it if we already enumerated associated descriptor
		if (!entity.hasAnyConfiguration() || !entity.hasTreeModel(entity.getCurrentConfigurationIndex(), streamIndex, &entity::model::ConfigurationTree::streamInputModels))
		{
			LOG_CONTROLLER_DEBUG(entityID, "Ignoring StreamInputRunning update from unsolicited notification because STREAM {} not enumerated yet", streamIndex);
			return;
		}

		updateStreamInputRunningStatus(entity, streamIndex, false);
	}
}

void ControllerImpl::onStreamOutputStopped(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		// If we need to update based on a notification, only do it if we already enumerated associated descriptor
		if (!entity.hasAnyConfiguration() || !entity.hasTreeModel(entity.getCurrentConfigurationIndex(), streamIndex, &entity::model::ConfigurationTree::streamOutputModels))
		{
			LOG_CONTROLLER_DEBUG(entityID, "Ignoring StreamOutputRunning update from unsolicited notification because STREAM {} not enumerated yet", streamIndex);
			return;
		}

		updateStreamOutputRunningStatus(entity, streamIndex, false);
	}
}

void ControllerImpl::onAvbInfoChanged(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvbInfo const& info) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		// If we need to update based on a notification, only do it if we already enumerated associated descriptor
		if (!entity.hasAnyConfiguration() || !entity.hasTreeModel(entity.getCurrentConfigurationIndex(), avbInterfaceIndex, &entity::model::ConfigurationTree::avbInterfaceModels))
		{
			LOG_CONTROLLER_DEBUG(entityID, "Ignoring AvBInfo update from unsolicited notification because AVB_INTERFACE {} not enumerated yet", avbInterfaceIndex);
			return;
		}

		updateAvbInfo(entity, avbInterfaceIndex, info);
	}
}

void ControllerImpl::onAsPathChanged(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AsPath const& asPath) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		// If we need to update based on a notification, only do it if we already enumerated associated descriptor
		if (!entity.hasAnyConfiguration() || !entity.hasTreeModel(entity.getCurrentConfigurationIndex(), avbInterfaceIndex, &entity::model::ConfigurationTree::avbInterfaceModels))
		{
			LOG_CONTROLLER_DEBUG(entityID, "Ignoring AsPath update from unsolicited notification because AVB_INTERFACE {} not enumerated yet", avbInterfaceIndex);
			return;
		}

		updateAsPath(entity, avbInterfaceIndex, asPath);
	}
}

void ControllerImpl::onEntityCountersChanged(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::EntityCounterValidFlags const validCounters, entity::model::DescriptorCounters const& counters) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;
		updateEntityCounters(entity, validCounters, counters);
	}
}

void ControllerImpl::onAvbInterfaceCountersChanged(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::AvbInterfaceCounterValidFlags const validCounters, entity::model::DescriptorCounters const& counters) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		// If we need to update based on a notification, only do it if we already enumerated associated descriptor
		if (!entity.hasAnyConfiguration() || !entity.hasTreeModel(entity.getCurrentConfigurationIndex(), avbInterfaceIndex, &entity::model::ConfigurationTree::avbInterfaceModels))
		{
			LOG_CONTROLLER_DEBUG(entityID, "Ignoring AvbInterfaceCounters update from unsolicited notification because AVB_INTERFACE {} not enumerated yet", avbInterfaceIndex);
			return;
		}

		updateAvbInterfaceCounters(entity, avbInterfaceIndex, validCounters, counters);
	}
}

void ControllerImpl::onClockDomainCountersChanged(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::model::ClockDomainIndex const clockDomainIndex, entity::ClockDomainCounterValidFlags const validCounters, entity::model::DescriptorCounters const& counters) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		// If we need to update based on a notification, only do it if we already enumerated associated descriptor
		if (!entity.hasAnyConfiguration() || !entity.hasTreeModel(entity.getCurrentConfigurationIndex(), clockDomainIndex, &entity::model::ConfigurationTree::clockDomainModels))
		{
			LOG_CONTROLLER_DEBUG(entityID, "Ignoring ClockDomainCounters update from unsolicited notification because CLOCK_DOMAIN {} not enumerated yet", clockDomainIndex);
			return;
		}

		updateClockDomainCounters(entity, clockDomainIndex, validCounters, counters);
	}
}

void ControllerImpl::onStreamInputCountersChanged(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::StreamInputCounterValidFlags const validCounters, entity::model::DescriptorCounters const& counters) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		// If we need to update based on a notification, only do it if we already enumerated associated descriptor
		if (!entity.hasAnyConfiguration() || !entity.hasTreeModel(entity.getCurrentConfigurationIndex(), streamIndex, &entity::model::ConfigurationTree::streamInputModels))
		{
			LOG_CONTROLLER_DEBUG(entityID, "Ignoring StreamInputCounters update from unsolicited notification because STREAM {} not enumerated yet", streamIndex);
			return;
		}

		updateStreamInputCounters(entity, streamIndex, validCounters, counters);
	}
}

void ControllerImpl::onStreamOutputCountersChanged(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::StreamOutputCounterValidFlags const validCounters, entity::model::DescriptorCounters const& counters) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		// If we need to update based on a notification, only do it if we already enumerated associated descriptor
		if (!entity.hasAnyConfiguration() || !entity.hasTreeModel(entity.getCurrentConfigurationIndex(), streamIndex, &entity::model::ConfigurationTree::streamOutputModels))
		{
			LOG_CONTROLLER_DEBUG(entityID, "Ignoring StreamOutputCounters update from unsolicited notification because STREAM {} not enumerated yet", streamIndex);
			return;
		}

		updateStreamOutputCounters(entity, streamIndex, validCounters, counters);
	}
}

void ControllerImpl::onStreamPortInputAudioMappingsAdded(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		// If we need to update based on a notification, only do it if we already enumerated associated descriptor
		if (!entity.hasAnyConfiguration() || !entity.hasTreeModel(entity.getCurrentConfigurationIndex(), streamPortIndex, &entity::model::ConfigurationTree::streamPortInputModels))
		{
			LOG_CONTROLLER_DEBUG(entityID, "Ignoring StreamPortInputAudioMappings update from unsolicited notification because STREAM_PORT {} not enumerated yet", streamPortIndex);
			return;
		}

		updateStreamPortInputAudioMappingsAdded(entity, streamPortIndex, mappings);
	}
}

void ControllerImpl::onStreamPortOutputAudioMappingsAdded(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		// If we need to update based on a notification, only do it if we already enumerated associated descriptor
		if (!entity.hasAnyConfiguration() || !entity.hasTreeModel(entity.getCurrentConfigurationIndex(), streamPortIndex, &entity::model::ConfigurationTree::streamPortOutputModels))
		{
			LOG_CONTROLLER_DEBUG(entityID, "Ignoring StreamPortOutputAudioMappings update from unsolicited notification because STREAM_PORT {} not enumerated yet", streamPortIndex);
			return;
		}

		updateStreamPortOutputAudioMappingsAdded(entity, streamPortIndex, mappings);
	}
}

void ControllerImpl::onStreamPortInputAudioMappingsRemoved(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		// If we need to update based on a notification, only do it if we already enumerated associated descriptor
		if (!entity.hasAnyConfiguration() || !entity.hasTreeModel(entity.getCurrentConfigurationIndex(), streamPortIndex, &entity::model::ConfigurationTree::streamPortInputModels))
		{
			LOG_CONTROLLER_DEBUG(entityID, "Ignoring StreamPortInputAudioMappings update from unsolicited notification because STREAM_PORT {} not enumerated yet", streamPortIndex);
			return;
		}

		updateStreamPortInputAudioMappingsRemoved(entity, streamPortIndex, mappings);
	}
}

void ControllerImpl::onStreamPortOutputAudioMappingsRemoved(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		// If we need to update based on a notification, only do it if we already enumerated associated descriptor
		if (!entity.hasAnyConfiguration() || !entity.hasTreeModel(entity.getCurrentConfigurationIndex(), streamPortIndex, &entity::model::ConfigurationTree::streamPortOutputModels))
		{
			LOG_CONTROLLER_DEBUG(entityID, "Ignoring StreamPortOutputAudioMappings update from unsolicited notification because STREAM_PORT {} not enumerated yet", streamPortIndex);
			return;
		}

		updateStreamPortOutputAudioMappingsRemoved(entity, streamPortIndex, mappings);
	}
}

void ControllerImpl::onMemoryObjectLengthChanged(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		// If we need to update based on a notification, only do it if we already enumerated associated descriptor
		if (!entity.hasConfigurationTree(configurationIndex) || !entity.hasTreeModel(entity.getCurrentConfigurationIndex(), memoryObjectIndex, &entity::model::ConfigurationTree::memoryObjectModels))
		{
			LOG_CONTROLLER_DEBUG(entityID, "Ignoring MemoryObjectLength update from unsolicited notification because MEMORY_OBJECT {} not enumerated yet", memoryObjectIndex);
			return;
		}

		updateMemoryObjectLength(entity, configurationIndex, memoryObjectIndex, length);
	}
}

void ControllerImpl::onOperationStatus(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::OperationID const operationID, std::uint16_t const percentComplete) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;
		updateOperationStatus(entity, descriptorType, descriptorIndex, operationID, percentComplete);
	}
}

/* Identification notifications */
void ControllerImpl::onEntityIdentifyNotification(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		// Lock to protect _entityIdentifications
		auto const lg = std::lock_guard{ _lock };

		// Get current time
		auto const currentTime = std::chrono::system_clock::now();

		auto [it, inserted] = _entityIdentifications.insert(std::make_pair(entityID, currentTime));
		if (inserted)
		{
			// Notify
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onIdentificationStarted, this, controlledEntity.get());
		}
		else
		{
			// Update the time
			it->second = currentTime;
		}
	}
}

/* **** Statistics **** */
void ControllerImpl::onAecpRetry(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const& entityID) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

		auto const value = entity.incrementAecpRetryCounter();

		// Entity was advertised to the user, notify observers
		if (entity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onAecpRetryCounterChanged, this, &entity, value);
		}
	}
}

void ControllerImpl::onAecpTimeout(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const& entityID) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

		auto const value = entity.incrementAecpTimeoutCounter();

		// Entity was advertised to the user, notify observers
		if (entity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onAecpTimeoutCounterChanged, this, &entity, value);
		}
	}
}

void ControllerImpl::onAecpUnexpectedResponse(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const& entityID) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

		auto const value = entity.incrementAecpUnexpectedResponseCounter();

		// Entity was advertised to the user, notify observers
		if (entity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onAecpUnexpectedResponseCounterChanged, this, &entity, value);
		}
	}
}

void ControllerImpl::onAecpResponseTime(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const& entityID, std::chrono::milliseconds const& responseTime) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

		auto const& previous = entity.getAecpResponseAverageTime();
		auto const& value = entity.updateAecpResponseTimeAverage(responseTime);

		// Entity was advertised to the user, notify observers
		if (entity.wasAdvertised() && previous != value)
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onAecpResponseAverageTimeChanged, this, &entity, value);
		}
	}
}

void ControllerImpl::onAemAecpUnsolicitedReceived(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const& entityID, la::avdecc::protocol::AecpSequenceID const sequenceID) noexcept
{
	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		AVDECC_ASSERT(_controller->isSelfLocked(), "Should only be called from the network thread (where ProtocolInterface is locked)");

		// Check for loss of unsolicited notification
		if (entity.hasLostUnsolicitedNotification(sequenceID))
		{
			LOG_CONTROLLER_WARN(entityID, "Unsolicited notification lost detected");

			// Update statistics
			auto const value = entity.incrementAemAecpUnsolicitedLossCounter();

			// Entity was advertised to the user, notify observers
			if (entity.wasAdvertised())
			{
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onAemAecpUnsolicitedLossCounterChanged, this, &entity, value);
			}

			// As part of #50 (for now), just unsubscribe from unsolicited notifications
			{
				// Immediately set as unsubscribed, we are already loosing packets we don't want to miss the response to our unsubscribe
				updateUnsolicitedNotificationsSubscription(entity, false);

				// Properly (try to) unregister from unsol
				unregisterUnsol(&entity);
			}
		}

		// Update statistics
		auto const value = entity.incrementAemAecpUnsolicitedCounter();

		// Entity was advertised to the user, notify observers
		if (entity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onAemAecpUnsolicitedCounterChanged, this, &entity, value);
		}
	}
}

} // namespace controller
} // namespace avdecc
} // namespace la
