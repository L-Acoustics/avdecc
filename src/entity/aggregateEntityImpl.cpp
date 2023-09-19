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
* @file aggregateEntityImpl.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/utils.hpp"

#include "logHelper.hpp"
#include "aggregateEntityImpl.hpp"
#include "controllerCapabilityDelegate.hpp"

#include <exception>
#include <cassert>
#include <chrono>
#include <unordered_map>
#include <algorithm>

namespace la
{
namespace avdecc
{
namespace entity
{
/* ************************************************************************** */
/* AggregateEntityImpl life cycle                                             */
/* ************************************************************************** */
AggregateEntityImpl::AggregateEntityImpl(protocol::ProtocolInterface* const protocolInterface, CommonInformation const& commonInformation, InterfacesInformation const& interfacesInformation, model::EntityTree const* const entityModelTree, controller::Delegate* const controllerDelegate)
	: LocalEntityImpl(protocolInterface, commonInformation, interfacesInformation)
{
	// Entity is controller capable
	if (commonInformation.controllerCapabilities.test(ControllerCapability::Implemented))
	{
		_controllerCapabilityDelegate = std::make_unique<controller::CapabilityDelegate>(getProtocolInterface(), controllerDelegate, *this, *this, entityModelTree);
	}

	// Entity is listener capable
	if (commonInformation.listenerCapabilities.test(ListenerCapability::Implemented))
	{
		AVDECC_ASSERT(false, "TODO: AggregateEntityImpl: Handle listener capability");
		//_listenerCapabilityDelegate = std::make_unique<listener::CapabilityDelegate>(entityID);
	}

	// Entity is talker capable
	if (commonInformation.talkerCapabilities.test(TalkerCapability::Implemented))
	{
		AVDECC_ASSERT(false, "TODO: AggregateEntityImpl: Handle talker capability");
		//_talkerCapabilityDelegate = std::make_unique<talker::CapabilityDelegate>(entityID);
	}

	// Register observer
	getProtocolInterface()->registerObserver(this);
}

AggregateEntityImpl::~AggregateEntityImpl() noexcept
{
	// Unregister ourself as a ProtocolInterface observer
	invokeProtectedMethod(&protocol::ProtocolInterface::unregisterObserver, getProtocolInterface(), this);

	// Remove all delegates
	_controllerCapabilityDelegate.reset();
	_listenerCapabilityDelegate.reset();
	_talkerCapabilityDelegate.reset();
}

void AggregateEntityImpl::destroy() noexcept
{
	delete this;
}

/* ************************************************************************** */
/* controller::Interface overrides                                            */
/* ************************************************************************** */
/* Discovery Protocol (ADP) */

/* Enumeration and Control Protocol (AECP) AEM */
void AggregateEntityImpl::acquireEntity(UniqueIdentifier const targetEntityID, bool const isPersistent, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, AcquireEntityHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).acquireEntity(targetEntityID, isPersistent, descriptorType, descriptorIndex, handler);
	}
}

void AggregateEntityImpl::releaseEntity(UniqueIdentifier const targetEntityID, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, ReleaseEntityHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).releaseEntity(targetEntityID, descriptorType, descriptorIndex, handler);
	}
}

void AggregateEntityImpl::lockEntity(UniqueIdentifier const targetEntityID, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, LockEntityHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).lockEntity(targetEntityID, descriptorType, descriptorIndex, handler);
	}
}

void AggregateEntityImpl::unlockEntity(UniqueIdentifier const targetEntityID, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, UnlockEntityHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).unlockEntity(targetEntityID, descriptorType, descriptorIndex, handler);
	}
}

void AggregateEntityImpl::queryEntityAvailable(UniqueIdentifier const targetEntityID, QueryEntityAvailableHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).queryEntityAvailable(targetEntityID, handler);
	}
}

void AggregateEntityImpl::queryControllerAvailable(UniqueIdentifier const targetEntityID, QueryControllerAvailableHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).queryControllerAvailable(targetEntityID, handler);
	}
}

void AggregateEntityImpl::registerUnsolicitedNotifications(UniqueIdentifier const targetEntityID, RegisterUnsolicitedNotificationsHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).registerUnsolicitedNotifications(targetEntityID, handler);
	}
}

void AggregateEntityImpl::unregisterUnsolicitedNotifications(UniqueIdentifier const targetEntityID, UnregisterUnsolicitedNotificationsHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).unregisterUnsolicitedNotifications(targetEntityID, handler);
	}
}

void AggregateEntityImpl::readEntityDescriptor(UniqueIdentifier const targetEntityID, EntityDescriptorHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readEntityDescriptor(targetEntityID, handler);
	}
}

void AggregateEntityImpl::readConfigurationDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, ConfigurationDescriptorHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readConfigurationDescriptor(targetEntityID, configurationIndex, handler);
	}
}

void AggregateEntityImpl::readAudioUnitDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AudioUnitIndex const audioUnitIndex, AudioUnitDescriptorHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readAudioUnitDescriptor(targetEntityID, configurationIndex, audioUnitIndex, handler);
	}
}

void AggregateEntityImpl::readStreamInputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, StreamInputDescriptorHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readStreamInputDescriptor(targetEntityID, configurationIndex, streamIndex, handler);
	}
}

void AggregateEntityImpl::readStreamOutputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, StreamOutputDescriptorHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readStreamOutputDescriptor(targetEntityID, configurationIndex, streamIndex, handler);
	}
}

