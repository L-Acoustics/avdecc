/*
* Copyright (C) 2016-2023, L-Acoustics and its contributors

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
static auto const s_MilanMandatoryStreamInputCounters = entity::StreamInputCounterValidFlags{ entity::StreamInputCounterValidFlag::MediaLocked, entity::StreamInputCounterValidFlag::MediaUnlocked, entity::StreamInputCounterValidFlag::StreamInterrupted, entity::StreamInputCounterValidFlag::SeqNumMismatch, entity::StreamInputCounterValidFlag::MediaReset, entity::StreamInputCounterValidFlag::TimestampUncertain, entity::StreamInputCounterValidFlag::UnsupportedFormat, entity::StreamInputCounterValidFlag::LateTimestamp, entity::StreamInputCounterValidFlag::EarlyTimestamp, entity::StreamInputCounterValidFlag::FramesRx }; // Milan-2019 Clause 6.8.10
static auto const s_MilanMandatoryStreamOutputCounters = entity::StreamOutputCounterValidFlags{ entity::StreamOutputCounterValidFlag::StreamStart, entity::StreamOutputCounterValidFlag::StreamStop, entity::StreamOutputCounterValidFlag::MediaReset, entity::StreamOutputCounterValidFlag::TimestampUncertain, entity::StreamOutputCounterValidFlag::FramesTx }; // Milan-2019 Clause 6.7.7
static auto const s_MilanMandatoryAvbInterfaceCounters = entity::AvbInterfaceCounterValidFlags{ entity::AvbInterfaceCounterValidFlag::LinkUp, entity::AvbInterfaceCounterValidFlag::LinkDown, entity::AvbInterfaceCounterValidFlag::GptpGmChanged }; // Milan-2019 Clause 6.6.3
static auto const s_MilanMandatoryClockDomainCounters = entity::ClockDomainCounterValidFlags{ entity::ClockDomainCounterValidFlag::Locked, entity::ClockDomainCounterValidFlag::Unlocked }; // Milan-2019 Clause 6.11.2

/* ************************************************************ */
/* Result handlers                                              */
/* ************************************************************ */
/* Enumeration and Control Protocol (AECP) handlers */
void ControllerImpl::onGetMilanInfoResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::MvuCommandStatus const status, entity::model::MilanInfo const& info) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onGetMilanInfoResult (ProtocolVersion={} FeaturesFlags={} CertificationVersion={}): {}", info.protocolVersion, utils::toHexString(utils::forceNumeric(info.featuresFlags.value())), info.certificationVersion, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedMilanInfo(ControlledEntityImpl::MilanInfoType::MilanInfo))
		{
			if (!!status)
			{
				// Flag the entity as "Milan compatible", if protocolVersion is 1
				switch (info.protocolVersion)
				{
					case 1:
						addCompatibilityFlag(this, *controlledEntity, ControlledEntity::CompatibilityFlag::Milan);
						break;
					default:
						LOG_CONTROLLER_WARN(entityID, "Unsupported Milan protocol_version: {}", info.protocolVersion);
						break;
				}
				controlledEntity->setMilanInfo(info);
			}
			else
			{
				if (!processGetMilanModelFailureStatus(status, controlledEntity.get(), ControlledEntityImpl::MilanInfoType::MilanInfo))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::GetMilanInfo);
					return;
				}
			}

			// Got all expected milan information
			if (controlledEntity->gotAllExpectedMilanInfo())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetMilanInfo);
				checkEnumerationSteps(controlledEntity.get());
			}
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
		auto& entity = *controlledEntity;

		if (entity.checkAndClearExpectedRegisterUnsol())
		{
			entity.setUnsolicitedNotificationsSupported(true); // Set to true by default, will be set to false if we get a failure status
			if (!!status)
			{
				entity.setSubscribedToUnsolicitedNotifications(true);
			}
			else
			{
				if (!processRegisterUnsolFailureStatus(status, &entity))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, &entity, QueryCommandError::RegisterUnsol);
					return;
				}
			}

			// Got all expected descriptors
			if (entity.gotExpectedRegisterUnsol())
			{
				// Clear this enumeration step and check for next one
				entity.clearEnumerationStep(ControlledEntityImpl::EnumerationStep::RegisterUnsol);
				checkEnumerationSteps(&entity);
			}
		}
	}
}

void ControllerImpl::onUnregisterUnsolicitedNotificationsResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onDeregisterUnsolicitedNotificationsResult: {}", entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		if (!!status)
		{
			entity.setSubscribedToUnsolicitedNotifications(false);
		}
	}
}

