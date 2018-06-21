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
* @file avdeccControllerImplHandlers.cpp
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
/* Result handlers                                              */
/* ************************************************************ */
/* Enumeration and Control Protocol (AECP) handlers */
void ControllerImpl::onEntityDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::EntityDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onEntityDescriptorResult: {}", entity::ControllerEntity::statusToString(status));

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(0, entity::model::DescriptorType::Entity, 0) && !controlledEntity->gotEnumerationError())
		{
			if (!!status)
			{
				try
				{
					controlledEntity->setEntityDescriptor(descriptor);
					for (auto index = entity::model::ConfigurationIndex(0u); index < descriptor.configurationsCount; ++index)
					{
						controlledEntity->setDescriptorExpected(0, entity::model::DescriptorType::Configuration, index);
						LOG_CONTROLLER_TRACE(entityID, "readConfigurationDescriptor (ConfigurationIndex={})", index);
						controller->readConfigurationDescriptor(entityID, index, std::bind(&ControllerImpl::onConfigurationDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
					}
				}
				catch (ControlledEntity::Exception const&)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::EntityDescriptor);
				}
			}
			else
			{
				if (!checkRescheduleQuery(status, controlledEntity.get(), 0, entity::model::DescriptorType::Entity, 0))
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::EntityDescriptor);
				}
			}
		}
	}
}

