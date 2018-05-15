#pragma message("TODO: In (almost) each onXXXResult, check if configurationIndex is still the currentConfiguration. If not then stop the query. Maybe find a way to stop processing inflight queries too.")
#pragma message("TODO: For descriptor queries, do not store and remove in a std::set. Instead store a tuple<DescriptorKey, bool completed, resultHandlers>, so that we keep track of already completed queries. When a query is requested, first check if the descriptor has been retrieved")

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
* @file avdeccControllerImpl.cpp
* @author Christophe Calmejane
*/

#include "avdeccControllerImpl.hpp"
#include "la/avdecc/logger.hpp"

namespace la
{
namespace avdecc
{
namespace controller
{

/* ************************************************************ */
/* Private methods used to update AEM and notify observers      */
/* ************************************************************ */
void ControllerImpl::updateEntity(ControlledEntityImpl& controlledEntity, entity::Entity const& entity, bool const alsoUpdateAvbInfo) const
{
	// Get previous entity info, so we can check what changed
	auto oldEntity = controlledEntity.getEntity();

	// Update entity info
	//auto const infoChanged = oldEntity != entity;
	controlledEntity.setEntity(entity);

	// Check for specific changes
	if (alsoUpdateAvbInfo)
	{
		auto const oldCaps = oldEntity.getEntityCapabilities();
		auto const caps = entity.getEntityCapabilities();
		// GPTP info change (if it's both previously and now supported)
		if (hasFlag(oldCaps, entity::EntityCapabilities::GptpSupported) && hasFlag(caps, entity::EntityCapabilities::GptpSupported) && hasFlag(caps, entity::EntityCapabilities::AemInterfaceIndexValid))
		{
			auto const oldGptpGrandmasterID = oldEntity.getGptpGrandmasterID();
			auto const oldGptpDomainNumber = oldEntity.getGptpDomainNumber();
			auto const newGptpGrandmasterID = entity.getGptpGrandmasterID();
			auto const newGptpDomainNumber = entity.getGptpDomainNumber();

			if (oldGptpGrandmasterID != newGptpGrandmasterID || oldGptpDomainNumber != newGptpDomainNumber)
			{
				try
				{
					auto const avbInterfaceIndex = entity.getInterfaceIndex();
					auto const& entityDescriptor = controlledEntity.getEntityDescriptor();
					auto& avbInterfaceDescriptor = controlledEntity.getAvbInterfaceDescriptor(entityDescriptor.dynamicModel.currentConfiguration, avbInterfaceIndex);

					auto info = avbInterfaceDescriptor.dynamicModel.avbInfo; // Copy the AvbInfo so we can alter values
					info.gptpGrandmasterID = newGptpGrandmasterID;
					info.gptpDomainNumber = newGptpDomainNumber;
					updateAvbInfo(controlledEntity, entity.getInterfaceIndex(), info, false);
				}
				catch (ControlledEntity::Exception const&)
				{
					// Ignore exceptions, in case we got an update of this entity before the AvbInterfaceDescriptor has been retrieved
				}
			}
		}
	}

	// Check for Advertise, in case the entity switched from a NotReady to Ready state
	checkAdvertiseEntity(&controlledEntity);
}

void ControllerImpl::updateAcquiredState(ControlledEntityImpl& controlledEntity, UniqueIdentifier const owningEntity, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const /*descriptorIndex*/, bool const undefined) const
{
#pragma message("TODO: Handle acquire state tree")
	if (descriptorType == entity::model::DescriptorType::Entity)
	{
		auto owningController{ getUninitializedIdentifier() };
		auto acquireState{ model::AcquireState::Undefined };
		if (undefined)
		{
			owningController = getUninitializedIdentifier();
			acquireState = model::AcquireState::Undefined;
		}
		else
		{
			owningController = owningEntity;

			if (!isValidUniqueIdentifier(owningEntity)) // No more controller
				acquireState = model::AcquireState::NotAcquired;
			else if (owningEntity == _controller->getEntityID()) // Controlled by myself
				acquireState = model::AcquireState::Acquired;
			else // Or acquired by another controller
				acquireState = model::AcquireState::AcquiredByOther;
		}

		controlledEntity.setAcquireState(acquireState);
		controlledEntity.setOwningController(owningController);

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onAcquireStateChanged, this, &controlledEntity, acquireState, owningController);
		}
	}
}

void ControllerImpl::updateConfiguration(entity::ControllerEntity const* const controller, ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex) const
{
	try
	{
		auto& entityDescriptor = controlledEntity.getEntityDescriptor();

		entityDescriptor.dynamicModel.currentConfiguration = configurationIndex;

		// Right now, simulate the entity going offline then online again - TODO: Handle multiple configurations, see https://github.com/L-Acoustics/avdecc/issues/3
		auto const e = controlledEntity.getEntity(); // Make a copy of the Entity object since it will be destroyed during onEntityOffline
		auto const entityID = e.getEntityID();
		auto* self = const_cast<ControllerImpl*>(this);
		self->onEntityOffline(controller, entityID);
		self->onEntityOnline(controller, entityID, e);
	}
	catch (Exception const&)
	{
		// Ignore Controller::Exception
	}
	catch (ControlledEntity::Exception const&)
	{
		// But rethrow ControlledEntity::Exception
		throw;
	}
}

void ControllerImpl::updateStreamInputFormat(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) const
{
	try
	{
		auto const& entityDescriptor = controlledEntity.getEntityDescriptor();
		auto& streamDescriptor = controlledEntity.getStreamInputDescriptor(entityDescriptor.dynamicModel.currentConfiguration, streamIndex);

		streamDescriptor.dynamicModel.currentFormat = streamFormat;

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamInputFormatChanged, this, &controlledEntity, streamIndex, streamFormat);
		}
	}
	catch (Exception const&)
	{
		// Ignore Controller::Exception
	}
	catch (ControlledEntity::Exception const&)
	{
		// But rethrow ControlledEntity::Exception
		throw;
	}
}