void ControllerImpl::onEntityDescriptorResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::EntityDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onEntityDescriptorStaticResult: {}", entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		auto& entity = *controlledEntity;

		if (entity.checkAndClearExpectedDescriptor(0, entity::model::DescriptorType::Entity, 0))
		{
			if (!!status)
			{
				// Validate some fields
				auto const& e = entity.getEntity();
				if (descriptor.entityID != e.getEntityID())
				{
					LOG_CONTROLLER_WARN(entityID, "EntityID is expected to be identical in ADP and ENTITY_DESCRIPTOR");
					removeCompatibilityFlag(this, entity, ControlledEntity::CompatibilityFlag::IEEE17221);
					entity.setIgnoreCachedEntityModel();
				}
				if (descriptor.entityModelID != e.getEntityModelID())
				{
					LOG_CONTROLLER_WARN(entityID, "EntityModelID is expected to be identical in ADP and ENTITY_DESCRIPTOR");
					removeCompatibilityFlag(this, entity, ControlledEntity::CompatibilityFlag::IEEE17221);
					entity.setIgnoreCachedEntityModel();
				}

				// Search in the AEM cache for the AEM of the active configuration (if not ignored)
				auto const entityModelID = descriptor.entityModelID;
				auto cachedModel = std::optional<model::EntityNode>{ std::nullopt };
				auto const& entityModelCache = EntityModelCache::getInstance();
				// If AEM Cache is Enabled and the entity has an EntityModelID defined
				if (!entity.shouldIgnoreCachedEntityModel() && entityModelCache.isCacheEnabled() && entityModelID)
				{
					if (EntityModelCache::isValidEntityModelID(entityModelID))
					{
						cachedModel = entityModelCache.getCachedEntityModel(entityModelID);
					}
					else
					{
						LOG_CONTROLLER_INFO(entityID, "AEM-CACHE: Ignoring invalid EntityModelID {} (invalid Vendor OUI-24)", utils::toHexString(entityModelID, true, false));
					}
				}

				// Already cached, no need to get the remaining of EnumerationSteps::GetStaticModel, proceed with EnumerationSteps::GetDescriptorDynamicInfo
				if (cachedModel && entity.setCachedEntityNode(std::move(*cachedModel), descriptor, _fullStaticModelEnumeration))
				{
					LOG_CONTROLLER_INFO(entityID, "AEM-CACHE: Loaded model for EntityModelID {}", utils::toHexString(entityModelID, true, false));
					entity.addEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDescriptorDynamicInfo);
				}
				else
				{
					entity.setEntityDescriptor(descriptor);
					for (auto index = entity::model::ConfigurationIndex(0u); index < descriptor.configurationsCount; ++index)
					{
						queryInformation(&entity, index, entity::model::DescriptorType::Configuration, 0u);
					}
				}
			}
			else
			{
				if (!processGetStaticModelFailureStatus(status, &entity, 0, entity::model::DescriptorType::Entity, 0))
				{
					entity.setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, &entity, QueryCommandError::EntityDescriptor);
					return;
				}
			}
			// Got all expected descriptors
			if (entity.gotAllExpectedDescriptors())
			{
				// Clear this enumeration step and check for next one
				entity.clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetStaticModel);
				checkEnumerationSteps(&entity);
			}
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
				try
				{
					controlledEntity->setConfigurationDescriptor(descriptor, configurationIndex);
					auto const isCurrentConfiguration = configurationIndex == *controlledEntity->getCurrentConfigurationIndex(TreeModelAccessStrategy::NotFoundBehavior::Throw);
					// Get full descriptors for active configuration or if _fullStaticModelEnumeration is set
					if (isCurrentConfiguration || _fullStaticModelEnumeration)
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
						// Get input jacks
						{
							auto countIt = descriptor.descriptorCounts.find(entity::model::DescriptorType::JackInput);
							if (countIt != descriptor.descriptorCounts.end() && countIt->second != 0)
							{
								auto count = countIt->second;
								for (auto index = entity::model::JackIndex(0); index < count; ++index)
								{
									// Get Jack Descriptor
									queryInformation(controlledEntity.get(), configurationIndex, entity::model::DescriptorType::JackInput, index);
								}
							}
						}
						// Get output jacks
						{
							auto countIt = descriptor.descriptorCounts.find(entity::model::DescriptorType::JackOutput);
							if (countIt != descriptor.descriptorCounts.end() && countIt->second != 0)
							{
								auto count = countIt->second;
								for (auto index = entity::model::JackIndex(0); index < count; ++index)
								{
									// Get Jack Descriptor
									queryInformation(controlledEntity.get(), configurationIndex, entity::model::DescriptorType::JackOutput, index);
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
						// Get controls
						{
							auto countIt = descriptor.descriptorCounts.find(entity::model::DescriptorType::Control);
							if (countIt != descriptor.descriptorCounts.end() && countIt->second != 0)
							{
								auto count = countIt->second;
								for (auto index = entity::model::ControlIndex(0); index < count; ++index)
								{
									queryInformation(controlledEntity.get(), configurationIndex, entity::model::DescriptorType::Control, index);
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
						// Get timings
						{
							auto countIt = descriptor.descriptorCounts.find(entity::model::DescriptorType::Timing);
							if (countIt != descriptor.descriptorCounts.end() && countIt->second != 0)
							{
								auto count = countIt->second;
								for (auto index = entity::model::TimingIndex(0); index < count; ++index)
								{
									queryInformation(controlledEntity.get(), configurationIndex, entity::model::DescriptorType::Timing, index);
								}
							}
						}
						// Get ptp instances
						{
							auto countIt = descriptor.descriptorCounts.find(entity::model::DescriptorType::PtpInstance);
							if (countIt != descriptor.descriptorCounts.end() && countIt->second != 0)
							{
								auto count = countIt->second;
								for (auto index = entity::model::PtpInstanceIndex(0); index < count; ++index)
								{
									queryInformation(controlledEntity.get(), configurationIndex, entity::model::DescriptorType::PtpInstance, index);
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
				catch (ControlledEntity::Exception const&)
				{
					controlledEntity->setGetFatalEnumerationError();
					return;
				}
			}
			else
			{
				if (!processGetStaticModelFailureStatus(status, controlledEntity.get(), 0, entity::model::DescriptorType::Configuration, configurationIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::ConfigurationDescriptor);
					return;
				}
			}

			// Got all expected descriptors
			if (controlledEntity->gotAllExpectedDescriptors())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetStaticModel);
				checkEnumerationSteps(controlledEntity.get());
			}
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
						AVDECC_ASSERT(descriptor.baseStreamInputPort == 0, "descriptor.baseStreamInputPort should probably be 0");
						for (auto index = entity::model::StreamPortIndex(0); index < descriptor.numberOfStreamInputPorts; ++index)
						{
							queryInformation(controlledEntity.get(), configurationIndex, entity::model::DescriptorType::StreamPortInput, descriptor.baseStreamInputPort + index);
						}
					}
				}
				// Get stream port output
				{
					if (descriptor.numberOfStreamOutputPorts != 0)
					{
						AVDECC_ASSERT(descriptor.baseStreamOutputPort == 0, "descriptor.baseStreamOutputPort should probably be 0");
						for (auto index = entity::model::StreamPortIndex(0); index < descriptor.numberOfStreamOutputPorts; ++index)
						{
							queryInformation(controlledEntity.get(), configurationIndex, entity::model::DescriptorType::StreamPortOutput, descriptor.baseStreamOutputPort + index);
						}
					}
				}
				// Get external port input
				// Get external port output
				// Get internal port input
				// Get internal port output
				// Get controls
				{
					if (descriptor.numberOfControls != 0)
					{
						for (auto index = entity::model::ControlIndex(0); index < descriptor.numberOfControls; ++index)
						{
							queryInformation(controlledEntity.get(), configurationIndex, entity::model::DescriptorType::Control, descriptor.baseControl + index);
						}
					}
				}
			}
			else
			{
				if (!processGetStaticModelFailureStatus(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::AudioUnit, audioUnitIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AudioUnitDescriptor);
					return;
				}
			}

			// Got all expected descriptors
			if (controlledEntity->gotAllExpectedDescriptors())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetStaticModel);
				checkEnumerationSteps(controlledEntity.get());
			}
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
				if (!processGetStaticModelFailureStatus(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::StreamInput, streamIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamInputDescriptor);
					return;
				}
			}

			// Got all expected descriptors
			if (controlledEntity->gotAllExpectedDescriptors())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetStaticModel);
				checkEnumerationSteps(controlledEntity.get());
			}
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
				if (!processGetStaticModelFailureStatus(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::StreamOutput, streamIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamOutputDescriptor);
					return;
				}
			}

			// Got all expected descriptors
			if (controlledEntity->gotAllExpectedDescriptors())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetStaticModel);
				checkEnumerationSteps(controlledEntity.get());
			}
		}
	}
}

