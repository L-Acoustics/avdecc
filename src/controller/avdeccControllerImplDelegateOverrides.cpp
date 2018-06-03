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
		// Check if AEM is supported by this entity
		if (hasFlag(entity.getEntityCapabilities(), entity::EntityCapabilities::AemSupported))
		{
			// Register for unsolicited notifications
			LOG_CONTROLLER_TRACE(entityID, "Registering for unsolicited notifications");
			controller->registerUnsolicitedNotifications(entityID, {});

			// Request its entity descriptor
			LOG_CONTROLLER_TRACE(entityID, "Requesting entity's descriptor");
			controlledEntity->setDescriptorExpected(0, entity::model::DescriptorType::Entity, 0);
			controller->readEntityDescriptor(entityID, std::bind(&ControllerImpl::onEntityDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
		}
		else
		{
			LOG_CONTROLLER_TRACE(entityID, "Entity does not support AEM, simply advertise it");
			checkAdvertiseEntity(controlledEntity.get());
		}
	}
	else
	{
		LOG_CONTROLLER_DEBUG(entityID, "onEntityOnline: Entity already registered, updating it");
		// This should not happen, but just in case... update it
		onEntityUpdate(controller, entityID, entity);
	}

}

void ControllerImpl::onEntityUpdate(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::Entity const& entity) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onEntityUpdate");

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

#ifdef DEBUG
	AVDECC_ASSERT(controlledEntity, "Unknown entity");
#endif
	if (controlledEntity)
	{
		updateEntity(*controlledEntity, entity);
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
		updateAcquiredState(*controlledEntity, getUninitializedIdentifier(), entity::model::DescriptorType::Entity, 0u, true);

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

		// Take a copy of the ControlledEntity so we don't have to keep the lock
		auto controlledEntity = getControlledEntityImpl(listenerStream.entityID);
		if (controlledEntity)
		{
			checkAdvertiseEntity(controlledEntity.get());
		}
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
		try
		{
			updateAcquiredState(*entity, owningEntity, descriptorType, descriptorIndex);
		}
		catch ([[maybe_unused]] controller::ControlledEntity::Exception const& e)
		{
			// Check if the entity went offline and online again or got an enumeration error (in which case this exception might be normal)
			if (!AVDECC_ASSERT_WITH_RET(!entity->wasAdvertised() || entity->gotEnumerationError(), "onEntityAcquired succeeded on the entity, but failed to update local model"))
			{
				LOG_CONTROLLER_WARN(entityID, "onEntityAcquired failed (Owner={} DescriptorType={} DescriptorIndex={}): {}", toHexString(owningEntity, true), to_integral(descriptorType), descriptorIndex, e.what());
			}
		}
	}
}

void ControllerImpl::onEntityReleased(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, UniqueIdentifier const owningEntity, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		try
		{
			updateAcquiredState(*entity, owningEntity, descriptorType, descriptorIndex);
		}
		catch ([[maybe_unused]] controller::ControlledEntity::Exception const& e)
		{
			// Check if the entity went offline and online again or got an enumeration error (in which case this exception might be normal)
			if (!AVDECC_ASSERT_WITH_RET(!entity->wasAdvertised() || entity->gotEnumerationError(), "onEntityReleased succeeded on the entity, but failed to update local model"))
			{
				LOG_CONTROLLER_WARN(entityID, "onEntityReleased failed (Owner={} DescriptorType={} DescriptorIndex={}): {}", toHexString(owningEntity, true), to_integral(descriptorType), descriptorIndex, e.what());
			}
		}
	}
}

void ControllerImpl::onConfigurationChanged(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		try
		{
			updateConfiguration(controller, *entity, configurationIndex);
		}
		catch ([[maybe_unused]] controller::ControlledEntity::Exception const& e)
		{
			// Check if the entity went offline and online again or got an enumeration error (in which case this exception might be normal)
			if (!AVDECC_ASSERT_WITH_RET(!entity->wasAdvertised() || entity->gotEnumerationError(), "onConfigurationChanged succeeded on the entity, but failed to update local model"))
			{
				LOG_CONTROLLER_WARN(entityID, "onConfigurationChanged failed (ConfigurationIndex={}): {}", configurationIndex, e.what());
			}
		}
	}
}

void ControllerImpl::onStreamInputFormatChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		try
		{
			updateStreamInputFormat(*entity, streamIndex, streamFormat);
		}
		catch ([[maybe_unused]] controller::ControlledEntity::Exception const& e)
		{
			// Check if the entity went offline and online again or got an enumeration error (in which case this exception might be normal)
			if (!AVDECC_ASSERT_WITH_RET(!entity->wasAdvertised() || entity->gotEnumerationError(), "onStreamInputFormatChanged succeeded on the entity, but failed to update local model"))
			{
				LOG_CONTROLLER_WARN(entityID, "onStreamInputFormatChanged failed (StreamIndex={} StreamFormat={}): {}", streamIndex, toHexString(streamFormat, true), e.what());
			}
		}
	}
}

