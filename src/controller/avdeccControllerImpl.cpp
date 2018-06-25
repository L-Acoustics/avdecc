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
#include "avdeccControllerLogHelper.hpp"

namespace la
{
namespace avdecc
{
namespace controller
{

/* ************************************************************ */
/* Private methods used to update AEM and notify observers      */
/* ************************************************************ */
void ControllerImpl::updateEntity(ControlledEntityImpl& controlledEntity, entity::Entity const& entity, bool const alsoUpdateAvbInfo) const noexcept
{
	// Get previous entity info, so we can check what changed
	auto oldEntity = controlledEntity.getEntity();

	// Update entity info
	controlledEntity.setEntity(entity);

	// Check for specific changes
	if (alsoUpdateAvbInfo)
	{
		auto const oldCaps = oldEntity.getEntityCapabilities();
		auto const caps = entity.getEntityCapabilities();
		// gPTP info change (if it's both previously and now supported)
		if (hasFlag(oldCaps, entity::EntityCapabilities::GptpSupported) && hasFlag(caps, entity::EntityCapabilities::GptpSupported) && hasFlag(caps, entity::EntityCapabilities::AemInterfaceIndexValid))
		{
			auto const oldGptpGrandmasterID = oldEntity.getGptpGrandmasterID();
			auto const oldGptpDomainNumber = oldEntity.getGptpDomainNumber();
			auto const newGptpGrandmasterID = entity.getGptpGrandmasterID();
			auto const newGptpDomainNumber = entity.getGptpDomainNumber();

			if (oldGptpGrandmasterID != newGptpGrandmasterID || oldGptpDomainNumber != newGptpDomainNumber)
			{
				auto const avbInterfaceIndex = entity.getInterfaceIndex();
				auto& avbInterfaceDynamicModel = controlledEntity.getNodeDynamicModel(controlledEntity.getCurrentConfigurationIndex(), avbInterfaceIndex, &model::ConfigurationDynamicTree::avbInterfaceDynamicModels);

				auto info = avbInterfaceDynamicModel.avbInfo; // Copy the AvbInfo so we can alter values
				info.gptpGrandmasterID = newGptpGrandmasterID;
				info.gptpDomainNumber = newGptpDomainNumber;
				updateAvbInfo(controlledEntity, entity.getInterfaceIndex(), info, false);
			}
		}
	}
}

void ControllerImpl::updateAcquiredState(ControlledEntityImpl& controlledEntity, UniqueIdentifier const owningEntity, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const /*descriptorIndex*/, bool const undefined) const noexcept
{
#pragma message("TODO: Handle acquire state tree")
	if (descriptorType == entity::model::DescriptorType::Entity)
	{
		auto owningController{ UniqueIdentifier{} };
		auto acquireState{ model::AcquireState::Undefined };
		if (undefined)
		{
			acquireState = model::AcquireState::Undefined;
		}
		else
		{
			owningController = owningEntity;

			if (!owningEntity) // No more controller
			{
				acquireState = model::AcquireState::NotAcquired;
			}
			else if (owningEntity == _controller->getEntityID()) // Controlled by myself
			{
				acquireState = model::AcquireState::Acquired;
			}
			else // Or acquired by another controller
			{
				acquireState = model::AcquireState::AcquiredByOther;
			}
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

void ControllerImpl::updateConfiguration(entity::ControllerEntity const* const controller, ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex) const noexcept
{
	controlledEntity.setCurrentConfiguration(configurationIndex);

	// Right now, simulate the entity going offline then online again - TODO: Handle multiple configurations, see https://github.com/L-Acoustics/avdecc/issues/3
	auto const e = controlledEntity.getEntity(); // Make a copy of the Entity object since it will be destroyed during onEntityOffline
	auto const entityID = e.getEntityID();
	auto* self = const_cast<ControllerImpl*>(this);
	self->onEntityOffline(controller, entityID);
	self->onEntityOnline(controller, entityID, e);
}

void ControllerImpl::updateStreamInputFormat(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) const noexcept
{
	controlledEntity.setStreamInputFormat(streamIndex, streamFormat);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamInputFormatChanged, this, &controlledEntity, streamIndex, streamFormat);
	}
}

void ControllerImpl::updateStreamOutputFormat(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat) const noexcept
{
	controlledEntity.setStreamOutputFormat(streamIndex, streamFormat);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputFormatChanged, this, &controlledEntity, streamIndex, streamFormat);
	}
}

void ControllerImpl::updateStreamInputInfo(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info) const noexcept
{
	// Update StreamInfo
	auto const previousInfo = controlledEntity.setStreamInputInfo(streamIndex, info);

	// Entity was advertised to the user, notify observers (check if info actually changed, in case it's a change in StreamingWait and the entity sent both Unsol)
	if (controlledEntity.wasAdvertised() && previousInfo != info)
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamInputInfoChanged, this, &controlledEntity, streamIndex, info);