void ControllerImpl::onJackInputDescriptorResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex, entity::model::JackDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onJackInputDescriptorResult (ConfigurationIndex={} JackIndex={}): {}", configurationIndex, jackIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::JackInput, jackIndex))
		{
			if (!!status)
			{
				controlledEntity->setJackInputDescriptor(descriptor, configurationIndex, jackIndex);

				// Get controls
				{
					if (descriptor.numberOfControls != 0)
					{
						for (auto index = entity::model::ControlIndex(0); index < descriptor.numberOfControls; ++index)
						{
							queryInformation(controlledEntity.get(), configurationIndex, entity::model::DescriptorType::Control, descriptor.baseControl + index);
						}
					}
				}
			}
			else
			{
				if (!processGetStaticModelFailureStatus(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::JackInput, jackIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::JackInputDescriptor);
					return;
				}
			}

			// Got all expected descriptors
			if (controlledEntity->gotAllExpectedDescriptors())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetStaticModel);
				checkEnumerationSteps(controlledEntity.get());
			}
		}
	}
}

void ControllerImpl::onJackOutputDescriptorResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex, entity::model::JackDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onJackOutputDescriptorResult (ConfigurationIndex={} JackIndex={}): {}", configurationIndex, jackIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::JackOutput, jackIndex))
		{
			if (!!status)
			{
				controlledEntity->setJackOutputDescriptor(descriptor, configurationIndex, jackIndex);

				// Get controls
				{
					if (descriptor.numberOfControls != 0)
					{
						for (auto index = entity::model::ControlIndex(0); index < descriptor.numberOfControls; ++index)
						{
							queryInformation(controlledEntity.get(), configurationIndex, entity::model::DescriptorType::Control, descriptor.baseControl + index);
						}
					}
				}
			}
			else
			{
				if (!processGetStaticModelFailureStatus(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::JackOutput, jackIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::JackOutputDescriptor);
					return;
				}
			}

			// Got all expected descriptors
			if (controlledEntity->gotAllExpectedDescriptors())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetStaticModel);
				checkEnumerationSteps(controlledEntity.get());
			}
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
			}
			else
			{
				if (!processGetStaticModelFailureStatus(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::AvbInterface, interfaceIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AvbInterfaceDescriptor);
					return;
				}
			}

			// Got all expected descriptors
			if (controlledEntity->gotAllExpectedDescriptors())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetStaticModel);
				checkEnumerationSteps(controlledEntity.get());
			}
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
				if (!processGetStaticModelFailureStatus(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::ClockSource, clockIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::ClockSourceDescriptor);
					return;
				}
			}

			// Got all expected descriptors
			if (controlledEntity->gotAllExpectedDescriptors())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetStaticModel);
				checkEnumerationSteps(controlledEntity.get());
			}
		}
	}
}

void ControllerImpl::onMemoryObjectDescriptorResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, entity::model::MemoryObjectDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onMemoryObjectDescriptorResult (ConfigurationIndex={} MemoryObjectIndex={}): {}", configurationIndex, memoryObjectIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::MemoryObject, memoryObjectIndex))
		{
			if (!!status)
			{
				// Validate some fields
				if (descriptor.length > descriptor.maximumLength)
				{
					auto& entity = *controlledEntity;
					LOG_CONTROLLER_WARN(entityID, "Invalid MemoryObject.length value (greater than MemoryObject.maximumLength value): {} > {}", descriptor.length, descriptor.maximumLength);
					removeCompatibilityFlag(this, entity, ControlledEntity::CompatibilityFlag::IEEE17221);
					entity.setIgnoreCachedEntityModel();
				}
				controlledEntity->setMemoryObjectDescriptor(descriptor, configurationIndex, memoryObjectIndex);
			}
			else
			{
				if (!processGetStaticModelFailureStatus(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::MemoryObject, memoryObjectIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::MemoryObjectDescriptor);
					return;
				}
			}

			// Got all expected descriptors
			if (controlledEntity->gotAllExpectedDescriptors())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetStaticModel);
				checkEnumerationSteps(controlledEntity.get());
			}
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
				auto const* const configNode = controlledEntity->getModelAccessStrategy().getConfigurationNode(configurationIndex, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
				if (configNode)
				{
					//auto const& configTree = controlledEntity->getConfigurationTree(configurationIndex);
					auto countLocales = std::uint16_t{ 0u };
					{
						auto const localeIt = configNode->staticModel.descriptorCounts.find(entity::model::DescriptorType::Locale);
						if (localeIt != configNode->staticModel.descriptorCounts.end())
						{
							countLocales = localeIt->second;
						}
					}
					auto const allLocalesLoaded = configNode->locales.size() == countLocales;
					// We got all locales, now load strings for the desired locale
					if (allLocalesLoaded)
					{
						auto* const entity = controlledEntity.get();
						chooseLocale(entity, configurationIndex, _preferedLocale,
							[this, entity, configurationIndex](entity::model::StringsIndex const stringsIndex)
							{
								// Strings not in cache, we need to query the device
								queryInformation(entity, configurationIndex, entity::model::DescriptorType::Strings, stringsIndex);
							});
					}
				}
			}
			else
			{
				if (!processGetStaticModelFailureStatus(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::Locale, localeIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::LocaleDescriptor);
					return;
				}
			}

			// Got all expected descriptors
			if (controlledEntity->gotAllExpectedDescriptors())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetStaticModel);
				checkEnumerationSteps(controlledEntity.get());
			}
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
				if (!processGetStaticModelFailureStatus(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::Strings, stringsIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StringsDescriptor);
					return;
				}
			}

			// Got all expected descriptors
			if (controlledEntity->gotAllExpectedDescriptors())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetStaticModel);
				checkEnumerationSteps(controlledEntity.get());
			}
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

				// Get controls
				{
					if (descriptor.numberOfControls != 0)
					{
						for (auto index = entity::model::ControlIndex(0); index < descriptor.numberOfControls; ++index)
						{
							queryInformation(controlledEntity.get(), configurationIndex, entity::model::DescriptorType::Control, descriptor.baseControl + index);
						}
					}
				}
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
				if (!processGetStaticModelFailureStatus(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::StreamPortInput, streamPortIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamPortInputDescriptor);
					return;
				}
			}

			// Got all expected descriptors
			if (controlledEntity->gotAllExpectedDescriptors())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetStaticModel);
				checkEnumerationSteps(controlledEntity.get());
			}
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

				// Get controls
				{
					if (descriptor.numberOfControls != 0)
					{
						for (auto index = entity::model::ControlIndex(0); index < descriptor.numberOfControls; ++index)
						{
							queryInformation(controlledEntity.get(), configurationIndex, entity::model::DescriptorType::Control, descriptor.baseControl + index);
						}
					}
				}
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
				if (!processGetStaticModelFailureStatus(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::StreamPortOutput, streamPortIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamPortOutputDescriptor);
					return;
				}
			}

			// Got all expected descriptors
			if (controlledEntity->gotAllExpectedDescriptors())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetStaticModel);
				checkEnumerationSteps(controlledEntity.get());
			}
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
				if (!processGetStaticModelFailureStatus(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::AudioCluster, clusterIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AudioClusterDescriptor);
					return;
				}
			}

			// Got all expected descriptors
			if (controlledEntity->gotAllExpectedDescriptors())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetStaticModel);
				checkEnumerationSteps(controlledEntity.get());
			}
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
				if (!processGetStaticModelFailureStatus(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::AudioMap, mapIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AudioMapDescriptor);
					return;
				}
			}

			// Got all expected descriptors
			if (controlledEntity->gotAllExpectedDescriptors())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetStaticModel);
				checkEnumerationSteps(controlledEntity.get());
			}
		}
	}
}

