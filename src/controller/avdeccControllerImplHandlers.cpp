/*
* Copyright (C) 2016-2019, L-Acoustics and its contributors

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
* @file avdeccControllerImplHandlers.cpp
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
static auto const s_MilanMandatoryStreamInputCounters = entity::StreamInputCounterValidFlags{ entity::StreamInputCounterValidFlag::MediaLocked, entity::StreamInputCounterValidFlag::MediaUnlocked, entity::StreamInputCounterValidFlag::StreamInterrupted, entity::StreamInputCounterValidFlag::SeqNumMismatch, entity::StreamInputCounterValidFlag::MediaReset, entity::StreamInputCounterValidFlag::TimestampUncertain, entity::StreamInputCounterValidFlag::UnsupportedFormat, entity::StreamInputCounterValidFlag::LateTimestamp, entity::StreamInputCounterValidFlag::EarlyTimestamp, entity::StreamInputCounterValidFlag::FramesRx }; // Milan Clause 6.8.10
static auto const s_MilanMandatoryStreamOutputCounters = entity::StreamOutputCounterValidFlags{ entity::StreamOutputCounterValidFlag::StreamStart, entity::StreamOutputCounterValidFlag::StreamStop, entity::StreamOutputCounterValidFlag::MediaReset, entity::StreamOutputCounterValidFlag::TimestampUncertain, entity::StreamOutputCounterValidFlag::FramesTx }; // Milan Clause 6.7.7
static auto const s_MilanMandatoryAvbInterfaceCounters = entity::AvbInterfaceCounterValidFlags{ entity::AvbInterfaceCounterValidFlag::LinkUp, entity::AvbInterfaceCounterValidFlag::LinkDown, entity::AvbInterfaceCounterValidFlag::GptpGmChanged }; // Milan Clause 6.6.3
static auto const s_MilanMandatoryClockDomainCounters = entity::ClockDomainCounterValidFlags{ entity::ClockDomainCounterValidFlag::Locked, entity::ClockDomainCounterValidFlag::Unlocked }; // Milan Clause 6.11.2

/* ************************************************************ */
/* Result handlers                                              */
/* ************************************************************ */
/* Enumeration and Control Protocol (AECP) handlers */
void ControllerImpl::onGetMilanInfoResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::MvuCommandStatus const status, entity::model::MilanInfo const& info) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onGetMilanInfoResult (ProtocolVersion={} FeaturesFlags={} CertificationVersion={}): {}", info.protocolVersion, info.featuresFlags.getValue(), info.certificationVersion, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedMilanInfo(ControlledEntityImpl::MilanInfoType::MilanInfo))
		{
			if (!!status)
			{
				// Flag the entity as "Milan compatible"
				addCompatibilityFlag(*controlledEntity, ControlledEntity::CompatibilityFlag::Milan);
				controlledEntity->setMilanInfo(info);
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), ControlledEntityImpl::MilanInfoType::MilanInfo))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::GetMilanInfo);
					return;
				}
			}
		}

		// Got all expected milan information
		if (controlledEntity->gotAllExpectedMilanInfo())
		{
			// Clear this enumeration step and check for next one
			controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetMilanInfo);
			checkEnumerationSteps(controlledEntity.get());
		}
	}
}

void ControllerImpl::onRegisterUnsolicitedNotificationsResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onRegisterUnsolicitedNotificationsResult: {}", entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (!!status)
		{
			controlledEntity->setSubscribedToUnsolicitedNotifications(true);
		}
		else
		{
			controlledEntity->setSubscribedToUnsolicitedNotifications(false);
#pragma message("TODO: Handle errors here, we might have a timeout and want to retry, like all other commands. If the error is critical, we have to flag the entity somehow (maybe a CompatibilityFlag saying we cannot track Unsol")
			removeCompatibilityFlag(*controlledEntity, ControlledEntity::CompatibilityFlag::Milan); // Right now, just clear the Milan flag, but it would be better to add a "processFailedStatus" like all other commands
		}

		// Clear this enumeration step and check for next one
		controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::RegisterUnsol);
		checkEnumerationSteps(controlledEntity.get());
	}
}