void ControllerImpl::updateStreamOutputFormat(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) const
{
	try
	{
		auto const& entityDescriptor = controlledEntity.getEntityDescriptor();
		auto& streamDescriptor = controlledEntity.getStreamOutputDescriptor(entityDescriptor.dynamicModel.currentConfiguration, streamIndex);

		streamDescriptor.dynamicModel.currentFormat = streamFormat;

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputFormatChanged, this, &controlledEntity, streamIndex, streamFormat);
		}
	}
	catch (Exception const&)
	{
		// Ignore Controller::Exception
	}
	catch (ControlledEntity::Exception const&)
	{
		// But rethrow ControlledEntity::Exception
		throw;
	}
}

void ControllerImpl::updateStreamInputInfo(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info, bool const alsoUpdateIsRunning) const
{
	try
	{
		auto const& entityDescriptor = controlledEntity.getEntityDescriptor();
		auto& streamDescriptor = controlledEntity.getStreamInputDescriptor(entityDescriptor.dynamicModel.currentConfiguration, streamIndex);

		auto const infoChanged = streamDescriptor.dynamicModel.streamInfo != info;
		streamDescriptor.dynamicModel.streamInfo = info;

		// Update stream running status
		if (alsoUpdateIsRunning)
		{
			updateStreamInputRunningStatus(controlledEntity, streamIndex, !hasFlag(info.streamInfoFlags, entity::StreamInfoFlags::StreamingWait), false);
		}

		// Entity was advertised to the user, notify observers (if info changed)
		if (controlledEntity.wasAdvertised() && infoChanged)
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamInputInfoChanged, this, &controlledEntity, streamIndex, info);
		}
	}
	catch (Exception const&)
	{
		// Ignore Controller::Exception
	}
	catch (ControlledEntity::Exception const&)
	{
		// But rethrow ControlledEntity::Exception
		throw;
	}
}

void ControllerImpl::updateStreamOutputInfo(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info, bool const alsoUpdateIsRunning) const
{
	try
	{
		auto const& entityDescriptor = controlledEntity.getEntityDescriptor();
		auto& streamDescriptor = controlledEntity.getStreamOutputDescriptor(entityDescriptor.dynamicModel.currentConfiguration, streamIndex);

		auto const infoChanged = streamDescriptor.dynamicModel.streamInfo != info;
		streamDescriptor.dynamicModel.streamInfo = info;

		// Update stream running status
		if (alsoUpdateIsRunning)
		{
			updateStreamOutputRunningStatus(controlledEntity, streamIndex, !hasFlag(info.streamInfoFlags, entity::StreamInfoFlags::StreamingWait), false);
		}

		// Entity was advertised to the user, notify observers (if info changed)
		if (controlledEntity.wasAdvertised() && infoChanged)
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputInfoChanged, this, &controlledEntity, streamIndex, info);
		}
	}
	catch (Exception const&)
	{
		// Ignore Controller::Exception
	}
	catch (ControlledEntity::Exception const&)
	{
		// But rethrow ControlledEntity::Exception
		throw;
	}
}