		// Check if Running Status changed (since it's a separate Controller event)
		auto const previousRunning = ControlledEntityImpl::isStreamRunningFlag(previousInfo.streamInfoFlags);
		auto const isRunning = ControlledEntityImpl::isStreamRunningFlag(info.streamInfoFlags);

		// Running status changed, notify observers
		if (previousRunning != isRunning)
		{
			if (isRunning)
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamInputStarted, this, &controlledEntity, streamIndex);
			else
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamInputStopped, this, &controlledEntity, streamIndex);
		}
	}
}

void ControllerImpl::updateStreamOutputInfo(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info) const noexcept
{
	// Update StreamInfo
	auto const previousInfo = controlledEntity.setStreamOutputInfo(streamIndex, info);

	// Entity was advertised to the user, notify observers (check if info actually changed, in case it's a change in StreamingWait and the entity sent both Unsol)
	if (controlledEntity.wasAdvertised() && previousInfo != info)
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputInfoChanged, this, &controlledEntity, streamIndex, info);

		// Check if Running Status changed (since it's a separate Controller event)
		auto const previousRunning = ControlledEntityImpl::isStreamRunningFlag(previousInfo.streamInfoFlags);
		auto const isRunning = ControlledEntityImpl::isStreamRunningFlag(info.streamInfoFlags);

		// Running status changed, notify observers
		if (previousRunning != isRunning)
		{
			if (isRunning)
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputStarted, this, &controlledEntity, streamIndex);
			else
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputStopped, this, &controlledEntity, streamIndex);
		}
	}
}

void ControllerImpl::updateEntityName(ControlledEntityImpl& controlledEntity, entity::model::AvdeccFixedString const& entityName) const noexcept
{
	controlledEntity.setEntityName(entityName);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityNameChanged, this, &controlledEntity, entityName);
	}
}

void ControllerImpl::updateEntityGroupName(ControlledEntityImpl& controlledEntity, entity::model::AvdeccFixedString const& entityGroupName) const noexcept
{
	controlledEntity.setEntityGroupName(entityGroupName);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityGroupNameChanged, this, &controlledEntity, entityGroupName);
	}
}

void ControllerImpl::updateConfigurationName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& configurationName) const noexcept
{
	controlledEntity.setConfigurationName(configurationIndex, configurationName);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onConfigurationNameChanged, this, &controlledEntity, configurationIndex, configurationName);
	}
}

void ControllerImpl::updateStreamInputName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamInputName) const noexcept
{
	controlledEntity.setObjectName(configurationIndex, streamIndex, &model::ConfigurationDynamicTree::streamInputDynamicModels, streamInputName);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamInputNameChanged, this, &controlledEntity, configurationIndex, streamIndex, streamInputName);
	}
}

void ControllerImpl::updateStreamOutputName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamOutputName) const noexcept
{
	controlledEntity.setObjectName(configurationIndex, streamIndex, &model::ConfigurationDynamicTree::streamOutputDynamicModels, streamOutputName);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamOutputNameChanged, this, &controlledEntity, configurationIndex, streamIndex, streamOutputName);
	}
}

void ControllerImpl::updateAudioUnitSamplingRate(ControlledEntityImpl& controlledEntity, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::SamplingRate const samplingRate) const noexcept
{
	controlledEntity.setSamplingRate(audioUnitIndex, samplingRate);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onAudioUnitSamplingRateChanged, this, &controlledEntity, audioUnitIndex, samplingRate);
	}
}

void ControllerImpl::updateClockSource(ControlledEntityImpl& controlledEntity, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockSourceIndex const clockSourceIndex) const noexcept
{
	controlledEntity.setClockSource(clockDomainIndex, clockSourceIndex);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onClockSourceChanged, this, &controlledEntity, clockDomainIndex, clockSourceIndex);
	}
}

void ControllerImpl::updateStreamInputRunningStatus(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, bool const isRunning) const noexcept
{
	// Some entities do not send an Unsolicited when they start/stop streaming, so we have to manually update the StreamInfo flags

	auto const& streamDynamicModel = controlledEntity.getNodeDynamicModel(controlledEntity.getCurrentConfigurationIndex(), streamIndex, &model::ConfigurationDynamicTree::streamInputDynamicModels);

	// Make a copy of current StreamInfo and simulate a change in it
	auto newInfo = streamDynamicModel.streamInfo;
	ControlledEntityImpl::setStreamRunningFlag(newInfo.streamInfoFlags, isRunning);
	updateStreamInputInfo(controlledEntity, streamIndex, newInfo);
}