void ControllerImpl::onEntityDescriptorResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::EntityDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onEntityDescriptorStaticResult: {}", entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(0, entity::model::DescriptorType::Entity, 0))
		{
			if (!!status)
			{
				// Search in the AEM cache for the AEM of the active configuration (if not ignored)
				auto const* const cachedStaticTree = controlledEntity->shouldIgnoreCachedEntityModel() ? nullptr : EntityModelCache::getInstance().getCachedEntityStaticTree(entityID, descriptor.currentConfiguration);

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
						queryInformation(controlledEntity.get(), index, entity::model::DescriptorType::Configuration, 0u);
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

void ControllerImpl::onConfigurationDescriptorResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ConfigurationDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onConfigurationDescriptorResult (ConfigurationIndex={}): {}", configurationIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::Configuration, 0u))
		{
			if (!!status)
			{
				controlledEntity->setConfigurationDescriptor(descriptor, configurationIndex);
				auto const isCurrentConfiguration = configurationIndex == controlledEntity->getCurrentConfigurationIndex();
				// Only get full descriptors for active configuration
				if (isCurrentConfiguration)
				{
					// Get Locales as soon as possible
					{
						auto countIt = descriptor.descriptorCounts.find(entity::model::DescriptorType::Locale);
						if (countIt != descriptor.descriptorCounts.end() && countIt->second != 0)
						{
							auto count = countIt->second;
							for (auto index = entity::model::LocaleIndex(0); index < count; ++index)
							{
								queryInformation(controlledEntity.get(), configurationIndex, entity::model::DescriptorType::Locale, index);
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
								queryInformation(controlledEntity.get(), configurationIndex, entity::model::DescriptorType::AudioUnit, index);
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
								queryInformation(controlledEntity.get(), configurationIndex, entity::model::DescriptorType::StreamInput, index);
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
								queryInformation(controlledEntity.get(), configurationIndex, entity::model::DescriptorType::StreamOutput, index);
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
								queryInformation(controlledEntity.get(), configurationIndex, entity::model::DescriptorType::AvbInterface, index);
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
								queryInformation(controlledEntity.get(), configurationIndex, entity::model::DescriptorType::ClockSource, index);
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
								queryInformation(controlledEntity.get(), configurationIndex, entity::model::DescriptorType::MemoryObject, index);
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
								queryInformation(controlledEntity.get(), configurationIndex, entity::model::DescriptorType::ClockDomain, index);
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
								queryInformation(controlledEntity.get(), configurationIndex, entity::model::DescriptorType::Locale, index);
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

void ControllerImpl::onAudioUnitDescriptorResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::AudioUnitDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onAudioUnitDescriptorResult (ConfigurationIndex={} AudioUnitIndex={}): {}", configurationIndex, audioUnitIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

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
							queryInformation(controlledEntity.get(), configurationIndex, entity::model::DescriptorType::StreamPortInput, index);
						}
					}
				}
				// Get stream port output
				{
					if (descriptor.numberOfStreamOutputPorts != 0)
					{
						for (auto index = entity::model::StreamPortIndex(0); index < descriptor.numberOfStreamOutputPorts; ++index)
						{
							queryInformation(controlledEntity.get(), configurationIndex, entity::model::DescriptorType::StreamPortOutput, index);
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

void ControllerImpl::onStreamInputDescriptorResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::StreamDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onStreamInputDescriptorResult (ConfigurationIndex={} StreamIndex={}): {}", configurationIndex, streamIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

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

void ControllerImpl::onStreamOutputDescriptorResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::StreamDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onStreamOutputDescriptorResult (ConfigurationIndex={} StreamIndex={}): {}", configurationIndex, streamIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

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

void ControllerImpl::onAvbInterfaceDescriptorResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const interfaceIndex, entity::model::AvbInterfaceDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onAvbInterfaceDescriptorResult (ConfigurationIndex={} InterfaceIndex={}): {}", configurationIndex, interfaceIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::AvbInterface, interfaceIndex))
		{
			if (!!status)
			{
				controlledEntity->setAvbInterfaceDescriptor(descriptor, configurationIndex, interfaceIndex);
				// Initialize the InterfaceLinkStatus to unknown (until we get the counters)
				updateAvbInterfaceLinkStatus(*controlledEntity, interfaceIndex, ControlledEntity::InterfaceLinkStatus::Unknown);
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

void ControllerImpl::onClockSourceDescriptorResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockIndex, entity::model::ClockSourceDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onClockSourceDescriptorResult (ConfigurationIndex={} ClockIndex={}): {}", configurationIndex, clockIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

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

void ControllerImpl::onMemoryObjectDescriptorResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, entity::model::MemoryObjectDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onMemoryObjectDescriptorResult (ConfigurationIndex={} ClockDomainIndex={}): {}", configurationIndex, memoryObjectIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

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

void ControllerImpl::onLocaleDescriptorResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::LocaleIndex const localeIndex, entity::model::LocaleDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onLocaleDescriptorResult (ConfigurationIndex={} LocaleIndex={}): {}", configurationIndex, localeIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

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
					chooseLocale(controlledEntity.get(), configurationIndex);
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

void ControllerImpl::onStringsDescriptorResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StringsIndex const stringsIndex, entity::model::StringsDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onStringsDescriptorResult (ConfigurationIndex={} StringsIndex={}): {}", configurationIndex, stringsIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

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

void ControllerImpl::onStreamPortInputDescriptorResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex, entity::model::StreamPortDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onStreamPortInputDescriptorResult (ConfigurationIndex={} StreamPortIndex={}): {}", configurationIndex, streamPortIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

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
							queryInformation(controlledEntity.get(), configurationIndex, entity::model::DescriptorType::AudioCluster, clusterIndex);
						}
					}
				}
				// Get static audio maps
				{
					for (auto mapIndexCounter = entity::model::MapIndex(0); mapIndexCounter < descriptor.numberOfMaps; ++mapIndexCounter)
					{
						auto const mapIndex = entity::model::MapIndex(mapIndexCounter + descriptor.baseMap);
						queryInformation(controlledEntity.get(), configurationIndex, entity::model::DescriptorType::AudioMap, mapIndex);
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

void ControllerImpl::onStreamPortOutputDescriptorResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamPortIndex const streamPortIndex, entity::model::StreamPortDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onStreamPortOutputDescriptorResult (ConfigurationIndex={} StreamPortIndex={}): {}", configurationIndex, streamPortIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

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
							queryInformation(controlledEntity.get(), configurationIndex, entity::model::DescriptorType::AudioCluster, clusterIndex);
						}
					}
				}
				// Get static audio maps
				{
					for (auto mapIndexCounter = entity::model::MapIndex(0); mapIndexCounter < descriptor.numberOfMaps; ++mapIndexCounter)
					{
						auto const mapIndex = entity::model::MapIndex(mapIndexCounter + descriptor.baseMap);
						queryInformation(controlledEntity.get(), configurationIndex, entity::model::DescriptorType::AudioMap, mapIndex);
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

void ControllerImpl::onAudioClusterDescriptorResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const clusterIndex, entity::model::AudioClusterDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onAudioClusterDescriptorResult (ConfigurationIndex={} ClusterIndex={}): {}", configurationIndex, clusterIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

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

void ControllerImpl::onAudioMapDescriptorResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::MapIndex const mapIndex, entity::model::AudioMapDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onAudioMapDescriptorResult (ConfigurationIndex={} MapIndex={}): {}", configurationIndex, mapIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

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

void ControllerImpl::onClockDomainDescriptorResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockDomainDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onClockDomainDescriptorResult (ConfigurationIndex={} ClockDomainIndex={}): {}", configurationIndex, clockDomainIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

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

void ControllerImpl::onGetStreamInputInfoResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onGetStreamInputInfoResult (StreamIndex={}): {}", streamIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDynamicInfo(configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamInfo, streamIndex))
		{
			if (!!status)
			{
				// Use the "update**" method, there are many things to do
				updateStreamInputInfo(*controlledEntity, streamIndex, info, true, true);
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

void ControllerImpl::onGetStreamOutputInfoResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::model::StreamInfo const& info, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onGetStreamOutputInfoResult (StreamIndex={}): {}", streamIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDynamicInfo(configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamInfo, streamIndex))
		{
			if (!!status)
			{
				// Use the "update**" method, there are many things to do
				updateStreamOutputInfo(*controlledEntity, streamIndex, info, true, true);
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

void ControllerImpl::onGetAcquiredStateResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, UniqueIdentifier const owningEntity) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onGetAcquiredStateResult (OwningEntity={}): {}", owningEntity, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDynamicInfo(0u, ControlledEntityImpl::DynamicInfoType::AcquiredState, 0u, 0u))
		{
			auto& entity = *controlledEntity;
			auto const [acquireState, owningController] = getAcquiredInfoFromStatus(entity, owningEntity, status, true);

			// Could not determine the AcquiredState
			if (acquireState == model::AcquireState::Undefined)
			{
				if (!processFailureStatus(status, controlledEntity.get(), 0u, ControlledEntityImpl::DynamicInfoType::AcquiredState, 0u, 0u))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AcquiredState);
					return;
				}
			}

			// Update acquired state
			updateAcquiredState(entity, acquireState, owningController);
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

void ControllerImpl::onGetLockedStateResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, UniqueIdentifier const lockingEntity) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onGetLockedStateResult (LockingEntity={}): {}", lockingEntity, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDynamicInfo(0u, ControlledEntityImpl::DynamicInfoType::LockedState, 0u, 0u))
		{
			auto& entity = *controlledEntity;
			auto const [lockState, lockingController] = getLockedInfoFromStatus(entity, lockingEntity, status, true);

			// Could not determine the AcquiredState
			if (lockState == model::LockState::Undefined)
			{
				if (!processFailureStatus(status, controlledEntity.get(), 0u, ControlledEntityImpl::DynamicInfoType::LockedState, 0u, 0u))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::LockedState);
					return;
				}
			}

			// Update locked state
			updateLockedState(entity, lockState, lockingController);
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

void ControllerImpl::onGetStreamPortInputAudioMapResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamPortIndex const streamPortIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, entity::model::AudioMappings const& mappings, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onGetStreamPortInputAudioMapResult (StreamPortIndex={} NumberMaps={} MapIndex={}): {}", streamPortIndex, numberOfMaps, mapIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDynamicInfo(configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamAudioMappings, streamPortIndex, mapIndex))
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
					queryInformation(controlledEntity.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamAudioMappings, streamPortIndex, mapIndex + 1);
					return;
				}
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamAudioMappings, streamPortIndex, mapIndex))
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

void ControllerImpl::onGetStreamPortOutputAudioMapResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamPortIndex const streamPortIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, entity::model::AudioMappings const& mappings, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onGetStreamPortOutputAudioMapResult (StreamPortIndex={} NumberMaps={} MapIndex={}): {}", streamPortIndex, numberOfMaps, mapIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDynamicInfo(configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamAudioMappings, streamPortIndex, mapIndex))
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
					queryInformation(controlledEntity.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamAudioMappings, streamPortIndex, mapIndex + 1);
					return;
				}
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamAudioMappings, streamPortIndex, mapIndex))
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

void ControllerImpl::onGetAvbInfoResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvbInfo const& info, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onGetAvbInfoResult (AvbInterfaceIndex={}): {}", avbInterfaceIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

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

void ControllerImpl::onGetAsPathResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AsPath const& asPath, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onGetAsPathResult (AvbInterfaceIndex={}): {}", avbInterfaceIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDynamicInfo(configurationIndex, ControlledEntityImpl::DynamicInfoType::GetAsPath, avbInterfaceIndex))
		{
			if (!!status)
			{
				controlledEntity->setAsPath(avbInterfaceIndex, asPath);
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::GetAsPath, avbInterfaceIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AsPath);
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

void ControllerImpl::onGetAvbInterfaceCountersResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::AvbInterfaceCounterValidFlags const validCounters, entity::model::DescriptorCounters const& counters, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onGetAvbInterfaceCountersResult (AvbInterfaceIndex={}): {}", avbInterfaceIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDynamicInfo(configurationIndex, ControlledEntityImpl::DynamicInfoType::GetAvbInterfaceCounters, avbInterfaceIndex))
		{
			if (!!status)
			{
				auto& entity = *controlledEntity;

				// If Milan device, validate mandatory counters are present
				if (entity.getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
				{
					if ((validCounters & s_MilanMandatoryAvbInterfaceCounters) != s_MilanMandatoryAvbInterfaceCounters)
					{
						LOG_CONTROLLER_WARN(entityID, "Milan mandatory counters missing for AVB_INTERFACE descriptor");
						removeCompatibilityFlag(entity, ControlledEntity::CompatibilityFlag::Milan);
					}
				}

				// Use the "update**" method, there are many things to do
				updateAvbInterfaceCounters(entity, avbInterfaceIndex, validCounters, counters);
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::GetAvbInterfaceCounters, avbInterfaceIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AvbInterfaceCounters);
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

void ControllerImpl::onGetClockDomainCountersResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ClockDomainIndex const clockDomainIndex, entity::ClockDomainCounterValidFlags const validCounters, entity::model::DescriptorCounters const& counters, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onGetClockDomainCountersResult (ClockDomainIndex={}): {}", clockDomainIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDynamicInfo(configurationIndex, ControlledEntityImpl::DynamicInfoType::GetClockDomainCounters, clockDomainIndex))
		{
			if (!!status)
			{
				auto& entity = *controlledEntity;

				// If Milan device, validate mandatory counters are present
				if (entity.getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
				{
					if ((validCounters & s_MilanMandatoryClockDomainCounters) != s_MilanMandatoryClockDomainCounters)
					{
						LOG_CONTROLLER_WARN(entityID, "Milan mandatory counters missing for CLOCK_DOMAIN descriptor");
						removeCompatibilityFlag(entity, ControlledEntity::CompatibilityFlag::Milan);
					}
				}

				// Use the "update**" method, there are many things to do
				updateClockDomainCounters(entity, clockDomainIndex, validCounters, counters);
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::GetClockDomainCounters, clockDomainIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::ClockDomainCounters);
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

void ControllerImpl::onGetStreamInputCountersResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::StreamInputCounterValidFlags const validCounters, entity::model::DescriptorCounters const& counters, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onGetStreamInputCountersResult (StreamIndex={}): {}", streamIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDynamicInfo(configurationIndex, ControlledEntityImpl::DynamicInfoType::GetStreamInputCounters, streamIndex))
		{
			if (!!status)
			{
				auto& entity = *controlledEntity;

				// If Milan device, validate mandatory counters are present
				if (entity.getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
				{
					if ((validCounters & s_MilanMandatoryStreamInputCounters) != s_MilanMandatoryStreamInputCounters)
					{
						LOG_CONTROLLER_WARN(entityID, "Milan mandatory counters missing for STREAM_INPUT descriptor");
						removeCompatibilityFlag(entity, ControlledEntity::CompatibilityFlag::Milan);
					}
				}

				// Use the "update**" method, there are many things to do
				updateStreamInputCounters(entity, streamIndex, validCounters, counters);
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::GetStreamInputCounters, streamIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamInputCounters);
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

void ControllerImpl::onGetStreamOutputCountersResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::StreamOutputCounterValidFlags const validCounters, entity::model::DescriptorCounters const& counters, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onGetStreamOutputCountersResult (StreamIndex={}): {}", streamIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDynamicInfo(configurationIndex, ControlledEntityImpl::DynamicInfoType::GetStreamOutputCounters, streamIndex))
		{
			if (!!status)
			{
				auto& entity = *controlledEntity;

				// If Milan device, validate mandatory counters are present
				if (entity.getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
				{
					if ((validCounters & s_MilanMandatoryStreamOutputCounters) != s_MilanMandatoryStreamOutputCounters)
					{
						LOG_CONTROLLER_WARN(entityID, "Milan mandatory counters missing for STREAM_OUTPUT descriptor");
						removeCompatibilityFlag(entity, ControlledEntity::CompatibilityFlag::Milan);
					}
				}

				// Use the "update**" method, there are many things to do
				updateStreamOutputCounters(entity, streamIndex, validCounters, counters);
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::GetStreamOutputCounters, streamIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamOutputCounters);
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

void ControllerImpl::onConfigurationNameResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& configurationName) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onConfigurationNameResult (ConfigurationIndex={}): {}", configurationIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ConfigurationName, 0u))
		{
			if (!!status)
			{
				controlledEntity->setConfigurationName(configurationIndex, configurationName);
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ConfigurationName, 0u, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::ConfigurationName);
					return;
				}
			}
		}

		// Got all expected descriptor dynamic information
		if (controlledEntity->gotAllExpectedDescriptorDynamicInfo())
		{
			// Clear this enumeration step and check for next one
			controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetDescriptorDynamicInfo);
			checkEnumerationSteps(controlledEntity.get());
		}
	}
}

void ControllerImpl::onAudioUnitNameResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::AvdeccFixedString const& audioUnitName) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onAudioUnitNameResult (ConfigurationIndex={} AudioUnitIndex={}): {}", configurationIndex, audioUnitIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::AudioUnitName, audioUnitIndex))
		{
			if (!!status)
			{
				controlledEntity->setObjectName(configurationIndex, audioUnitIndex, &model::ConfigurationDynamicTree::audioUnitDynamicModels, audioUnitName);
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::AudioUnitName, audioUnitIndex, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AudioUnitName);
					return;
				}
			}
		}

		// Got all expected descriptor dynamic information
		if (controlledEntity->gotAllExpectedDescriptorDynamicInfo())
		{
			// Clear this enumeration step and check for next one
			controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetDescriptorDynamicInfo);
			checkEnumerationSteps(controlledEntity.get());
		}
	}
}