void ControllerImpl::onConfigurationDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ConfigurationDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onConfigurationDescriptorResult (ConfigurationIndex={}): {}", configurationIndex, entity::ControllerEntity::statusToString(status));

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(0, entity::model::DescriptorType::Configuration, configurationIndex) && !controlledEntity->gotEnumerationError())
		{
			if (!!status)
			{
				try
				{
					controlledEntity->setConfigurationDescriptor(descriptor, configurationIndex);
					auto const& entityDescriptor = controlledEntity->getEntityDescriptor();
					auto const isCurrentConfiguration = configurationIndex == entityDescriptor.dynamicModel.currentConfiguration;
					// Only get full descriptors for active configuration
					if (isCurrentConfiguration)
					{
						// Get Locales
						{
							auto countIt = descriptor.descriptorCounts.find(entity::model::DescriptorType::Locale);
							if (countIt != descriptor.descriptorCounts.end() && countIt->second != 0)
							{
								auto count = countIt->second;
								for (auto index = entity::model::LocaleIndex(0); index < count; ++index)
								{
									controlledEntity->setDescriptorExpected(configurationIndex, entity::model::DescriptorType::Locale, index);
									LOG_CONTROLLER_TRACE(entityID, "readLocaleDescriptor (ConfigurationIndex={} LocaleIndex={})", configurationIndex, index);
									controller->readLocaleDescriptor(entityID, configurationIndex, index, std::bind(&ControllerImpl::onLocaleDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
								}
							}
						}
						// Get audio units
						{
							auto countIt = descriptor.descriptorCounts.find(entity::model::DescriptorType::AudioUnit);
							if (countIt != descriptor.descriptorCounts.end() && countIt->second != 0)
							{
								auto count = countIt->second;
								for (auto index = entity::model::AudioUnitIndex(0); index < count; ++index)
								{
									controlledEntity->setDescriptorExpected(configurationIndex, entity::model::DescriptorType::AudioUnit, index);
									LOG_CONTROLLER_TRACE(entityID, "readAudioUnitDescriptor (ConfigurationIndex={} AudioUnitIndex={})", configurationIndex, index);
									controller->readAudioUnitDescriptor(entityID, configurationIndex, index, std::bind(&ControllerImpl::onAudioUnitDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
								}
							}
						}
						// Get input streams
						{
							auto countIt = descriptor.descriptorCounts.find(entity::model::DescriptorType::StreamInput);
							if (countIt != descriptor.descriptorCounts.end() && countIt->second != 0)
							{
								auto count = countIt->second;
								for (auto index = entity::model::StreamIndex(0); index < count; ++index)
								{
									// Get RX_STATE
									controlledEntity->setDynamicInfoExpected(configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamState, index);
									LOG_CONTROLLER_TRACE(entityID, "getListenerStreamState (StreamIndex={})", index);
									controller->getListenerStreamState({ entityID, index }, std::bind(&ControllerImpl::onGetListenerStreamStateResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, configurationIndex));

									// Get Stream Info
									controlledEntity->setDynamicInfoExpected(configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamInfo, index);
									LOG_CONTROLLER_TRACE(entityID, "getStreamInputInfo (StreamIndex={})", index);
									controller->getStreamInputInfo(entityID, index, std::bind(&ControllerImpl::onGetStreamInputInfoResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, configurationIndex));
									
									// Get Stream Descriptor
									controlledEntity->setDescriptorExpected(configurationIndex, entity::model::DescriptorType::StreamInput, index);
									LOG_CONTROLLER_TRACE(entityID, "readStreamInputDescriptor (ConfigurationIndex={} StreamIndex={})", configurationIndex, index);
									controller->readStreamInputDescriptor(entityID, configurationIndex, index, std::bind(&ControllerImpl::onStreamInputDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
								}
							}
						}
						// Get output streams
						{
							auto countIt = descriptor.descriptorCounts.find(entity::model::DescriptorType::StreamOutput);
							if (countIt != descriptor.descriptorCounts.end() && countIt->second != 0)
							{
								auto count = countIt->second;
								for (auto index = entity::model::StreamIndex(0); index < count; ++index)
								{
									// Get TX_STATE
									controlledEntity->setDynamicInfoExpected(configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamState, index);
									LOG_CONTROLLER_TRACE(entityID, "getTalkerStreamState (StreamIndex={})", index);
									controller->getTalkerStreamState({ entityID, index }, std::bind(&ControllerImpl::onGetTalkerStreamStateResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, configurationIndex));

									// Get Stream Info
									controlledEntity->setDynamicInfoExpected(configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamInfo, index);
									LOG_CONTROLLER_TRACE(entityID, "getStreamOutputInfo (StreamIndex={})", index);
									controller->getStreamOutputInfo(entityID, index, std::bind(&ControllerImpl::onGetStreamOutputInfoResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, configurationIndex));

									// Get Stream Descriptor
									controlledEntity->setDescriptorExpected(configurationIndex, entity::model::DescriptorType::StreamOutput, index);
									LOG_CONTROLLER_TRACE(entityID, "readStreamOutputDescriptor (ConfigurationIndex={} StreamIndex={})", configurationIndex, index);
									controller->readStreamOutputDescriptor(entityID, configurationIndex, index, std::bind(&ControllerImpl::onStreamOutputDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
								}
							}
						}
						// Get avb interfaces
						{
							auto countIt = descriptor.descriptorCounts.find(entity::model::DescriptorType::AvbInterface);
							if (countIt != descriptor.descriptorCounts.end() && countIt->second != 0)
							{
								auto count = countIt->second;
								for (auto index = entity::model::AvbInterfaceIndex(0); index < count; ++index)
								{
									// Get AVB Info
									controlledEntity->setDynamicInfoExpected(configurationIndex, ControlledEntityImpl::DynamicInfoType::GetAvbInfo, index);
									LOG_CONTROLLER_TRACE(entityID, "getAvbInfo (AvbInterfaceIndex={})", index);
									controller->getAvbInfo(entityID, index, std::bind(&ControllerImpl::onGetAvbInfoResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, configurationIndex));

									// Get AVBInterface Descriptor
									controlledEntity->setDescriptorExpected(configurationIndex, entity::model::DescriptorType::AvbInterface, index);
									LOG_CONTROLLER_TRACE(entityID, "readAvbInterfaceDescriptor (ConfigurationIndex={}, AvbInterfaceIndex={})", configurationIndex, index);
									controller->readAvbInterfaceDescriptor(entityID, configurationIndex, index, std::bind(&ControllerImpl::onAvbInterfaceDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
								}
							}
						}
						// Get clock sources
						{
							auto countIt = descriptor.descriptorCounts.find(entity::model::DescriptorType::ClockSource);
							if (countIt != descriptor.descriptorCounts.end() && countIt->second != 0)
							{
								auto count = countIt->second;
								for (auto index = entity::model::ClockSourceIndex(0); index < count; ++index)
								{
									controlledEntity->setDescriptorExpected(configurationIndex, entity::model::DescriptorType::ClockSource, index);
									LOG_CONTROLLER_TRACE(entityID, "readClockSourceDescriptor (ConfigurationIndex={} ClockSourceIndex={})", configurationIndex, index);
									controller->readClockSourceDescriptor(entityID, configurationIndex, index, std::bind(&ControllerImpl::onClockSourceDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
								}
							}
						}
						// Get memory objects
						{
							auto countIt = descriptor.descriptorCounts.find(entity::model::DescriptorType::MemoryObject);
							if (countIt != descriptor.descriptorCounts.end() && countIt->second != 0)
							{
								auto count = countIt->second;
								for (auto index = entity::model::MemoryObjectIndex(0); index < count; ++index)
								{
									// Get Memory Object Descriptor
									controlledEntity->setDescriptorExpected(configurationIndex, entity::model::DescriptorType::MemoryObject, index);
									LOG_CONTROLLER_TRACE(entityID, "readMemoryObjectDescriptor (ConfigurationIndex={}, MemoryObjectIndex={})", configurationIndex, index);
									controller->readMemoryObjectDescriptor(entityID, configurationIndex, index, std::bind(&ControllerImpl::onMemoryObjectDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
								}
							}
						}
						// Get clock domains
						{
							auto countIt = descriptor.descriptorCounts.find(entity::model::DescriptorType::ClockDomain);
							if (countIt != descriptor.descriptorCounts.end() && countIt->second != 0)
							{
								auto count = countIt->second;
								for (auto index = entity::model::ClockDomainIndex(0); index < count; ++index)
								{
									controlledEntity->setDescriptorExpected(configurationIndex, entity::model::DescriptorType::ClockDomain, index);
									LOG_CONTROLLER_TRACE(entityID, "readClockDomainDescriptor (ConfigurationIndex={}, ClockDomainIndex={})", configurationIndex, index);
									controller->readClockDomainDescriptor(entityID, configurationIndex, index, std::bind(&ControllerImpl::onClockDomainDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
								}
							}
						}
					}
					// For non-active configurations, just get locales (and strings)
					else
					{
						// Get Locales
						{
							auto countIt = descriptor.descriptorCounts.find(entity::model::DescriptorType::Locale);
							if (countIt != descriptor.descriptorCounts.end() && countIt->second != 0)
							{
								auto count = countIt->second;
								for (auto index = entity::model::LocaleIndex(0); index < count; ++index)
								{
									controlledEntity->setDescriptorExpected(configurationIndex, entity::model::DescriptorType::Locale, index);
									LOG_CONTROLLER_TRACE(entityID, "readLocaleDescriptor (ConfigurationIndex={} LocaleIndex={})", configurationIndex, index);
									controller->readLocaleDescriptor(entityID, configurationIndex, index, std::bind(&ControllerImpl::onLocaleDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
								}
							}
						}
					}
				}
				catch (ControlledEntity::Exception const&)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::ConfigurationDescriptor);
				}
			}
			else
			{
				if (!checkRescheduleQuery(status, controlledEntity.get(), 0, entity::model::DescriptorType::Configuration, configurationIndex))
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::ConfigurationDescriptor);
				}
			}
		}
	}
}

void ControllerImpl::onAudioUnitDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::AudioUnitDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onAudioUnitDescriptorResult (ConfigurationIndex={} AudioUnitIndex={}): {}", configurationIndex, audioUnitIndex, entity::ControllerEntity::statusToString(status));

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::AudioUnit, audioUnitIndex) && !controlledEntity->gotEnumerationError())
		{
			if (!!status)
			{
				try
				{
					controlledEntity->setAudioUnitDescriptor(descriptor, configurationIndex, audioUnitIndex);

					// Get stream port input
					{
						if (descriptor.numberOfStreamInputPorts != 0)
						{
							for (auto index = entity::model::StreamPortIndex(0); index < descriptor.numberOfStreamInputPorts; ++index)
							{
								controlledEntity->setDescriptorExpected(configurationIndex, entity::model::DescriptorType::StreamPortInput, index);
								LOG_CONTROLLER_TRACE(entityID, "readStreamPortInputDescriptor (ConfigurationIndex={}, StreamPortIndex={})", configurationIndex, index);
								controller->readStreamPortInputDescriptor(entityID, configurationIndex, index, std::bind(&ControllerImpl::onStreamPortInputDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
							}
						}
					}
					// Get stream port output
					{
						if (descriptor.numberOfStreamOutputPorts != 0)
						{
							for (auto index = entity::model::StreamPortIndex(0); index < descriptor.numberOfStreamOutputPorts; ++index)
							{
								controlledEntity->setDescriptorExpected(configurationIndex, entity::model::DescriptorType::StreamPortOutput, index);
								LOG_CONTROLLER_TRACE(entityID, "readStreamPortOutputDescriptor (ConfigurationIndex={} StreamPortIndex={})", configurationIndex, index);
								controller->readStreamPortOutputDescriptor(entityID, configurationIndex, index, std::bind(&ControllerImpl::onStreamPortOutputDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
							}
						}
					}
					checkAdvertiseEntity(controlledEntity.get());
				}
				catch (ControlledEntity::Exception const&)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AudioUnitDescriptor);
				}
			}
			else
			{
				if (!checkRescheduleQuery(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::AudioUnit, audioUnitIndex))
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AudioUnitDescriptor);
				}
			}
		}
	}
}

void ControllerImpl::onStreamInputDescriptorResult(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::StreamDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onStreamInputDescriptorResult (ConfigurationIndex={} StreamIndex={}): {}", configurationIndex, streamIndex, entity::ControllerEntity::statusToString(status));

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::StreamInput, streamIndex) && !controlledEntity->gotEnumerationError())
		{
			if (!!status)
			{
				try
				{
					controlledEntity->setStreamInputDescriptor(descriptor, configurationIndex, streamIndex);
					checkAdvertiseEntity(controlledEntity.get());
				}
				catch (ControlledEntity::Exception const&)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamInputDescriptor);
				}
			}
			else
			{
				if (!checkRescheduleQuery(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::StreamInput, streamIndex))
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamInputDescriptor);
				}
			}
		}
	}
}

void ControllerImpl::onStreamOutputDescriptorResult(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::StreamDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onStreamOutputDescriptorResult (ConfigurationIndex={} StreamIndex={}): {}", configurationIndex, streamIndex, entity::ControllerEntity::statusToString(status));

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::StreamOutput, streamIndex) && !controlledEntity->gotEnumerationError())
		{
			if (!!status)
			{
				try
				{
					controlledEntity->setStreamOutputDescriptor(descriptor, configurationIndex, streamIndex);
					checkAdvertiseEntity(controlledEntity.get());
				}
				catch (ControlledEntity::Exception const&)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamOutputDescriptor);
				}
			}
			else
			{
				if (!checkRescheduleQuery(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::StreamOutput, streamIndex))
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamOutputDescriptor);
				}
			}
		}
	}
}

