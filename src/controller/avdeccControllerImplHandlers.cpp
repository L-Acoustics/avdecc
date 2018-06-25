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
void ControllerImpl::onGetMilanVersionResult(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onGetMilanVersionResult: {}", entity::ControllerEntity::statusToString(status));

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (!!status)
		{
			controlledEntity->setCompatibility(ControlledEntity::Compatibility::Milan);
		}
		else
		{
			controlledEntity->setCompatibility(ControlledEntity::Compatibility::IEEE17221);
			//if (!processFailureStatus(status, controlledEntity.get(), 0, entity::model::DescriptorType::Entity, 0)) // Utiliser un autre code, ce n'est pas Entity!!
			//{
			//	controlledEntity->setGetFatalEnumerationError();
			//	notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::EntityDescriptor);
			//	return;
			//}
		}

		// Clear this enumeration step and check for next one
		controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetMilanVersion);
		checkEnumerationSteps(controlledEntity.get());
	}
}

void ControllerImpl::onRegisterUnsolicitedNotificationsResult(entity::ControllerEntity const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onRegisterUnsolicitedNotificationsResult: {}", entity::ControllerEntity::statusToString(status));

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (!!status)
		{
			// Nothing special to do right now
		}
		else
		{
			controlledEntity->setCompatibility(ControlledEntity::Compatibility::IEEE17221);
		}

		// Clear this enumeration step and check for next one
		controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::RegisterUnsol);
		checkEnumerationSteps(controlledEntity.get());
	}
}