void ControllerImpl::updateEntityName(ControlledEntityImpl& controlledEntity, entity::model::AvdeccFixedString const& entityName) const
{
	try
	{
		auto& entityDescriptor = controlledEntity.getEntityDescriptor();

		entityDescriptor.dynamicModel.entityName = entityName;

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityNameChanged, this, &controlledEntity, entityName);
		}
	}
	catch (Exception const&)
	{
		// Ignore Controller::Exception
	}
	catch (ControlledEntity::Exception const&)
	{
		// But rethrow ControlledEntity::Exception
		throw;
	}
}

void ControllerImpl::updateEntityGroupName(ControlledEntityImpl& controlledEntity, entity::model::AvdeccFixedString const& entityGroupName) const
{
	try
	{
		auto& entityDescriptor = controlledEntity.getEntityDescriptor();

		entityDescriptor.dynamicModel.groupName = entityGroupName;

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityGroupNameChanged, this, &controlledEntity, entityGroupName);
		}
	}
	catch (Exception const&)
	{
		// Ignore Controller::Exception
	}
	catch (ControlledEntity::Exception const&)
	{
		// But rethrow ControlledEntity::Exception
		throw;
	}
}

void ControllerImpl::updateConfigurationName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& configurationName) const
{
	try
	{
		auto& configurationDescriptor = controlledEntity.getConfigurationDescriptor(configurationIndex);

		configurationDescriptor.dynamicModel.objectName = configurationName;

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onConfigurationNameChanged, this, &controlledEntity, configurationIndex, configurationName);
		}
	}
	catch (Exception const&)
	{
		// Ignore Controller::Exception
	}
	catch (ControlledEntity::Exception const&)
	{
		// But rethrow ControlledEntity::Exception
		throw;
	}
}

void ControllerImpl::updateStreamInputName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamInputName) const
{
	try
	{
		auto& entityDescriptor = controlledEntity.getEntityDescriptor();

#pragma message("TODO: Handle multiple configurations, not just the active one (no need to use getConfigurationNode, getStreamInputNode shall be used directly)")
		if (configurationIndex == entityDescriptor.dynamicModel.currentConfiguration)
		{
			auto& streamDescriptor = controlledEntity.getStreamInputDescriptor(configurationIndex, streamIndex);
			streamDescriptor.dynamicModel.objectName = streamInputName;
		}

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamInputNameChanged, this, &controlledEntity, configurationIndex, streamIndex, streamInputName);
		}
	}
	catch (Exception const&)
	{
		// Ignore Controller::Exception
	}
	catch (ControlledEntity::Exception const&)
	{
		// But rethrow ControlledEntity::Exception
		throw;
	}
}

void ControllerImpl::updateStreamOutputName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamOutputName) const
{
	try
	{
		auto& entityDescriptor = controlledEntity.getEntityDescriptor();

#pragma message("TODO: Handle multiple configurations, not just the active one (no need to use getConfigurationNode, getStreamOutputNode shall be used directly)")
		if (configurationIndex == entityDescriptor.dynamicModel.currentConfiguration)
		{
			auto& streamDescriptor = controlledEntity.getStreamOutputDescriptor(configurationIndex, streamIndex);
			streamDescriptor.dynamicModel.objectName = streamOutputName;
		}

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputNameChanged, this, &controlledEntity, configurationIndex, streamIndex, streamOutputName);
		}
	}
	catch (Exception const&)
	{
		// Ignore Controller::Exception
	}
	catch (ControlledEntity::Exception const&)
	{
		// But rethrow ControlledEntity::Exception
		throw;
	}
}

void ControllerImpl::updateAudioUnitSamplingRate(ControlledEntityImpl& controlledEntity, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::SamplingRate const samplingRate) const
{
	try
	{
		auto const& entityDescriptor = controlledEntity.getEntityDescriptor();
		auto& audioUnitDescriptor = controlledEntity.getAudioUnitDescriptor(entityDescriptor.dynamicModel.currentConfiguration, audioUnitIndex);

		audioUnitDescriptor.dynamicModel.currentSamplingRate = samplingRate;

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onAudioUnitSamplingRateChanged, this, &controlledEntity, audioUnitIndex, samplingRate);
		}
	}
	catch (Exception const&)
	{
		// Ignore Controller::Exception
	}
	catch (ControlledEntity::Exception const&)
	{
		// But rethrow ControlledEntity::Exception
		throw;
	}
}