void ControllerImpl::onControlDescriptorResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ControlIndex const controlIndex, entity::model::ControlDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onControlDescriptorResult (ConfigurationIndex={} ControlIndex={}): {}", configurationIndex, controlIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::Control, controlIndex))
		{
			if (!!status)
			{
				controlledEntity->setControlDescriptor(descriptor, configurationIndex, controlIndex);
			}
			else
			{
				if (!processGetStaticModelFailureStatus(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::Control, controlIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::ControlDescriptor);
					return;
				}
			}

			// Got all expected descriptors
			if (controlledEntity->gotAllExpectedDescriptors())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetStaticModel);
				checkEnumerationSteps(controlledEntity.get());
			}
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
				if (!processGetStaticModelFailureStatus(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::ClockDomain, clockDomainIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::ClockDomainDescriptor);
					return;
				}
			}

			// Got all expected descriptors
			if (controlledEntity->gotAllExpectedDescriptors())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetStaticModel);
				checkEnumerationSteps(controlledEntity.get());
			}
		}
	}
}

void ControllerImpl::onTimingDescriptorResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::TimingIndex const timingIndex, entity::model::TimingDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onTimingDescriptorResult (ConfigurationIndex={} TimingIndex={}): {}", configurationIndex, timingIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::Timing, timingIndex))
		{
			if (!!status)
			{
				controlledEntity->setTimingDescriptor(descriptor, configurationIndex, timingIndex);
			}
			else
			{
				if (!processGetStaticModelFailureStatus(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::Timing, timingIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::TimingDescriptor);
					return;
				}
			}

			// Got all expected descriptors
			if (controlledEntity->gotAllExpectedDescriptors())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetStaticModel);
				checkEnumerationSteps(controlledEntity.get());
			}
		}
	}
}

void ControllerImpl::onPtpInstanceDescriptorResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpInstanceIndex const ptpInstanceIndex, entity::model::PtpInstanceDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onPtpInstanceDescriptorResult (ConfigurationIndex={} PtpInstanceIndex={}): {}", configurationIndex, ptpInstanceIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::PtpInstance, ptpInstanceIndex))
		{
			if (!!status)
			{
				controlledEntity->setPtpInstanceDescriptor(descriptor, configurationIndex, ptpInstanceIndex);

				// Get controls
				{
					if (descriptor.numberOfControls != 0)
					{
						for (auto index = entity::model::ControlIndex(0); index < descriptor.numberOfControls; ++index)
						{
							queryInformation(controlledEntity.get(), configurationIndex, entity::model::DescriptorType::Control, descriptor.baseControl + index);
						}
					}
				}
				// Get ptp ports
				{
					if (descriptor.numberOfPtpPorts != 0)
					{
						for (auto index = entity::model::PtpPortIndex(0); index < descriptor.numberOfPtpPorts; ++index)
						{
							queryInformation(controlledEntity.get(), configurationIndex, entity::model::DescriptorType::PtpPort, descriptor.basePtpPort + index);
						}
					}
				}
			}
			else
			{
				if (!processGetStaticModelFailureStatus(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::PtpInstance, ptpInstanceIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::PtpInstanceDescriptor);
					return;
				}
			}

			// Got all expected descriptors
			if (controlledEntity->gotAllExpectedDescriptors())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetStaticModel);
				checkEnumerationSteps(controlledEntity.get());
			}
		}
	}
}