void AggregateEntityImpl::readJackInputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::JackIndex const jackIndex, JackInputDescriptorHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readJackInputDescriptor(targetEntityID, configurationIndex, jackIndex, handler);
	}
}

void AggregateEntityImpl::readJackOutputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::JackIndex const jackIndex, JackOutputDescriptorHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readJackOutputDescriptor(targetEntityID, configurationIndex, jackIndex, handler);
	}
}

void AggregateEntityImpl::readAvbInterfaceDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AvbInterfaceIndex const avbInterfaceIndex, AvbInterfaceDescriptorHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readAvbInterfaceDescriptor(targetEntityID, configurationIndex, avbInterfaceIndex, handler);
	}
}

void AggregateEntityImpl::readClockSourceDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockSourceIndex const clockSourceIndex, ClockSourceDescriptorHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readClockSourceDescriptor(targetEntityID, configurationIndex, clockSourceIndex, handler);
	}
}

void AggregateEntityImpl::readMemoryObjectDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MemoryObjectIndex const memoryObjectIndex, MemoryObjectDescriptorHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readMemoryObjectDescriptor(targetEntityID, configurationIndex, memoryObjectIndex, handler);
	}
}

void AggregateEntityImpl::readLocaleDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::LocaleIndex const localeIndex, LocaleDescriptorHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readLocaleDescriptor(targetEntityID, configurationIndex, localeIndex, handler);
	}
}

void AggregateEntityImpl::readStringsDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StringsIndex const stringsIndex, StringsDescriptorHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readStringsDescriptor(targetEntityID, configurationIndex, stringsIndex, handler);
	}
}

void AggregateEntityImpl::readStreamPortInputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamPortIndex const streamPortIndex, StreamPortInputDescriptorHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readStreamPortInputDescriptor(targetEntityID, configurationIndex, streamPortIndex, handler);
	}
}

void AggregateEntityImpl::readStreamPortOutputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamPortIndex const streamPortIndex, StreamPortOutputDescriptorHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readStreamPortOutputDescriptor(targetEntityID, configurationIndex, streamPortIndex, handler);
	}
}

void AggregateEntityImpl::readExternalPortInputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ExternalPortIndex const externalPortIndex, ExternalPortInputDescriptorHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readExternalPortInputDescriptor(targetEntityID, configurationIndex, externalPortIndex, handler);
	}
}

void AggregateEntityImpl::readExternalPortOutputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ExternalPortIndex const externalPortIndex, ExternalPortOutputDescriptorHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readExternalPortOutputDescriptor(targetEntityID, configurationIndex, externalPortIndex, handler);
	}
}

void AggregateEntityImpl::readInternalPortInputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::InternalPortIndex const internalPortIndex, InternalPortInputDescriptorHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readInternalPortInputDescriptor(targetEntityID, configurationIndex, internalPortIndex, handler);
	}
}

void AggregateEntityImpl::readInternalPortOutputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::InternalPortIndex const internalPortIndex, InternalPortOutputDescriptorHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readInternalPortOutputDescriptor(targetEntityID, configurationIndex, internalPortIndex, handler);
	}
}

void AggregateEntityImpl::readAudioClusterDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClusterIndex const clusterIndex, AudioClusterDescriptorHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readAudioClusterDescriptor(targetEntityID, configurationIndex, clusterIndex, handler);
	}
}

void AggregateEntityImpl::readAudioMapDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MapIndex const mapIndex, AudioMapDescriptorHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readAudioMapDescriptor(targetEntityID, configurationIndex, mapIndex, handler);
	}
}

void AggregateEntityImpl::readControlDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ControlIndex const controlIndex, ControlDescriptorHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readControlDescriptor(targetEntityID, configurationIndex, controlIndex, handler);
	}
}

void AggregateEntityImpl::readClockDomainDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockDomainIndex const clockDomainIndex, ClockDomainDescriptorHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readClockDomainDescriptor(targetEntityID, configurationIndex, clockDomainIndex, handler);
	}
}

void AggregateEntityImpl::readTimingDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::TimingIndex const timingIndex, TimingDescriptorHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readTimingDescriptor(targetEntityID, configurationIndex, timingIndex, handler);
	}
}

void AggregateEntityImpl::readPtpInstanceDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::PtpInstanceIndex const ptpInstanceIndex, PtpInstanceDescriptorHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readPtpInstanceDescriptor(targetEntityID, configurationIndex, ptpInstanceIndex, handler);
	}
}

void AggregateEntityImpl::readPtpPortDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::PtpPortIndex const ptpPortIndex, PtpPortDescriptorHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readPtpPortDescriptor(targetEntityID, configurationIndex, ptpPortIndex, handler);
	}
}

void AggregateEntityImpl::setConfiguration(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, SetConfigurationHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setConfiguration(targetEntityID, configurationIndex, handler);
	}
}

void AggregateEntityImpl::getConfiguration(UniqueIdentifier const targetEntityID, GetConfigurationHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getConfiguration(targetEntityID, handler);
	}
}

void AggregateEntityImpl::setStreamInputFormat(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, model::StreamFormat const streamFormat, SetStreamInputFormatHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setStreamInputFormat(targetEntityID, streamIndex, streamFormat, handler);
	}
}

void AggregateEntityImpl::getStreamInputFormat(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, GetStreamInputFormatHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getStreamInputFormat(targetEntityID, streamIndex, handler);
	}
}