void ControllerImpl::onAudioUnitSamplingRateResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::AudioUnitIndex const audioUnitIndex, entity::model::SamplingRate const samplingRate, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onAudioUnitSamplingRateResult (ConfigurationIndex={} AudioUnitIndex={}): {}", configurationIndex, audioUnitIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::AudioUnitSamplingRate, audioUnitIndex))
		{
			if (!!status)
			{
				controlledEntity->setSamplingRate(audioUnitIndex, samplingRate);
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::AudioUnitSamplingRate, audioUnitIndex, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AudioUnitSamplingRate);
					return;
				}
			}
		}

		// Got all expected descriptor dynamic information
		if (controlledEntity->gotAllExpectedDescriptorDynamicInfo())
		{
			// Clear this enumeration step and check for next one
			controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetDescriptorDynamicInfo);
			checkEnumerationSteps(controlledEntity.get());
		}
	}
}

void ControllerImpl::onInputStreamNameResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamInputName) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onInputStreamNameResult (ConfigurationIndex={} StreamIndex={}): {}", configurationIndex, streamIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::InputStreamName, streamIndex))
		{
			if (!!status)
			{
				controlledEntity->setObjectName(configurationIndex, streamIndex, &model::ConfigurationDynamicTree::streamInputDynamicModels, streamInputName);
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::InputStreamName, streamIndex, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::InputStreamName);
					return;
				}
			}
		}

		// Got all expected descriptor dynamic information
		if (controlledEntity->gotAllExpectedDescriptorDynamicInfo())
		{
			// Clear this enumeration step and check for next one
			controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetDescriptorDynamicInfo);
			checkEnumerationSteps(controlledEntity.get());
		}
	}
}

