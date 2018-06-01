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
#include "la/avdecc/logger.hpp"

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
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onEntityDescriptorResult(") + toHexString(entityID, true) + "," + entity::ControllerEntity::statusToString(status) + ")");

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
						Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("readConfigurationDescriptor(") + toHexString(entityID, true) + "," + std::to_string(index) + ")");
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
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onConfigurationDescriptorResult(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

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
									Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("readLocaleDescriptor(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(index) + ")");
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
									Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("readAudioUnitDescriptor(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(index) + ")");
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
									Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("getListenerStreamState(") + toHexString(entityID, true) + "," + std::to_string(index) + ")");
									controller->getListenerStreamState({ entityID, index }, std::bind(&ControllerImpl::onGetListenerStreamStateResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, configurationIndex));

									// Get Stream Info
									controlledEntity->setDynamicInfoExpected(configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamInfo, index);
									Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("getStreamInputInfo(") + toHexString(entityID, true) + "," + std::to_string(index) + ")");
									controller->getStreamInputInfo(entityID, index, std::bind(&ControllerImpl::onGetStreamInputInfoResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, configurationIndex));
									
									// Get Stream Descriptor
									controlledEntity->setDescriptorExpected(configurationIndex, entity::model::DescriptorType::StreamInput, index);
									Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("readStreamInputDescriptor(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(index) + ")");
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
									Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("getTalkerStreamState(") + toHexString(entityID, true) + "," + std::to_string(index) + ")");
									controller->getTalkerStreamState({ entityID, index }, std::bind(&ControllerImpl::onGetTalkerStreamStateResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6, configurationIndex));

									// Get Stream Info
									controlledEntity->setDynamicInfoExpected(configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamInfo, index);
									Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("getStreamOutputInfo(") + toHexString(entityID, true) + "," + std::to_string(index) + ")");
									controller->getStreamOutputInfo(entityID, index, std::bind(&ControllerImpl::onGetStreamOutputInfoResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, configurationIndex));

									// Get Stream Descriptor
									controlledEntity->setDescriptorExpected(configurationIndex, entity::model::DescriptorType::StreamOutput, index);
									Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("readStreamOutputDescriptor(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(index) + ")");
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
									Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("getAvbInfo(") + toHexString(entityID, true) + "," + std::to_string(index) + ")");
									controller->getAvbInfo(entityID, index, std::bind(&ControllerImpl::onGetAvbInfoResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, configurationIndex));

									// Get AVBInterface Descriptor
									controlledEntity->setDescriptorExpected(configurationIndex, entity::model::DescriptorType::AvbInterface, index);
									Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("readAvbInterfaceDescriptor(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(index) + ")");
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
									Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("readClockSourceDescriptor(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(index) + ")");
									controller->readClockSourceDescriptor(entityID, configurationIndex, index, std::bind(&ControllerImpl::onClockSourceDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
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
									Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("readClockDomainDescriptor(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(index) + ")");
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
									Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("readLocaleDescriptor(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(index) + ")");
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
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onAudioUnitDescriptorResult(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(audioUnitIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

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
								Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("readStreamPortInputDescriptor(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(index) + ")");
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
								Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("readStreamPortOutputDescriptor(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(index) + ")");
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
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onStreamInputDescriptorResult(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(streamIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

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
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onStreamOutputDescriptorResult(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(streamIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

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
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onAvbInterfaceDescriptorResult(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(interfaceIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

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
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onClockSourceDescriptorResult(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(clockIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

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

void ControllerImpl::onLocaleDescriptorResult(entity::ControllerEntity const* const controller, UniqueIdentifier const entityID, entity::ControllerEntity::AemCommandStatus const status, entity::model::ConfigurationIndex const configurationIndex, entity::model::LocaleIndex const localeIndex, entity::model::LocaleDescriptor const& descriptor) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onLocaleDescriptorResult(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(localeIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

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
								Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("readStringsDescriptor(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(localeNode->baseStringDescriptorIndex + index) + ")");
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
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onStringsDescriptorResult(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(stringsIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

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
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onStreamPortInputDescriptorResult(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(streamPortIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

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
								Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("readAudioClusterDescriptor(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(clusterIndex) + ")");
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
								Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("readAudioMapDescriptor(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(mapIndex) + ")");
								controller->readAudioMapDescriptor(entityID, configurationIndex, mapIndex, std::bind(&ControllerImpl::onAudioMapDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
							}
						}
						else
						{
							// TODO: Clause 7.4.44.3 recommands to Lock or Acquire the entity before getting the dynamic audio map
							controlledEntity->setDynamicInfoExpected(configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamAudioMappings, streamPortIndex);
							Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("getStreamPortInputAudioMap(") + toHexString(entityID, true) + "," + std::to_string(streamPortIndex) + ")");
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
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onStreamPortOutputDescriptorResult(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(streamPortIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

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
								Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("readAudioClusterDescriptor(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(clusterIndex) + ")");
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
								Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("readAudioMapDescriptor(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(mapIndex) + ")");
								controller->readAudioMapDescriptor(entityID, configurationIndex, mapIndex, std::bind(&ControllerImpl::onAudioMapDescriptorResult, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
							}
						}
						else
						{
							// TODO: Clause 7.4.44.3 recommands to Lock or Acquire the entity before getting the dynamic audio map
							controlledEntity->setDynamicInfoExpected(configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamAudioMappings, streamPortIndex);
							Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("getStreamPortOutputAudioMap(") + toHexString(entityID, true) + "," + std::to_string(streamPortIndex) + ")");
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
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onAudioClusterDescriptorResult(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(clusterIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

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
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onAudioMapDescriptorResult(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(mapIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

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
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onClockDomainDescriptorResult(") + toHexString(entityID, true) + "," + std::to_string(configurationIndex) + "," + std::to_string(clockDomainIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

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
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onGetStreamInputInfoResult(") + toHexString(entityID, true) + "," + std::to_string(streamIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

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
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onGetStreamOutputInfoResult(") + toHexString(entityID, true) + "," + std::to_string(streamIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

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
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onGetStreamPortInputAudioMapResult(") + toHexString(entityID, true) + "," + std::to_string(streamPortIndex) + "," + std::to_string(numberOfMaps) + "," + std::to_string(mapIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

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
						controlledEntity->clearPortInputStreamAudioMappings(configurationIndex, streamPortIndex);
					bool isComplete = mapIndex == (numberOfMaps - 1);
					controlledEntity->addPortInputStreamAudioMappings(configurationIndex, streamPortIndex, mappings);
					if (!isComplete)
					{
						controlledEntity->setDynamicInfoExpected(configurationIndex, ControlledEntityImpl::DynamicInfoType::InputStreamAudioMappings, streamPortIndex);
						Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("getStreamPortInputAudioMap(") + toHexString(entityID, true) + "," + std::to_string(streamPortIndex) + ")");
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
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onGetStreamPortOutputAudioMapResult(") + toHexString(entityID, true) + "," + std::to_string(streamPortIndex) + "," + std::to_string(numberOfMaps) + "," + std::to_string(mapIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

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
						controlledEntity->clearPortOutputStreamAudioMappings(configurationIndex, streamPortIndex);
					bool isComplete = mapIndex == (numberOfMaps - 1);
					controlledEntity->addPortOutputStreamAudioMappings(configurationIndex, streamPortIndex, mappings);
					if (!isComplete)
					{
						controlledEntity->setDynamicInfoExpected(configurationIndex, ControlledEntityImpl::DynamicInfoType::OutputStreamAudioMappings, streamPortIndex);
						Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("getStreamPortOutputAudioMap(") + toHexString(entityID, true) + "," + std::to_string(streamPortIndex) + ")");
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
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onGetAvbInfoResult(") + toHexString(entityID, true) + "," + std::to_string(avbInterfaceIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

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
void ControllerImpl::onConnectStreamResult(entity::ControllerEntity const* const /*controller*/, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onConnectStreamResult(") + toHexString(talkerStream.entityID, true) + "," + std::to_string(talkerStream.streamIndex) + "," + toHexString(listenerStream.entityID, true) + "," + std::to_string(listenerStream.streamIndex) + "," + std::to_string(connectionCount) + "," + toHexString(to_integral(flags), true) + "," + entity::ControllerEntity::statusToString(status) + ")");
}

void ControllerImpl::onDisconnectStreamResult(entity::ControllerEntity const* const /*controller*/, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, uint16_t const connectionCount, entity::ConnectionFlags const flags, entity::ControllerEntity::ControlStatus const status) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onDisconnectStreamResult(") + toHexString(talkerStream.entityID, true) + "," + std::to_string(talkerStream.streamIndex) + "," + toHexString(listenerStream.entityID, true) + "," + std::to_string(listenerStream.streamIndex) + "," + std::to_string(connectionCount) + "," + toHexString(to_integral(flags), true) + "," + entity::ControllerEntity::statusToString(status) + ")");
}

void ControllerImpl::onGetTalkerStreamStateResult(entity::ControllerEntity const* const controller, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& /*listenerStream*/, uint16_t const connectionCount, entity::ConnectionFlags const /*flags*/, entity::ControllerEntity::ControlStatus const status, entity::model::ConfigurationIndex const configurationIndex) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onGetTalkerStreamStateResult(") + toHexString(talkerStream.entityID, true) + "," + std::to_string(talkerStream.streamIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

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
					Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("getTalkerStreamConnection(") + toHexString(talkerStream.entityID, true) + "," + std::to_string(talkerStream.streamIndex) + "," + std::to_string(index) + ")");
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
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onGetListenerStreamStateResult(") + toHexString(listenerStream.entityID, true) + "," + std::to_string(listenerStream.streamIndex) + "," + entity::ControllerEntity::statusToString(status) + ")");

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

void ControllerImpl::onGetTalkerStreamConnectionResult(entity::ControllerEntity const* const /*controller*/, entity::model::StreamIdentification const& talkerStream, entity::model::StreamIdentification const& listenerStream, uint16_t const connectionCount, entity::ConnectionFlags const /*flags*/, entity::ControllerEntity::ControlStatus const status, entity::model::ConfigurationIndex const configurationIndex, std::uint16_t const connectionIndex) noexcept
{
	Logger::getInstance().log(Logger::Layer::Controller, Logger::Level::Trace, std::string("onGetTalkerStreamConnectionResult(") + toHexString(talkerStream.entityID, true) + "," + std::to_string(talkerStream.streamIndex) + "," + std::to_string(connectionIndex) + "," + std::to_string(connectionCount) + "," + entity::ControllerEntity::statusToString(status) + ")");

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