void ControllerImpl::updateClockSource(ControlledEntityImpl& controlledEntity, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockSourceIndex const clockSourceIndex) const
{
	try
	{
		auto const& entityDescriptor = controlledEntity.getEntityDescriptor();
		auto& domainDescriptor = controlledEntity.getClockDomainDescriptor(entityDescriptor.dynamicModel.currentConfiguration, clockDomainIndex);

		domainDescriptor.dynamicModel.clockSourceIndex = clockSourceIndex;

		// Entity was advertised to the user, notify observers
		if (controlledEntity.wasAdvertised())
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onClockSourceChanged, this, &controlledEntity, clockDomainIndex, clockSourceIndex);
		}
	}
	catch (Exception const&)
	{
		// Ignore Controller::Exception
	}
	catch (ControlledEntity::Exception const&)
	{
		// But rethrow ControlledEntity::Exception
		throw;
	}
}

void ControllerImpl::updateStreamInputRunningStatus(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, bool const isRunning, bool const alsoUpdateStreamInfo) const
{
	try
	{
		auto const& entityDescriptor = controlledEntity.getEntityDescriptor();
		auto& streamDescriptor = controlledEntity.getStreamInputDescriptor(entityDescriptor.dynamicModel.currentConfiguration, streamIndex);

		auto const infoChanged = streamDescriptor.dynamicModel.isRunning != isRunning;
		streamDescriptor.dynamicModel.isRunning = isRunning;

		// Also update the flags in StreamInfo since some entities do not send an unsolicited when StreamInfo flags change due to Start/Stop Streaming
		if (alsoUpdateStreamInfo)
		{
			auto info = streamDescriptor.dynamicModel.streamInfo; // Copy the StreamInfo so we can alter the flags
			if (isRunning)
			{
				la::avdecc::clearFlag(info.streamInfoFlags, entity::StreamInfoFlags::StreamingWait);
			}
			else
			{
				la::avdecc::addFlag(info.streamInfoFlags, entity::StreamInfoFlags::StreamingWait);
			}
			updateStreamInputInfo(controlledEntity, streamIndex, info, false);
		}

		// Entity was advertised to the user, notify observers (if info changed)
		if (controlledEntity.wasAdvertised() && infoChanged)
		{
			if (isRunning)
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamInputStarted, this, &controlledEntity, streamIndex);
			else
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamInputStopped, this, &controlledEntity, streamIndex);
		}
	}
	catch (Exception const&)
	{
		// Ignore Controller::Exception
	}
	catch (ControlledEntity::Exception const&)
	{
		// But rethrow ControlledEntity::Exception
		throw;
	}
}

void ControllerImpl::updateStreamOutputRunningStatus(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, bool const isRunning, bool const alsoUpdateStreamInfo) const
{
	try
	{
		auto const& entityDescriptor = controlledEntity.getEntityDescriptor();
		auto& streamDescriptor = controlledEntity.getStreamOutputDescriptor(entityDescriptor.dynamicModel.currentConfiguration, streamIndex);

		auto const infoChanged = streamDescriptor.dynamicModel.isRunning != isRunning;
		streamDescriptor.dynamicModel.isRunning = isRunning;

		// Also update the flags in StreamInfo since some entities do not send an unsolicited when StreamInfo flags change due to Start/Stop Streaming
		if (alsoUpdateStreamInfo)
		{
			auto info = streamDescriptor.dynamicModel.streamInfo; // Copy the StreamInfo so we can alter the flags
			if (isRunning)
			{
				la::avdecc::clearFlag(info.streamInfoFlags, entity::StreamInfoFlags::StreamingWait);
			}
			else
			{
				la::avdecc::addFlag(info.streamInfoFlags, entity::StreamInfoFlags::StreamingWait);
			}
			updateStreamOutputInfo(controlledEntity, streamIndex, info, false);
		}

		// Entity was advertised to the user, notify observers (if info changed)
		if (controlledEntity.wasAdvertised() && infoChanged)
		{
			if (isRunning)
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputStarted, this, &controlledEntity, streamIndex);
			else
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputStopped, this, &controlledEntity, streamIndex);
		}
	}
	catch (Exception const&)
	{
		// Ignore Controller::Exception
	}
	catch (ControlledEntity::Exception const&)
	{
		// But rethrow ControlledEntity::Exception
		throw;
	}
}