void ControllerImpl::onStreamOutputFormatChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		try
		{
			updateStreamOutputFormat(*entity, streamIndex, streamFormat);
		}
		catch ([[maybe_unused]] controller::ControlledEntity::Exception const& e)
		{
			// Check if the entity went offline and online again or got an enumeration error (in which case this exception might be normal)
			if (!AVDECC_ASSERT_WITH_RET(!entity->wasAdvertised() || entity->gotEnumerationError(), "onStreamOutputFormatChanged succeeded on the entity, but failed to update local model"))
			{
				LOG_CONTROLLER_WARN(entityID, "onStreamOutputFormatChanged failed (StreamIndex={} StreamFormat={}): {}", streamIndex, toHexString(streamFormat, true), e.what());
			}
		}
	}
}

void ControllerImpl::onStreamPortInputAudioMappingsChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, entity::model::AudioMappings const& mappings) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		try
		{
			// Only support the case where numberOfMaps == 1
			if (numberOfMaps != 1 || mapIndex != 0)
				return;

			auto const& entityDescriptor = controlledEntity->getEntityDescriptor();
			controlledEntity->clearPortInputStreamAudioMappings(entityDescriptor.dynamicModel.currentConfiguration, streamPortIndex);
			updateStreamPortInputAudioMappingsAdded(*entity, streamPortIndex, mappings);
		}
		catch ([[maybe_unused]] controller::ControlledEntity::Exception const& e)
		{
			// Check if the entity went offline and online again or got an enumeration error (in which case this exception might be normal)
			if (!AVDECC_ASSERT_WITH_RET(!entity->wasAdvertised() || entity->gotEnumerationError(), "onStreamInputAudioMappingsChanged succeeded on the entity, but failed to update local model"))
			{
				LOG_CONTROLLER_WARN(entityID, "onStreamPortInputAudioMappingsChanged failed (StreamPortIndex={} NumberMaps={} MapIndex={}): {}", streamPortIndex, numberOfMaps, mapIndex, e.what());
			}
		}
	}
}

void ControllerImpl::onStreamPortOutputAudioMappingsChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamPortIndex const streamPortIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, entity::model::AudioMappings const& mappings) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		try
		{
			// Only support the case where numberOfMaps == 1
			if (numberOfMaps != 1 || mapIndex != 0)
				return;

			auto const& entityDescriptor = controlledEntity->getEntityDescriptor();
			controlledEntity->clearPortOutputStreamAudioMappings(entityDescriptor.dynamicModel.currentConfiguration, streamPortIndex);
			updateStreamPortOutputAudioMappingsAdded(*entity, streamPortIndex, mappings);
		}
		catch ([[maybe_unused]] controller::ControlledEntity::Exception const& e)
		{
			// Check if the entity went offline and online again or got an enumeration error (in which case this exception might be normal)
			if (!AVDECC_ASSERT_WITH_RET(!entity->wasAdvertised() || entity->gotEnumerationError(), "onStreamOutputAudioMappingsChanged succeeded on the entity, but failed to update local model"))
			{
				LOG_CONTROLLER_WARN(entityID, "onStreamPortOutputAudioMappingsChanged failed (StreamPortIndex={} NumberMaps={} MapIndex={}): {}", streamPortIndex, numberOfMaps, mapIndex, e.what());
			}
		}
	}
}

void ControllerImpl::onStreamInputInfoChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		try
		{
			updateStreamInputInfo(*entity, streamIndex, info);
		}
		catch ([[maybe_unused]] controller::ControlledEntity::Exception const& e)
		{
			// Check if the entity went offline and online again or got an enumeration error (in which case this exception might be normal)
			if (!AVDECC_ASSERT_WITH_RET(!entity->wasAdvertised() || entity->gotEnumerationError(), "onStreamInputInfoChanged succeeded on the entity, but failed to update local model"))
			{
				LOG_CONTROLLER_WARN(entityID, "onStreamInputInfoChanged failed (StreamIndex={}): {}", streamIndex, e.what());
			}
		}
	}
}

void ControllerImpl::onStreamOutputInfoChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		try
		{
			updateStreamOutputInfo(*entity, streamIndex, info);
		}
		catch ([[maybe_unused]] controller::ControlledEntity::Exception const& e)
		{
			// Check if the entity went offline and online again or got an enumeration error (in which case this exception might be normal)
			if (!AVDECC_ASSERT_WITH_RET(!entity->wasAdvertised() || entity->gotEnumerationError(), "onStreamOutputInfoChanged succeeded on the entity, but failed to update local model"))
			{
				LOG_CONTROLLER_WARN(entityID, "onStreamOutputInfoChanged failed (StreamIndex={}): {}", streamIndex, e.what());
			}
		}
	}
}