void ControllerImpl::onAvbInterfaceDescriptorResult(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const interfaceIndex, entity::model::AvbInterfaceDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onAvbInterfaceDescriptorResult (ConfigurationIndex={} InterfaceIndex={}): {}", configurationIndex, interfaceIndex, entity::ControllerEntity::statusToString(status));

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::AvbInterface, interfaceIndex) && !controlledEntity->gotEnumerationError())
		{
			if (!!status)
			{
				try
				{
					controlledEntity->setAvbInterfaceDescriptor(descriptor, configurationIndex, interfaceIndex);
					checkAdvertiseEntity(controlledEntity.get());
				}
				catch (ControlledEntity::Exception const&)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AvbInterfaceDescriptor);
				}
			}
			else
			{
				if (!checkRescheduleQuery(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::AvbInterface, interfaceIndex))
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AvbInterfaceDescriptor);
				}
			}
		}
	}
}

void ControllerImpl::onClockSourceDescriptorResult(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockIndex, entity::model::ClockSourceDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onClockSourceDescriptorResult (ConfigurationIndex={} ClockIndex={}): {}", configurationIndex, clockIndex, entity::ControllerEntity::statusToString(status));

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::ClockSource, clockIndex) && !controlledEntity->gotEnumerationError())
		{
			if (!!status)
			{
				try
				{
					controlledEntity->setClockSourceDescriptor(descriptor, configurationIndex, clockIndex);
					checkAdvertiseEntity(controlledEntity.get());
				}
				catch (ControlledEntity::Exception const&)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::ClockSourceDescriptor);
				}
			}
			else
			{
				if (!checkRescheduleQuery(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::ClockSource, clockIndex))
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::ClockSourceDescriptor);
				}
			}
		}
	}
}