void ControllerImpl::onEntityDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::EntityDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onEntityDescriptorStaticResult: {}", entity::ControllerEntity::statusToString(status));

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto controlledEntity = getControlledEntityImpl(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(0, entity::model::DescriptorType::Entity, 0))
		{
			if (!!status)
			{
				// Search in the AEM cache for the AEM of the active configuration
				auto const* const cachedStaticTree = EntityModelCache::getInstance().getCachedEntityStaticTree(entityID, descriptor.currentConfiguration);

				// Already cached, no need to get the remaining of EnumerationSteps::GetStaticModel, proceed with EnumerationSteps::GetDescriptorDynamicInfo
				if (cachedStaticTree && controlledEntity->setCachedEntityStaticTree(*cachedStaticTree, descriptor))
				{
					controlledEntity->addEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetDescriptorDynamicInfo);
				}
				else
				{
					controlledEntity->setEntityDescriptor(descriptor);
					for (auto index = entity::model::ConfigurationIndex(0u); index < descriptor.configurationsCount; ++index)
					{
						controlledEntity->setDescriptorExpected(0, entity::model::DescriptorType::Configuration, index);
						LOG_CONTROLLER_TRACE(entityID, "readConfigurationDescriptor (ConfigurationIndex={})", index);
						controller->readConfigurationDescriptor(entityID, index, std::bind(&ControllerImpl::onConfigurationDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
					}
				}
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), 0, entity::model::DescriptorType::Entity, 0))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::EntityDescriptor);
					return;
				}
			}
		}

		// Got all expected descriptors
		if (controlledEntity->gotAllExpectedDescriptors())
		{
			// Clear this enumeration step and check for next one
			controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetStaticModel);
			checkEnumerationSteps(controlledEntity.get());
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
		if (controlledEntity->checkAndClearExpectedDescriptor(0, entity::model::DescriptorType::Configuration, configurationIndex))
		{
			if (!!status)
			{
				controlledEntity->setConfigurationDescriptor(descriptor, configurationIndex);
				auto const isCurrentConfiguration = configurationIndex == controlledEntity->getCurrentConfigurationIndex();
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
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), 0, entity::model::DescriptorType::Configuration, configurationIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::ConfigurationDescriptor);
					return;
				}
			}
		}

		// Got all expected descriptors
		if (controlledEntity->gotAllExpectedDescriptors())
		{
			// Clear this enumeration step and check for next one
			controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetStaticModel);
			checkEnumerationSteps(controlledEntity.get());
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
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::AudioUnit, audioUnitIndex))
		{
			if (!!status)
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
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::AudioUnit, audioUnitIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AudioUnitDescriptor);
					return;
				}
			}
		}

		// Got all expected descriptors
		if (controlledEntity->gotAllExpectedDescriptors())
		{
			// Clear this enumeration step and check for next one
			controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetStaticModel);
			checkEnumerationSteps(controlledEntity.get());
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
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::StreamInput, streamIndex))
		{
			if (!!status)
			{
				controlledEntity->setStreamInputDescriptor(descriptor, configurationIndex, streamIndex);
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::StreamInput, streamIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamInputDescriptor);
					return;
				}
			}
		}

		// Got all expected descriptors
		if (controlledEntity->gotAllExpectedDescriptors())
		{
			// Clear this enumeration step and check for next one
			controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetStaticModel);
			checkEnumerationSteps(controlledEntity.get());
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
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::StreamOutput, streamIndex))
		{
			if (!!status)
			{
				controlledEntity->setStreamOutputDescriptor(descriptor, configurationIndex, streamIndex);
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::StreamOutput, streamIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamOutputDescriptor);
					return;
				}
			}
		}

		// Got all expected descriptors
		if (controlledEntity->gotAllExpectedDescriptors())
		{
			// Clear this enumeration step and check for next one
			controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetStaticModel);
			checkEnumerationSteps(controlledEntity.get());
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
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::AvbInterface, interfaceIndex))
		{
			if (!!status)
			{
				controlledEntity->setAvbInterfaceDescriptor(descriptor, configurationIndex, interfaceIndex);
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::AvbInterface, interfaceIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AvbInterfaceDescriptor);
					return;
				}
			}
		}

		// Got all expected descriptors
		if (controlledEntity->gotAllExpectedDescriptors())
		{
			// Clear this enumeration step and check for next one
			controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetStaticModel);
			checkEnumerationSteps(controlledEntity.get());
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
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::ClockSource, clockIndex))
		{
			if (!!status)
			{
				controlledEntity->setClockSourceDescriptor(descriptor, configurationIndex, clockIndex);
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::ClockSource, clockIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::ClockSourceDescriptor);
					return;
				}
			}
		}

		// Got all expected descriptors
		if (controlledEntity->gotAllExpectedDescriptors())
		{
			// Clear this enumeration step and check for next one
			controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetStaticModel);
			checkEnumerationSteps(controlledEntity.get());
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
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::MemoryObject, memoryObjectIndex))
		{
			if (!!status)
			{
				controlledEntity->setMemoryObjectDescriptor(descriptor, configurationIndex, memoryObjectIndex);
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::MemoryObject, memoryObjectIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::MemoryObjectDescriptor);
					return;
				}
			}
		}

		// Got all expected descriptors
		if (controlledEntity->gotAllExpectedDescriptors())
		{
			// Clear this enumeration step and check for next one
			controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetStaticModel);
			checkEnumerationSteps(controlledEntity.get());
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
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::Locale, localeIndex))
		{
			if (!!status)
			{
				controlledEntity->setLocaleDescriptor(descriptor, configurationIndex, localeIndex);
				auto const& configStaticTree = controlledEntity->getConfigurationStaticTree(configurationIndex);
				std::uint16_t countLocales{ 0u };
				{
					auto const localeIt = configStaticTree.staticModel.descriptorCounts.find(entity::model::DescriptorType::Locale);
					if (localeIt != configStaticTree.staticModel.descriptorCounts.end())
						countLocales = localeIt->second;
				}
				auto const allLocalesLoaded = configStaticTree.localeStaticModels.size() == countLocales;
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
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::Locale, localeIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::LocaleDescriptor);
					return;
				}
			}
		}

		// Got all expected descriptors
		if (controlledEntity->gotAllExpectedDescriptors())
		{
			// Clear this enumeration step and check for next one
			controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetStaticModel);
			checkEnumerationSteps(controlledEntity.get());
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
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::Strings, stringsIndex))
		{
			if (!!status)
			{
				controlledEntity->setStringsDescriptor(descriptor, configurationIndex, stringsIndex);
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::Strings, stringsIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StringsDescriptor);
					return;
				}
			}
		}

		// Got all expected descriptors
		if (controlledEntity->gotAllExpectedDescriptors())
		{
			// Clear this enumeration step and check for next one
			controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetStaticModel);
			checkEnumerationSteps(controlledEntity.get());
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
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::StreamPortInput, streamPortIndex))
		{
			if (!!status)
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
				// Get static audio maps
				{
					for (auto mapIndexCounter = entity::model::MapIndex(0); mapIndexCounter < descriptor.numberOfMaps; ++mapIndexCounter)
					{
						auto const mapIndex = entity::model::MapIndex(mapIndexCounter + descriptor.baseMap);
						controlledEntity->setDescriptorExpected(configurationIndex, entity::model::DescriptorType::AudioMap, mapIndex);
						LOG_CONTROLLER_TRACE(entityID, "readAudioMapDescriptor (ConfigurationIndex={} MapIndex={})", configurationIndex, mapIndex);
						controller->readAudioMapDescriptor(entityID, configurationIndex, mapIndex, std::bind(&ControllerImpl::onAudioMapDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
					}
				}
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::StreamPortInput, streamPortIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamPortInputDescriptor);
					return;
				}
			}
		}

		// Got all expected descriptors
		if (controlledEntity->gotAllExpectedDescriptors())
		{
			// Clear this enumeration step and check for next one
			controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetStaticModel);
			checkEnumerationSteps(controlledEntity.get());
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
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::StreamPortOutput, streamPortIndex))
		{
			if (!!status)
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
				// Get static audio maps
				{
					for (auto mapIndexCounter = entity::model::MapIndex(0); mapIndexCounter < descriptor.numberOfMaps; ++mapIndexCounter)
					{
						auto const mapIndex = entity::model::MapIndex(mapIndexCounter + descriptor.baseMap);
						controlledEntity->setDescriptorExpected(configurationIndex, entity::model::DescriptorType::AudioMap, mapIndex);
						LOG_CONTROLLER_TRACE(entityID, "readAudioMapDescriptor (ConfigurationIndex={} MapIndex={})", configurationIndex, mapIndex);
						controller->readAudioMapDescriptor(entityID, configurationIndex, mapIndex, std::bind(&ControllerImpl::onAudioMapDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
					}
				}
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::StreamPortOutput, streamPortIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamPortOutputDescriptor);
					return;
				}
			}
		}

		// Got all expected descriptors
		if (controlledEntity->gotAllExpectedDescriptors())
		{
			// Clear this enumeration step and check for next one
			controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetStaticModel);
			checkEnumerationSteps(controlledEntity.get());
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
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::AudioCluster, clusterIndex))
		{
			if (!!status)
			{
				controlledEntity->setAudioClusterDescriptor(descriptor, configurationIndex, clusterIndex);
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::AudioCluster, clusterIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AudioClusterDescriptor);
					return;
				}
			}
		}

		// Got all expected descriptors
		if (controlledEntity->gotAllExpectedDescriptors())
		{
			// Clear this enumeration step and check for next one
			controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetStaticModel);
			checkEnumerationSteps(controlledEntity.get());
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
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::AudioMap, mapIndex))
		{
			if (!!status)
			{
				controlledEntity->setAudioMapDescriptor(descriptor, configurationIndex, mapIndex);
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::AudioMap, mapIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AudioMapDescriptor);
					return;
				}
			}
		}

		// Got all expected descriptors
		if (controlledEntity->gotAllExpectedDescriptors())
		{
			// Clear this enumeration step and check for next one
			controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetStaticModel);
			checkEnumerationSteps(controlledEntity.get());
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
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::ClockDomain, clockDomainIndex))
		{
			if (!!status)
			{
				controlledEntity->setClockDomainDescriptor(descriptor, configurationIndex, clockDomainIndex);
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::ClockDomain, clockDomainIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::ClockDomainDescriptor);
					return;
				}
			}
		}

		// Got all expected descriptors
		if (controlledEntity->gotAllExpectedDescriptors())
		{
			// Clear this enumeration step and check for next one
			controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetStaticModel);
			checkEnumerationSteps(controlledEntity.get());
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
		if (controlledEntity->checkAndClearExpectedDynamicInfo(configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamInfo, streamIndex))
		{
			if (!!status)
			{
				controlledEntity->setStreamInputInfo(streamIndex, info);
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamInfo, streamIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::ListenerStreamInfo);
					return;
				}
			}
		}

		// Got all expected dynamic information
		if (controlledEntity->gotAllExpectedDynamicInfo())
		{
			// Clear this enumeration step and check for next one
			controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetDynamicInfo);
			checkEnumerationSteps(controlledEntity.get());
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
		if (controlledEntity->checkAndClearExpectedDynamicInfo(configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamInfo, streamIndex))
		{
			if (!!status)
			{
				controlledEntity->setStreamOutputInfo(streamIndex, info);
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamInfo, streamIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::TalkerStreamInfo);
					return;
				}
			}
		}

		// Got all expected dynamic information
		if (controlledEntity->gotAllExpectedDynamicInfo())
		{
			// Clear this enumeration step and check for next one
			controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetDynamicInfo);
			checkEnumerationSteps(controlledEntity.get());
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
		if (controlledEntity->checkAndClearExpectedDynamicInfo(configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamAudioMappings, streamPortIndex))
		{
			if (!!status)
			{
				if (mapIndex == 0)
				{
					controlledEntity->clearStreamPortInputAudioMappings(streamPortIndex);
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
				controlledEntity->addStreamPortInputAudioMappings(streamPortIndex, mappings);
				if (!isComplete)
				{
					controlledEntity->setDynamicInfoExpected(configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamAudioMappings, streamPortIndex);
					LOG_CONTROLLER_TRACE(entityID, "getStreamPortInputAudioMap (StreamPortIndex={})", streamPortIndex);
					controller->getStreamPortInputAudioMap(entityID, streamPortIndex, entity::model::MapIndex(mapIndex + 1), std::bind(&ControllerImpl::onGetStreamPortInputAudioMapResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7, configurationIndex));
					return;
				}
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamAudioMappings, streamPortIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamInputAudioMap);
					return;
				}
#if !defined(IGNORE_NEITHER_STATIC_NOR_DYNAMIC_MAPPINGS)
				// If we are requesting the dynamic mappings it's because no audio map was defined. This command should never return NotImplement nor NotSupported
				if (status == entity::ControllerEntity::AemCommandStatus::NotImplemented || status == entity::ControllerEntity::AemCommandStatus::NotSupported)
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamInputAudioMap);
					return;
				}