void ControllerImpl::onEntityNameChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::AvdeccFixedString const& entityName) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		try
		{
			updateEntityName(*entity, entityName);
		}
		catch ([[maybe_unused]] controller::ControlledEntity::Exception const& e)
		{
			// Check if the entity went offline and online again or got an enumeration error (in which case this exception might be normal)
			if (!AVDECC_ASSERT_WITH_RET(!entity->wasAdvertised() || entity->gotEnumerationError(), "onEntityNameChanged succeeded on the entity, but failed to update local model"))
			{
				LOG_CONTROLLER_WARN(entityID, "onEntityNameChanged failed (Name={}): {}", entityName.str(), e.what());
			}
		}
	}
}

void ControllerImpl::onEntityGroupNameChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::AvdeccFixedString const& entityGroupName) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		try
		{
			updateEntityGroupName(*entity, entityGroupName);
		}
		catch ([[maybe_unused]] controller::ControlledEntity::Exception const& e)
		{
			// Check if the entity went offline and online again or got an enumeration error (in which case this exception might be normal)
			if (!AVDECC_ASSERT_WITH_RET(!entity->wasAdvertised() || entity->gotEnumerationError(), "onEntityGroupNameChanged succeeded on the entity, but failed to update local model"))
			{
				LOG_CONTROLLER_WARN(entityID, "onEntityGroupNameChanged failed (GroupName={}): {}", entityGroupName.str(), e.what());
			}
		}
	}
}

void ControllerImpl::onConfigurationNameChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& configurationName) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		try
		{
			updateConfigurationName(*entity, configurationIndex, configurationName);
		}
		catch ([[maybe_unused]] controller::ControlledEntity::Exception const& e)
		{
			// Check if the entity went offline and online again or got an enumeration error (in which case this exception might be normal)
			if (!AVDECC_ASSERT_WITH_RET(!entity->wasAdvertised() || entity->gotEnumerationError(), "onConfigurationNameChanged succeeded on the entity, but failed to update local model"))
			{
				LOG_CONTROLLER_WARN(entityID, "onConfigurationNameChanged failed (ConfigurationIndex={} Name={}): {}", configurationIndex, configurationName.str(), e.what());
			}
		}
	}
}

void ControllerImpl::onStreamInputNameChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamName) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		try
		{
			updateStreamInputName(*entity, configurationIndex, streamIndex, streamName);
		}
		catch ([[maybe_unused]] controller::ControlledEntity::Exception const& e)
		{
			// Check if the entity went offline and online again or got an enumeration error (in which case this exception might be normal)
			if (!AVDECC_ASSERT_WITH_RET(!entity->wasAdvertised() || entity->gotEnumerationError(), "onStreamInputNameChanged succeeded on the entity, but failed to update local model"))
			{
				LOG_CONTROLLER_WARN(entityID, "onStreamInputNameChanged failed (ConfigurationIndex={} StreamIndex={} Name={}): {}", configurationIndex, streamIndex, streamName.str(), e.what());
			}
		}
	}
}

void ControllerImpl::onStreamOutputNameChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamName) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		try
		{
			updateStreamOutputName(*entity, configurationIndex, streamIndex, streamName);
		}
		catch ([[maybe_unused]] controller::ControlledEntity::Exception const& e)
		{
			// Check if the entity went offline and online again or got an enumeration error (in which case this exception might be normal)
			if (!AVDECC_ASSERT_WITH_RET(!entity->wasAdvertised() || entity->gotEnumerationError(), "onStreamOutputNameChanged succeeded on the entity, but failed to update local model"))
			{
				LOG_CONTROLLER_WARN(entityID, "onStreamOutputNameChanged failed (ConfigurationIndex={} StreamIndex={} Name={}): {}", configurationIndex, streamIndex, streamName.str(), e.what());
			}
		}
	}
}

void ControllerImpl::onAudioUnitSamplingRateChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::SamplingRate const samplingRate) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		try
		{
			updateAudioUnitSamplingRate(*entity, audioUnitIndex, samplingRate);
		}
		catch ([[maybe_unused]] controller::ControlledEntity::Exception const& e)
		{
			// Check if the entity went offline and online again or got an enumeration error (in which case this exception might be normal)
			if (!AVDECC_ASSERT_WITH_RET(!entity->wasAdvertised() || entity->gotEnumerationError(), "onAudioUnitSamplingRateChanged succeeded on the entity, but failed to update local model"))
			{
				LOG_CONTROLLER_WARN(entityID, "onAudioUnitSamplingRateChanged failed (AudioUnitIndex={} SamplingRate={}): {}", audioUnitIndex, samplingRate, e.what());
			}
		}
	}
}