void ControllerImpl::updateStreamOutputRunningStatus(ControlledEntityImpl& controlledEntity, entity::model::StreamIndex const streamIndex, bool const isRunning) const noexcept
{
	// Some entities do not send an Unsolicited when they start/stop streaming, so we have to manually update the StreamInfo flags

	auto const& streamDynamicModel = controlledEntity.getNodeDynamicModel(controlledEntity.getCurrentConfigurationIndex(), streamIndex, &model::ConfigurationDynamicTree::streamOutputDynamicModels);

	// Make a copy of current StreamInfo and simulate a change in it
	auto newInfo = streamDynamicModel.streamInfo;
	ControlledEntityImpl::setStreamRunningFlag(newInfo.streamInfoFlags, isRunning);
	updateStreamOutputInfo(controlledEntity, streamIndex, newInfo);
}

void ControllerImpl::updateAvbInfo(ControlledEntityImpl& controlledEntity, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvbInfo const& info, bool const alsoUpdateEntity) const noexcept
{
	// Update AvbInfo
	auto const previousInfo = controlledEntity.setAvbInfo(avbInterfaceIndex, info);

	// Also update the gPTP values in ADP entity info
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

	// Entity was advertised to the user, notify observers (check if info actually changed)
	if (controlledEntity.wasAdvertised() && previousInfo != info)
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onAvbInfoChanged, this, &controlledEntity, avbInterfaceIndex, info);

		// Check if gPTP changed (since it's a separate Controller event)
		if (previousInfo.gptpGrandmasterID != info.gptpGrandmasterID || previousInfo.gptpDomainNumber != info.gptpDomainNumber)
		{
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onGptpChanged, this, &controlledEntity, avbInterfaceIndex, info.gptpGrandmasterID, info.gptpDomainNumber);
		}
	}
}

void ControllerImpl::updateStreamPortInputAudioMappingsAdded(ControlledEntityImpl& controlledEntity, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) const noexcept
{
	controlledEntity.addStreamPortInputAudioMappings(streamPortIndex, mappings);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamPortInputAudioMappingsChanged, this, &controlledEntity, streamPortIndex);
	}
}

void ControllerImpl::updateStreamPortInputAudioMappingsRemoved(ControlledEntityImpl& controlledEntity, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) const noexcept
{
	controlledEntity.removeStreamPortInputAudioMappings(streamPortIndex, mappings);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamPortInputAudioMappingsChanged, this, &controlledEntity, streamPortIndex);
	}
}

void ControllerImpl::updateStreamPortOutputAudioMappingsAdded(ControlledEntityImpl& controlledEntity, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) const noexcept
{
	controlledEntity.addStreamPortOutputAudioMappings(streamPortIndex, mappings);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamPortOutputAudioMappingsChanged, this, &controlledEntity, streamPortIndex);
	}
}

void ControllerImpl::updateStreamPortOutputAudioMappingsRemoved(ControlledEntityImpl& controlledEntity, entity::model::StreamPortIndex const streamPortIndex, entity::model::AudioMappings const& mappings) const noexcept
{
	controlledEntity.removeStreamPortOutputAudioMappings(streamPortIndex, mappings);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamPortOutputAudioMappingsChanged, this, &controlledEntity, streamPortIndex);
	}
}