void AggregateEntityImpl::setStreamOutputFormat(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, model::StreamFormat const streamFormat, SetStreamOutputFormatHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setStreamOutputFormat(targetEntityID, streamIndex, streamFormat, handler);
	}
}

void AggregateEntityImpl::getStreamOutputFormat(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, GetStreamOutputFormatHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getStreamOutputFormat(targetEntityID, streamIndex, handler);
	}
}

void AggregateEntityImpl::getStreamPortInputAudioMap(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::MapIndex const mapIndex, GetStreamPortInputAudioMapHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getStreamPortInputAudioMap(targetEntityID, streamPortIndex, mapIndex, handler);
	}
}

void AggregateEntityImpl::getStreamPortOutputAudioMap(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::MapIndex const mapIndex, GetStreamPortOutputAudioMapHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getStreamPortOutputAudioMap(targetEntityID, streamPortIndex, mapIndex, handler);
	}
}

void AggregateEntityImpl::addStreamPortInputAudioMappings(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::AudioMappings const& mappings, AddStreamPortInputAudioMappingsHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).addStreamPortInputAudioMappings(targetEntityID, streamPortIndex, mappings, handler);
	}
}

void AggregateEntityImpl::addStreamPortOutputAudioMappings(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::AudioMappings const& mappings, AddStreamPortOutputAudioMappingsHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).addStreamPortOutputAudioMappings(targetEntityID, streamPortIndex, mappings, handler);
	}
}

void AggregateEntityImpl::removeStreamPortInputAudioMappings(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::AudioMappings const& mappings, RemoveStreamPortInputAudioMappingsHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).removeStreamPortInputAudioMappings(targetEntityID, streamPortIndex, mappings, handler);
	}
}

void AggregateEntityImpl::removeStreamPortOutputAudioMappings(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::AudioMappings const& mappings, RemoveStreamPortOutputAudioMappingsHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).removeStreamPortOutputAudioMappings(targetEntityID, streamPortIndex, mappings, handler);
	}
}

void AggregateEntityImpl::setStreamInputInfo(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, model::StreamInfo const& info, SetStreamInputInfoHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setStreamInputInfo(targetEntityID, streamIndex, info, handler);
	}
}

void AggregateEntityImpl::setStreamOutputInfo(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, model::StreamInfo const& info, SetStreamOutputInfoHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setStreamOutputInfo(targetEntityID, streamIndex, info, handler);
	}
}

void AggregateEntityImpl::getStreamInputInfo(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, GetStreamInputInfoHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getStreamInputInfo(targetEntityID, streamIndex, handler);
	}
}

void AggregateEntityImpl::getStreamOutputInfo(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, GetStreamOutputInfoHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getStreamOutputInfo(targetEntityID, streamIndex, handler);
	}
}

void AggregateEntityImpl::setEntityName(UniqueIdentifier const targetEntityID, model::AvdeccFixedString const& entityName, SetEntityNameHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setEntityName(targetEntityID, entityName, handler);
	}
}

void AggregateEntityImpl::getEntityName(UniqueIdentifier const targetEntityID, GetEntityNameHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getEntityName(targetEntityID, handler);
	}
}

void AggregateEntityImpl::setEntityGroupName(UniqueIdentifier const targetEntityID, model::AvdeccFixedString const& entityGroupName, SetEntityGroupNameHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setEntityGroupName(targetEntityID, entityGroupName, handler);
	}
}

void AggregateEntityImpl::getEntityGroupName(UniqueIdentifier const targetEntityID, GetEntityGroupNameHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getEntityGroupName(targetEntityID, handler);
	}
}

void AggregateEntityImpl::setConfigurationName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AvdeccFixedString const& configurationName, SetConfigurationNameHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setConfigurationName(targetEntityID, configurationIndex, configurationName, handler);
	}
}

void AggregateEntityImpl::getConfigurationName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, GetConfigurationNameHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getConfigurationName(targetEntityID, configurationIndex, handler);
	}
}

void AggregateEntityImpl::setAudioUnitName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AudioUnitIndex const audioUnitIndex, model::AvdeccFixedString const& audioUnitName, SetAudioUnitNameHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setAudioUnitName(targetEntityID, configurationIndex, audioUnitIndex, audioUnitName, handler);
	}
}

void AggregateEntityImpl::getAudioUnitName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AudioUnitIndex const audioUnitIndex, GetAudioUnitNameHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getAudioUnitName(targetEntityID, configurationIndex, audioUnitIndex, handler);
	}
}

void AggregateEntityImpl::setStreamInputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, model::AvdeccFixedString const& streamInputName, SetStreamInputNameHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setStreamInputName(targetEntityID, configurationIndex, streamIndex, streamInputName, handler);
	}
}

void AggregateEntityImpl::getStreamInputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, GetStreamInputNameHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getStreamInputName(targetEntityID, configurationIndex, streamIndex, handler);
	}
}

void AggregateEntityImpl::setStreamOutputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, model::AvdeccFixedString const& streamOutputName, SetStreamOutputNameHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setStreamOutputName(targetEntityID, configurationIndex, streamIndex, streamOutputName, handler);
	}
}

void AggregateEntityImpl::getStreamOutputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, GetStreamOutputNameHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getStreamOutputName(targetEntityID, configurationIndex, streamIndex, handler);
	}
}