void ControllerImpl::updateAvbInfo(ControlledEntityImpl& controlledEntity, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvbInfo const& info, bool const alsoUpdateEntity) const
{
	try
	{
		auto const& entityDescriptor = controlledEntity.getEntityDescriptor();
		auto& avbInterfaceDescriptor = controlledEntity.getAvbInterfaceDescriptor(entityDescriptor.dynamicModel.currentConfiguration, avbInterfaceIndex);

		auto const infoChanged = avbInterfaceDescriptor.dynamicModel.avbInfo != info;
		auto const gptpChanged = avbInterfaceDescriptor.dynamicModel.avbInfo.gptpGrandmasterID != info.gptpGrandmasterID || avbInterfaceDescriptor.dynamicModel.avbInfo.gptpDomainNumber != info.gptpDomainNumber;
		avbInterfaceDescriptor.dynamicModel.avbInfo = info;

		// Also update the Gpgp values in ADP entity info
		if (alsoUpdateEntity)
		{
			auto entity{ ModifiableEntity(controlledEntity.getEntity()) }; // Copy the Entity so we can alter values
			auto const caps = entity.getEntityCapabilities();
			if (hasFlag(caps, entity::EntityCapabilities::GptpSupported) &&
				(!hasFlag(caps, entity::EntityCapabilities::AemInterfaceIndexValid) || entity.getInterfaceIndex() == avbInterfaceIndex))
			{
				entity.setGptpGrandmasterID(info.gptpGrandmasterID);
				entity.setGptpDomainNumber(info.gptpDomainNumber);
				updateEntity(controlledEntity, entity, false);
			}
		}

		// Entity was advertised to the user, notify observers (if info changed)
		if (controlledEntity.wasAdvertised() && infoChanged)
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onAvbInfoChanged, this, &controlledEntity, avbInterfaceIndex, info);

			if (gptpChanged)
			{
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onGptpChanged, this, &controlledEntity, avbInterfaceIndex, info.gptpGrandmasterID, info.gptpDomainNumber);
			}
		}
	}
	catch (Exception const&)
	{
		// Ignore Controller::Exception
	}
	catch (ControlledEntity::Exception const&)
	{
		// But rethrow ControlledEntity::Exception
		throw;
	}
}

void ControllerImpl::updateStreamPortInputAudioMappingsAdded(ControlledEntityImpl& controlledEntity, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) const
{
	auto const& entityDescriptor = controlledEntity.getEntityDescriptor();
	controlledEntity.addPortInputStreamAudioMappings(entityDescriptor.dynamicModel.currentConfiguration, streamPortIndex, mappings);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamPortInputAudioMappingsChanged, this, &controlledEntity, streamPortIndex);
	}
}

void ControllerImpl::updateStreamPortInputAudioMappingsRemoved(ControlledEntityImpl& controlledEntity, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) const
{
	auto const& entityDescriptor = controlledEntity.getEntityDescriptor();
	controlledEntity.removePortInputStreamAudioMappings(entityDescriptor.dynamicModel.currentConfiguration, streamPortIndex, mappings);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamPortInputAudioMappingsChanged, this, &controlledEntity, streamPortIndex);
	}
}

void ControllerImpl::updateStreamPortOutputAudioMappingsAdded(ControlledEntityImpl& controlledEntity, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) const
{
	auto const& entityDescriptor = controlledEntity.getEntityDescriptor();
	controlledEntity.addPortOutputStreamAudioMappings(entityDescriptor.dynamicModel.currentConfiguration, streamPortIndex, mappings);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamPortOutputAudioMappingsChanged, this, &controlledEntity, streamPortIndex);
	}
}

void ControllerImpl::updateStreamPortOutputAudioMappingsRemoved(ControlledEntityImpl& controlledEntity, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) const
{
	auto const& entityDescriptor = controlledEntity.getEntityDescriptor();
	controlledEntity.removePortOutputStreamAudioMappings(entityDescriptor.dynamicModel.currentConfiguration, streamPortIndex, mappings);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamPortOutputAudioMappingsChanged, this, &controlledEntity, streamPortIndex);
	}
}

