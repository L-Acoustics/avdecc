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
void ControllerImpl::onTransportError(entity::ControllerEntity const* const /*controller*/) noexcept
{
	notifyObserversMethod<Controller::Observer>(&Controller::Observer::onTransportError, this);
}

/* Discovery Protocol (ADP) delegate */
void ControllerImpl::onEntityOnline(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::Entity const& entity) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onEntityOnline");

	auto const caps = entity.getEntityCapabilities();
	if (hasFlag(caps, entity::EntityCapabilities::EntityNotReady))
	{
		LOG_CONTROLLER_TRACE(entityID, "Entity is declared as 'Not Ready', ignoring it right now");
		return;
	}
	if (hasFlag(caps, entity::EntityCapabilities::GeneralControllerIgnore))
	{
		LOG_CONTROLLER_TRACE(entityID, "Entity is declared as 'General Controller Ignore', ignoring it");
		return;
	}

	OnlineControlledEntity controlledEntity{};

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
			controlledEntity = _controlledEntities.insert(std::make_pair(entityID, std::make_shared<ControlledEntityImpl>(entity))).first->second;
		}
	}

	if (controlledEntity)
	{
		// New entity get everything we can from it
		auto steps{ ControlledEntityImpl::EnumerationSteps::None };

		// The entity supports AEM, also get information related to AEM
		if (hasFlag(caps, entity::EntityCapabilities::AemSupported))
		{
			// Only get MilanVersion if the Entity supports AEM (Milan requires AEM anyway)
			steps |= ControlledEntityImpl::EnumerationSteps::GetMilanVersion | ControlledEntityImpl::EnumerationSteps::RegisterUnsol | ControlledEntityImpl::EnumerationSteps::GetStaticModel | ControlledEntityImpl::EnumerationSteps::GetDynamicInfo;
		}

		// Currently, we have nothing more to get if the entity does not support AEM

		// Set Steps
		controlledEntity->addEnumerationSteps(steps);

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

void ControllerImpl::onEntityUpdate(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::Entity const& entity) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onEntityUpdate");

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

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

void ControllerImpl::onEntityOffline(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onEntityOffline");

	OnlineControlledEntity controlledEntity{};

	// Cleanup and remove the entity
	{
		// Lock to protect _controlledEntities
		std::lock_guard<decltype(_lock)> const lg(_lock);

		auto entityIt = _controlledEntities.find(entityID);
		if (entityIt != _controlledEntities.end())
		{
			controlledEntity = entityIt->second;
			_controlledEntities.erase(entityIt);
		}
	}

	if (controlledEntity)
	{
		updateAcquiredState(*controlledEntity, UniqueIdentifier{}, entity::model::DescriptorType::Entity, 0u, true);

		// Entity was advertised to the user, notify observers
		if (controlledEntity->wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityOffline, this, controlledEntity.get());
			controlledEntity->setAdvertised(false);
		}
	}
}

/* Connection Management Protocol sniffed messages (ACMP) */
void ControllerImpl::onControllerConnectResponseSniffed(entity::ControllerEntity const* const /*controller*/, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, uint16_t const /*connectionCount*/, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept
{
	if (!!status)
	{
		// Do not trust the connectionCount value to determine if the listener is connected, but rather use the status code (SUCCESS means connection is established)
		handleListenerStreamStateNotification(talkerStream, listenerStream, true, flags, true);
	}
	// We don't care about sniffed errors
}

void ControllerImpl::onControllerDisconnectResponseSniffed(entity::ControllerEntity const* const /*controller*/, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, uint16_t const /*connectionCount*/, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept
{
	if (!!status)
	{
		// Do not trust the connectionCount value to determine if the listener is disconnected, but rather use the status code (SUCCESS means disconnected)
		handleListenerStreamStateNotification(talkerStream, listenerStream, false, flags, true);
	}
	// We don't care about sniffed errors
}

void ControllerImpl::onListenerConnectResponseSniffed(entity::ControllerEntity const* const /*controller*/, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, uint16_t const /*connectionCount*/, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept
{
	if (!!status)
	{
		// Do not trust the connectionCount value to determine if the listener is connected, but rather use the status code (SUCCESS means connection is established)
		handleTalkerStreamStateNotification(talkerStream, listenerStream, true, flags, true);
	}
	// We don't care about sniffed errors
}

void ControllerImpl::onListenerDisconnectResponseSniffed(entity::ControllerEntity const* const /*controller*/, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, uint16_t const /*connectionCount*/, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept
{
	if (!!status)
	{
		// Do not trust the connectionCount value to determine if the listener is disconnected, but rather use the status code (SUCCESS means disconnected)
		handleTalkerStreamStateNotification(talkerStream, listenerStream, false, flags, true);
	}
	// We don't care about sniffed errors
}

void ControllerImpl::onGetListenerStreamStateResponseSniffed(entity::ControllerEntity const* const /*controller*/, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept
{
	if (!!status)
	{
		// In a GET_RX_STATE_RESPONSE message, the connectionCount is set to 1 if the stream is connected and 0 if not connected (See Marc Illouz clarification document, and hopefully someday as a corrigendum)
		handleListenerStreamStateNotification(talkerStream, listenerStream, connectionCount != 0, flags, true);
	}
	// We don't care about sniffed errors
}

/* Unsolicited notifications (not triggered for our own commands, the command's 'result' method will be called in that case) and only if command has no error */
void ControllerImpl::onEntityAcquired(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, UniqueIdentifier const owningEntity, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		updateAcquiredState(*entity, owningEntity, descriptorType, descriptorIndex);
	}
}

void ControllerImpl::onEntityReleased(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, UniqueIdentifier const owningEntity, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		updateAcquiredState(*entity, owningEntity, descriptorType, descriptorIndex);
	}
}

void ControllerImpl::onConfigurationChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		updateConfiguration(controller, *entity, configurationIndex);
	}
}

void ControllerImpl::onStreamInputFormatChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		updateStreamInputFormat(*entity, streamIndex, streamFormat);
	}
}