void AggregateEntityImpl::setJackInputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::JackIndex const jackIndex, model::AvdeccFixedString const& jackInputName, SetJackInputNameHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setJackInputName(targetEntityID, configurationIndex, jackIndex, jackInputName, handler);
	}
}

void AggregateEntityImpl::getJackInputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::JackIndex const jackIndex, GetJackInputNameHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getJackInputName(targetEntityID, configurationIndex, jackIndex, handler);
	}
}

void AggregateEntityImpl::setJackOutputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::JackIndex const jackIndex, model::AvdeccFixedString const& jackOutputName, SetJackOutputNameHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setJackOutputName(targetEntityID, configurationIndex, jackIndex, jackOutputName, handler);
	}
}

void AggregateEntityImpl::getJackOutputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::JackIndex const jackIndex, GetJackOutputNameHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getJackOutputName(targetEntityID, configurationIndex, jackIndex, handler);
	}
}

void AggregateEntityImpl::setAvbInterfaceName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AvbInterfaceIndex const avbInterfaceIndex, model::AvdeccFixedString const& avbInterfaceName, SetAvbInterfaceNameHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setAvbInterfaceName(targetEntityID, configurationIndex, avbInterfaceIndex, avbInterfaceName, handler);
	}
}

void AggregateEntityImpl::getAvbInterfaceName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AvbInterfaceIndex const avbInterfaceIndex, GetAvbInterfaceNameHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getAvbInterfaceName(targetEntityID, configurationIndex, avbInterfaceIndex, handler);
	}
}

void AggregateEntityImpl::setClockSourceName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockSourceIndex const clockSourceIndex, model::AvdeccFixedString const& clockSourceName, SetClockSourceNameHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setClockSourceName(targetEntityID, configurationIndex, clockSourceIndex, clockSourceName, handler);
	}
}

void AggregateEntityImpl::getClockSourceName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockSourceIndex const clockSourceIndex, GetClockSourceNameHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getClockSourceName(targetEntityID, configurationIndex, clockSourceIndex, handler);
	}
}

void AggregateEntityImpl::setMemoryObjectName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MemoryObjectIndex const memoryObjectIndex, model::AvdeccFixedString const& memoryObjectName, SetMemoryObjectNameHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setMemoryObjectName(targetEntityID, configurationIndex, memoryObjectIndex, memoryObjectName, handler);
	}
}

void AggregateEntityImpl::getMemoryObjectName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MemoryObjectIndex const memoryObjectIndex, GetMemoryObjectNameHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getMemoryObjectName(targetEntityID, configurationIndex, memoryObjectIndex, handler);
	}
}

void AggregateEntityImpl::setAudioClusterName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClusterIndex const audioClusterIndex, model::AvdeccFixedString const& audioClusterName, SetAudioClusterNameHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setAudioClusterName(targetEntityID, configurationIndex, audioClusterIndex, audioClusterName, handler);
	}
}

void AggregateEntityImpl::getAudioClusterName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClusterIndex const audioClusterIndex, GetAudioClusterNameHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getAudioClusterName(targetEntityID, configurationIndex, audioClusterIndex, handler);
	}
}

void AggregateEntityImpl::setControlName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ControlIndex const controlIndex, model::AvdeccFixedString const& controlName, SetControlNameHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setControlName(targetEntityID, configurationIndex, controlIndex, controlName, handler);
	}
}

void AggregateEntityImpl::getControlName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ControlIndex const controlIndex, GetControlNameHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getControlName(targetEntityID, configurationIndex, controlIndex, handler);
	}
}

void AggregateEntityImpl::setClockDomainName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockDomainIndex const clockDomainIndex, model::AvdeccFixedString const& clockDomainName, SetClockDomainNameHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setClockDomainName(targetEntityID, configurationIndex, clockDomainIndex, clockDomainName, handler);
	}
}

void AggregateEntityImpl::getClockDomainName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockDomainIndex const clockDomainIndex, GetClockDomainNameHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getClockDomainName(targetEntityID, configurationIndex, clockDomainIndex, handler);
	}
}

void AggregateEntityImpl::setTimingName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::TimingIndex const timingIndex, model::AvdeccFixedString const& timingName, SetTimingNameHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setTimingName(targetEntityID, configurationIndex, timingIndex, timingName, handler);
	}
}

void AggregateEntityImpl::getTimingName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::TimingIndex const timingIndex, GetTimingNameHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getTimingName(targetEntityID, configurationIndex, timingIndex, handler);
	}
}

void AggregateEntityImpl::setPtpInstanceName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::PtpInstanceIndex const ptpInstanceIndex, model::AvdeccFixedString const& ptpInstanceName, SetPtpInstanceNameHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setPtpInstanceName(targetEntityID, configurationIndex, ptpInstanceIndex, ptpInstanceName, handler);
	}
}

void AggregateEntityImpl::getPtpInstanceName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::PtpInstanceIndex const ptpInstanceIndex, GetPtpInstanceNameHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getPtpInstanceName(targetEntityID, configurationIndex, ptpInstanceIndex, handler);
	}
}

void AggregateEntityImpl::setPtpPortName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::PtpPortIndex const ptpPortIndex, model::AvdeccFixedString const& ptpPortName, SetPtpPortNameHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setPtpPortName(targetEntityID, configurationIndex, ptpPortIndex, ptpPortName, handler);
	}
}