void ControllerImpl::onInputStreamFormatResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onInputStreamFormatResult (ConfigurationIndex={} StreamIndex={}): {}", configurationIndex, streamIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::InputStreamFormat, streamIndex))
		{
			if (!!status)
			{
				// Use the "update**" method, there are many things to do
				updateStreamInputFormat(*controlledEntity, streamIndex, streamFormat);
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::InputStreamFormat, streamIndex, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::InputStreamFormat);
					return;
				}
			}
		}

		// Got all expected descriptor dynamic information
		if (controlledEntity->gotAllExpectedDescriptorDynamicInfo())
		{
			// Clear this enumeration step and check for next one
			controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetDescriptorDynamicInfo);
			checkEnumerationSteps(controlledEntity.get());
		}
	}
}

void ControllerImpl::onOutputStreamNameResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::StreamIndex const streamIndex, entity::model::AvdeccFixedString const& streamOutputName) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onOutputStreamNameResult (ConfigurationIndex={} StreamIndex={}): {}", configurationIndex, streamIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::OutputStreamName, streamIndex))
		{
			if (!!status)
			{
				controlledEntity->setObjectName(configurationIndex, streamIndex, &model::ConfigurationDynamicTree::streamOutputDynamicModels, streamOutputName);
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::OutputStreamName, streamIndex, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::OutputStreamName);
					return;
				}
			}
		}

		// Got all expected descriptor dynamic information
		if (controlledEntity->gotAllExpectedDescriptorDynamicInfo())
		{
			// Clear this enumeration step and check for next one
			controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetDescriptorDynamicInfo);
			checkEnumerationSteps(controlledEntity.get());
		}
	}
}