void ControllerImpl::onClockSourceChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockSourceIndex const clockSourceIndex) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		try
		{
			updateClockSource(*entity, clockDomainIndex, clockSourceIndex);
		}
		catch ([[maybe_unused]] controller::ControlledEntity::Exception const& e)
		{
			// Check if the entity went offline and online again or got an enumeration error (in which case this exception might be normal)
			if (!AVDECC_ASSERT_WITH_RET(!entity->wasAdvertised() || entity->gotEnumerationError(), "onClockSourceChanged succeeded on the entity, but failed to update local model"))
			{
				LOG_CONTROLLER_WARN(entityID, "onClockSourceChanged failed (ClockDomainIndex={} ClockSourceIndex={}): {}", clockDomainIndex, clockSourceIndex, e.what());
			}
		}
	}
}

void ControllerImpl::onStreamInputStarted(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		try
		{
			updateStreamInputRunningStatus(*entity, streamIndex, true);
		}
		catch ([[maybe_unused]] controller::ControlledEntity::Exception const& e)
		{
			// Check if the entity went offline and online again or got an enumeration error (in which case this exception might be normal)
			if (!AVDECC_ASSERT_WITH_RET(!entity->wasAdvertised() || entity->gotEnumerationError(), "onStreamInputStarted succeeded on the entity, but failed to update local model"))
			{
				LOG_CONTROLLER_WARN(entityID, "onStreamInputStarted failed (StreamIndex={}): {}", streamIndex, e.what());
			}
		}
	}
}

void ControllerImpl::onStreamOutputStarted(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		try
		{
			updateStreamOutputRunningStatus(*entity, streamIndex, true);
		}
		catch ([[maybe_unused]] controller::ControlledEntity::Exception const& e)
		{
			// Check if the entity went offline and online again or got an enumeration error (in which case this exception might be normal)
			if (!AVDECC_ASSERT_WITH_RET(!entity->wasAdvertised() || entity->gotEnumerationError(), "onStreamOutputStarted succeeded on the entity, but failed to update local model"))
			{
				LOG_CONTROLLER_WARN(entityID, "onStreamOutputStarted failed (StreamIndex={}): {}", streamIndex, e.what());
			}
		}
	}
}

void ControllerImpl::onStreamInputStopped(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		try
		{
			updateStreamInputRunningStatus(*entity, streamIndex, false);
		}
		catch ([[maybe_unused]] controller::ControlledEntity::Exception const& e)
		{
			// Check if the entity went offline and online again or got an enumeration error (in which case this exception might be normal)
			if (!AVDECC_ASSERT_WITH_RET(!entity->wasAdvertised() || entity->gotEnumerationError(), "onStreamInputStarted succeeded on the entity, but failed to update local model"))
			{
				LOG_CONTROLLER_WARN(entityID, "onStreamInputStarted failed (StreamIndex={}): {}", streamIndex, e.what());
			}
		}
	}
}

void ControllerImpl::onStreamOutputStopped(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::StreamIndex const streamIndex) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		try
		{
			updateStreamOutputRunningStatus(*entity, streamIndex, false);
		}
		catch ([[maybe_unused]] controller::ControlledEntity::Exception const& e)
		{
			// Check if the entity went offline and online again or got an enumeration error (in which case this exception might be normal)
			if (!AVDECC_ASSERT_WITH_RET(!entity->wasAdvertised() || entity->gotEnumerationError(), "onStreamOutputStarted succeeded on the entity, but failed to update local model"))
			{
				LOG_CONTROLLER_WARN(entityID, "onStreamOutputStarted failed (StreamIndex={}): {}", streamIndex, e.what());
			}
		}
	}
}

void ControllerImpl::onAvbInfoChanged(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvbInfo const& info) noexcept
{
	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		auto* const entity = controlledEntity.get();
		try
		{
			updateAvbInfo(*entity, avbInterfaceIndex, info);
		}
		catch ([[maybe_unused]] controller::ControlledEntity::Exception const& e)
		{
			// Check if the entity went offline and online again or got an enumeration error (in which case this exception might be normal)
			if (!AVDECC_ASSERT_WITH_RET(!entity->wasAdvertised() || entity->gotEnumerationError(), "onAvbInfoChanged succeeded on the entity, but failed to update local model"))
			{
				LOG_CONTROLLER_WARN(entityID, "onAvbInfoChanged failed (AvbInterfaceIndex={}): {}", avbInterfaceIndex, e.what());
			}
		}
	}
}

} // namespace controller
} // namespace avdecc
} // namespace la