void ControllerImpl::onPtpPortDescriptorResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpPortIndex const ptpPortIndex, entity::model::PtpPortDescriptor const& descriptor) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onPtpPortDescriptorResult (ConfigurationIndex={} PtpPortIndex={}): {}", configurationIndex, ptpPortIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptor(configurationIndex, entity::model::DescriptorType::PtpPort, ptpPortIndex))
		{
			if (!!status)
			{
				controlledEntity->setPtpPortDescriptor(descriptor, configurationIndex, ptpPortIndex);
			}
			else
			{
				if (!processGetStaticModelFailureStatus(status, controlledEntity.get(), configurationIndex, entity::model::DescriptorType::PtpPort, ptpPortIndex))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::PtpPortDescriptor);
					return;
				}
			}

			// Got all expected descriptors
			if (controlledEntity->gotAllExpectedDescriptors())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetStaticModel);
				checkEnumerationSteps(controlledEntity.get());
			}
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
				updateStreamInputInfo(*controlledEntity, streamIndex, info, true, true, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			}
			else
			{
				if (!processGetAecpDynamicInfoFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamInfo, streamIndex, 0u, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::ListenerStreamInfo);
					return;
				}
			}

			// Got all expected dynamic information
			if (controlledEntity->gotAllExpectedDynamicInfo())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDynamicInfo);
				checkEnumerationSteps(controlledEntity.get());
			}
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
				updateStreamOutputInfo(*controlledEntity, streamIndex, info, true, true, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			}
			else
			{
				if (!processGetAecpDynamicInfoFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamInfo, streamIndex, 0u, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::TalkerStreamInfo);
					return;
				}
			}

			// Got all expected dynamic information
			if (controlledEntity->gotAllExpectedDynamicInfo())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDynamicInfo);
				checkEnumerationSteps(controlledEntity.get());
			}
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
				if (!processGetAecpDynamicInfoFailureStatus(status, controlledEntity.get(), 0u, ControlledEntityImpl::DynamicInfoType::AcquiredState, 0u, 0u, true))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AcquiredState);
					return;
				}
			}

			// Update acquired state
			updateAcquiredState(entity, acquireState, owningController);

			// Got all expected dynamic information
			if (controlledEntity->gotAllExpectedDynamicInfo())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDynamicInfo);
				checkEnumerationSteps(controlledEntity.get());
			}
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

			// Could not determine the LockState
			if (lockState == model::LockState::Undefined)
			{
				if (!processGetAecpDynamicInfoFailureStatus(status, controlledEntity.get(), 0u, ControlledEntityImpl::DynamicInfoType::LockedState, 0u, 0u, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::LockedState);
					return;
				}
			}

			// Update locked state
			updateLockedState(entity, lockState, lockingController);

			// Got all expected dynamic information
			if (controlledEntity->gotAllExpectedDynamicInfo())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDynamicInfo);
				checkEnumerationSteps(controlledEntity.get());
			}
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
					controlledEntity->clearStreamPortInputAudioMappings(streamPortIndex, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
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
				controlledEntity->addStreamPortInputAudioMappings(streamPortIndex, mappings, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
				if (!isComplete)
				{
					queryInformation(controlledEntity.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamAudioMappings, streamPortIndex, mapIndex + 1);
					return;
				}
			}
			else
			{
				if (!processGetAecpDynamicInfoFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamAudioMappings, streamPortIndex, mapIndex, false))
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

			// Got all expected dynamic information
			if (controlledEntity->gotAllExpectedDynamicInfo())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDynamicInfo);
				checkEnumerationSteps(controlledEntity.get());
			}
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
					controlledEntity->clearStreamPortOutputAudioMappings(streamPortIndex, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
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
				controlledEntity->addStreamPortOutputAudioMappings(streamPortIndex, mappings, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
				if (!isComplete)
				{
					queryInformation(controlledEntity.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamAudioMappings, streamPortIndex, mapIndex + 1);
					return;
				}
			}
			else
			{
				if (!processGetAecpDynamicInfoFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamAudioMappings, streamPortIndex, mapIndex, false))
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

			// Got all expected dynamic information
			if (controlledEntity->gotAllExpectedDynamicInfo())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDynamicInfo);
				checkEnumerationSteps(controlledEntity.get());
			}
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
			auto& entity = *controlledEntity;
			if (!!status)
			{
				// Use the "update**" method, there are many things to do
				updateAvbInfo(entity, avbInterfaceIndex, info, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			}
			else
			{
				if (!processGetAecpDynamicInfoFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::GetAvbInfo, avbInterfaceIndex, 0u, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AvbInfo);
					return;
				}

				// Special case for gPTP info, we always want to have valid gPTP information in the AvbInterfaceDescriptor model (updated when an ADP is received, or GET_AVB_INFO unsolicited)
				// So we have to retrieve the matching ADP information to force an update of the cached model
				auto const* const avbInterfaceStaticModel = entity.getModelAccessStrategy().getAvbInterfaceNodeStaticModel(configurationIndex, avbInterfaceIndex, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
				if (avbInterfaceStaticModel)
				{
					auto const& macAddress = avbInterfaceStaticModel->macAddress;
					auto& e = controlledEntity->getEntity();
					auto const caps = e.getEntityCapabilities();
					if (caps.test(entity::EntityCapability::GptpSupported))
					{
						// Search which InterfaceInformation matches this AvbInterfaceIndex (searching by Index, or by MacAddress in case the Index was not specified in ADP)
						for (auto& [interfaceIndex, interfaceInfo] : e.getInterfacesInformation())
						{
							// Do we even have gPTP info on this InterfaceInfo
							if (interfaceInfo.gptpGrandmasterID)
							{
								// Match with the passed AvbInterfaceIndex, or with macAddress if this ADP is the GlobalAvbInterfaceIndex
								if (interfaceIndex == avbInterfaceIndex || (interfaceIndex == entity::Entity::GlobalAvbInterfaceIndex && macAddress == interfaceInfo.macAddress))
								{
									// Force InterfaceInfo with these gPTP info
									updateGptpInformation(entity, avbInterfaceIndex, macAddress, *interfaceInfo.gptpGrandmasterID, *interfaceInfo.gptpDomainNumber, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
								}
							}
						}
					}
				}
			}

			// Got all expected dynamic information
			if (controlledEntity->gotAllExpectedDynamicInfo())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDynamicInfo);
				checkEnumerationSteps(controlledEntity.get());
			}
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
				updateAsPath(*controlledEntity, avbInterfaceIndex, asPath, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			}
			else
			{
				if (!processGetAecpDynamicInfoFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::GetAsPath, avbInterfaceIndex, 0u, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AsPath);
					return;
				}
			}

			// Got all expected dynamic information
			if (controlledEntity->gotAllExpectedDynamicInfo())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDynamicInfo);
				checkEnumerationSteps(controlledEntity.get());
			}
		}
	}
}