/* ************************************************************ */
/* Private methods                                              */
/* ************************************************************ */
void ControllerImpl::getMilanVersion(ControlledEntityImpl* const entity) noexcept
{
	auto const entityID = entity->getEntity().getEntityID();

	// TODO: Properly get Milan version, right now, let's assume all entities are Milan compatible
	LOG_CONTROLLER_TRACE(entityID, "Getting MILAN version");
	auto const tempFunc = std::bind(&ControllerImpl::onGetMilanVersionResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
	tempFunc(_controller, entityID, entity::ControllerEntity::AemCommandStatus::Success);
}

void ControllerImpl::registerUnsol(ControlledEntityImpl* const entity) noexcept
{
	auto const entityID = entity->getEntity().getEntityID();

	// Register for unsolicited notifications
	LOG_CONTROLLER_TRACE(entityID, "registerUnsolicitedNotifications ()");
	_controller->registerUnsolicitedNotifications(entityID, std::bind(&ControllerImpl::onRegisterUnsolicitedNotificationsResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

void ControllerImpl::getStaticModel(ControlledEntityImpl* const entity) noexcept
{
	auto const entityID = entity->getEntity().getEntityID();

	// Always start with Entity Descriptor, the response from it will schedule subsequent descriptors queries
	LOG_CONTROLLER_TRACE(entityID, "readEntityDescriptor ()");
	entity->setDescriptorExpected(0, entity::model::DescriptorType::Entity, 0);
	_controller->readEntityDescriptor(entityID, std::bind(&ControllerImpl::onEntityDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
}

void ControllerImpl::getDynamicInfo(ControlledEntityImpl* const entity) noexcept
{
	auto const entityID = entity->getEntity().getEntityID();

	auto const caps = entity->getEntity().getEntityCapabilities();
	// Check if AEM is supported by this entity
	if (hasFlag(caps, entity::EntityCapabilities::AemSupported))
	{
		auto const configurationIndex = entity->getCurrentConfigurationIndex();
		auto const& configStaticTree = entity->getConfigurationStaticTree(configurationIndex);

		// Get StreamInfo and RX_STATE for each StreamInput descriptors
		{
			auto const count = configStaticTree.streamInputStaticModels.size();
			for (auto index = entity::model::StreamIndex(0); index < count; ++index)
			{
				// StreamInfo
				entity->setDynamicInfoExpected(configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamInfo, index);
				LOG_CONTROLLER_TRACE(entityID, "getStreamInputInfo (StreamIndex={})", index);
				_controller->getStreamInputInfo(entityID, index, std::bind(&ControllerImpl::onGetStreamInputInfoResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, configurationIndex));

				// RX_STATE
				entity->setDynamicInfoExpected(configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamState, index);
				LOG_CONTROLLER_TRACE(entityID, "getListenerStreamState (StreamIndex={})", index);
				_controller->getListenerStreamState({ entityID, index }, std::bind(&ControllerImpl::onGetListenerStreamStateResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, configurationIndex));

			}
		}
		// Get StreamInfo and TX_STATE for each StreamOutput descriptors
		{
			auto const count = configStaticTree.streamOutputStaticModels.size();
			for (auto index = entity::model::StreamIndex(0); index < count; ++index)
			{
				// StreamInfo
				entity->setDynamicInfoExpected(configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamInfo, index);
				LOG_CONTROLLER_TRACE(entityID, "getStreamOutputInfo (StreamIndex={})", index);
				_controller->getStreamOutputInfo(entityID, index, std::bind(&ControllerImpl::onGetStreamOutputInfoResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, configurationIndex));

				// TX_STATE
				entity->setDynamicInfoExpected(configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamState, index);
				LOG_CONTROLLER_TRACE(entityID, "getTalkerStreamState (StreamIndex={})", index);
				_controller->getTalkerStreamState({ entityID, index }, std::bind(&ControllerImpl::onGetTalkerStreamStateResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, configurationIndex));
			}
		}
		// Get AvbInfo for each AvbInterface descriptors
		{
			auto const count = configStaticTree.avbInterfaceStaticModels.size();
			for (auto index = entity::model::AvbInterfaceIndex(0); index < count; ++index)
			{
				// AvbInfo
				entity->setDynamicInfoExpected(configurationIndex, ControlledEntityImpl::DynamicInfoType::GetAvbInfo, index);
				LOG_CONTROLLER_TRACE(entityID, "getAvbInfo (AvbInterfaceIndex={})", index);
				_controller->getAvbInfo(entityID, index, std::bind(&ControllerImpl::onGetAvbInfoResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, configurationIndex));
			}
		}

		// Get AudioMappings for each StreamPortInput descriptors
		{
			auto const count = configStaticTree.streamPortInputStaticModels.size();
			for (auto index = entity::model::StreamPortIndex(0); index < count; ++index)
			{
				auto const& staticModel = entity->getNodeStaticModel(configurationIndex, index, &model::ConfigurationStaticTree::streamPortInputStaticModels);
				if (staticModel.numberOfMaps == 0)
				{
					// TODO: Clause 7.4.44.3 recommands to Lock or Acquire the entity before getting the dynamic audio map
					entity->setDynamicInfoExpected(configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamAudioMappings, index);
					LOG_CONTROLLER_TRACE(entityID, "getStreamPortInputAudioMap (StreamPortIndex={})", index);
					_controller->getStreamPortInputAudioMap(entityID, index, entity::model::MapIndex(0u), std::bind(&ControllerImpl::onGetStreamPortInputAudioMapResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7, configurationIndex));
				}
			}
		}
		// Get AudioMappings for each StreamPortOutput descriptors
		{
			auto const count = configStaticTree.streamPortOutputStaticModels.size();
			for (auto index = entity::model::StreamPortIndex(0); index < count; ++index)
			{
				auto const& staticModel = entity->getNodeStaticModel(configurationIndex, index, &model::ConfigurationStaticTree::streamPortOutputStaticModels);
				if (staticModel.numberOfMaps == 0)
				{
					// TODO: Clause 7.4.44.3 recommands to Lock or Acquire the entity before getting the dynamic audio map
					entity->setDynamicInfoExpected(configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamAudioMappings, index);
					LOG_CONTROLLER_TRACE(entityID, "getStreamPortOutputAudioMap (StreamPortIndex={})", index);
					_controller->getStreamPortOutputAudioMap(entityID, index, entity::model::MapIndex(0u), std::bind(&ControllerImpl::onGetStreamPortOutputAudioMapResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7, configurationIndex));
				}
			}
		}
	}

	// Got all expected dynamic information
	if (entity->gotAllExpectedDynamicInfo())
	{
		// Clear this enumeration step and check for next one
		entity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetDynamicInfo);
		checkEnumerationSteps(entity);
	}
}

void ControllerImpl::getDescriptorDynamicInfo(ControlledEntityImpl* const entity) noexcept
{
	auto const entityID = entity->getEntity().getEntityID();

	auto const caps = entity->getEntity().getEntityCapabilities();
	// Check if AEM is supported by this entity
	if (hasFlag(caps, entity::EntityCapabilities::AemSupported))
	{
		assert(false && "TODO: Get all information from DescriptorDynamicInfoType");
	}

	// Get all expected descriptor dynamic information
	if (entity->gotAllExpectedDescriptorDynamicInfo())
	{
		// Clear this enumeration step and check for next one
		entity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetDescriptorDynamicInfo);
		checkEnumerationSteps(entity);
	}
}

void ControllerImpl::checkEnumerationSteps(ControlledEntityImpl* const entity) noexcept
{
	auto const steps = entity->getEnumerationSteps();

	if (hasFlag(steps, ControlledEntityImpl::EnumerationSteps::GetMilanVersion))
	{
		getMilanVersion(entity);
		return;
	}
	if (hasFlag(steps, ControlledEntityImpl::EnumerationSteps::RegisterUnsol))
	{
		registerUnsol(entity);
		return;
	}
	if (hasFlag(steps, ControlledEntityImpl::EnumerationSteps::GetStaticModel))
	{
		getStaticModel(entity);
		return;
	}
	if (hasFlag(steps, ControlledEntityImpl::EnumerationSteps::GetDescriptorDynamicInfo))
	{
		getDescriptorDynamicInfo(entity);
		return;
	}
	if (hasFlag(steps, ControlledEntityImpl::EnumerationSteps::GetDynamicInfo))
	{
		getDynamicInfo(entity);
		return;
	}

	// Ready to advertise the entity
	if (!entity->wasAdvertised())
	{
		if (!entity->gotFatalEnumerationError())
		{
			entity->setAdvertised(true);
			notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityOnline, this, entity);
		}
	}
}


ControllerImpl::FailureAction ControllerImpl::getFailureAction(entity::ControllerEntity::AemCommandStatus const status) const noexcept
{
	switch (status)
	{
		// Cases we want to schedule a retry
		case entity::ControllerEntity::AemCommandStatus::LockedByOther: // Should not happen for a read operation but some devices are bugged, so retry anyway
			[[fallthrough]];
		case entity::ControllerEntity::AemCommandStatus::AcquiredByOther: // Should not happen for a read operation but some devices are bugged, so retry anyway
			[[fallthrough]];
		case entity::ControllerEntity::AemCommandStatus::NoResources:
			[[fallthrough]];
		case entity::ControllerEntity::AemCommandStatus::InProgress:
			[[fallthrough]];
		case entity::ControllerEntity::AemCommandStatus::StreamIsRunning:
			[[fallthrough]];
		case entity::ControllerEntity::AemCommandStatus::TimedOut:
		{
			return FailureAction::Retry;
		}

		// Cases we want to ignore and continue enumeration
		case entity::ControllerEntity::AemCommandStatus::NoSuchDescriptor:
			[[fallthrough]];
		case entity::ControllerEntity::AemCommandStatus::NotAuthenticated:
			[[fallthrough]];
		case entity::ControllerEntity::AemCommandStatus::AuthenticationDisabled:
			[[fallthrough]];
		case entity::ControllerEntity::AemCommandStatus::BadArguments:
			[[fallthrough]];
		case entity::ControllerEntity::AemCommandStatus::NotImplemented:
			[[fallthrough]];
		case entity::ControllerEntity::AemCommandStatus::NotSupported:
		{
			return FailureAction::Ignore;
		}

		// Cases that are errors and we want to discard this entity
		case entity::ControllerEntity::AemCommandStatus::UnknownEntity:
			[[fallthrough]];
		case entity::ControllerEntity::AemCommandStatus::EntityMisbehaving:
			[[fallthrough]];
		case entity::ControllerEntity::AemCommandStatus::NetworkError:
			[[fallthrough]];
		case entity::ControllerEntity::AemCommandStatus::ProtocolError:
			[[fallthrough]];
		case entity::ControllerEntity::AemCommandStatus::InternalError:
			[[fallthrough]];
		default:
		{
			return FailureAction::Fatal;
		}
	}
}

ControllerImpl::FailureAction ControllerImpl::getFailureAction(entity::ControllerEntity::ControlStatus const status) const noexcept
{
#pragma message("TODO: Based on status code, reschedule a query")
	switch (status)
	{
		// Cases we want to schedule a retry
		case entity::ControllerEntity::ControlStatus::TalkerDestMacFail:
			[[fallthrough]];
		case entity::ControllerEntity::ControlStatus::TalkerNoBandwidth:
			[[fallthrough]];
		case entity::ControllerEntity::ControlStatus::TalkerExclusive:
			[[fallthrough]];
		case entity::ControllerEntity::ControlStatus::ListenerTalkerTimeout:
			[[fallthrough]];
		case entity::ControllerEntity::ControlStatus::ListenerExclusive:
			[[fallthrough]];
		case entity::ControllerEntity::ControlStatus::StateUnavailable:
			[[fallthrough]];
		case entity::ControllerEntity::ControlStatus::CouldNotSendMessage:
			[[fallthrough]];
		case entity::ControllerEntity::ControlStatus::TimedOut:
		{
			return FailureAction::Retry;
		}

		// Cases we want to ignore and continue enumeration
		case entity::ControllerEntity::ControlStatus::TalkerNoStreamIndex:
			[[fallthrough]];
		case entity::ControllerEntity::ControlStatus::NotConnected:
			[[fallthrough]];
		case entity::ControllerEntity::ControlStatus::NoSuchConnection:
			[[fallthrough]];
		case entity::ControllerEntity::ControlStatus::ControllerNotAuthorized:
			[[fallthrough]];
		case entity::ControllerEntity::ControlStatus::IncompatibleRequest:
			[[fallthrough]];
		case entity::ControllerEntity::ControlStatus::NotSupported:
		{
			return FailureAction::Ignore;
		}

		// Cases that are errors and we want to discard this entity
		case entity::ControllerEntity::ControlStatus::UnknownEntity:
			[[fallthrough]];
		case entity::ControllerEntity::ControlStatus::ListenerUnknownID:
			[[fallthrough]];
		case entity::ControllerEntity::ControlStatus::TalkerUnknownID:
			[[fallthrough]];
		case entity::ControllerEntity::ControlStatus::TalkerMisbehaving:
			[[fallthrough]];
		case entity::ControllerEntity::ControlStatus::ListenerMisbehaving:
			[[fallthrough]];
		case entity::ControllerEntity::ControlStatus::NetworkError:
			[[fallthrough]];
		case entity::ControllerEntity::ControlStatus::ProtocolError:
			[[fallthrough]];
		case entity::ControllerEntity::ControlStatus::InternalError:
			[[fallthrough]];
		default:
		{
			return FailureAction::Fatal;
		}
	}
}

void ControllerImpl::rescheduleQuery(ControlledEntityImpl* const /*entity*/, entity::model::ConfigurationIndex const /*configurationIndex*/, entity::model::DescriptorType const /*descriptorType*/, entity::model::DescriptorIndex const /*descriptorIndex*/) const noexcept
{
#pragma message("TODO: Reschedule the query")
	// For easy reschedule, instead of calling single commands everywhere in the code, have a single method that gets passed entityID, descriptorType and descriptorIndex
	// The method will switch-case on that, print the log message, set the expectedDescriptor, make the call
	// So here we can just schedule a call to that method
}

void ControllerImpl::rescheduleQuery(ControlledEntityImpl* const /*entity*/, entity::model::ConfigurationIndex const /*configurationIndex*/, ControlledEntityImpl::DynamicInfoType const /*dynamicInfoType*/, entity::model::DescriptorIndex const /*descriptorIndex*/) const noexcept
{
#pragma message("TODO: Reschedule the query")
	// For easy reschedule, instead of calling single commands everywhere in the code, have a single method that gets passed entityID, descriptorType and descriptorIndex
	// The method will switch-case on that, print the log message, set the expectedDescriptor, make the call
	// So here we can just schedule a call to that method
}

void ControllerImpl::rescheduleQuery(ControlledEntityImpl* const /*entity*/, entity::model::ConfigurationIndex const /*configurationIndex*/, ControlledEntityImpl::DynamicInfoType const /*dynamicInfoType*/, entity::model::DescriptorIndex const /*descriptorIndex*/, std::uint16_t const /*connectionIndex*/) const noexcept
{
#pragma message("TODO: Reschedule the query")
	// For easy reschedule, instead of calling single commands everywhere in the code, have a single method that gets passed entityID, descriptorType and descriptorIndex
	// The method will switch-case on that, print the log message, set the expectedDescriptor, make the call
	// So here we can just schedule a call to that method
}

void ControllerImpl::rescheduleQuery(ControlledEntityImpl* const /*entity*/, entity::model::ConfigurationIndex const /*configurationIndex*/, ControlledEntityImpl::DescriptorDynamicInfoType const /*descriptorDynamicInfoType*/, entity::model::DescriptorIndex const /*descriptorIndex*/) const noexcept
{
#pragma message("TODO: Reschedule the query")
	// For easy reschedule, instead of calling single commands everywhere in the code, have a single method that gets passed entityID, descriptorType and descriptorIndex
	// The method will switch-case on that, print the log message, set the expectedDescriptor, make the call
	// So here we can just schedule a call to that method
}

/* This method handles non-success AemCommandStatus returned while getting EnumerationSteps::GetStaticModel (AEM) */
bool ControllerImpl::processFailureStatus(entity::ControllerEntity::AemCommandStatus const status, ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex) const noexcept
{
	switch (getFailureAction(status))
	{
		case FailureAction::Ignore:
			// Try to get the Descriptor Dynamic Information, because we might be missing some crucial info we can get with direct commands
			entity->addEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetDescriptorDynamicInfo);
			return true;
		case FailureAction::Retry:
			// TODO: Handle max retry mechanism before removing this #if 0 (store a retryCount in the entity, declare a MAX_RETRY_COUNT, maybe have an algorithm that waits more and more after each try)
#if 0
			if (retryCount < MAX_RETRY_COUNT)
			{
				rescheduleQuery(entity, configurationIndex, descriptorType, descriptorIndex);
			}
			else
#else
			(void)configurationIndex;
			(void)descriptorType;
			(void)descriptorIndex;
#endif
			{
				// Try to get the Descriptor Dynamic Information, because we might be missing some crucial info we can get with direct commands
				entity->addEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetDescriptorDynamicInfo);
			}
			return true;
		case FailureAction::Fatal:
			return false;
		default:
			return false;
	}
}

/* This method handles non-success AemCommandStatus returned while getting EnumerationSteps::GetDynamicInfo for AECP commands */
bool ControllerImpl::processFailureStatus(entity::ControllerEntity::AemCommandStatus const status, ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex const descriptorIndex) const noexcept
{
	switch (getFailureAction(status))
	{
		case FailureAction::Ignore:
			return true;
		case FailureAction::Retry:
			// TODO: Have a max retry count as well
			rescheduleQuery(entity, configurationIndex, dynamicInfoType, descriptorIndex);
			return true;
		case FailureAction::Fatal:
			return false;
		default:
			return false;
	}
}

/* This method handles non-success ControlStatus returned while getting EnumerationSteps::GetDynamicInfo for ACMP commands */
bool ControllerImpl::processFailureStatus(entity::ControllerEntity::ControlStatus const status, ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const connectionIndex) const noexcept
{
	switch (getFailureAction(status))
	{
		case FailureAction::Ignore:
			return true;
		case FailureAction::Retry:
			// TODO: Have a max retry count as well
			rescheduleQuery(entity, configurationIndex, dynamicInfoType, descriptorIndex, connectionIndex);
			return true;
		case FailureAction::Fatal:
			return false;
		default:
			return false;
	}
}
/* This method handles non-success AemCommandStatus returned while getting EnumerationSteps::GetDescriptorDynamicInfo (AEM) */
bool ControllerImpl::processFailureStatus(entity::ControllerEntity::AemCommandStatus const status, ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType const descriptorDynamicInfoType, entity::model::DescriptorIndex const descriptorIndex) const noexcept
{
	switch (getFailureAction(status))
	{
		case FailureAction::Ignore:
			return true;
		case FailureAction::Retry:
			// TODO: Handle max retry mechanism before removing this #if 0 (store a retryCount in the entity, declare a MAX_RETRY_COUNT, maybe have an algorithm that waits more and more after each try)
#if 0
			if (retryCount < MAX_RETRY_COUNT)
			{
				rescheduleQuery(entity, configurationIndex, descriptorDynamicInfoType, descriptorIndex);
			}
			else
#else
			(void)entity;
			(void)configurationIndex;
			(void)descriptorDynamicInfoType;
			(void)descriptorIndex;
#endif
			return true;
		case FailureAction::Fatal:
			return false;
		default:
			return false;
	}
}

void ControllerImpl::handleListenerStreamStateNotification(entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, bool const isConnected, entity::ConnectionFlags const flags, bool const changedByOther) const noexcept
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
		if (!talkerStream.entityID)
		{
			LOG_CONTROLLER_WARN(UniqueIdentifier::getNullUniqueIdentifier(), "Listener StreamState notification advertises being connected but with no Talker Identification (ListenerID={} ListenerIndex={})", listenerStream.entityID.getValue(), listenerStream.streamIndex);
			conState = model::StreamConnectionState::State::NotConnected;
		}
		else
		{
			talkerStreamIdentification = talkerStream;
		}
	}

	// Build a StreamConnectionState
	auto const state = model::StreamConnectionState{ listenerStream, talkerStreamIdentification, conState };

	// Check if Listener is online so we can update the StreamState
	{
		// Take a copy of the ControlledEntity so we don't have to keep the lock
		auto listenerEntity = getControlledEntityImpl(listenerStream.entityID);

		if (listenerEntity)
		{
			auto const previousState = listenerEntity->setStreamInputConnectionState(listenerStream.streamIndex, state);

			// Entity was advertised to the user, notify observers
			if (listenerEntity->wasAdvertised() && previousState != state)
			{
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamConnectionChanged, this, state, changedByOther);
			}
		}
	}
}