void ControllerImpl::onMemoryObjectDescriptorResult(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, entity::model::MemoryObjectDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onMemoryObjectDescriptorResult (ConfigurationIndex={} ClockDomainIndex={}): {}", configurationIndex, memoryObjectIndex, entity::ControllerEntity::statusToString(status));

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::MemoryObject, memoryObjectIndex) && !controlledEntity->gotEnumerationError())
		{
			if (!!status)
			{
				try
				{
					controlledEntity->setMemoryObjectDescriptor(descriptor, configurationIndex, memoryObjectIndex);
					checkAdvertiseEntity(controlledEntity.get());
				}
				catch (ControlledEntity::Exception const&)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::MemoryObjectDescriptor);
				}
			}
			else
			{
				if (!checkRescheduleQuery(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::MemoryObject, memoryObjectIndex))
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::MemoryObjectDescriptor);
				}
			}
		}
	}
}

void ControllerImpl::onLocaleDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::LocaleIndex const localeIndex, entity::model::LocaleDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onLocaleDescriptorResult (ConfigurationIndex={} LocaleIndex={}): {}", configurationIndex, localeIndex, entity::ControllerEntity::statusToString(status));

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::Locale, localeIndex) && !controlledEntity->gotEnumerationError())
		{
			if (!!status)
			{
				try
				{
					controlledEntity->setLocaleDescriptor(descriptor, configurationIndex, localeIndex);
					auto const& configDescriptor = controlledEntity->getConfigurationDescriptor(configurationIndex);
					std::uint16_t countLocales{ 0u };
					{
						auto const localeIt = configDescriptor.staticModel.descriptorCounts.find(entity::model::DescriptorType::Locale);
						if (localeIt != configDescriptor.staticModel.descriptorCounts.end())
							countLocales = localeIt->second;
					}
					auto const allLocalesLoaded = configDescriptor.localeDescriptors.size() == countLocales;
					// We got all locales, now load strings for the desired locale
					if (allLocalesLoaded)
					{
						model::LocaleNodeStaticModel const* localeNode{ nullptr };
						localeNode = controlledEntity->findLocaleNode(configurationIndex, _preferedLocale);
						if (localeNode == nullptr)
						{
#pragma message("TODO: Split _preferedLocale into language/country, then if findLocaleDescriptor fails and language is not 'en', try to find a locale for 'en'")
							localeNode = controlledEntity->findLocaleNode(configurationIndex, "en");
						}
						if (localeNode != nullptr)
						{
							controlledEntity->setSelectedLocaleBaseIndex(configurationIndex, localeNode->baseStringDescriptorIndex);
							for (auto index = entity::model::StringsIndex(0); index < localeNode->numberOfStringDescriptors; ++index)
							{
								controlledEntity->setDescriptorExpected(configurationIndex, entity::model::DescriptorType::Strings, index);
								LOG_CONTROLLER_TRACE(entityID, "readStringsDescriptor (ConfigurationIndex={} StringsIndex={})", configurationIndex, localeNode->baseStringDescriptorIndex + index);
								controller->readStringsDescriptor(entityID, configurationIndex, localeNode->baseStringDescriptorIndex + index, std::bind(&ControllerImpl::onStringsDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
							}
						}
					}
					checkAdvertiseEntity(controlledEntity.get());
				}
				catch (ControlledEntity::Exception const&)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::LocaleDescriptor);
				}
			}
			else
			{
				if (!checkRescheduleQuery(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::Locale, localeIndex))
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::LocaleDescriptor);
				}
			}
		}
	}
}

void ControllerImpl::onStringsDescriptorResult(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StringsIndex const stringsIndex, entity::model::StringsDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onStringsDescriptorResult (ConfigurationIndex={} StringsIndex={}): {}", configurationIndex, stringsIndex, entity::ControllerEntity::statusToString(status));

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::Strings, stringsIndex) && !controlledEntity->gotEnumerationError())
		{
			if (!!status)
			{
				try
				{
					controlledEntity->setStringsDescriptor(descriptor, configurationIndex, stringsIndex);
					checkAdvertiseEntity(controlledEntity.get());
				}
				catch (ControlledEntity::Exception const&)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StringsDescriptor);
				}
			}
			else
			{
				if (!checkRescheduleQuery(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::Strings, stringsIndex))
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StringsDescriptor);
				}
			}
		}
	}
}