#endif // !IGNORE_NEITHER_STATIC_NOR_DYNAMIC_MAPPINGS
			}
		}

		// Got all expected dynamic information
		if (controlledEntity->gotAllExpectedDynamicInfo())
		{
			// Clear this enumeration step and check for next one
			controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetDynamicInfo);
			checkEnumerationSteps(controlledEntity.get());
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
		if (controlledEntity->checkAndClearExpectedDynamicInfo(configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamAudioMappings, streamPortIndex))
		{
			if (!!status)
			{
				if (mapIndex == 0)
				{
					controlledEntity->clearStreamPortOutputAudioMappings(streamPortIndex);
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
				controlledEntity->addStreamPortOutputAudioMappings(streamPortIndex, mappings);
				if (!isComplete)
				{
					controlledEntity->setDynamicInfoExpected(configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamAudioMappings, streamPortIndex);
					LOG_CONTROLLER_TRACE(entityID, "getStreamPortOutputAudioMap (StreamPortIndex={})", streamPortIndex);
					controller->getStreamPortOutputAudioMap(entityID, streamPortIndex, entity::model::MapIndex(mapIndex + 1), std::bind(&ControllerImpl::onGetStreamPortOutputAudioMapResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, std::placeholders::_7, configurationIndex));
					return;
				}
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamAudioMappings, streamPortIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamOutputAudioMap);
					return;
				}
#if !defined(IGNORE_NEITHER_STATIC_NOR_DYNAMIC_MAPPINGS)
				// If we are requesting the dynamic mappings it's because no audio map was defined. This command should never return NotImplement nor NotSupported
				if (status == entity::ControllerEntity::AemCommandStatus::NotImplemented || status == entity::ControllerEntity::AemCommandStatus::NotSupported)
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamOutputAudioMap);
					return;
				}