void AggregateEntityImpl::getPtpPortName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::PtpPortIndex const ptpPortIndex, GetPtpPortNameHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getPtpPortName(targetEntityID, configurationIndex, ptpPortIndex, handler);
	}
}

void AggregateEntityImpl::setAssociation(UniqueIdentifier const targetEntityID, UniqueIdentifier const associationID, SetAssociationHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setAssociationID(targetEntityID, associationID, handler);
	}
}

void AggregateEntityImpl::getAssociation(UniqueIdentifier const targetEntityID, GetAssociationHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getAssociationID(targetEntityID, handler);
	}
}

void AggregateEntityImpl::setAudioUnitSamplingRate(UniqueIdentifier const targetEntityID, model::AudioUnitIndex const audioUnitIndex, model::SamplingRate const samplingRate, SetAudioUnitSamplingRateHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setAudioUnitSamplingRate(targetEntityID, audioUnitIndex, samplingRate, handler);
	}
}

void AggregateEntityImpl::getAudioUnitSamplingRate(UniqueIdentifier const targetEntityID, model::AudioUnitIndex const audioUnitIndex, GetAudioUnitSamplingRateHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getAudioUnitSamplingRate(targetEntityID, audioUnitIndex, handler);
	}
}

void AggregateEntityImpl::setVideoClusterSamplingRate(UniqueIdentifier const targetEntityID, model::ClusterIndex const videoClusterIndex, model::SamplingRate const samplingRate, SetVideoClusterSamplingRateHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setVideoClusterSamplingRate(targetEntityID, videoClusterIndex, samplingRate, handler);
	}
}

void AggregateEntityImpl::getVideoClusterSamplingRate(UniqueIdentifier const targetEntityID, model::ClusterIndex const videoClusterIndex, GetVideoClusterSamplingRateHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getVideoClusterSamplingRate(targetEntityID, videoClusterIndex, handler);
	}
}

void AggregateEntityImpl::setSensorClusterSamplingRate(UniqueIdentifier const targetEntityID, model::ClusterIndex const sensorClusterIndex, model::SamplingRate const samplingRate, SetSensorClusterSamplingRateHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setSensorClusterSamplingRate(targetEntityID, sensorClusterIndex, samplingRate, handler);
	}
}

void AggregateEntityImpl::getSensorClusterSamplingRate(UniqueIdentifier const targetEntityID, model::ClusterIndex const sensorClusterIndex, GetSensorClusterSamplingRateHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getSensorClusterSamplingRate(targetEntityID, sensorClusterIndex, handler);
	}
}

void AggregateEntityImpl::setClockSource(UniqueIdentifier const targetEntityID, model::ClockDomainIndex const clockDomainIndex, model::ClockSourceIndex const clockSourceIndex, SetClockSourceHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setClockSource(targetEntityID, clockDomainIndex, clockSourceIndex, handler);
	}
}

void AggregateEntityImpl::getClockSource(UniqueIdentifier const targetEntityID, model::ClockDomainIndex const clockDomainIndex, GetClockSourceHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getClockSource(targetEntityID, clockDomainIndex, handler);
	}
}

void AggregateEntityImpl::setControlValues(UniqueIdentifier const targetEntityID, model::ControlIndex const controlIndex, model::ControlValues const& controlValues, SetControlValuesHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setControlValues(targetEntityID, controlIndex, controlValues, handler);
	}
}

void AggregateEntityImpl::getControlValues(UniqueIdentifier const targetEntityID, model::ControlIndex const controlIndex, GetControlValuesHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getControlValues(targetEntityID, controlIndex, handler);
	}
}

void AggregateEntityImpl::startStreamInput(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, StartStreamInputHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).startStreamInput(targetEntityID, streamIndex, handler);
	}
}

void AggregateEntityImpl::startStreamOutput(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, StartStreamOutputHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).startStreamOutput(targetEntityID, streamIndex, handler);
	}
}

void AggregateEntityImpl::stopStreamInput(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, StopStreamInputHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).stopStreamInput(targetEntityID, streamIndex, handler);
	}
}

void AggregateEntityImpl::stopStreamOutput(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, StopStreamOutputHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).stopStreamOutput(targetEntityID, streamIndex, handler);
	}
}

void AggregateEntityImpl::getAvbInfo(UniqueIdentifier const targetEntityID, model::AvbInterfaceIndex const avbInterfaceIndex, GetAvbInfoHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getAvbInfo(targetEntityID, avbInterfaceIndex, handler);
	}
}

void AggregateEntityImpl::getAsPath(UniqueIdentifier const targetEntityID, model::AvbInterfaceIndex const avbInterfaceIndex, GetAsPathHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getAsPath(targetEntityID, avbInterfaceIndex, handler);
	}
}

void AggregateEntityImpl::getEntityCounters(UniqueIdentifier const targetEntityID, GetEntityCountersHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getEntityCounters(targetEntityID, handler);
	}
}

void AggregateEntityImpl::getAvbInterfaceCounters(UniqueIdentifier const targetEntityID, model::AvbInterfaceIndex const avbInterfaceIndex, GetAvbInterfaceCountersHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getAvbInterfaceCounters(targetEntityID, avbInterfaceIndex, handler);
	}
}

void AggregateEntityImpl::getClockDomainCounters(UniqueIdentifier const targetEntityID, model::ClockDomainIndex const clockDomainIndex, GetClockDomainCountersHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getClockDomainCounters(targetEntityID, clockDomainIndex, handler);
	}
}

