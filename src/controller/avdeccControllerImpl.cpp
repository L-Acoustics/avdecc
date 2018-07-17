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
#include "avdeccEntityModelCache.hpp"

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

void ControllerImpl::updateAudioUnitName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::AvdeccFixedString const& audioUnitName) const noexcept
{
	controlledEntity.setObjectName(configurationIndex, audioUnitIndex, &model::ConfigurationDynamicTree::audioUnitDynamicModels, audioUnitName);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onAudioUnitNameChanged, this, &controlledEntity, configurationIndex, audioUnitIndex, audioUnitName);
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

void ControllerImpl::updateAvbInterfaceName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvdeccFixedString const& avbInterfaceName) const noexcept
{
	controlledEntity.setObjectName(configurationIndex, avbInterfaceIndex, &model::ConfigurationDynamicTree::avbInterfaceDynamicModels, avbInterfaceName);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onAvbInterfaceNameChanged, this, &controlledEntity, configurationIndex, avbInterfaceIndex, avbInterfaceName);
	}
}

void ControllerImpl::updateClockSourceName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockSourceIndex, entity::model::AvdeccFixedString const& clockSourceName) const noexcept
{
	controlledEntity.setObjectName(configurationIndex, clockSourceIndex, &model::ConfigurationDynamicTree::clockSourceDynamicModels, clockSourceName);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onClockSourceNameChanged, this, &controlledEntity, configurationIndex, clockSourceIndex, clockSourceName);
	}
}

void ControllerImpl::updateMemoryObjectName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, entity::model::AvdeccFixedString const& memoryObjectName) const noexcept
{
	controlledEntity.setObjectName(configurationIndex, memoryObjectIndex, &model::ConfigurationDynamicTree::memoryObjectDynamicModels, memoryObjectName);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onMemoryObjectNameChanged, this, &controlledEntity, configurationIndex, memoryObjectIndex, memoryObjectName);
	}
}

void ControllerImpl::updateAudioClusterName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const audioClusterIndex, entity::model::AvdeccFixedString const& audioClusterName) const noexcept
{
	controlledEntity.setObjectName(configurationIndex, audioClusterIndex, &model::ConfigurationDynamicTree::audioClusterDynamicModels, audioClusterName);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onAudioClusterNameChanged, this, &controlledEntity, configurationIndex, audioClusterIndex, audioClusterName);
	}
}

void ControllerImpl::updateClockDomainName(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::AvdeccFixedString const& clockDomainName) const noexcept
{
	controlledEntity.setObjectName(configurationIndex, clockDomainIndex, &model::ConfigurationDynamicTree::clockDomainDynamicModels, clockDomainName);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onClockDomainNameChanged, this, &controlledEntity, configurationIndex, clockDomainIndex, clockDomainName);
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