void ControllerImpl::onOutputStreamFormatResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::StreamIndex const streamIndex, entity::model::StreamFormat const streamFormat, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onOutputStreamFormatResult (ConfigurationIndex={} StreamIndex={}): {}", configurationIndex, streamIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::OutputStreamFormat, streamIndex))
		{
			if (!!status)
			{
				// Use the "update**" method, there are many things to do
				updateStreamOutputFormat(*controlledEntity, streamIndex, streamFormat);
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::OutputStreamFormat, streamIndex, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::OutputStreamFormat);
					return;
				}
			}
		}

		// Got all expected descriptor dynamic information
		if (controlledEntity->gotAllExpectedDescriptorDynamicInfo())
		{
			// Clear this enumeration step and check for next one
			controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetDescriptorDynamicInfo);
			checkEnumerationSteps(controlledEntity.get());
		}
	}
}

void ControllerImpl::onAvbInterfaceNameResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvbInterfaceIndex const avbInterfaceIndex, entity::model::AvdeccFixedString const& avbInterfaceName) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onAvbInterfaceNameResult (ConfigurationIndex={} AvbInterfaceIndex={}): {}", configurationIndex, avbInterfaceIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::AvbInterfaceName, avbInterfaceIndex))
		{
			if (!!status)
			{
				controlledEntity->setObjectName(configurationIndex, avbInterfaceIndex, &model::ConfigurationDynamicTree::avbInterfaceDynamicModels, avbInterfaceName);
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::AvbInterfaceName, avbInterfaceIndex, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AvbInterfaceName);
					return;
				}
			}
		}

		// Got all expected descriptor dynamic information
		if (controlledEntity->gotAllExpectedDescriptorDynamicInfo())
		{
			// Clear this enumeration step and check for next one
			controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetDescriptorDynamicInfo);
			checkEnumerationSteps(controlledEntity.get());
		}
	}
}