void AggregateEntityImpl::getStreamInputCounters(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, GetStreamInputCountersHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getStreamInputCounters(targetEntityID, streamIndex, handler);
	}
}

void AggregateEntityImpl::getStreamOutputCounters(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, GetStreamOutputCountersHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getStreamOutputCounters(targetEntityID, streamIndex, handler);
	}
}

void AggregateEntityImpl::reboot(UniqueIdentifier const targetEntityID, RebootHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).reboot(targetEntityID, handler);
	}
}

void AggregateEntityImpl::rebootToFirmware(UniqueIdentifier const targetEntityID, model::MemoryObjectIndex const memoryObjectIndex, RebootToFirmwareHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).rebootToFirmware(targetEntityID, memoryObjectIndex, handler);
	}
}

void AggregateEntityImpl::startOperation(UniqueIdentifier const targetEntityID, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, model::MemoryObjectOperationType const operationType, MemoryBuffer const& memoryBuffer, StartOperationHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).startOperation(targetEntityID, descriptorType, descriptorIndex, operationType, memoryBuffer, handler);
	}
}

void AggregateEntityImpl::abortOperation(UniqueIdentifier const targetEntityID, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, model::OperationID const operationID, AbortOperationHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).abortOperation(targetEntityID, descriptorType, descriptorIndex, operationID, handler);
	}
}

void AggregateEntityImpl::setMemoryObjectLength(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length, SetMemoryObjectLengthHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setMemoryObjectLength(targetEntityID, configurationIndex, memoryObjectIndex, length, handler);
	}
}

void AggregateEntityImpl::getMemoryObjectLength(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MemoryObjectIndex const memoryObjectIndex, GetMemoryObjectLengthHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getMemoryObjectLength(targetEntityID, configurationIndex, memoryObjectIndex, handler);
	}
}

/* Enumeration and Control Protocol (AECP) AA */
void AggregateEntityImpl::addressAccess(UniqueIdentifier const targetEntityID, addressAccess::Tlvs const& tlvs, AddressAccessHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).addressAccess(targetEntityID, tlvs, handler);
	}
}

/* Enumeration and Control Protocol (AECP) MVU (Milan Vendor Unique) */
void AggregateEntityImpl::getMilanInfo(UniqueIdentifier const targetEntityID, GetMilanInfoHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getMilanInfo(targetEntityID, handler);
	}
}

/* Connection Management Protocol (ACMP) */
void AggregateEntityImpl::connectStream(model::StreamIdentification const& talkerStream, model::StreamIdentification const& listenerStream, ConnectStreamHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).connectStream(talkerStream, listenerStream, handler);
	}
}

void AggregateEntityImpl::disconnectStream(model::StreamIdentification const& talkerStream, model::StreamIdentification const& listenerStream, DisconnectStreamHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).disconnectStream(talkerStream, listenerStream, handler);
	}
}

void AggregateEntityImpl::disconnectTalkerStream(model::StreamIdentification const& talkerStream, model::StreamIdentification const& listenerStream, DisconnectTalkerStreamHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).disconnectTalkerStream(talkerStream, listenerStream, handler);
	}
}

void AggregateEntityImpl::getTalkerStreamState(model::StreamIdentification const& talkerStream, GetTalkerStreamStateHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getTalkerStreamState(talkerStream, handler);
	}
}

void AggregateEntityImpl::getListenerStreamState(model::StreamIdentification const& listenerStream, GetListenerStreamStateHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getListenerStreamState(listenerStream, handler);
	}
}

void AggregateEntityImpl::getTalkerStreamConnection(model::StreamIdentification const& talkerStream, std::uint16_t const connectionIndex, GetTalkerStreamConnectionHandler const& handler) const noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getTalkerStreamConnection(talkerStream, connectionIndex, handler);
	}
}

/* ************************************************************************** */
/* AggregateEntity overrides                                                  */
/* ************************************************************************** */
void AggregateEntityImpl::setControllerDelegate(controller::Delegate* const delegate) noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_controllerCapabilityDelegate != nullptr, "Controller method should have a valid ControllerCapabilityDelegate"))
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setControllerDelegate(delegate);
	}
}

/*void AggregateEntityImpl::setListenerDelegate(listener::Delegate* const delegate) noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_listenerCapabilityDelegate != nullptr, "Listener method should have a valid ListenerCapabilityDelegate"))
	{
		static_cast<listener::CapabilityDelegate&>(*_listenerCapabilityDelegate).setListenerDelegate(delegate);
	}
}

void AggregateEntityImpl::setTalkerDelegate(talker::Delegate* const delegate) noexcept
{
	if (AVDECC_ASSERT_WITH_RET(_talkerCapabilityDelegate != nullptr, "Talker method should have a valid TalkerCapabilityDelegate"))
	{
		static_cast<talker::CapabilityDelegate&>(*_talkerCapabilityDelegate).setTalkerDelegate(delegate);
	}
}*/

/* ************************************************************************** */
/* protocol::ProtocolInterface::Observer overrides                            */
/* ************************************************************************** */
/* **** Global notifications **** */
void AggregateEntityImpl::onTransportError(protocol::ProtocolInterface* const pi) noexcept
{
	if (_controllerCapabilityDelegate != nullptr)
	{
		_controllerCapabilityDelegate->onTransportError(pi);
	}
	if (_listenerCapabilityDelegate != nullptr)
	{
		_listenerCapabilityDelegate->onTransportError(pi);
	}
	if (_talkerCapabilityDelegate != nullptr)
	{
		_talkerCapabilityDelegate->onTransportError(pi);
	}
}