#endif // !IGNORE_NEITHER_STATIC_NOR_DYNAMIC_MAPPINGS
			}
		}

		// Got all expected dynamic information
		if (controlledEntity->gotAllExpectedDynamicInfo())
		{
			// Clear this enumeration step and check for next one
			controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetDynamicInfo);
			checkEnumerationSteps(controlledEntity.get());
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
		if (controlledEntity->checkAndClearExpectedDynamicInfo(configurationIndex, ControlledEntityImpl::DynamicInfoType::GetAvbInfo, avbInterfaceIndex))
		{
			if (!!status)
			{
				controlledEntity->setAvbInfo(avbInterfaceIndex, info);
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::GetAvbInfo, avbInterfaceIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AvbInfo);
					return;
				}
			}
		}

		// Got all expected dynamic information
		if (controlledEntity->gotAllExpectedDynamicInfo())
		{
			// Clear this enumeration step and check for next one
			controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetDynamicInfo);
			checkEnumerationSteps(controlledEntity.get());
		}
	}
}

/* Connection Management Protocol (ACMP) handlers */
void ControllerImpl::onConnectStreamResult(entity::ControllerEntity const* const /*controller*/, [[maybe_unused]] entity::model::StreamIdentification const& talkerStream, [[maybe_unused]] entity::model::StreamIdentification const& listenerStream, [[maybe_unused]] uint16_t const connectionCount, [[maybe_unused]] entity::ConnectionFlags const flags, [[maybe_unused]] entity::ControllerEntity::ControlStatus const status) noexcept
{
	LOG_CONTROLLER_TRACE(la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), "onConnectStreamResult (TalkerID={} TalkerIndex={} ListenerID={} ListenerIndex={} ConnectionCount={} Flags={}): {}", toHexString(talkerStream.entityID, true), talkerStream.streamIndex, toHexString(listenerStream.entityID, true), listenerStream.streamIndex, connectionCount, toHexString(to_integral(flags), true), entity::ControllerEntity::statusToString(status));
}