void ControllerImpl::onStreamPortInputDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex, entity::model::StreamPortDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onStreamPortInputDescriptorResult (ConfigurationIndex={} StreamPortIndex={}): {}", configurationIndex, streamPortIndex, entity::ControllerEntity::statusToString(status));

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::StreamPortInput, streamPortIndex) && !controlledEntity->gotEnumerationError())
		{
			if (!!status)
			{
				try
				{
					controlledEntity->setStreamPortInputDescriptor(descriptor, configurationIndex, streamPortIndex);
					// Get audio clusters
					{
						if (descriptor.numberOfClusters != 0)
						{
							for (auto clusterIndexCounter = entity::model::ClusterIndex(0); clusterIndexCounter < descriptor.numberOfClusters; ++clusterIndexCounter)
							{
								auto const clusterIndex = entity::model::ClusterIndex(clusterIndexCounter + descriptor.baseCluster);
								controlledEntity->setDescriptorExpected(configurationIndex, entity::model::DescriptorType::AudioCluster, clusterIndex);
								LOG_CONTROLLER_TRACE(entityID, "readAudioClusterDescriptor (ConfigurationIndex={} ClusterIndex={})", configurationIndex, clusterIndex);
								controller->readAudioClusterDescriptor(entityID, configurationIndex, clusterIndex, std::bind(&ControllerImpl::onAudioClusterDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
							}
						}
					}
					// Get audio maps (static or dynamic)
					{
						if (descriptor.numberOfMaps != 0)
						{
							for (auto mapIndexCounter = entity::model::MapIndex(0); mapIndexCounter < descriptor.numberOfMaps; ++mapIndexCounter)
							{
								auto const mapIndex = entity::model::MapIndex(mapIndexCounter + descriptor.baseMap);
								controlledEntity->setDescriptorExpected(configurationIndex, entity::model::DescriptorType::AudioMap, mapIndex);
								LOG_CONTROLLER_TRACE(entityID, "readAudioMapDescriptor (ConfigurationIndex={} MapIndex={})", configurationIndex, mapIndex);
								controller->readAudioMapDescriptor(entityID, configurationIndex, mapIndex, std::bind(&ControllerImpl::onAudioMapDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
							}
						}
						else
						{
							// TODO: Clause 7.4.44.3 recommands to Lock or Acquire the entity before getting the dynamic audio map
							controlledEntity->setDynamicInfoExpected(configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamAudioMappings, streamPortIndex);
							LOG_CONTROLLER_TRACE(entityID, "getStreamPortInputAudioMap (StreamPortIndex={})", streamPortIndex);
							controller->getStreamPortInputAudioMap(entityID, streamPortIndex, entity::model::MapIndex(0u), std::bind(&ControllerImpl::onGetStreamPortInputAudioMapResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7, configurationIndex));
						}
					}
					checkAdvertiseEntity(controlledEntity.get());
				}
				catch (ControlledEntity::Exception const&)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamPortInputDescriptor);
				}
			}
			else
			{
				if (!checkRescheduleQuery(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::StreamPortInput, streamPortIndex))
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamPortInputDescriptor);
				}
			}
		}
	}
}

void ControllerImpl::onStreamPortOutputDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex, entity::model::StreamPortDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onStreamPortOutputDescriptorResult (ConfigurationIndex={} StreamPortIndex={}): {}", configurationIndex, streamPortIndex, entity::ControllerEntity::statusToString(status));

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::StreamPortOutput, streamPortIndex) && !controlledEntity->gotEnumerationError())
		{
			if (!!status)
			{
				try
				{
					controlledEntity->setStreamPortOutputDescriptor(descriptor, configurationIndex, streamPortIndex);
					// Get audio clusters
					{
						if (descriptor.numberOfClusters != 0)
						{
							for (auto clusterIndexCounter = entity::model::ClusterIndex(0); clusterIndexCounter < descriptor.numberOfClusters; ++clusterIndexCounter)
							{
								auto const clusterIndex = entity::model::ClusterIndex(clusterIndexCounter + descriptor.baseCluster);
								controlledEntity->setDescriptorExpected(configurationIndex, entity::model::DescriptorType::AudioCluster, clusterIndex);
								LOG_CONTROLLER_TRACE(entityID, "readAudioClusterDescriptor (ConfigurationIndex={} ClusterIndex={})", configurationIndex, clusterIndex);
								controller->readAudioClusterDescriptor(entityID, configurationIndex, clusterIndex, std::bind(&ControllerImpl::onAudioClusterDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
							}
						}
					}
					// Get audio maps (static or dynamic)
					{
						if (descriptor.numberOfMaps != 0)
						{
							for (auto mapIndexCounter = entity::model::MapIndex(0); mapIndexCounter < descriptor.numberOfMaps; ++mapIndexCounter)
							{
								auto const mapIndex = entity::model::MapIndex(mapIndexCounter + descriptor.baseMap);
								controlledEntity->setDescriptorExpected(configurationIndex, entity::model::DescriptorType::AudioMap, mapIndex);
								LOG_CONTROLLER_TRACE(entityID, "readAudioMapDescriptor (ConfigurationIndex={} MapIndex={})", configurationIndex, mapIndex);
								controller->readAudioMapDescriptor(entityID, configurationIndex, mapIndex, std::bind(&ControllerImpl::onAudioMapDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
							}
						}
						else
						{
							// TODO: Clause 7.4.44.3 recommands to Lock or Acquire the entity before getting the dynamic audio map
							controlledEntity->setDynamicInfoExpected(configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamAudioMappings, streamPortIndex);
							LOG_CONTROLLER_TRACE(entityID, "getStreamPortOutputAudioMap (StreamPortIndex={})", streamPortIndex);
							controller->getStreamPortOutputAudioMap(entityID, streamPortIndex, entity::model::MapIndex(0u), std::bind(&ControllerImpl::onGetStreamPortOutputAudioMapResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7, configurationIndex));
						}
					}
					checkAdvertiseEntity(controlledEntity.get());
				}
				catch (ControlledEntity::Exception const&)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamPortOutputDescriptor);
				}
			}
			else
			{
				if (!checkRescheduleQuery(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::StreamPortOutput, streamPortIndex))
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamPortOutputDescriptor);
				}
			}
		}
	}
}