void ControllerImpl::onGetEntityCountersResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::EntityCounterValidFlags const validCounters, entity::model::DescriptorCounters const& counters) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onGetEntityCountersResult (): {}", entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDynamicInfo(0u, ControlledEntityImpl::DynamicInfoType::GetEntityCounters, 0u))
		{
			if (!!status)
			{
				auto& entity = *controlledEntity;

				// Use the "update**" method, there are many things to do
				updateEntityCounters(entity, validCounters, counters, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			}
			else
			{
				if (!processGetAecpDynamicInfoFailureStatus(status, controlledEntity.get(), 0u, ControlledEntityImpl::DynamicInfoType::GetEntityCounters, 0u, 0u, true))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::EntityCounters);
					return;
				}
			}

			// Got all expected dynamic information
			if (controlledEntity->gotAllExpectedDynamicInfo())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDynamicInfo);
				checkEnumerationSteps(controlledEntity.get());
			}
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
						removeCompatibilityFlag(this, entity, ControlledEntity::CompatibilityFlag::Milan);
					}
				}

				// Use the "update**" method, there are many things to do
				updateAvbInterfaceCounters(entity, avbInterfaceIndex, validCounters, counters, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			}
			else
			{
				if (!processGetAecpDynamicInfoFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::GetAvbInterfaceCounters, avbInterfaceIndex, 0u, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AvbInterfaceCounters);
					return;
				}
			}

			// Got all expected dynamic information
			if (controlledEntity->gotAllExpectedDynamicInfo())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDynamicInfo);
				checkEnumerationSteps(controlledEntity.get());
			}
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
						removeCompatibilityFlag(this, entity, ControlledEntity::CompatibilityFlag::Milan);
					}
				}

				// Use the "update**" method, there are many things to do
				updateClockDomainCounters(entity, clockDomainIndex, validCounters, counters, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			}
			else
			{
				if (!processGetAecpDynamicInfoFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::GetClockDomainCounters, clockDomainIndex, 0u, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::ClockDomainCounters);
					return;
				}
			}

			// Got all expected dynamic information
			if (controlledEntity->gotAllExpectedDynamicInfo())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDynamicInfo);
				checkEnumerationSteps(controlledEntity.get());
			}
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
						removeCompatibilityFlag(this, entity, ControlledEntity::CompatibilityFlag::Milan);
					}
				}

				// Use the "update**" method, there are many things to do
				updateStreamInputCounters(entity, streamIndex, validCounters, counters, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			}
			else
			{
				if (!processGetAecpDynamicInfoFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::GetStreamInputCounters, streamIndex, 0u, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamInputCounters);
					return;
				}
			}

			// Got all expected dynamic information
			if (controlledEntity->gotAllExpectedDynamicInfo())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDynamicInfo);
				checkEnumerationSteps(controlledEntity.get());
			}
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
						removeCompatibilityFlag(this, entity, ControlledEntity::CompatibilityFlag::Milan);
					}
				}

				// Use the "update**" method, there are many things to do
				updateStreamOutputCounters(entity, streamIndex, validCounters, counters, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			}
			else
			{
				if (!processGetAecpDynamicInfoFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::GetStreamOutputCounters, streamIndex, 0u, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::StreamOutputCounters);
					return;
				}
			}

			// Got all expected dynamic information
			if (controlledEntity->gotAllExpectedDynamicInfo())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDynamicInfo);
				checkEnumerationSteps(controlledEntity.get());
			}
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
				updateConfigurationName(*controlledEntity, configurationIndex, configurationName, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			}
			else
			{
				if (!processGetDescriptorDynamicInfoFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ConfigurationName, 0u, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::ConfigurationName);
					return;
				}
			}

			// Got all expected descriptor dynamic information
			if (controlledEntity->gotAllExpectedDescriptorDynamicInfo())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDescriptorDynamicInfo);
				checkEnumerationSteps(controlledEntity.get());
			}
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
				updateAudioUnitName(*controlledEntity, configurationIndex, audioUnitIndex, audioUnitName, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			}
			else
			{
				if (!processGetDescriptorDynamicInfoFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::AudioUnitName, audioUnitIndex, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AudioUnitName);
					return;
				}
			}

			// Got all expected descriptor dynamic information
			if (controlledEntity->gotAllExpectedDescriptorDynamicInfo())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDescriptorDynamicInfo);
				checkEnumerationSteps(controlledEntity.get());
			}
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
				updateAudioUnitSamplingRate(*controlledEntity, audioUnitIndex, samplingRate, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			}
			else
			{
				if (!processGetDescriptorDynamicInfoFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::AudioUnitSamplingRate, audioUnitIndex, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AudioUnitSamplingRate);
					return;
				}
			}

			// Got all expected descriptor dynamic information
			if (controlledEntity->gotAllExpectedDescriptorDynamicInfo())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDescriptorDynamicInfo);
				checkEnumerationSteps(controlledEntity.get());
			}
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
				updateStreamInputName(*controlledEntity, configurationIndex, streamIndex, streamInputName, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			}
			else
			{
				if (!processGetDescriptorDynamicInfoFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::InputStreamName, streamIndex, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::InputStreamName);
					return;
				}
			}

			// Got all expected descriptor dynamic information
			if (controlledEntity->gotAllExpectedDescriptorDynamicInfo())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDescriptorDynamicInfo);
				checkEnumerationSteps(controlledEntity.get());
			}
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
				updateStreamInputFormat(*controlledEntity, streamIndex, streamFormat, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			}
			else
			{
				if (!processGetDescriptorDynamicInfoFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::InputStreamFormat, streamIndex, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::InputStreamFormat);
					return;
				}
			}

			// Got all expected descriptor dynamic information
			if (controlledEntity->gotAllExpectedDescriptorDynamicInfo())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDescriptorDynamicInfo);
				checkEnumerationSteps(controlledEntity.get());
			}
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
				updateStreamOutputName(*controlledEntity, configurationIndex, streamIndex, streamOutputName, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			}
			else
			{
				if (!processGetDescriptorDynamicInfoFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::OutputStreamName, streamIndex, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::OutputStreamName);
					return;
				}
			}

			// Got all expected descriptor dynamic information
			if (controlledEntity->gotAllExpectedDescriptorDynamicInfo())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDescriptorDynamicInfo);
				checkEnumerationSteps(controlledEntity.get());
			}
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
				updateStreamOutputFormat(*controlledEntity, streamIndex, streamFormat, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			}
			else
			{
				if (!processGetDescriptorDynamicInfoFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::OutputStreamFormat, streamIndex, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::OutputStreamFormat);
					return;
				}
			}

			// Got all expected descriptor dynamic information
			if (controlledEntity->gotAllExpectedDescriptorDynamicInfo())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDescriptorDynamicInfo);
				checkEnumerationSteps(controlledEntity.get());
			}
		}
	}
}

void ControllerImpl::onInputJackNameResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex, entity::model::AvdeccFixedString const& jackInputName) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onInputJackNameResult (ConfigurationIndex={} JackIndex={}): {}", configurationIndex, jackIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::InputJackName, jackIndex))
		{
			if (!!status)
			{
				updateJackInputName(*controlledEntity, configurationIndex, jackIndex, jackInputName, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			}
			else
			{
				if (!processGetDescriptorDynamicInfoFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::InputJackName, jackIndex, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::InputJackName);
					return;
				}
			}

			// Got all expected descriptor dynamic information
			if (controlledEntity->gotAllExpectedDescriptorDynamicInfo())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDescriptorDynamicInfo);
				checkEnumerationSteps(controlledEntity.get());
			}
		}
	}
}

void ControllerImpl::onOutputJackNameResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::JackIndex const jackIndex, entity::model::AvdeccFixedString const& jackOutputName) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onOutputJackNameResult (ConfigurationIndex={} JackIndex={}): {}", configurationIndex, jackIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::OutputJackName, jackIndex))
		{
			if (!!status)
			{
				updateJackOutputName(*controlledEntity, configurationIndex, jackIndex, jackOutputName, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			}
			else
			{
				if (!processGetDescriptorDynamicInfoFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::OutputJackName, jackIndex, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::OutputJackName);
					return;
				}
			}

			// Got all expected descriptor dynamic information
			if (controlledEntity->gotAllExpectedDescriptorDynamicInfo())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDescriptorDynamicInfo);
				checkEnumerationSteps(controlledEntity.get());
			}
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
				updateAvbInterfaceName(*controlledEntity, configurationIndex, avbInterfaceIndex, avbInterfaceName, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			}
			else
			{
				if (!processGetDescriptorDynamicInfoFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::AvbInterfaceName, avbInterfaceIndex, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AvbInterfaceName);
					return;
				}
			}

			// Got all expected descriptor dynamic information
			if (controlledEntity->gotAllExpectedDescriptorDynamicInfo())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDescriptorDynamicInfo);
				checkEnumerationSteps(controlledEntity.get());
			}
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
				updateClockSourceName(*controlledEntity, configurationIndex, clockSourceIndex, clockSourceName, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			}
			else
			{
				if (!processGetDescriptorDynamicInfoFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ClockSourceName, clockSourceIndex, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::ClockSourceName);
					return;
				}
			}

			// Got all expected descriptor dynamic information
			if (controlledEntity->gotAllExpectedDescriptorDynamicInfo())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDescriptorDynamicInfo);
				checkEnumerationSteps(controlledEntity.get());
			}
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
				updateMemoryObjectName(*controlledEntity, configurationIndex, memoryObjectIndex, memoryObjectName, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			}
			else
			{
				if (!processGetDescriptorDynamicInfoFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::MemoryObjectName, memoryObjectIndex, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::MemoryObjectName);
					return;
				}
			}

			// Got all expected descriptor dynamic information
			if (controlledEntity->gotAllExpectedDescriptorDynamicInfo())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDescriptorDynamicInfo);
				checkEnumerationSteps(controlledEntity.get());
			}
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
				updateMemoryObjectLength(*controlledEntity, configurationIndex, memoryObjectIndex, length, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			}
			else
			{
				if (!processGetDescriptorDynamicInfoFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::MemoryObjectLength, memoryObjectIndex, true))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::MemoryObjectLength);
					return;
				}
			}

			// Got all expected descriptor dynamic information
			if (controlledEntity->gotAllExpectedDescriptorDynamicInfo())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDescriptorDynamicInfo);
				checkEnumerationSteps(controlledEntity.get());
			}
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
				updateAudioClusterName(*controlledEntity, configurationIndex, audioClusterIndex, audioClusterName, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			}
			else
			{
				if (!processGetDescriptorDynamicInfoFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::AudioClusterName, audioClusterIndex, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::AudioClusterName);
					return;
				}
			}

			// Got all expected descriptor dynamic information
			if (controlledEntity->gotAllExpectedDescriptorDynamicInfo())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDescriptorDynamicInfo);
				checkEnumerationSteps(controlledEntity.get());
			}
		}
	}
}