void ControllerImpl::updateMemoryObjectLength(ControlledEntityImpl& controlledEntity, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length) const noexcept
{
	controlledEntity.setMemoryObjectLength(configurationIndex, memoryObjectIndex, length);

	// Entity was advertised to the user, notify observers
	if (controlledEntity.wasAdvertised())
	{
		notifyObserversMethod<Controller::Observer>(&Controller::Observer::onMemoryObjectLengthChanged, this, &controlledEntity, configurationIndex, memoryObjectIndex, length);
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
void ControllerImpl::chooseLocale(ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	model::LocaleNodeStaticModel const* localeNode{ nullptr };
	localeNode = entity->findLocaleNode(configurationIndex, _preferedLocale);
	if (localeNode == nullptr)
	{
#pragma message("TODO: Split _preferedLocale into language/country, then if findLocaleDescriptor fails and language is not 'en', try to find a locale for 'en'")
		localeNode = entity->findLocaleNode(configurationIndex, "en");
	}
	if (localeNode != nullptr)
	{
		auto const& configStaticTree = entity->getConfigurationStaticTree(configurationIndex);

		entity->setSelectedLocaleBaseIndex(configurationIndex, localeNode->baseStringDescriptorIndex);
		for (auto index = entity::model::StringsIndex(0); index < localeNode->numberOfStringDescriptors; ++index)
		{
			// Check if we already have the Strings descriptor
			auto const stringsIndex = static_cast<decltype(index)>(localeNode->baseStringDescriptorIndex + index);
			auto const stringsStaticModelIt = configStaticTree.stringsStaticModels.find(stringsIndex);
			if (stringsStaticModelIt != configStaticTree.stringsStaticModels.end())
			{
				// Already in cache, no need to query (just have to copy strings to Configuration for quick access)
				auto const& stringsStaticModel = stringsStaticModelIt->second;
				entity->setLocalizedStrings(configurationIndex, stringsIndex, stringsStaticModel.strings);
			}
			else
			{
				queryInformation(entity, configurationIndex, entity::model::DescriptorType::Strings, stringsIndex);
			}
		}
	}
}

void ControllerImpl::queryInformation(ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, std::chrono::milliseconds const delayQuery) noexcept
{
	// Immediately set as expected
	entity->setDescriptorExpected(configurationIndex, descriptorType, descriptorIndex);

	auto const entityID = entity->getEntity().getEntityID();
	std::function<void(entity::ControllerEntity*)> queryFunc{};

	switch (descriptorType)
	{
		case entity::model::DescriptorType::Entity:
			queryFunc = [this, entityID](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readEntityDescriptor ()");
				controller->readEntityDescriptor(entityID, std::bind(&ControllerImpl::onEntityDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
			};
			break;
		case entity::model::DescriptorType::Configuration:
			queryFunc = [this, entityID, configurationIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readConfigurationDescriptor (ConfigurationIndex={})", configurationIndex);
				controller->readConfigurationDescriptor(entityID, configurationIndex, std::bind(&ControllerImpl::onConfigurationDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
			};
			break;
		case entity::model::DescriptorType::AudioUnit:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readAudioUnitDescriptor (ConfigurationIndex={} AudioUnitIndex={})", configurationIndex, descriptorIndex);
				controller->readAudioUnitDescriptor(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onAudioUnitDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case entity::model::DescriptorType::StreamInput:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readStreamInputDescriptor (ConfigurationIndex={} StreamIndex={})", configurationIndex, descriptorIndex);
				controller->readStreamInputDescriptor(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onStreamInputDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case entity::model::DescriptorType::StreamOutput:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readStreamOutputDescriptor (ConfigurationIndex={} StreamIndex={})", configurationIndex, descriptorIndex);
				controller->readStreamOutputDescriptor(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onStreamOutputDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case entity::model::DescriptorType::AvbInterface:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readAvbInterfaceDescriptor (ConfigurationIndex={}, AvbInterfaceIndex={})", configurationIndex, descriptorIndex);
				controller->readAvbInterfaceDescriptor(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onAvbInterfaceDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case entity::model::DescriptorType::ClockSource:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readClockSourceDescriptor (ConfigurationIndex={} ClockSourceIndex={})", configurationIndex, descriptorIndex);
				controller->readClockSourceDescriptor(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onClockSourceDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case entity::model::DescriptorType::MemoryObject:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readMemoryObjectDescriptor (ConfigurationIndex={}, MemoryObjectIndex={})", configurationIndex, descriptorIndex);
				controller->readMemoryObjectDescriptor(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onMemoryObjectDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case entity::model::DescriptorType::Locale:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readLocaleDescriptor (ConfigurationIndex={} LocaleIndex={})", configurationIndex, descriptorIndex);
				controller->readLocaleDescriptor(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onLocaleDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case entity::model::DescriptorType::Strings:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readStringsDescriptor (ConfigurationIndex={} StringsIndex={})", configurationIndex, descriptorIndex);
				controller->readStringsDescriptor(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onStringsDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case entity::model::DescriptorType::StreamPortInput:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readStreamPortInputDescriptor (ConfigurationIndex={}, StreamPortIndex={})", configurationIndex, descriptorIndex);
				controller->readStreamPortInputDescriptor(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onStreamPortInputDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case entity::model::DescriptorType::StreamPortOutput:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readStreamPortOutputDescriptor (ConfigurationIndex={} StreamPortIndex={})", configurationIndex, descriptorIndex);
				controller->readStreamPortOutputDescriptor(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onStreamPortOutputDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case entity::model::DescriptorType::AudioCluster:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readAudioClusterDescriptor (ConfigurationIndex={} ClusterIndex={})", configurationIndex, descriptorIndex);
				controller->readAudioClusterDescriptor(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onAudioClusterDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case entity::model::DescriptorType::AudioMap:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readAudioMapDescriptor (ConfigurationIndex={} MapIndex={})", configurationIndex, descriptorIndex);
				controller->readAudioMapDescriptor(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onAudioMapDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case entity::model::DescriptorType::ClockDomain:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "readClockDomainDescriptor (ConfigurationIndex={}, ClockDomainIndex={})", configurationIndex, descriptorIndex);
				controller->readClockDomainDescriptor(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onClockDomainDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		default:
			AVDECC_ASSERT(false, "Unhandled DescriptorType");
			break;
	}

	// Not delayed, call now
	if (delayQuery == std::chrono::milliseconds{ 0 })
	{
		if (queryFunc)
		{
			queryFunc(_controller);
		}
	}
	else
	{
#pragma message("TODO: Use a single thread for ALL query retries (all types as well), that is destroyed when the controller is destroyed (we don't want to crash, do we?)")
		std::thread([this, delayQuery, queryFunc, entityID]
		{
			std::this_thread::sleep_for(delayQuery);
			if (_controller)
			{
				// Take a copy of the ControlledEntity so we don't have to keep the lock
				auto controlledEntity = getControlledEntityImpl(entityID);

				// Entity still online
				if (controlledEntity)
				{
					queryFunc(_controller);
				}
			}
		}).detach();
	}
}

void ControllerImpl::queryInformation(ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex const descriptorIndex, std::chrono::milliseconds const delayQuery) noexcept
{
	// Immediately set as expected
	entity->setDynamicInfoExpected(configurationIndex, dynamicInfoType, descriptorIndex);

	auto const entityID = entity->getEntity().getEntityID();
	std::function<void(entity::ControllerEntity*)> queryFunc{};

	switch (dynamicInfoType)
	{
		case ControlledEntityImpl::DynamicInfoType::InputStreamAudioMappings:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getStreamPortInputAudioMap (StreamPortIndex={})", descriptorIndex);
				controller->getStreamPortInputAudioMap(entityID, descriptorIndex, entity::model::MapIndex(0u), std::bind(&ControllerImpl::onGetStreamPortInputAudioMapResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7, configurationIndex));
			};
			break;
		case ControlledEntityImpl::DynamicInfoType::OutputStreamAudioMappings:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getStreamPortOutputAudioMap (StreamPortIndex={})", descriptorIndex);
				controller->getStreamPortOutputAudioMap(entityID, descriptorIndex, entity::model::MapIndex(0u), std::bind(&ControllerImpl::onGetStreamPortOutputAudioMapResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7, configurationIndex));
			};
			break;
		case ControlledEntityImpl::DynamicInfoType::InputStreamState:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getListenerStreamState (StreamIndex={})", descriptorIndex);
				controller->getListenerStreamState({ entityID, descriptorIndex }, std::bind(&ControllerImpl::onGetListenerStreamStateResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, configurationIndex));
			};
			break;
		case ControlledEntityImpl::DynamicInfoType::OutputStreamState:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getTalkerStreamState (StreamIndex={})", descriptorIndex);
				controller->getTalkerStreamState({ entityID, descriptorIndex }, std::bind(&ControllerImpl::onGetTalkerStreamStateResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, configurationIndex));
			};
			break;
		case ControlledEntityImpl::DynamicInfoType::OutputStreamConnection:
			AVDECC_ASSERT(false, "Another overload of this method should be called for this DynamicInfoType");
			break;
		case ControlledEntityImpl::DynamicInfoType::InputStreamInfo:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getStreamInputInfo (StreamIndex={})", descriptorIndex);
				controller->getStreamInputInfo(entityID, descriptorIndex, std::bind(&ControllerImpl::onGetStreamInputInfoResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, configurationIndex));
			};
			break;
		case ControlledEntityImpl::DynamicInfoType::OutputStreamInfo:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getStreamOutputInfo (StreamIndex={})", descriptorIndex);
				controller->getStreamOutputInfo(entityID, descriptorIndex, std::bind(&ControllerImpl::onGetStreamOutputInfoResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, configurationIndex));
			};
			break;
		case ControlledEntityImpl::DynamicInfoType::GetAvbInfo:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getAvbInfo (AvbInterfaceIndex={})", descriptorIndex);
				controller->getAvbInfo(entityID, descriptorIndex, std::bind(&ControllerImpl::onGetAvbInfoResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, configurationIndex));
			};
			break;
		case ControlledEntityImpl::DynamicInfoType::GetAsPath:
			//queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			//{
			//};
			assert(false && "Todo");
			break;
		default:
			AVDECC_ASSERT(false, "Unhandled DynamicInfoType");
			break;
	}

	// Not delayed, call now
	if (delayQuery == std::chrono::milliseconds{ 0 })
	{
		if (queryFunc)
		{
			queryFunc(_controller);
		}
	}
	else
	{
#pragma message("TODO: Use a single thread for ALL query retries (all types as well), that is destroyed when the controller is destroyed (we don't want to crash, do we?)")
		std::thread([this, delayQuery, queryFunc, entityID]
		{
			std::this_thread::sleep_for(delayQuery);
			if (_controller)
			{
				// Take a copy of the ControlledEntity so we don't have to keep the lock
				auto controlledEntity = getControlledEntityImpl(entityID);

				// Entity still online
				if (controlledEntity)
				{
					queryFunc(_controller);
				}
			}
		}).detach();
	}
}

void ControllerImpl::queryInformation(ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DynamicInfoType const dynamicInfoType, entity::model::StreamIdentification const& talkerStream, std::uint16_t const connectionIndex, std::chrono::milliseconds const delayQuery) noexcept
{
	if (!AVDECC_ASSERT_WITH_RET(dynamicInfoType == ControlledEntityImpl::DynamicInfoType::OutputStreamConnection, "Another overload of this method should be called for DynamicInfoType different than OutputStreamConnection"))
	{
		return;
	}

	// Immediately set as expected
	entity->setDynamicInfoExpected(configurationIndex, dynamicInfoType, talkerStream.streamIndex, connectionIndex);

	auto const entityID = entity->getEntity().getEntityID();
	std::function<void(entity::ControllerEntity*)> queryFunc{};

	queryFunc = [this, configurationIndex, talkerStream, connectionIndex](entity::ControllerEntity* const controller) noexcept
	{
		LOG_CONTROLLER_TRACE(UniqueIdentifier::getNullUniqueIdentifier(), "getTalkerStreamConnection (TalkerID={} TalkerIndex={} ConnectionIndex={})", toHexString(talkerStream.entityID, true), talkerStream.streamIndex, connectionIndex);
		controller->getTalkerStreamConnection(talkerStream, connectionIndex, std::bind(&ControllerImpl::onGetTalkerStreamConnectionResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, configurationIndex, connectionIndex));
	};

	// Not delayed, call now
	if (delayQuery == std::chrono::milliseconds{ 0 })
	{
		if (queryFunc)
		{
			queryFunc(_controller);
		}
	}
	else
	{
#pragma message("TODO: Use a single thread for ALL query retries (all types as well), that is destroyed when the controller is destroyed (we don't want to crash, do we?)")
		std::thread([this, delayQuery, queryFunc, entityID]
		{
			std::this_thread::sleep_for(delayQuery);
			if (_controller)
			{
				// Take a copy of the ControlledEntity so we don't have to keep the lock
				auto controlledEntity = getControlledEntityImpl(entityID);

				// Entity still online
				if (controlledEntity)
				{
					queryFunc(_controller);
				}
			}
		}).detach();
	}
}

void ControllerImpl::queryInformation(ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType const descriptorDynamicInfoType, entity::model::DescriptorIndex const descriptorIndex, std::chrono::milliseconds const delayQuery) noexcept
{
	// Immediately set as expected
	entity->setDescriptorDynamicInfoExpected(configurationIndex, descriptorDynamicInfoType, descriptorIndex);

	auto const entityID = entity->getEntity().getEntityID();
	std::function<void(entity::ControllerEntity*)> queryFunc{};

	switch (descriptorDynamicInfoType)
	{
		case ControlledEntityImpl::DescriptorDynamicInfoType::ConfigurationName:
			queryFunc = [this, entityID, configurationIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getConfigurationName (ConfigurationIndex={})", configurationIndex);
				controller->getConfigurationName(entityID, configurationIndex, std::bind(&ControllerImpl::onConfigurationNameResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
			};
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::AudioUnitName:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getAudioUnitName (ConfigurationIndex={} AudioUnitIndex={})", configurationIndex, descriptorIndex);
				controller->getAudioUnitName(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onAudioUnitNameResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::AudioUnitSamplingRate:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getAudioUnitSamplingRate (ConfigurationIndex={} AudioUnitIndex={})", configurationIndex, descriptorIndex);
				controller->getAudioUnitSamplingRate(entityID, descriptorIndex, std::bind(&ControllerImpl::onAudioUnitSamplingRateResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, configurationIndex));
			};
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::InputStreamName:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getStreamInputName (ConfigurationIndex={} StreamIndex={})", configurationIndex, descriptorIndex);
				controller->getStreamInputName(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onInputStreamNameResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::InputStreamFormat:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getStreamInputFormat (ConfigurationIndex={} StreamIndex={})", configurationIndex, descriptorIndex);
				controller->getStreamInputFormat(entityID, descriptorIndex, std::bind(&ControllerImpl::onInputStreamFormatResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, configurationIndex));
			};
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::OutputStreamName:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getStreamOutputName (ConfigurationIndex={} StreamIndex={})", configurationIndex, descriptorIndex);
				controller->getStreamOutputName(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onOutputStreamNameResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::OutputStreamFormat:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getStreamOutputFormat (ConfigurationIndex={} StreamIndex={})", configurationIndex, descriptorIndex);
				controller->getStreamOutputFormat(entityID, descriptorIndex, std::bind(&ControllerImpl::onOutputStreamFormatResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, configurationIndex));
			};
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::AvbInterfaceName:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getAvbInterfaceName (ConfigurationIndex={} AvbInterfaceIndex={})", configurationIndex, descriptorIndex);
				controller->getAvbInterfaceName(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onAvbInterfaceNameResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::ClockSourceName:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getClockSourceName (ConfigurationIndex={} ClockSourceIndex={})", configurationIndex, descriptorIndex);
				controller->getClockSourceName(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onClockSourceNameResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::MemoryObjectName:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getMemoryObjectName (ConfigurationIndex={} MemoryObjectIndex={})", configurationIndex, descriptorIndex);
				controller->getMemoryObjectName(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onMemoryObjectNameResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::MemoryObjectLength:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getMemoryObjectLength (ConfigurationIndex={} MemoryObjectIndex={})", configurationIndex, descriptorIndex);
				controller->getMemoryObjectLength(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onMemoryObjectLengthResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::AudioClusterName:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getAudioClusterName (ConfigurationIndex={} AudioClusterIndex={})", configurationIndex, descriptorIndex);
				controller->getAudioClusterName(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onAudioClusterNameResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::ClockDomainName:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getClockDomainName (ConfigurationIndex={} ClockDomainIndex={})", configurationIndex, descriptorIndex);
				controller->getClockDomainName(entityID, configurationIndex, descriptorIndex, std::bind(&ControllerImpl::onClockDomainNameResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
			};
			break;
		case ControlledEntityImpl::DescriptorDynamicInfoType::ClockDomainSourceIndex:
			queryFunc = [this, entityID, configurationIndex, descriptorIndex](entity::ControllerEntity* const controller) noexcept
			{
				LOG_CONTROLLER_TRACE(entityID, "getClockSource (ConfigurationIndex={} ClockDomainIndex={})", configurationIndex, descriptorIndex);
				controller->getClockSource(entityID, descriptorIndex, std::bind(&ControllerImpl::onClockDomainSourceIndexResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, configurationIndex));
			};
			break;
		default:
			AVDECC_ASSERT(false, "Unhandled DescriptorDynamicInfoType");
			break;
	}

	// Not delayed, call now
	if (delayQuery == std::chrono::milliseconds{ 0 })
	{
		if (queryFunc)
		{
			queryFunc(_controller);
		}
	}
	else
	{
#pragma message("TODO: Use a single thread for ALL query retries (all types as well), that is destroyed when the controller is destroyed (we don't want to crash, do we?)")
		std::thread([this, delayQuery, queryFunc, entityID]
		{
			std::this_thread::sleep_for(delayQuery);
			if (_controller)
			{
				// Take a copy of the ControlledEntity so we don't have to keep the lock
				auto controlledEntity = getControlledEntityImpl(entityID);

				// Entity still online
				if (controlledEntity)
				{
					queryFunc(_controller);
				}
			}
		}).detach();
	}
}

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
	// Always start with Entity Descriptor, the response from it will schedule subsequent descriptors queries
	queryInformation(entity, 0, entity::model::DescriptorType::Entity, 0);
}

void ControllerImpl::getDynamicInfo(ControlledEntityImpl* const entity) noexcept
{
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
				queryInformation(entity, configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamInfo, index);

				// RX_STATE
				queryInformation(entity, configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamState, index);
			}
		}
		// Get StreamInfo and TX_STATE for each StreamOutput descriptors
		{
			auto const count = configStaticTree.streamOutputStaticModels.size();
			for (auto index = entity::model::StreamIndex(0); index < count; ++index)
			{
				// StreamInfo
				queryInformation(entity, configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamInfo, index);

				// TX_STATE
				queryInformation(entity, configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamState, index);
			}
		}
		// Get AvbInfo for each AvbInterface descriptors
		{
			auto const count = configStaticTree.avbInterfaceStaticModels.size();
			for (auto index = entity::model::AvbInterfaceIndex(0); index < count; ++index)
			{
				// AvbInfo
				queryInformation(entity, configurationIndex, ControlledEntityImpl::DynamicInfoType::GetAvbInfo, index);
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
					queryInformation(entity, configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamAudioMappings, index);
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
					queryInformation(entity, configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamAudioMappings, index);
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
	auto const caps = entity->getEntity().getEntityCapabilities();
	// Check if AEM is supported by this entity
	if (hasFlag(caps, entity::EntityCapabilities::AemSupported))
	{
		auto const& entityStaticTree = entity->getEntityStaticTree();
		auto const currentConfigurationIndex = entity->getCurrentConfigurationIndex();

		// Get DynamicModel for each Configuration descriptors
		for (auto configurationIndex = entity::model::ConfigurationIndex(0u); configurationIndex < entityStaticTree.configurationStaticTrees.size(); ++configurationIndex)
		{
			auto const& configStaticTree = entity->getConfigurationStaticTree(configurationIndex);
			auto& configDynamicModel = entity->getConfigurationNodeDynamicModel(configurationIndex);

			// We can set the currentConfiguration value right now, we know it
			configDynamicModel.isActiveConfiguration = configurationIndex == currentConfigurationIndex;

			// Get ConfigurationName
			queryInformation(entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ConfigurationName, 0u);

			// And only for the current configuration, get DynamicModel for sub-descriptors
			if (configDynamicModel.isActiveConfiguration)
			{
				// Choose a locale
				chooseLocale(entity, configurationIndex);

				// Get DynamicModel for each AudioUnit descriptors
				{
					{
						auto const count = configStaticTree.audioUnitStaticModels.size();
						for (auto index = entity::model::AudioUnitIndex(0); index < count; ++index)
						{
							// Get AudioUnitName
							queryInformation(entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::AudioUnitName, index);
							// Get AudioUnitSamplingRate
							queryInformation(entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::AudioUnitSamplingRate, index);
						}
					}
				}
				// Get DynamicModel for each StreamInput descriptors
				{
					auto const count = configStaticTree.streamInputStaticModels.size();
					for (auto index = entity::model::StreamIndex(0); index < count; ++index)
					{
						// Get InputStreamName
						queryInformation(entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::InputStreamName, index);
						// Get InputStreamFormat
						queryInformation(entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::InputStreamFormat, index);
					}
				}
				// Get DynamicModel for each StreamOutput descriptors
				{
					auto const count = configStaticTree.streamOutputStaticModels.size();
					for (auto index = entity::model::StreamIndex(0); index < count; ++index)
					{
						// Get OutputStreamName
						queryInformation(entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::OutputStreamName, index);
						// Get OutputStreamFormat
						queryInformation(entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::OutputStreamFormat, index);
					}
				}
				// Get DynamicModel for each AvbInterface descriptors
				{
					auto const count = configStaticTree.avbInterfaceStaticModels.size();
					for (auto index = entity::model::AvbInterfaceIndex(0); index < count; ++index)
					{
						// Get AvbInterfaceName
						queryInformation(entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::AvbInterfaceName, index);
					}
				}
				// Get DynamicModel for each ClockSource descriptors
				{
					auto const count = configStaticTree.clockSourceStaticModels.size();
					for (auto index = entity::model::ClockSourceIndex(0); index < count; ++index)
					{
						// Get ClockSourceName
						queryInformation(entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ClockSourceName, index);
					}
				}
				// Get DynamicModel for each MemoryObject descriptors
				{
					auto const count = configStaticTree.memoryObjectStaticModels.size();
					for (auto index = entity::model::MemoryObjectIndex(0); index < count; ++index)
					{
						// Get MemoryObjectName
						queryInformation(entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::MemoryObjectName, index);
						// Get MemoryObjectLength
						queryInformation(entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::MemoryObjectLength, index);
					}
				}
				// Get DynamicModel for each AudioCluster descriptors
				{
					auto const count = configStaticTree.audioClusterStaticModels.size();
					for (auto index = entity::model::ClusterIndex(0); index < count; ++index)
					{
						// Get AudioClusterName
						queryInformation(entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::AudioClusterName, index);
					}
				}
				// Get DynamicModel for each ClockDomain descriptors
				{
					auto const count = configStaticTree.clockDomainStaticModels.size();
					for (auto index = entity::model::ClockDomainIndex(0); index < count; ++index)
					{
						// Get ClockDomainName
						queryInformation(entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ClockDomainName, index);
						// Get ClockDomainSourceIndex
						queryInformation(entity, configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ClockDomainSourceIndex, index);
					}
				}
			}
		}
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
			// Store EntityModel in the cache for later use
			EntityModelCache::getInstance().cacheEntityStaticTree(entity->getEntity().getEntityID(), entity->getCurrentConfigurationIndex(), entity->getEntityStaticTree());

			// Advertise the entity
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
		{
			return FailureAction::Ignore;
		}

		// Cases the caller should decide whether to continue enumeration or not
		case entity::ControllerEntity::AemCommandStatus::NotImplemented:
			[[fallthrough]];
		case entity::ControllerEntity::AemCommandStatus::NotSupported:
		{
			return FailureAction::NotSupported;
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
		{
			return FailureAction::Ignore;
		}

		// Cases the caller should decide whether to continue enumeration or not
		case entity::ControllerEntity::ControlStatus::NotSupported:
		{
			return FailureAction::NotSupported;
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

/* This method handles non-success AemCommandStatus returned while getting EnumerationSteps::GetStaticModel (AEM) */
bool ControllerImpl::processFailureStatus(entity::ControllerEntity::AemCommandStatus const status, ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex) noexcept
{
	switch (getFailureAction(status))
	{
		case FailureAction::Ignore:
			[[fallthrough]];
		case FailureAction::NotSupported:
			return true;
		case FailureAction::Retry:
		{
#ifdef __cpp_structured_bindings
			auto const [shouldRetry, retryTimer] = entity->getQueryDescriptorRetryTimer();
#else // !__cpp_structured_bindings
			auto const result = entity->getQueryDescriptorRetryTimer();
			auto const shouldRetry = std::get<0>(result);
			auto const retryTimer = std::get<1>(result);
#endif // __cpp_structured_bindings
			if (shouldRetry)
			{
				queryInformation(entity, configurationIndex, descriptorType, descriptorIndex, retryTimer);
			}
			return true;
		}
		case FailureAction::Fatal:
			return false;
		default:
			return false;
	}
}

/* This method handles non-success AemCommandStatus returned while getting EnumerationSteps::GetDynamicInfo for AECP commands */
bool ControllerImpl::processFailureStatus(entity::ControllerEntity::AemCommandStatus const status, ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex const descriptorIndex) noexcept
{
	switch (getFailureAction(status))
	{
		case FailureAction::Ignore:
			[[fallthrough]];
		case FailureAction::NotSupported:
			return true;
		case FailureAction::Retry:
		{
#ifdef __cpp_structured_bindings
			auto const[shouldRetry, retryTimer] = entity->getQueryDynamicInfoRetryTimer();
#else // !__cpp_structured_bindings
			auto const result = entity->getQueryDynamicInfoRetryTimer();
			auto const shouldRetry = std::get<0>(result);
			auto const retryTimer = std::get<1>(result);
#endif // __cpp_structured_bindings
			if (shouldRetry)
			{
				queryInformation(entity, configurationIndex, dynamicInfoType, descriptorIndex, retryTimer);
			}
			return true;
		}
		case FailureAction::Fatal:
			return false;
		default:
			return false;
	}
}

/* This method handles non-success ControlStatus returned while getting EnumerationSteps::GetDynamicInfo for ACMP commands */
bool ControllerImpl::processFailureStatus(entity::ControllerEntity::ControlStatus const status, ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DynamicInfoType const dynamicInfoType, entity::model::DescriptorIndex const descriptorIndex) noexcept
{
	switch (getFailureAction(status))
	{
		case FailureAction::Ignore:
			[[fallthrough]];
		case FailureAction::NotSupported:
			return true;
		case FailureAction::Retry:
		{
#ifdef __cpp_structured_bindings
			auto const[shouldRetry, retryTimer] = entity->getQueryDynamicInfoRetryTimer();
#else // !__cpp_structured_bindings
			auto const result = entity->getQueryDynamicInfoRetryTimer();
			auto const shouldRetry = std::get<0>(result);
			auto const retryTimer = std::get<1>(result);
#endif // __cpp_structured_bindings
			if (shouldRetry)
			{
				queryInformation(entity, configurationIndex, dynamicInfoType, descriptorIndex, retryTimer);
			}
			return true;
		}
		case FailureAction::Fatal:
			return false;
		default:
			return false;
	}
}

/* This method handles non-success ControlStatus returned while getting EnumerationSteps::GetDynamicInfo for ACMP commands with a connection index */
bool ControllerImpl::processFailureStatus(entity::ControllerEntity::ControlStatus const status, ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DynamicInfoType const dynamicInfoType, entity::model::StreamIdentification const& talkerStream, std::uint16_t const connectionIndex) noexcept
{
	switch (getFailureAction(status))
	{
		case FailureAction::Ignore:
			[[fallthrough]];
		case FailureAction::NotSupported:
			return true;
		case FailureAction::Retry:
		{
#ifdef __cpp_structured_bindings
			auto const[shouldRetry, retryTimer] = entity->getQueryDynamicInfoRetryTimer();
#else // !__cpp_structured_bindings
			auto const result = entity->getQueryDynamicInfoRetryTimer();
			auto const shouldRetry = std::get<0>(result);
			auto const retryTimer = std::get<1>(result);
#endif // __cpp_structured_bindings
			if (shouldRetry)
			{
				queryInformation(entity, configurationIndex, dynamicInfoType, talkerStream, connectionIndex, retryTimer);
			}
			return true;
		}
		case FailureAction::Fatal:
			return false;
		default:
			return false;
	}
}

/* This method handles non-success AemCommandStatus returned while getting EnumerationSteps::GetDescriptorDynamicInfo (AEM) */
bool ControllerImpl::processFailureStatus(entity::ControllerEntity::AemCommandStatus const status, ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType const descriptorDynamicInfoType, entity::model::DescriptorIndex const descriptorIndex) noexcept
{
	switch (getFailureAction(status))
	{
		case FailureAction::Ignore:
			return true;
		case FailureAction::Retry:
		{
#ifdef __cpp_structured_bindings
			auto const[shouldRetry, retryTimer] = entity->getQueryDescriptorDynamicInfoRetryTimer();
#else // !__cpp_structured_bindings
			auto const result = entity->getQueryDescriptorDynamicInfoRetryTimer();
			auto const shouldRetry = std::get<0>(result);
			auto const retryTimer = std::get<1>(result);
#endif // __cpp_structured_bindings
			if (shouldRetry)
			{
				queryInformation(entity, configurationIndex, descriptorDynamicInfoType, descriptorIndex, retryTimer);
				return true;
			}
			[[fallthrough]];
		}
		case FailureAction::NotSupported:
		{
			// Failed to retrieve single DescriptorDynamicInformation, retrieve the corresponding descriptor instead if possible, otherwise switch back to full StaticModel enumeration
			auto const success = fetchCorrespondingDescriptor(entity, configurationIndex, descriptorDynamicInfoType, descriptorIndex);

			// Fallback to full StaticModel enumeration
			if (!success)
			{
				entity->setIgnoreCachedEntityModel();
				entity->clearAllExpectedDescriptorDynamicInfo();
				entity->addEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetStaticModel);
				LOG_CONTROLLER_ERROR(entity->getEntity().getEntityID(), "Failed to use cached EntityModel (too many DescriptorDynamic query retries), falling back to full StaticModel enumeration");
			}
			return true;
		}
		case FailureAction::Fatal:
			return false;
		default:
			return false;
	}
}

bool ControllerImpl::fetchCorrespondingDescriptor(ControlledEntityImpl* const entity, entity::model::ConfigurationIndex const configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType const descriptorDynamicInfoType, entity::model::DescriptorIndex const descriptorIndex) noexcept
{
	auto descriptorType{ entity::model::DescriptorType::Invalid };

	switch (descriptorDynamicInfoType)
	{
			case ControlledEntityImpl::DescriptorDynamicInfoType::ConfigurationName:
				descriptorType = entity::model::DescriptorType::Configuration;
				break;
			case ControlledEntityImpl::DescriptorDynamicInfoType::AudioUnitName:
				descriptorType = entity::model::DescriptorType::AudioUnit;
				// Clear other DescriptorDynamicInfo that will be retrieved by the full Descriptor
				entity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::AudioUnitSamplingRate, descriptorIndex);
				// Clear other DescriptorDynamicInfo that will be retrieved by subtree calls
				entity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::AudioClusterName, descriptorIndex);
				break;
			case ControlledEntityImpl::DescriptorDynamicInfoType::AudioUnitSamplingRate:
				descriptorType = entity::model::DescriptorType::AudioUnit;
				// Clear other DescriptorDynamicInfo that will be retrieved by the full Descriptor
				entity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::AudioUnitName, descriptorIndex);
				// Clear other DescriptorDynamicInfo that will be retrieved by subtree calls
				entity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::AudioClusterName, descriptorIndex);
				break;
			case ControlledEntityImpl::DescriptorDynamicInfoType::InputStreamName:
				descriptorType = entity::model::DescriptorType::StreamInput;
				// Clear other DescriptorDynamicInfo that will be retrieved by the full Descriptor
				entity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::InputStreamFormat, descriptorIndex);
				break;
			case ControlledEntityImpl::DescriptorDynamicInfoType::InputStreamFormat:
				descriptorType = entity::model::DescriptorType::StreamInput;
				// Clear other DescriptorDynamicInfo that will be retrieved by the full Descriptor
				entity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::InputStreamName, descriptorIndex);
				break;
			case ControlledEntityImpl::DescriptorDynamicInfoType::OutputStreamName:
				descriptorType = entity::model::DescriptorType::StreamOutput;
				// Clear other DescriptorDynamicInfo that will be retrieved by the full Descriptor
				entity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::OutputStreamFormat, descriptorIndex);
				break;
			case ControlledEntityImpl::DescriptorDynamicInfoType::OutputStreamFormat:
				descriptorType = entity::model::DescriptorType::StreamOutput;
				// Clear other DescriptorDynamicInfo that will be retrieved by the full Descriptor
				entity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::OutputStreamName, descriptorIndex);
				break;
			case ControlledEntityImpl::DescriptorDynamicInfoType::AvbInterfaceName:
				descriptorType = entity::model::DescriptorType::AvbInterface;
				break;
			case ControlledEntityImpl::DescriptorDynamicInfoType::ClockSourceName:
				descriptorType = entity::model::DescriptorType::ClockSource;
				break;
			case ControlledEntityImpl::DescriptorDynamicInfoType::MemoryObjectName:
				descriptorType = entity::model::DescriptorType::MemoryObject;
				// Clear other DescriptorDynamicInfo that will be retrieved by the full Descriptor
				entity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::MemoryObjectLength, descriptorIndex);
				break;
			case ControlledEntityImpl::DescriptorDynamicInfoType::MemoryObjectLength:
				descriptorType = entity::model::DescriptorType::MemoryObject;
				// Clear other DescriptorDynamicInfo that will be retrieved by the full Descriptor
				entity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::MemoryObjectName, descriptorIndex);
				break;
			case ControlledEntityImpl::DescriptorDynamicInfoType::AudioClusterName:
				descriptorType = entity::model::DescriptorType::AudioCluster;
				break;
			case ControlledEntityImpl::DescriptorDynamicInfoType::ClockDomainName:
				descriptorType = entity::model::DescriptorType::ClockDomain;
				// Clear other DescriptorDynamicInfo that will be retrieved by the full Descriptor
				entity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ClockDomainSourceIndex, descriptorIndex);
				break;
			case ControlledEntityImpl::DescriptorDynamicInfoType::ClockDomainSourceIndex:
				descriptorType = entity::model::DescriptorType::ClockDomain;
				// Clear other DescriptorDynamicInfo that will be retrieved by the full Descriptor
				entity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ClockDomainName, descriptorIndex);
				break;
			default:
				AVDECC_ASSERT(false, "Unhandled DescriptorDynamicInfoType");
				break;
	}

	if (!!descriptorType)
	{
		LOG_CONTROLLER_DEBUG(entity->getEntity().getEntityID(), "Failed to get DescriptorDynamicInfo, trying to get the corresponding Descriptor");
		queryInformation(entity, configurationIndex, descriptorType, descriptorIndex);
		return true;
	}

	return false;
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