void ControllerImpl::onAudioClusterDescriptorResult(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const clusterIndex, entity::model::AudioClusterDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onAudioClusterDescriptorResult (ConfigurationIndex={} ClusterIndex={}): {}", configurationIndex, clusterIndex, entity::ControllerEntity::statusToString(status));

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::AudioCluster, clusterIndex) && !controlledEntity->gotEnumerationError())
		{
			if (!!status)
			{
				try
				{
					controlledEntity->setAudioClusterDescriptor(descriptor, configurationIndex, clusterIndex);
					checkAdvertiseEntity(controlledEntity.get());
				}
				catch (ControlledEntity::Exception const&)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AudioClusterDescriptor);
				}
			}
			else
			{
				if (!checkRescheduleQuery(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::AudioCluster, clusterIndex))
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AudioClusterDescriptor);
				}
			}
		}
	}
}

void ControllerImpl::onAudioMapDescriptorResult(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const mapIndex, entity::model::AudioMapDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onAudioMapDescriptorResult (ConfigurationIndex={} MapIndex={}): {}", configurationIndex, mapIndex, entity::ControllerEntity::statusToString(status));

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::AudioMap, mapIndex) && !controlledEntity->gotEnumerationError())
		{
			if (!!status)
			{
				try
				{
					controlledEntity->setAudioMapDescriptor(descriptor, configurationIndex, mapIndex);
					checkAdvertiseEntity(controlledEntity.get());
				}
				catch (ControlledEntity::Exception const&)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AudioMapDescriptor);
				}
			}
			else
			{
				if (!checkRescheduleQuery(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::AudioMap, mapIndex))
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AudioMapDescriptor);
				}
			}
		}
	}
}

void ControllerImpl::onClockDomainDescriptorResult(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockDomainDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onClockDomainDescriptorResult (ConfigurationIndex={} ClockDomainIndex={}): {}", configurationIndex, clockDomainIndex, entity::ControllerEntity::statusToString(status));

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::ClockDomain, clockDomainIndex) && !controlledEntity->gotEnumerationError())
		{
			if (!!status)
			{
				try
				{
					controlledEntity->setClockDomainDescriptor(descriptor, configurationIndex, clockDomainIndex);
					checkAdvertiseEntity(controlledEntity.get());
				}
				catch (ControlledEntity::Exception const&)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::ClockDomainDescriptor);
				}
			}
			else
			{
				if (!checkRescheduleQuery(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::ClockDomain, clockDomainIndex))
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::ClockDomainDescriptor);
				}
			}
		}
	}
}

void ControllerImpl::onGetStreamInputInfoResult(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onGetStreamInputInfoResult (StreamIndex={}): {}", streamIndex, entity::ControllerEntity::statusToString(status));

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDynamicInfo(configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamInfo, streamIndex) && !controlledEntity->gotEnumerationError())
		{
			if (!!status)
			{
				try
				{
					controlledEntity->setInputStreamInfo(configurationIndex, streamIndex, info);
				}
				catch (ControlledEntity::Exception const&)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::ListenerStreamInfo);
				}
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamInfo, streamIndex))
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::ListenerStreamInfo);
				}
			}
			checkAdvertiseEntity(controlledEntity.get());
		}
	}
}

void ControllerImpl::onGetStreamOutputInfoResult(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onGetStreamOutputInfoResult (StreamIndex={}): {}", streamIndex, entity::ControllerEntity::statusToString(status));

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDynamicInfo(configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamInfo, streamIndex) && !controlledEntity->gotEnumerationError())
		{
			if (!!status)
			{
				try
				{
					controlledEntity->setOutputStreamInfo(configurationIndex, streamIndex, info);
				}
				catch (ControlledEntity::Exception const&)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::TalkerStreamInfo);
				}
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamInfo, streamIndex))
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::TalkerStreamInfo);
				}
			}
			checkAdvertiseEntity(controlledEntity.get());
		}
	}
}

void ControllerImpl::onGetStreamPortInputAudioMapResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamPortIndex const streamPortIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, entity::model::AudioMappings const& mappings, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onGetStreamPortInputAudioMapResult (StreamPortIndex={} NumberMaps={} MapIndex={}): {}", streamPortIndex, numberOfMaps, mapIndex, entity::ControllerEntity::statusToString(status));

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDynamicInfo(configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamAudioMappings, streamPortIndex) && !controlledEntity->gotEnumerationError())
		{
			if (!!status)
			{
				try
				{
					if (mapIndex == 0)
					{
						controlledEntity->clearPortInputStreamAudioMappings(configurationIndex, streamPortIndex);
					}
					bool isComplete{ true };
					if (numberOfMaps != 0)
					{
						isComplete = mapIndex == (numberOfMaps - 1);
					}
					else if (!mappings.empty())
					{
						LOG_CONTROLLER_WARN(entityID, "onGetStreamPortInputAudioMapResult returned 0 as numberOfMaps but mappings array is not empty");
					}
					controlledEntity->addPortInputStreamAudioMappings(configurationIndex, streamPortIndex, mappings);
					if (!isComplete)
					{
						controlledEntity->setDynamicInfoExpected(configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamAudioMappings, streamPortIndex);
						LOG_CONTROLLER_TRACE(entityID, "getStreamPortInputAudioMap (StreamPortIndex={})", streamPortIndex);
						controller->getStreamPortInputAudioMap(entityID, streamPortIndex, entity::model::MapIndex(mapIndex + 1), std::bind(&ControllerImpl::onGetStreamPortInputAudioMapResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7, configurationIndex));
						return;
					}
				}
				catch (ControlledEntity::Exception const&)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamInputAudioMap);
				}
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamAudioMappings, streamPortIndex))
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamInputAudioMap);
				}