void ControllerImpl::onStreamOutputFormatChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		updateStreamOutputFormat(*entity, streamIndex, streamFormat);
	}
}

void ControllerImpl::onStreamPortInputAudioMappingsChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, entity::model::AudioMappings const& mappings) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		// Only support the case where numberOfMaps == 1
		if (numberOfMaps != 1 || mapIndex != 0)
			return;

		controlledEntity->clearStreamPortInputAudioMappings(streamPortIndex);
		updateStreamPortInputAudioMappingsAdded(*entity, streamPortIndex, mappings);
	}
}

void ControllerImpl::onStreamPortOutputAudioMappingsChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, entity::model::AudioMappings const& mappings) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		// Only support the case where numberOfMaps == 1
		if (numberOfMaps != 1 || mapIndex != 0)
			return;

		controlledEntity->clearStreamPortOutputAudioMappings(streamPortIndex);
		updateStreamPortOutputAudioMappingsAdded(*entity, streamPortIndex, mappings);
	}
}

void ControllerImpl::onStreamInputInfoChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		updateStreamInputInfo(*entity, streamIndex, info);
	}
}

void ControllerImpl::onStreamOutputInfoChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		updateStreamOutputInfo(*entity, streamIndex, info);
	}
}

void ControllerImpl::onEntityNameChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::AvdeccFixedString const& entityName) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		updateEntityName(*entity, entityName);
	}
}

void ControllerImpl::onEntityGroupNameChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::AvdeccFixedString const& entityGroupName) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		updateEntityGroupName(*entity, entityGroupName);
	}
}

void ControllerImpl::onConfigurationNameChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& configurationName) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		updateConfigurationName(*entity, configurationIndex, configurationName);
	}
}

void ControllerImpl::onStreamInputNameChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamName) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		updateStreamInputName(*entity, configurationIndex, streamIndex, streamName);
	}
}

void ControllerImpl::onStreamOutputNameChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamName) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		updateStreamOutputName(*entity, configurationIndex, streamIndex, streamName);
	}
}

void ControllerImpl::onAudioUnitSamplingRateChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::SamplingRate const samplingRate) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		updateAudioUnitSamplingRate(*entity, audioUnitIndex, samplingRate);
	}
}

void ControllerImpl::onClockSourceChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockSourceIndex const clockSourceIndex) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		updateClockSource(*entity, clockDomainIndex, clockSourceIndex);
	}
}

void ControllerImpl::onStreamInputStarted(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		updateStreamInputRunningStatus(*entity, streamIndex, true);
	}
}

void ControllerImpl::onStreamOutputStarted(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		updateStreamOutputRunningStatus(*entity, streamIndex, true);
	}
}

void ControllerImpl::onStreamInputStopped(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		updateStreamInputRunningStatus(*entity, streamIndex, false);
	}
}

void ControllerImpl::onStreamOutputStopped(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		updateStreamOutputRunningStatus(*entity, streamIndex, false);
	}
}

void ControllerImpl::onAvbInfoChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvbInfo const& info) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		updateAvbInfo(*entity, avbInterfaceIndex, info);
	}
}

} // namespace controller
} // namespace avdecc
} // namespace la