/* ************************************************************ */
/* Private methods                                              */
/* ************************************************************ */
void ControllerImpl::checkAdvertiseEntity(ControlledEntityImpl* const entity) const noexcept
{
	if (!entity->wasAdvertised())
	{
		auto const caps = entity->getEntity().getEntityCapabilities();
		if (entity->gotAllExpectedDescriptors() && entity->gotAllExpectedDynamicInfo() && !hasFlag(caps, entity::EntityCapabilities::EntityNotReady | entity::EntityCapabilities::GeneralControllerIgnore))
		{
			entity->setAdvertised(true);
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityOnline, this, entity);
		}
	}
}

bool ControllerImpl::checkRescheduleQuery(entity::ControllerEntity::AemCommandStatus const /*status*/, ControlledEntityImpl* const /*entity*/, entity::model::ConfigurationIndex const /*configurationIndex*/, entity::model::DescriptorType const /*descriptorType*/, entity::model::DescriptorIndex const /*descriptorIndex*/) const noexcept
{
#pragma message("TODO: Based on status code, reschedule a query") // We might want to have a map or micro-methods to run the query based on the parameters (and first-time queries shall use them too)
	return false;
}

bool ControllerImpl::checkRescheduleQuery(entity::ControllerEntity::AemCommandStatus const /*status*/, ControlledEntityImpl* const /*entity*/, entity::model::ConfigurationIndex const /*configurationIndex*/, ControlledEntityImpl::DynamicInfoType const /*dynamicInfoType*/, entity::model::DescriptorIndex const /*descriptorIndex*/) const noexcept
{
#pragma message("TODO: Based on status code, reschedule a query")
	return false;
}

bool ControllerImpl::checkRescheduleQuery(entity::ControllerEntity::ControlStatus const /*status*/, ControlledEntityImpl* const /*entity*/, entity::model::ConfigurationIndex const /*configurationIndex*/, ControlledEntityImpl::DynamicInfoType const /*dynamicInfoType*/, entity::model::DescriptorIndex const /*descriptorIndex*/, std::uint16_t const /*connectionIndex*/) const noexcept
{
#pragma message("TODO: Based on status code, reschedule a query")
	return false;
}

void ControllerImpl::handleListenerStreamStateNotification(entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, bool const isConnected, entity::ConnectionFlags const flags, bool const changedByOther) const noexcept
{
	try
	{
		// Build StreamConnectionState::State
		auto conState{ model::StreamConnectionState::State::NotConnected };
		if (isConnected)
		{
			conState = model::StreamConnectionState::State::Connected;
		}
		else if (avdecc::hasFlag(flags, entity::ConnectionFlags::FastConnect))
		{
			conState = model::StreamConnectionState::State::FastConnecting;
		}

		// Build Talker StreamIdentification
		auto talkerStreamIdentification{ entity::model::StreamIdentification{} };
		if (conState != model::StreamConnectionState::State::NotConnected)
		{
			AVDECC_ASSERT(isValidUniqueIdentifier(talkerStream.entityID), "Connected or FastConnecting to an invalid TalkerID");
			talkerStreamIdentification = talkerStream;
		}

		// Build a StreamConnectionState
		auto const state = model::StreamConnectionState{ listenerStream, talkerStreamIdentification, conState };

		// Check if Listener is online so we can update the StreamState
		{
			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto listenerEntity = getControlledEntityImpl(listenerStream.entityID);

			if (listenerEntity)
			{
				auto const cachedState = listenerEntity->getConnectedSinkState(listenerStream.streamIndex);

				// Check the previous state, and detect if it changed
				if (state != cachedState)
				{
					// Update our internal cache
					auto const& entityDescriptor = listenerEntity->getEntityDescriptor();
					listenerEntity->setInputStreamState(state, entityDescriptor.dynamicModel.currentConfiguration, listenerStream.streamIndex);

					// Entity was advertised to the user, notify observers
					if (listenerEntity->wasAdvertised())
					{
						notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamConnectionChanged, this, state, changedByOther);
					}
				}
			}
		}
	}
	catch (ControlledEntity::Exception const&)
	{
		// We don't care about exceptions from getConnectedSinkState
	}
	catch (...)
	{
		AVDECC_ASSERT(false, "Unknown exception");
	}
}