void ControllerImpl::onClockSourceNameResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockSourceIndex const clockSourceIndex, entity::model::AvdeccFixedString const& clockSourceName) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onClockSourceNameResult (ConfigurationIndex={} ClockSourceIndex={}): {}", configurationIndex, clockSourceIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ClockSourceName, clockSourceIndex))
		{
			if (!!status)
			{
				controlledEntity->setObjectName(configurationIndex, clockSourceIndex, &model::ConfigurationDynamicTree::clockSourceDynamicModels, clockSourceName);
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ClockSourceName, clockSourceIndex, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::ClockSourceName);
					return;
				}
			}
		}

		// Got all expected descriptor dynamic information
		if (controlledEntity->gotAllExpectedDescriptorDynamicInfo())
		{
			// Clear this enumeration step and check for next one
			controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetDescriptorDynamicInfo);
			checkEnumerationSteps(controlledEntity.get());
		}
	}
}

void ControllerImpl::onMemoryObjectNameResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, entity::model::AvdeccFixedString const& memoryObjectName) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onMemoryObjectNameResult (ConfigurationIndex={} MemoryObjectIndex={}): {}", configurationIndex, memoryObjectIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::MemoryObjectName, memoryObjectIndex))
		{
			if (!!status)
			{
				controlledEntity->setObjectName(configurationIndex, memoryObjectIndex, &model::ConfigurationDynamicTree::memoryObjectDynamicModels, memoryObjectName);
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::MemoryObjectName, memoryObjectIndex, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::MemoryObjectName);
					return;
				}
			}
		}

		// Got all expected descriptor dynamic information
		if (controlledEntity->gotAllExpectedDescriptorDynamicInfo())
		{
			// Clear this enumeration step and check for next one
			controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetDescriptorDynamicInfo);
			checkEnumerationSteps(controlledEntity.get());
		}
	}
}