void ControllerImpl::onControlNameResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::ControlIndex const controlIndex, entity::model::AvdeccFixedString const& controlName) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onControlNameResult (ConfigurationIndex={} ControlIndex={}): {}", configurationIndex, controlIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ControlName, controlIndex))
		{
			if (!!status)
			{
				updateControlName(*controlledEntity, configurationIndex, controlIndex, controlName, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			}
			else
			{
				if (!processGetDescriptorDynamicInfoFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ControlName, controlIndex, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::ControlName);
					return;
				}
			}

			// Got all expected descriptor dynamic information
			if (controlledEntity->gotAllExpectedDescriptorDynamicInfo())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDescriptorDynamicInfo);
				checkEnumerationSteps(controlledEntity.get());
			}
		}
	}
}

void ControllerImpl::onControlValuesResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ControlIndex const controlIndex, MemoryBuffer const& packedControlValues, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onControlValuesResult (ConfigurationIndex={} ControlIndex={}): {}", configurationIndex, controlIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ControlValues, controlIndex))
		{
			auto st = status;

			if (!!st)
			{
				// Use the "update**" method, there are many things to do
				if (!updateControlValues(*controlledEntity, controlIndex, packedControlValues, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull))
				{
					st = entity::ControllerEntity::AemCommandStatus::ProtocolError;
				}
			}

			if (!st && !processGetDescriptorDynamicInfoFailureStatus(st, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ControlValues, controlIndex, false))
			{
				controlledEntity->setGetFatalEnumerationError();
				notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::ControlValues);
				return;
			}

			// Got all expected descriptor dynamic information
			if (controlledEntity->gotAllExpectedDescriptorDynamicInfo())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDescriptorDynamicInfo);
				checkEnumerationSteps(controlledEntity.get());
			}
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
				updateClockDomainName(*controlledEntity, configurationIndex, clockDomainIndex, clockDomainName, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			}
			else
			{
				if (!processGetDescriptorDynamicInfoFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ClockDomainName, clockDomainIndex, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::ClockDomainName);
					return;
				}
			}

			// Got all expected descriptor dynamic information
			if (controlledEntity->gotAllExpectedDescriptorDynamicInfo())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDescriptorDynamicInfo);
				checkEnumerationSteps(controlledEntity.get());
			}
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
				updateClockSource(*controlledEntity, clockDomainIndex, clockSourceIndex, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			}
			else
			{
				if (!processGetDescriptorDynamicInfoFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::ClockDomainSourceIndex, clockDomainIndex, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::ClockDomainSourceIndex);
					return;
				}
			}

			// Got all expected descriptor dynamic information
			if (controlledEntity->gotAllExpectedDescriptorDynamicInfo())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDescriptorDynamicInfo);
				checkEnumerationSteps(controlledEntity.get());
			}
		}
	}
}

void ControllerImpl::onTimingNameResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::TimingIndex const timingIndex, entity::model::AvdeccFixedString const& timingName) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onTimingNameResult (ConfigurationIndex={} TimingIndex={}): {}", configurationIndex, timingIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::TimingName, timingIndex))
		{
			if (!!status)
			{
				updateTimingName(*controlledEntity, configurationIndex, timingIndex, timingName, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			}
			else
			{
				if (!processGetDescriptorDynamicInfoFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::TimingName, timingIndex, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::TimingName);
					return;
				}
			}

			// Got all expected descriptor dynamic information
			if (controlledEntity->gotAllExpectedDescriptorDynamicInfo())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDescriptorDynamicInfo);
				checkEnumerationSteps(controlledEntity.get());
			}
		}
	}
}

void ControllerImpl::onPtpInstanceNameResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpInstanceIndex const ptpInstanceIndex, entity::model::AvdeccFixedString const& ptpInstanceName) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onPtpInstanceNameResult (ConfigurationIndex={} PtpInstanceIndex={}): {}", configurationIndex, ptpInstanceIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::PtpInstanceName, ptpInstanceIndex))
		{
			if (!!status)
			{
				updatePtpInstanceName(*controlledEntity, configurationIndex, ptpInstanceIndex, ptpInstanceName, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			}
			else
			{
				if (!processGetDescriptorDynamicInfoFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::PtpInstanceName, ptpInstanceIndex, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::PtpInstanceName);
					return;
				}
			}

			// Got all expected descriptor dynamic information
			if (controlledEntity->gotAllExpectedDescriptorDynamicInfo())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDescriptorDynamicInfo);
				checkEnumerationSteps(controlledEntity.get());
			}
		}
	}
}

void ControllerImpl::onPtpPortNameResult(entity::controller::Interface const* const /*controller*/, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::PtpPortIndex const ptpPortIndex, entity::model::AvdeccFixedString const& ptpPortName) noexcept
{
	LOG_CONTROLLER_TRACE(entityID, "onPtpPortNameResult (ConfigurationIndex={} PtpPortIndex={}): {}", configurationIndex, ptpPortIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledEntity = getControlledEntityImplGuard(entityID);

	if (controlledEntity)
	{
		if (controlledEntity->checkAndClearExpectedDescriptorDynamicInfo(configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::PtpPortName, ptpPortIndex))
		{
			if (!!status)
			{
				updatePtpPortName(*controlledEntity, configurationIndex, ptpPortIndex, ptpPortName, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			}
			else
			{
				if (!processGetDescriptorDynamicInfoFailureStatus(status, controlledEntity.get(), configurationIndex, ControlledEntityImpl::DescriptorDynamicInfoType::PtpPortName, ptpPortIndex, false))
				{
					controlledEntity->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, controlledEntity.get(), QueryCommandError::PtpPortName);
					return;
				}
			}

			// Got all expected descriptor dynamic information
			if (controlledEntity->gotAllExpectedDescriptorDynamicInfo())
			{
				// Clear this enumeration step and check for next one
				controlledEntity->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDescriptorDynamicInfo);
				checkEnumerationSteps(controlledEntity.get());
			}
		}
	}
}