void ControllerImpl::handleTalkerStreamStateNotification(entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, bool const isConnected, entity::ConnectionFlags const flags, bool const changedByOther) const noexcept
{
	try
	{
		// Build Talker StreamIdentification
		auto const isFastConnect = avdecc::hasFlag(flags, entity::ConnectionFlags::FastConnect);
		auto talkerStreamIdentification{ entity::model::StreamIdentification{} };
		if (isConnected || isFastConnect)
		{
			AVDECC_ASSERT(isValidUniqueIdentifier(talkerStream.entityID), "Connected or FastConnecting to an invalid TalkerID");
			talkerStreamIdentification = talkerStream;
		}

		if (isFastConnect)
		{
			handleListenerStreamStateNotification(talkerStream, listenerStream, isConnected, flags, changedByOther);
		}

		// Check if Talker is valid and online so we can update the StreamConnections
		if (isValidUniqueIdentifier(talkerStream.entityID))
		{
			// Take a copy of the ControlledEntity so we don't have to keep the lock
			auto talkerEntity = getControlledEntityImpl(talkerStream.entityID);

			if (talkerEntity)
			{
				// Update our internal cache
				auto const& entityDescriptor = talkerEntity->getEntityDescriptor();
				auto shouldNotify{ false }; // Only notify if we actually changed the connections list
				if (isConnected)
				{
					shouldNotify = talkerEntity->addStreamOutputConnection(entityDescriptor.dynamicModel.currentConfiguration, talkerStream.streamIndex, listenerStream);
				}
				else
				{
					shouldNotify = talkerEntity->delStreamOutputConnection(entityDescriptor.dynamicModel.currentConfiguration, talkerStream.streamIndex, listenerStream);
				}
				// Entity was advertised to the user, notify observers
				if (shouldNotify && talkerEntity->wasAdvertised())
				{
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamConnectionsChanged, this, talkerEntity.get(), talkerStream.streamIndex, talkerEntity->getStreamOutputConnections(talkerStream.streamIndex));
				}
			}
		}
	}
	catch (ControlledEntity::Exception const&)
	{
		// We don't care about exceptions from getConnectedSinkState
	}
	catch (...)
	{
		AVDECC_ASSERT(false, "Unknown exception");
	}
}

void ControllerImpl::clearTalkerStreamConnections(ControlledEntityImpl* const talkerEntity, entity::model::StreamIndex const talkerStreamIndex) const noexcept
{
	try
	{
		auto const& entityDescriptor = talkerEntity->getEntityDescriptor();
		talkerEntity->clearStreamOutputConnections(entityDescriptor.dynamicModel.currentConfiguration, talkerStreamIndex);
	}
	catch (ControlledEntity::Exception const&)
	{
		// We don't care about exceptions
	}
	catch (...)
	{
		AVDECC_ASSERT(false, "Unknown exception");
	}
}

void ControllerImpl::addTalkerStreamConnection(ControlledEntityImpl* const talkerEntity, entity::model::StreamIndex const talkerStreamIndex, entity::model::StreamIdentification const& listenerStream) const noexcept
{
	try
	{
		// Update our internal cache
		auto const& entityDescriptor = talkerEntity->getEntityDescriptor();
		talkerEntity->addStreamOutputConnection(entityDescriptor.dynamicModel.currentConfiguration, talkerStreamIndex, listenerStream);
	}
	catch (ControlledEntity::Exception const&)
	{
		// We don't care about exceptions
	}
	catch (...)
	{
		AVDECC_ASSERT(false, "Unknown exception");
	}
}

void ControllerImpl::delTalkerStreamConnection(ControlledEntityImpl* const talkerEntity, entity::model::StreamIndex const talkerStreamIndex, entity::model::StreamIdentification const& listenerStream) const noexcept
{
	try
	{
		// Update our internal cache
		auto const& entityDescriptor = talkerEntity->getEntityDescriptor();
		talkerEntity->delStreamOutputConnection(entityDescriptor.dynamicModel.currentConfiguration, talkerStreamIndex, listenerStream);
	}
	catch (ControlledEntity::Exception const&)
	{
		// We don't care about exceptions
	}
	catch (...)
	{
		AVDECC_ASSERT(false, "Unknown exception");
	}
}

} // namespace controller
} // namespace avdecc
} // namespace la