void ControllerImpl::onMemoryObjectLengthResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onMemoryObjectLengthResult (ConfigurationIndex={} MemoryObjectIndex={}): {}", configurationIndex, memoryObjectIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::MemoryObjectLength, memoryObjectIndex))
		{
			if (!!status)
			{
				controlledEntity->setMemoryObjectLength(configurationIndex, memoryObjectIndex, length);
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::MemoryObjectLength, memoryObjectIndex, true))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::MemoryObjectLength);
					return;
				}
			}
		}

		// Got all expected descriptor dynamic information
		if (controlledEntity->gotAllExpectedDescriptorDynamicInfo())
		{
			// Clear this enumeration step and check for next one
			controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetDescriptorDynamicInfo);
			checkEnumerationSteps(controlledEntity.get());
		}
	}
}

void ControllerImpl::onAudioClusterNameResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClusterIndex const audioClusterIndex, entity::model::AvdeccFixedString const& audioClusterName) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onAudioClusterNameResult (ConfigurationIndex={} AudioClusterIndex={}): {}", configurationIndex, audioClusterIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::AudioClusterName, audioClusterIndex))
		{
			if (!!status)
			{
				controlledEntity->setObjectName(configurationIndex, audioClusterIndex, &model::ConfigurationDynamicTree::audioClusterDynamicModels, audioClusterName);
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::AudioClusterName, audioClusterIndex, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AudioClusterName);
					return;
				}
			}
		}

		// Got all expected descriptor dynamic information
		if (controlledEntity->gotAllExpectedDescriptorDynamicInfo())
		{
			// Clear this enumeration step and check for next one
			controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetDescriptorDynamicInfo);
			checkEnumerationSteps(controlledEntity.get());
		}
	}
}

void ControllerImpl::onClockDomainNameResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::AvdeccFixedString const& clockDomainName) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onClockDomainNameResult (ConfigurationIndex={} ClockDomainIndex={}): {}", configurationIndex, clockDomainIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ClockDomainName, clockDomainIndex))
		{
			if (!!status)
			{
				controlledEntity->setObjectName(configurationIndex, clockDomainIndex, &model::ConfigurationDynamicTree::clockDomainDynamicModels, clockDomainName);
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ClockDomainName, clockDomainIndex, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::ClockDomainName);
					return;
				}
			}
		}

		// Got all expected descriptor dynamic information
		if (controlledEntity->gotAllExpectedDescriptorDynamicInfo())
		{
			// Clear this enumeration step and check for next one
			controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetDescriptorDynamicInfo);
			checkEnumerationSteps(controlledEntity.get());
		}
	}
}

void ControllerImpl::onClockDomainSourceIndexResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ClockDomainIndex const clockDomainIndex, entity::model::ClockSourceIndex const clockSourceIndex, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onClockDomainSourceIndexResult (ConfigurationIndex={} ClockDomainIndex={}): {}", configurationIndex, clockDomainIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ClockDomainSourceIndex, clockDomainIndex))
		{
			if (!!status)
			{
				controlledEntity->setClockSource(clockDomainIndex, clockSourceIndex);
			}
			else
			{
				if (!processFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ClockDomainSourceIndex, clockDomainIndex, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::ClockDomainSourceIndex);
					return;
				}
			}
		}

		// Got all expected descriptor dynamic information
		if (controlledEntity->gotAllExpectedDescriptorDynamicInfo())
		{
			// Clear this enumeration step and check for next one
			controlledEntity->clearEnumerationSteps(ControlledEntityImpl::EnumerationSteps::GetDescriptorDynamicInfo);
			checkEnumerationSteps(controlledEntity.get());
		}
	}
}