/* Connection Management Protocol (ACMP) handlers */
void ControllerImpl::onConnectStreamResult(entity::controller::Interface const* const /*controller*/, [[maybe_unused]] entity::model::StreamIdentification const& talkerStream, [[maybe_unused]] entity::model::StreamIdentification const& listenerStream, [[maybe_unused]] std::uint16_t const connectionCount, [[maybe_unused]] entity::ConnectionFlags const flags, [[maybe_unused]] entity::ControllerEntity::ControlStatus const status) noexcept
{
	LOG_CONTROLLER_TRACE(UniqueIdentifier::getNullUniqueIdentifier(), "onConnectStreamResult (TalkerID={} TalkerIndex={} ListenerID={} ListenerIndex={} ConnectionCount={} Flags={}): {}", utils::toHexString(talkerStream.entityID, true), talkerStream.streamIndex, utils::toHexString(listenerStream.entityID, true), listenerStream.streamIndex, connectionCount, utils::toHexString(utils::forceNumeric(flags.value()), true), entity::ControllerEntity::statusToString(status));
}

void ControllerImpl::onDisconnectStreamResult(entity::controller::Interface const* const /*controller*/, [[maybe_unused]] entity::model::StreamIdentification const& talkerStream, [[maybe_unused]] entity::model::StreamIdentification const& listenerStream, [[maybe_unused]] std::uint16_t const connectionCount, [[maybe_unused]] entity::ConnectionFlags const flags, [[maybe_unused]] entity::ControllerEntity::ControlStatus const status) noexcept
{
	LOG_CONTROLLER_TRACE(UniqueIdentifier::getNullUniqueIdentifier(), "onDisconnectStreamResult (TalkerID={} TalkerIndex={} ListenerID={} ListenerIndex={} ConnectionCount={} Flags={}): {}", utils::toHexString(talkerStream.entityID, true), talkerStream.streamIndex, utils::toHexString(listenerStream.entityID, true), listenerStream.streamIndex, connectionCount, utils::toHexString(utils::forceNumeric(flags.value()), true), entity::ControllerEntity::statusToString(status));
}

void ControllerImpl::onGetTalkerStreamStateResult(entity::controller::Interface const* const /*controller*/, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& /*listenerStream*/, [[maybe_unused]] std::uint16_t const connectionCount, entity::ConnectionFlags const /*flags*/, entity::ControllerEntity::ControlStatus const status, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	LOG_CONTROLLER_TRACE(UniqueIdentifier::getNullUniqueIdentifier(), "onGetTalkerStreamStateResult (TalkerID={} TalkerIndex={} ConnectionCount={} ConfigurationIndex={}): {}", utils::toHexString(talkerStream.entityID, true), talkerStream.streamIndex, connectionCount, configurationIndex, entity::ControllerEntity::statusToString(status));

	// Take a "scoped locked" shared copy of the ControlledEntity
	auto controlledTalker = getControlledEntityImplGuard(talkerStream.entityID);

	if (controlledTalker)
	{
		auto& talker = *controlledTalker;
		if (talker.checkAndClearExpectedDynamicInfo(configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamState, talkerStream.streamIndex))
		{
			if (!!status)
			{
				clearTalkerStreamConnections(&talker, talkerStream.streamIndex, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);

				// Milan device should return 0 as connectionCount
				if (talker.getCompatibilityFlags().test(ControlledEntity::CompatibilityFlag::Milan))
				{
					if (connectionCount != 0)
					{
						LOG_CONTROLLER_WARN(talker.getEntity().getEntityID(), "Milan device must advertise 0 connection_count in GET_TX_STATE_RESPONSE");
						removeCompatibilityFlag(this, talker, ControlledEntity::CompatibilityFlag::Milan);
					}
				}

				for (auto index = std::uint16_t(0); index < connectionCount; ++index)
				{
					queryInformation(&talker, configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamConnection, talkerStream, index);
				}
			}
			else
			{
				if (!processGetAcmpDynamicInfoFailureStatus(status, &talker, configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamState, talkerStream.streamIndex, false))
				{
					talker.setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, &talker, QueryCommandError::TalkerStreamState);
					return;
				}
			}

			// Got all expected dynamic information
			if (talker.gotAllExpectedDynamicInfo())
			{
				// Clear this enumeration step and check for next one
				talker.clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDynamicInfo);
				checkEnumerationSteps(&talker);
			}
		}
	}
}

void ControllerImpl::onGetListenerStreamStateResult(entity::controller::Interface const* const /*controller*/, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	LOG_CONTROLLER_TRACE(UniqueIdentifier::getNullUniqueIdentifier(), "onGetListenerStreamStateResult (ListenerID={} ListenerIndex={} ConnectionCount={} Flags={} ConfigurationIndex={}): {}", utils::toHexString(listenerStream.entityID, true), listenerStream.streamIndex, connectionCount, utils::toHexString(utils::forceNumeric(flags.value()), true), configurationIndex, entity::ControllerEntity::statusToString(status));

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
				if (!processGetAcmpDynamicInfoFailureStatus(status, listener.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamState, listenerStream.streamIndex, false))
				{
					listener->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, listener.get(), QueryCommandError::ListenerStreamState);
					return;
				}
			}

			// Got all expected dynamic information
			if (listener->gotAllExpectedDynamicInfo())
			{
				// Clear this enumeration step and check for next one
				listener->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDynamicInfo);
				checkEnumerationSteps(listener.get());
			}
		}
	}
}

void ControllerImpl::onGetTalkerStreamConnectionResult(entity::controller::Interface const* const /*controller*/, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, [[maybe_unused]] std::uint16_t const connectionCount, entity::ConnectionFlags const /*flags*/, entity::ControllerEntity::ControlStatus const status, entity::model::ConfigurationIndex const configurationIndex, std::uint16_t const connectionIndex) noexcept
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
				addTalkerStreamConnection(talker.get(), talkerStream.streamIndex, listenerStream, TreeModelAccessStrategy::NotFoundBehavior::LogAndReturnNull);
			}
			else
			{
				if (!processGetAcmpDynamicInfoFailureStatus(status, talker.get(), configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamConnection, talkerStream, connectionIndex, true))
				{
					talker->setGetFatalEnumerationError();
					notifyObserversMethod<Controller::Observer>(&Controller::Observer::onEntityQueryError, this, talker.get(), QueryCommandError::TalkerStreamConnection);
					return;
				}
			}

			// Got all expected dynamic information
			if (talker->gotAllExpectedDynamicInfo())
			{
				// Clear this enumeration step and check for next one
				talker->clearEnumerationStep(ControlledEntityImpl::EnumerationStep::GetDynamicInfo);
				checkEnumerationSteps(talker.get());
			}
		}
	}
}

} // namespace controller
} // namespace avdecc
} // namespace la