#if !defined(IGNORE_NEITHER_STATIC_NOR_DYNAMIC_MAPPINGS)
				// If we are requesting the dynamic mappings it's because no audio map was defined. This command should never return NotImplement nor NotSupported
				if (status == entity::ControllerEntity::AemCommandStatus::NotImplemented || status == entity::ControllerEntity::AemCommandStatus::NotSupported)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamInputAudioMap);
				}
#endif // !IGNORE_NEITHER_STATIC_NOR_DYNAMIC_MAPPINGS
			}
			checkAdvertiseEntity(controlledEntity.get());
		}
	}
}

void ControllerImpl::onGetStreamPortOutputAudioMapResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamPortIndex const streamPortIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, entity::model::AudioMappings const& mappings, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onGetStreamPortOutputAudioMapResult (StreamPortIndex={} NumberMaps={} MapIndex={}): {}", streamPortIndex, numberOfMaps, mapIndex, entity::ControllerEntity::statusToString(status));

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDynamicInfo(configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamAudioMappings, streamPortIndex) && !controlledEntity->gotEnumerationError())
		{
			if (!!status)
			{
				try
				{
					if (mapIndex == 0)
					{
						controlledEntity->clearPortOutputStreamAudioMappings(configurationIndex, streamPortIndex);
					}
					bool isComplete{ true };
					if (numberOfMaps != 0)
					{
						isComplete = mapIndex == (numberOfMaps - 1);
					}
					else if (!mappings.empty())
					{
						LOG_CONTROLLER_WARN(entityID, "onGetStreamPortOutputAudioMapResult returned 0 as numberOfMaps but mappings array is not empty");
					}
					controlledEntity->addPortOutputStreamAudioMappings(configurationIndex, streamPortIndex, mappings);
					if (!isComplete)
					{
						controlledEntity->setDynamicInfoExpected(configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamAudioMappings, streamPortIndex);
						LOG_CONTROLLER_TRACE(entityID, "getStreamPortOutputAudioMap (StreamPortIndex={})", streamPortIndex);
						controller->getStreamPortOutputAudioMap(entityID, streamPortIndex, entity::model::MapIndex(mapIndex + 1), std::bind(&ControllerImpl::onGetStreamPortOutputAudioMapResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7, configurationIndex));
						return;
					}
				}
				catch (ControlledEntity::Exception const&)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamOutputAudioMap);
				}
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamAudioMappings, streamPortIndex))
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamOutputAudioMap);
				}
#if !defined(IGNORE_NEITHER_STATIC_NOR_DYNAMIC_MAPPINGS)
				// If we are requesting the dynamic mappings it's because no audio map was defined. This command should never return NotImplement nor NotSupported
				if (status == entity::ControllerEntity::AemCommandStatus::NotImplemented || status == entity::ControllerEntity::AemCommandStatus::NotSupported)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamOutputAudioMap);
				}
#endif // !IGNORE_NEITHER_STATIC_NOR_DYNAMIC_MAPPINGS
			}
			checkAdvertiseEntity(controlledEntity.get());
		}
	}
}

void ControllerImpl::onGetAvbInfoResult(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvbInfo const& info, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onGetAvbInfoResult (AvbInterfaceIndex={}): {}", avbInterfaceIndex, entity::ControllerEntity::statusToString(status));

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDynamicInfo(configurationIndex, ControlledEntityImpl::DynamicInfoType::GetAvbInfo, avbInterfaceIndex) && !controlledEntity->gotEnumerationError())
		{
			if (!!status)
			{
				try
				{
					controlledEntity->setAvbInfo(configurationIndex, avbInterfaceIndex, info);
				}
				catch (ControlledEntity::Exception const&)
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AvbInfo);
				}
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::GetAvbInfo, avbInterfaceIndex))
				{
					controlledEntity->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AvbInfo);
				}
			}
			checkAdvertiseEntity(controlledEntity.get());
		}
	}
}

/* Connection Management Protocol (ACMP) handlers */
void ControllerImpl::onConnectStreamResult(entity::ControllerEntity const* const /*controller*/, [[maybe_unused]] entity::model::StreamIdentification const& talkerStream, [[maybe_unused]] entity::model::StreamIdentification const& listenerStream, [[maybe_unused]] uint16_t const connectionCount, [[maybe_unused]] entity::ConnectionFlags const flags, [[maybe_unused]] entity::ControllerEntity::ControlStatus const status) noexcept
{
	LOG_CONTROLLER_TRACE(la::avdecc::getNullIdentifier(), "onConnectStreamResult (TalkerID={} TalkerIndex={} ListenerID={} ListenerIndex={} ConnectionCount={} Flags={}): {}", toHexString(talkerStream.entityID, true), talkerStream.streamIndex, toHexString(listenerStream.entityID, true), listenerStream.streamIndex, connectionCount, toHexString(to_integral(flags), true), entity::ControllerEntity::statusToString(status));
}