void ControllerImpl::onDisconnectStreamResult(entity::ControllerEntity const* const /*controller*/, [[maybe_unused]] entity::model::StreamIdentification const& talkerStream, [[maybe_unused]] entity::model::StreamIdentification const& listenerStream, [[maybe_unused]] uint16_t const connectionCount, [[maybe_unused]] entity::ConnectionFlags const flags, [[maybe_unused]] entity::ControllerEntity::ControlStatus const status) noexcept
{
	LOG_CONTROLLER_TRACE(la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), "onDisconnectStreamResult (TalkerID={} TalkerIndex={} ListenerID={} ListenerIndex={} ConnectionCount={} Flags={}): {}", toHexString(talkerStream.entityID, true), talkerStream.streamIndex, toHexString(listenerStream.entityID, true), listenerStream.streamIndex, connectionCount, toHexString(to_integral(flags), true), entity::ControllerEntity::statusToString(status));
}

void ControllerImpl::onGetTalkerStreamStateResult(entity::ControllerEntity const* const controller, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& /*listenerStream*/, [[maybe_unused]] uint16_t const connectionCount, entity::ConnectionFlags const /*flags*/, entity::ControllerEntity::ControlStatus const status, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	LOG_CONTROLLER_TRACE(la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), "onGetTalkerStreamStateResult (TalkerID={} TalkerIndex={} ConnectionCount={} ConfigurationIndex={}): {}", toHexString(talkerStream.entityID, true), talkerStream.streamIndex, connectionCount, configurationIndex, entity::ControllerEntity::statusToString(status));

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto talker = getControlledEntityImpl(talkerStream.entityID);

	if (talker)
	{
		if (talker->checkAndClearExpectedDynamicInfo(configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamState, talkerStream.streamIndex))
		{
			if (!!status)
			{
				clearTalkerStreamConnections(talker.get(), talkerStream.streamIndex);

				for (auto index = std::uint16_t(0); index < connectionCount; ++index)
				{
					talker->setDynamicInfoExpected(configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamConnection, talkerStream.streamIndex, index);
					LOG_CONTROLLER_TRACE(la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), "getTalkerStreamConnection (TalkerID={} TalkerIndex={} ConnectionIndex={})", toHexString(talkerStream.entityID, true), talkerStream.streamIndex, index);
					controller->getTalkerStreamConnection(talkerStream, index, std::bind(&ControllerImpl::onGetTalkerStreamConnectionResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, configurationIndex, index));
				}
			}
			else
			{
				if (!processFailureStatus(status, talker.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamState, talkerStream.streamIndex))
				{
					talker->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, talker.get(), QueryCommandError::TalkerStreamState);
					return;
				}
			}
		}

		// Got all expected dynamic information
		if (talker->gotAllExpectedDynamicInfo())
		{
			// Clear this enumeration step and check for next one
			talker->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetDynamicInfo);
			checkEnumerationSteps(talker.get());
		}
	}
}