void ControllerImpl::handleTalkerStreamStateNotification(entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, bool const isConnected, entity::ConnectionFlags const flags, bool const changedByOther) const noexcept
{
	// Build Talker StreamIdentification
	auto const isFastConnect = avdecc::hasFlag(flags, entity::ConnectionFlags::FastConnect);
	auto talkerStreamIdentification{ entity::model::StreamIdentification{} };
	if (isConnected || isFastConnect)
	{
		AVDECC_ASSERT(talkerStream.entityID, "Connected or FastConnecting to an invalid TalkerID");
		talkerStreamIdentification = talkerStream;
	}

	if (isFastConnect)
	{
		handleListenerStreamStateNotification(talkerStream, listenerStream, isConnected, flags, changedByOther);
	}

	// Check if Talker is valid and online so we can update the StreamConnections
	if (talkerStream.entityID)
	{
		// Take a copy of the ControlledEntity so we don't have to keep the lock
		auto talkerEntity = getControlledEntityImpl(talkerStream.entityID);

		if (talkerEntity)
		{
			// Update our internal cache
			auto shouldNotify{ false }; // Only notify if we actually changed the connections list
			if (isConnected)
			{
				shouldNotify = talkerEntity->addStreamOutputConnection(talkerStream.streamIndex, listenerStream);
			}
			else
			{
				shouldNotify = talkerEntity->delStreamOutputConnection(talkerStream.streamIndex, listenerStream);
			}
			// Entity was advertised to the user, notify observers
			if (shouldNotify && talkerEntity->wasAdvertised())
			{
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onStreamConnectionsChanged, this, talkerEntity.get(), talkerStream.streamIndex, talkerEntity->getStreamOutputConnections(talkerStream.streamIndex));
			}
		}
	}
}

void ControllerImpl::clearTalkerStreamConnections(ControlledEntityImpl* const talkerEntity, entity::model::StreamIndex const talkerStreamIndex) const noexcept
{
	talkerEntity->clearStreamOutputConnections(talkerStreamIndex);
}

void ControllerImpl::addTalkerStreamConnection(ControlledEntityImpl* const talkerEntity, entity::model::StreamIndex const talkerStreamIndex, entity::model::StreamIdentification const& listenerStream) const noexcept
{
	// Update our internal cache
	talkerEntity->addStreamOutputConnection(talkerStreamIndex, listenerStream);
}

void ControllerImpl::delTalkerStreamConnection(ControlledEntityImpl* const talkerEntity, entity::model::StreamIndex const talkerStreamIndex, entity::model::StreamIdentification const& listenerStream) const noexcept
{
	// Update our internal cache
	talkerEntity->delStreamOutputConnection(talkerStreamIndex, listenerStream);
}

} // namespace controller
} // namespace avdecc
} // namespace la