void ControllerImpl::onDisconnectStreamResult(entity::ControllerEntity const* const /*controller*/, [[maybe_unused]] entity::model::StreamIdentification const& talkerStream, [[maybe_unused]] entity::model::StreamIdentification const& listenerStream, [[maybe_unused]] uint16_t const connectionCount, [[maybe_unused]] entity::ConnectionFlags const flags, [[maybe_unused]] entity::ControllerEntity::ControlStatus const status) noexcept
{
	LOG_CONTROLLER_TRACE(la::avdecc::getNullIdentifier(), "onDisconnectStreamResult (TalkerID={} TalkerIndex={} ListenerID={} ListenerIndex={} ConnectionCount={} Flags={}): {}", toHexString(talkerStream.entityID, true), talkerStream.streamIndex, toHexString(listenerStream.entityID, true), listenerStream.streamIndex, connectionCount, toHexString(to_integral(flags), true), entity::ControllerEntity::statusToString(status));
}

void ControllerImpl::onGetTalkerStreamStateResult(entity::ControllerEntity const* const controller, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& /*listenerStream*/, [[maybe_unused]] uint16_t const connectionCount, entity::ConnectionFlags const /*flags*/, entity::ControllerEntity::ControlStatus const status, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	LOG_CONTROLLER_TRACE(la::avdecc::getNullIdentifier(), "onGetTalkerStreamStateResult (TalkerID={} TalkerIndex={} ConnectionCount={} ConfigurationIndex={}): {}", toHexString(talkerStream.entityID, true), talkerStream.streamIndex, connectionCount, configurationIndex, entity::ControllerEntity::statusToString(status));

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto talker = getControlledEntityImpl(talkerStream.entityID);

	if (talker)
	{
		if (talker->checkAndClearExpectedDynamicInfo(configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamState, talkerStream.streamIndex) && !talker->gotEnumerationError())
		{
			if (!!status)
			{
				clearTalkerStreamConnections(talker.get(), talkerStream.streamIndex);

				for (auto index = std::uint16_t(0); index < connectionCount; ++index)
				{
					talker->setDynamicInfoExpected(configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamConnection, talkerStream.streamIndex, index);
					LOG_CONTROLLER_TRACE(la::avdecc::getNullIdentifier(), "getTalkerStreamConnection (TalkerID={} TalkerIndex={} ConnectionIndex={})", toHexString(talkerStream.entityID, true), talkerStream.streamIndex, index);
					controller->getTalkerStreamConnection(talkerStream, index, std::bind(&ControllerImpl::onGetTalkerStreamConnectionResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, configurationIndex, index));
				}

				checkAdvertiseEntity(talker.get());
			}
			else
			{
				if (!checkRescheduleQuery(status, talker.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamState, talkerStream.streamIndex))
				{
					talker->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, talker.get(), QueryCommandError::TalkerStreamState);
				}
			}
		}
	}
}

void ControllerImpl::onGetListenerStreamStateResult(entity::ControllerEntity const* const /*controller*/, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	LOG_CONTROLLER_TRACE(la::avdecc::getNullIdentifier(), "onGetListenerStreamStateResult (ListenerID={} ListenerIndex={} ConnectionCount={} Flags={} ConfigurationIndex={}): {}", toHexString(listenerStream.entityID, true), listenerStream.streamIndex, connectionCount, toHexString(to_integral(flags), true), configurationIndex, entity::ControllerEntity::statusToString(status));

	if (!!status)
	{
		// In a GET_RX_STATE_RESPONSE message, the connectionCount is set to 1 if the stream is connected and 0 if not connected (See Marc Illouz clarification document, and hopefully someday as a corrigendum)
		handleListenerStreamStateNotification(talkerStream, listenerStream, connectionCount != 0, flags, false);
	}

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto listener = getControlledEntityImpl(listenerStream.entityID);

	if (listener)
	{
		if (listener->checkAndClearExpectedDynamicInfo(configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamState, listenerStream.streamIndex) && !listener->gotEnumerationError())
		{
			if (!!status)
			{
				checkAdvertiseEntity(listener.get());
			}
			else
			{
				if (!checkRescheduleQuery(status, listener.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamState, listenerStream.streamIndex))
				{
					listener->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, listener.get(), QueryCommandError::ListenerStreamState);
				}
			}
		}
	}
}

void ControllerImpl::onGetTalkerStreamConnectionResult(entity::ControllerEntity const* const /*controller*/, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, [[maybe_unused]] uint16_t const connectionCount, entity::ConnectionFlags const /*flags*/, entity::ControllerEntity::ControlStatus const status, entity::model::ConfigurationIndex const configurationIndex, std::uint16_t const connectionIndex) noexcept
{
	LOG_CONTROLLER_TRACE(la::avdecc::getNullIdentifier(), "onGetTalkerStreamConnectionResult (TalkerID={} TalkerIndex={} ListenerID={} ListenerIndex={} ConnectionCount={} ConfigurationIndex={} ConnectionIndex={}): {}", toHexString(talkerStream.entityID, true), talkerStream.streamIndex, toHexString(listenerStream.entityID, true), listenerStream.streamIndex, connectionCount, configurationIndex, connectionIndex, entity::ControllerEntity::statusToString(status));

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto talker = getControlledEntityImpl(talkerStream.entityID);

	if (talker)
	{
		if (talker->checkAndClearExpectedDynamicInfo(configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamConnection, talkerStream.streamIndex, connectionIndex) && !talker->gotEnumerationError())
		{
			if (!!status)
			{
				addTalkerStreamConnection(talker.get(), talkerStream.streamIndex, listenerStream);
				checkAdvertiseEntity(talker.get());
			}
			else
			{
				if (!checkRescheduleQuery(status, talker.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamConnection, talkerStream.streamIndex, connectionIndex))
				{
					talker->setEnumerationError(true);
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, talker.get(), QueryCommandError::TalkerStreamConnection);
				}
			}
		}
	}
}

} // namespace controller
} // namespace avdecc
} // namespace la