/* **** Discovery notifications **** */
void AggregateEntityImpl::onLocalEntityOnline(protocol::ProtocolInterface* const pi, Entity const& entity) noexcept
{
	if (_controllerCapabilityDelegate != nullptr)
	{
		_controllerCapabilityDelegate->onLocalEntityOnline(pi, entity);
	}
	if (_listenerCapabilityDelegate != nullptr)
	{
		_listenerCapabilityDelegate->onLocalEntityOnline(pi, entity);
	}
	if (_talkerCapabilityDelegate != nullptr)
	{
		_talkerCapabilityDelegate->onLocalEntityOnline(pi, entity);
	}
}

void AggregateEntityImpl::onLocalEntityOffline(protocol::ProtocolInterface* const pi, UniqueIdentifier const entityID) noexcept
{
	if (_controllerCapabilityDelegate != nullptr)
	{
		_controllerCapabilityDelegate->onLocalEntityOffline(pi, entityID);
	}
	if (_listenerCapabilityDelegate != nullptr)
	{
		_listenerCapabilityDelegate->onLocalEntityOffline(pi, entityID);
	}
	if (_talkerCapabilityDelegate != nullptr)
	{
		_talkerCapabilityDelegate->onLocalEntityOffline(pi, entityID);
	}
}

void AggregateEntityImpl::onLocalEntityUpdated(protocol::ProtocolInterface* const pi, Entity const& entity) noexcept
{
	if (_controllerCapabilityDelegate != nullptr)
	{
		_controllerCapabilityDelegate->onLocalEntityUpdated(pi, entity);
	}
	if (_listenerCapabilityDelegate != nullptr)
	{
		_listenerCapabilityDelegate->onLocalEntityUpdated(pi, entity);
	}
	if (_talkerCapabilityDelegate != nullptr)
	{
		_talkerCapabilityDelegate->onLocalEntityUpdated(pi, entity);
	}
}

void AggregateEntityImpl::onRemoteEntityOnline(protocol::ProtocolInterface* const pi, Entity const& entity) noexcept
{
	if (_controllerCapabilityDelegate != nullptr)
	{
		_controllerCapabilityDelegate->onRemoteEntityOnline(pi, entity);
	}
	if (_listenerCapabilityDelegate != nullptr)
	{
		_listenerCapabilityDelegate->onRemoteEntityOnline(pi, entity);
	}
	if (_talkerCapabilityDelegate != nullptr)
	{
		_talkerCapabilityDelegate->onRemoteEntityOnline(pi, entity);
	}
}

void AggregateEntityImpl::onRemoteEntityOffline(protocol::ProtocolInterface* const pi, UniqueIdentifier const entityID) noexcept
{
	if (_controllerCapabilityDelegate != nullptr)
	{
		_controllerCapabilityDelegate->onRemoteEntityOffline(pi, entityID);
	}
	if (_listenerCapabilityDelegate != nullptr)
	{
		_listenerCapabilityDelegate->onRemoteEntityOffline(pi, entityID);
	}
	if (_talkerCapabilityDelegate != nullptr)
	{
		_talkerCapabilityDelegate->onRemoteEntityOffline(pi, entityID);
	}
}

void AggregateEntityImpl::onRemoteEntityUpdated(protocol::ProtocolInterface* const pi, Entity const& entity) noexcept
{
	if (_controllerCapabilityDelegate != nullptr)
	{
		_controllerCapabilityDelegate->onRemoteEntityUpdated(pi, entity);
	}
	if (_listenerCapabilityDelegate != nullptr)
	{
		_listenerCapabilityDelegate->onRemoteEntityUpdated(pi, entity);
	}
	if (_talkerCapabilityDelegate != nullptr)
	{
		_talkerCapabilityDelegate->onRemoteEntityUpdated(pi, entity);
	}
}

/* **** AECP notifications **** */
void AggregateEntityImpl::onAecpAemUnsolicitedResponse(protocol::ProtocolInterface* const pi, protocol::AemAecpdu const& aecpdu) noexcept
{
	if (_controllerCapabilityDelegate != nullptr)
	{
		_controllerCapabilityDelegate->onAecpAemUnsolicitedResponse(pi, aecpdu);
	}
	if (_listenerCapabilityDelegate != nullptr)
	{
		_listenerCapabilityDelegate->onAecpAemUnsolicitedResponse(pi, aecpdu);
	}
	if (_talkerCapabilityDelegate != nullptr)
	{
		_talkerCapabilityDelegate->onAecpAemUnsolicitedResponse(pi, aecpdu);
	}
}
void AggregateEntityImpl::onAecpAemIdentifyNotification(protocol::ProtocolInterface* const pi, protocol::AemAecpdu const& aecpdu) noexcept
{
	if (_controllerCapabilityDelegate != nullptr)
	{
		_controllerCapabilityDelegate->onAecpAemIdentifyNotification(pi, aecpdu);
	}
	if (_listenerCapabilityDelegate != nullptr)
	{
		_listenerCapabilityDelegate->onAecpAemIdentifyNotification(pi, aecpdu);
	}
	if (_talkerCapabilityDelegate != nullptr)
	{
		_talkerCapabilityDelegate->onAecpAemIdentifyNotification(pi, aecpdu);
	}
}