void ControllerImpl::onGetListenerStreamStateResult(entity::ControllerEntity const* const /*controller*/, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	LOG_CONTROLLER_TRACE(la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), "onGetListenerStreamStateResult (ListenerID={} ListenerIndex={} ConnectionCount={} Flags={} ConfigurationIndex={}): {}", toHexString(listenerStream.entityID, true), listenerStream.streamIndex, connectionCount, toHexString(to_integral(flags), true), configurationIndex, entity::ControllerEntity::statusToString(status));

	if (!!status)
	{
		// In a GET_RX_STATE_RESPONSE message, the connectionCount is set to 1 if the stream is connected and 0 if not connected (See Marc Illouz clarification document, and hopefully someday as a corrigendum)
		handleListenerStreamStateNotification(talkerStream, listenerStream, connectionCount != 0, flags, false);
	}

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto listener = getControlledEntityImpl(listenerStream.entityID);

	if (listener)
	{
		if (listener->checkAndClearExpectedDynamicInfo(configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamState, listenerStream.streamIndex))
		{
			if (!!status)
			{
				// Nothing to do
			}
			else
			{
				if (!processFailureStatus(status, listener.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamState, listenerStream.streamIndex))
				{
					listener->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, listener.get(), QueryCommandError::ListenerStreamState);
					return;
				}
			}
		}

		// Got all expected dynamic information
		if (listener->gotAllExpectedDynamicInfo())
		{
			// Clear this enumeration step and check for next one
			listener->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetDynamicInfo);
			checkEnumerationSteps(listener.get());
		}
	}
}

void ControllerImpl::onGetTalkerStreamConnectionResult(entity::ControllerEntity const* const /*controller*/, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, [[maybe_unused]] uint16_t const connectionCount, entity::ConnectionFlags const /*flags*/, entity::ControllerEntity::ControlStatus const status, entity::model::ConfigurationIndex const configurationIndex, std::uint16_t const connectionIndex) noexcept
{
	LOG_CONTROLLER_TRACE(la::avdecc::UniqueIdentifier::getNullUniqueIdentifier(), "onGetTalkerStreamConnectionResult (TalkerID={} TalkerIndex={} ListenerID={} ListenerIndex={} ConnectionCount={} ConfigurationIndex={} ConnectionIndex={}): {}", toHexString(talkerStream.entityID, true), talkerStream.streamIndex, toHexString(listenerStream.entityID, true), listenerStream.streamIndex, connectionCount, configurationIndex, connectionIndex, entity::ControllerEntity::statusToString(status));

	// Take a copy of the ControlledEntity so we don't have to keep the lock
	auto talker = getControlledEntityImpl(talkerStream.entityID);

	if (talker)
	{
		if (talker->checkAndClearExpectedDynamicInfo(configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamConnection, talkerStream.streamIndex, connectionIndex))
		{
			if (!!status)
			{
				addTalkerStreamConnection(talker.get(), talkerStream.streamIndex, listenerStream);
			}
			else
			{
				if (!processFailureStatus(status, talker.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamConnection, talkerStream.streamIndex, connectionIndex))
				{
					talker->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, talker.get(), QueryCommandError::TalkerStreamConnection);
					return;
				}
			}
		}

		// Got all expected dynamic information
		if (talker->gotAllExpectedDynamicInfo())
		{
			// Clear this enumeration step and check for next one
			talker->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetDynamicInfo);
			checkEnumerationSteps(talker.get());
		}
	}
}

} // namespace controller
} // namespace avdecc
} // namespace la