/* Connection Management Protocol (ACMP) handlers */
void ControllerImpl::onConnectStreamResult(entity::controller::Interface const* const /*controller*/, [[maybe_unused]] entity::model::StreamIdentification const& talkerStream, [[maybe_unused]] entity::model::StreamIdentification const& listenerStream, [[maybe_unused]] uint16_t const connectionCount, [[maybe_unused]] entity::ConnectionFlags const flags, [[maybe_unused]] entity::ControllerEntity::ControlStatus const status) noexcept
{
	LOG_CONTROLLER_TRACE(UniqueIdentifier::getNullUniqueIdentifier(), "onConnectStreamResult (TalkerID={} TalkerIndex={} ListenerID={} ListenerIndex={} ConnectionCount={} Flags={}): {}", utils::toHexString(talkerStream.entityID, true), talkerStream.streamIndex, utils::toHexString(listenerStream.entityID, true), listenerStream.streamIndex, connectionCount, utils::toHexString(utils::to_integral(flags), true), entity::ControllerEntity::statusToString(status));
}

void ControllerImpl::onDisconnectStreamResult(entity::controller::Interface const* const /*controller*/, [[maybe_unused]] entity::model::StreamIdentification const& talkerStream, [[maybe_unused]] entity::model::StreamIdentification const& listenerStream, [[maybe_unused]] uint16_t const connectionCount, [[maybe_unused]] entity::ConnectionFlags const flags, [[maybe_unused]] entity::ControllerEntity::ControlStatus const status) noexcept
{
	LOG_CONTROLLER_TRACE(UniqueIdentifier::getNullUniqueIdentifier(), "onDisconnectStreamResult (TalkerID={} TalkerIndex={} ListenerID={} ListenerIndex={} ConnectionCount={} Flags={}): {}", utils::toHexString(talkerStream.entityID, true), talkerStream.streamIndex, utils::toHexString(listenerStream.entityID, true), listenerStream.streamIndex, connectionCount, utils::toHexString(utils::to_integral(flags), true), entity::ControllerEntity::statusToString(status));
}

void ControllerImpl::onGetTalkerStreamStateResult(entity::controller::Interface const* const /*controller*/, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& /*listenerStream*/, [[maybe_unused]] uint16_t const connectionCount, entity::ConnectionFlags const /*flags*/, entity::ControllerEntity::ControlStatus const status, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	LOG_CONTROLLER_TRACE(UniqueIdentifier::getNullUniqueIdentifier(), "onGetTalkerStreamStateResult (TalkerID={} TalkerIndex={} ConnectionCount={} ConfigurationIndex={}): {}", utils::toHexString(talkerStream.entityID, true), talkerStream.streamIndex, connectionCount, configurationIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto talker = getControlledEntityImplGuard(talkerStream.entityID);

	if (talker)
	{
		if (talker->checkAndClearExpectedDynamicInfo(configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamState, talkerStream.streamIndex))
		{
			if (!!status)
			{
				clearTalkerStreamConnections(talker.get(), talkerStream.streamIndex);

				for (auto index = std::uint16_t(0); index < connectionCount; ++index)
				{
					queryInformation(talker.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamConnection, talkerStream, index);
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

void ControllerImpl::onGetListenerStreamStateResult(entity::controller::Interface const* const /*controller*/, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	LOG_CONTROLLER_TRACE(UniqueIdentifier::getNullUniqueIdentifier(), "onGetListenerStreamStateResult (ListenerID={} ListenerIndex={} ConnectionCount={} Flags={} ConfigurationIndex={}): {}", utils::toHexString(listenerStream.entityID, true), listenerStream.streamIndex, connectionCount, utils::toHexString(utils::to_integral(flags), true), configurationIndex, entity::ControllerEntity::statusToString(status));

	if (!!status)
	{
		// In a GET_RX_STATE_RESPONSE message, the connectionCount is set to 1 if the stream is connected and 0 if not connected (See Marc Illouz clarification document, and hopefully someday as a corrigendum)
		handleListenerStreamStateNotification(talkerStream, listenerStream, connectionCount != 0, flags, false);
	}

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto listener = getControlledEntityImplGuard(listenerStream.entityID);

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

void ControllerImpl::onGetTalkerStreamConnectionResult(entity::controller::Interface const* const /*controller*/, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, [[maybe_unused]] uint16_t const connectionCount, entity::ConnectionFlags const /*flags*/, entity::ControllerEntity::ControlStatus const status, entity::model::ConfigurationIndex const configurationIndex, std::uint16_t const connectionIndex) noexcept
{
	LOG_CONTROLLER_TRACE(UniqueIdentifier::getNullUniqueIdentifier(), "onGetTalkerStreamConnectionResult (TalkerID={} TalkerIndex={} ListenerID={} ListenerIndex={} ConnectionCount={} ConfigurationIndex={} ConnectionIndex={}): {}", utils::toHexString(talkerStream.entityID, true), talkerStream.streamIndex, utils::toHexString(listenerStream.entityID, true), listenerStream.streamIndex, connectionCount, configurationIndex, connectionIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto talker = getControlledEntityImplGuard(talkerStream.entityID);

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
				if (!processFailureStatus(status, talker.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamConnection, talkerStream, connectionIndex))
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