/* **** ACMP notifications **** */
void AggregateEntityImpl::onAcmpCommand(protocol::ProtocolInterface* const pi, protocol::Acmpdu const& acmpdu) noexcept
{
	if (_controllerCapabilityDelegate != nullptr)
	{
		_controllerCapabilityDelegate->onAcmpCommand(pi, acmpdu);
	}
	if (_listenerCapabilityDelegate != nullptr)
	{
		_listenerCapabilityDelegate->onAcmpCommand(pi, acmpdu);
	}
	if (_talkerCapabilityDelegate != nullptr)
	{
		_talkerCapabilityDelegate->onAcmpCommand(pi, acmpdu);
	}
}

void AggregateEntityImpl::onAcmpResponse(protocol::ProtocolInterface* const pi, protocol::Acmpdu const& acmpdu) noexcept
{
	if (_controllerCapabilityDelegate != nullptr)
	{
		_controllerCapabilityDelegate->onAcmpResponse(pi, acmpdu);
	}
	if (_listenerCapabilityDelegate != nullptr)
	{
		_listenerCapabilityDelegate->onAcmpResponse(pi, acmpdu);
	}
	if (_talkerCapabilityDelegate != nullptr)
	{
		_talkerCapabilityDelegate->onAcmpResponse(pi, acmpdu);
	}
}

/* **** Statistics **** */
void AggregateEntityImpl::onAecpRetry(protocol::ProtocolInterface* const pi, UniqueIdentifier const& entityID) noexcept
{
	if (_controllerCapabilityDelegate != nullptr)
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).onAecpRetry(pi, entityID);
	}
	// Listener and Talker don't implement retry mechanism
}

void AggregateEntityImpl::onAecpTimeout(protocol::ProtocolInterface* const pi, UniqueIdentifier const& entityID) noexcept
{
	if (_controllerCapabilityDelegate != nullptr)
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).onAecpTimeout(pi, entityID);
	}
	// Listener and Talker don't implement retry mechanism
}

void AggregateEntityImpl::onAecpUnexpectedResponse(protocol::ProtocolInterface* const pi, UniqueIdentifier const& entityID) noexcept
{
	if (_controllerCapabilityDelegate != nullptr)
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).onAecpUnexpectedResponse(pi, entityID);
	}
	// Listener and Talker don't implement retry mechanism
}

void AggregateEntityImpl::onAecpResponseTime(protocol::ProtocolInterface* const pi, UniqueIdentifier const& entityID, std::chrono::milliseconds const& responseTime) noexcept
{
	if (_controllerCapabilityDelegate != nullptr)
	{
		static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).onAecpResponseTime(pi, entityID, responseTime);
	}
	// Listener and Talker don't really care about statistics
}

/* ************************************************************************** */
/* LocalEntityImpl overrides                                                  */
/* ************************************************************************** */
bool AggregateEntityImpl::onUnhandledAecpCommand(protocol::ProtocolInterface* const pi, protocol::Aecpdu const& aecpdu) noexcept
{
	auto handled = false;

	if (!handled && _controllerCapabilityDelegate != nullptr)
	{
		handled = _controllerCapabilityDelegate->onUnhandledAecpCommand(pi, aecpdu);
	}
	if (!handled && _listenerCapabilityDelegate != nullptr)
	{
		handled = _listenerCapabilityDelegate->onUnhandledAecpCommand(pi, aecpdu);
	}
	if (!handled && _talkerCapabilityDelegate != nullptr)
	{
		handled = _talkerCapabilityDelegate->onUnhandledAecpCommand(pi, aecpdu);
	}

	return handled;
}

bool AggregateEntityImpl::onUnhandledAecpVuCommand(protocol::ProtocolInterface* const pi, protocol::VuAecpdu::ProtocolIdentifier const& protocolIdentifier, protocol::Aecpdu const& aecpdu) noexcept
{
	auto handled = false;

	if (!handled && _controllerCapabilityDelegate != nullptr)
	{
		handled = _controllerCapabilityDelegate->onUnhandledAecpVuCommand(pi, protocolIdentifier, aecpdu);
	}
	if (!handled && _listenerCapabilityDelegate != nullptr)
	{
		handled = _listenerCapabilityDelegate->onUnhandledAecpVuCommand(pi, protocolIdentifier, aecpdu);
	}
	if (!handled && _talkerCapabilityDelegate != nullptr)
	{
		handled = _talkerCapabilityDelegate->onUnhandledAecpVuCommand(pi, protocolIdentifier, aecpdu);
	}

	return handled;
}

/* ************************************************************************** */
/* AggregateEntity methods                                                    */
/* ************************************************************************** */
/** Entry point */
AggregateEntity* LA_AVDECC_CALL_CONVENTION AggregateEntity::createRawAggregateEntity(protocol::ProtocolInterface* const protocolInterface, CommonInformation const& commonInformation, InterfacesInformation const& interfacesInformation, model::EntityTree const* const entityModelTree, controller::Delegate* const controllerDelegate)
{
	return new LocalEntityGuard<AggregateEntityImpl>(protocolInterface, commonInformation, interfacesInformation, entityModelTree, controllerDelegate);
}

/** Constructor */
AggregateEntity::AggregateEntity(CommonInformation const& commonInformation, InterfacesInformation const& interfacesInformation)
	: LocalEntity(commonInformation, interfacesInformation)
{
}

} // namespace entity
} // namespace avdecc
} // namespace la
