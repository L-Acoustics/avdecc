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
* @file controllerEntityImpl.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/utils.hpp"

#include "logHelper.hpp"
#include "controllerEntityImpl.hpp"
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
/* ControllerEntityImpl life cycle                                            */
/* ************************************************************************** */
ControllerEntityImpl::ControllerEntityImpl(protocol::ProtocolInterface* const protocolInterface, CommonInformation const& commonInformation, InterfacesInformation const& interfacesInformation, model::EntityTree const* const entityModelTree, controller::Delegate* const controllerDelegate)
	: LocalEntityImpl(protocolInterface, commonInformation, interfacesInformation)
{
	// Entity is controller capable
	_controllerCapabilityDelegate = std::make_unique<controller::CapabilityDelegate>(getProtocolInterface(), controllerDelegate, *this, *this, entityModelTree);

	// Register observer
	auto* const pi = getProtocolInterface();
	pi->registerObserver(this);

	// Send a first DISCOVER message
	pi->discoverRemoteEntities();
}

ControllerEntityImpl::~ControllerEntityImpl() noexcept
{
	// Unregister ourself as a ProtocolInterface observer
	invokeProtectedMethod(&protocol::ProtocolInterface::unregisterObserver, getProtocolInterface(), this);

	// Remove controller capability delegate
	_controllerCapabilityDelegate.reset();
}

void ControllerEntityImpl::destroy() noexcept
{
	delete this;
}

/* ************************************************************************** */
/* ControllerEntity overrides                                                 */
/* ************************************************************************** */
/* Discovery Protocol (ADP) */

/* Enumeration and Control Protocol (AECP) AEM */
void ControllerEntityImpl::acquireEntity(UniqueIdentifier const targetEntityID, bool const isPersistent, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, AcquireEntityHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).acquireEntity(targetEntityID, isPersistent, descriptorType, descriptorIndex, handler);
}

void ControllerEntityImpl::releaseEntity(UniqueIdentifier const targetEntityID, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, ReleaseEntityHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).releaseEntity(targetEntityID, descriptorType, descriptorIndex, handler);
}

void ControllerEntityImpl::lockEntity(UniqueIdentifier const targetEntityID, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, LockEntityHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).lockEntity(targetEntityID, descriptorType, descriptorIndex, handler);
}

void ControllerEntityImpl::unlockEntity(UniqueIdentifier const targetEntityID, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, UnlockEntityHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).unlockEntity(targetEntityID, descriptorType, descriptorIndex, handler);
}

void ControllerEntityImpl::queryEntityAvailable(UniqueIdentifier const targetEntityID, QueryEntityAvailableHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).queryEntityAvailable(targetEntityID, handler);
}

void ControllerEntityImpl::queryControllerAvailable(UniqueIdentifier const targetEntityID, QueryControllerAvailableHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).queryControllerAvailable(targetEntityID, handler);
}

void ControllerEntityImpl::registerUnsolicitedNotifications(UniqueIdentifier const targetEntityID, RegisterUnsolicitedNotificationsHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).registerUnsolicitedNotifications(targetEntityID, handler);
}

void ControllerEntityImpl::unregisterUnsolicitedNotifications(UniqueIdentifier const targetEntityID, UnregisterUnsolicitedNotificationsHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).unregisterUnsolicitedNotifications(targetEntityID, handler);
}

void ControllerEntityImpl::readEntityDescriptor(UniqueIdentifier const targetEntityID, EntityDescriptorHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readEntityDescriptor(targetEntityID, handler);
}

void ControllerEntityImpl::readConfigurationDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, ConfigurationDescriptorHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readConfigurationDescriptor(targetEntityID, configurationIndex, handler);
}

void ControllerEntityImpl::readAudioUnitDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AudioUnitIndex const audioUnitIndex, AudioUnitDescriptorHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readAudioUnitDescriptor(targetEntityID, configurationIndex, audioUnitIndex, handler);
}

void ControllerEntityImpl::readStreamInputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, StreamInputDescriptorHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readStreamInputDescriptor(targetEntityID, configurationIndex, streamIndex, handler);
}

void ControllerEntityImpl::readStreamOutputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, StreamOutputDescriptorHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readStreamOutputDescriptor(targetEntityID, configurationIndex, streamIndex, handler);
}

void ControllerEntityImpl::readJackInputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::JackIndex const jackIndex, JackInputDescriptorHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readJackInputDescriptor(targetEntityID, configurationIndex, jackIndex, handler);
}

void ControllerEntityImpl::readJackOutputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::JackIndex const jackIndex, JackOutputDescriptorHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readJackOutputDescriptor(targetEntityID, configurationIndex, jackIndex, handler);
}

void ControllerEntityImpl::readAvbInterfaceDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AvbInterfaceIndex const avbInterfaceIndex, AvbInterfaceDescriptorHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readAvbInterfaceDescriptor(targetEntityID, configurationIndex, avbInterfaceIndex, handler);
}

void ControllerEntityImpl::readClockSourceDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockSourceIndex const clockSourceIndex, ClockSourceDescriptorHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readClockSourceDescriptor(targetEntityID, configurationIndex, clockSourceIndex, handler);
}

void ControllerEntityImpl::readMemoryObjectDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MemoryObjectIndex const memoryObjectIndex, MemoryObjectDescriptorHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readMemoryObjectDescriptor(targetEntityID, configurationIndex, memoryObjectIndex, handler);
}

void ControllerEntityImpl::readLocaleDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::LocaleIndex const localeIndex, LocaleDescriptorHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readLocaleDescriptor(targetEntityID, configurationIndex, localeIndex, handler);
}

void ControllerEntityImpl::readStringsDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StringsIndex const stringsIndex, StringsDescriptorHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readStringsDescriptor(targetEntityID, configurationIndex, stringsIndex, handler);
}

void ControllerEntityImpl::readStreamPortInputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamPortIndex const streamPortIndex, StreamPortInputDescriptorHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readStreamPortInputDescriptor(targetEntityID, configurationIndex, streamPortIndex, handler);
}

void ControllerEntityImpl::readStreamPortOutputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamPortIndex const streamPortIndex, StreamPortOutputDescriptorHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readStreamPortOutputDescriptor(targetEntityID, configurationIndex, streamPortIndex, handler);
}

void ControllerEntityImpl::readExternalPortInputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ExternalPortIndex const externalPortIndex, ExternalPortInputDescriptorHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readExternalPortInputDescriptor(targetEntityID, configurationIndex, externalPortIndex, handler);
}

void ControllerEntityImpl::readExternalPortOutputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ExternalPortIndex const externalPortIndex, ExternalPortOutputDescriptorHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readExternalPortOutputDescriptor(targetEntityID, configurationIndex, externalPortIndex, handler);
}

void ControllerEntityImpl::readInternalPortInputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::InternalPortIndex const internalPortIndex, InternalPortInputDescriptorHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readInternalPortInputDescriptor(targetEntityID, configurationIndex, internalPortIndex, handler);
}

void ControllerEntityImpl::readInternalPortOutputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::InternalPortIndex const internalPortIndex, InternalPortOutputDescriptorHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readInternalPortOutputDescriptor(targetEntityID, configurationIndex, internalPortIndex, handler);
}

void ControllerEntityImpl::readAudioClusterDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClusterIndex const clusterIndex, AudioClusterDescriptorHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readAudioClusterDescriptor(targetEntityID, configurationIndex, clusterIndex, handler);
}

void ControllerEntityImpl::readAudioMapDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MapIndex const mapIndex, AudioMapDescriptorHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readAudioMapDescriptor(targetEntityID, configurationIndex, mapIndex, handler);
}

void ControllerEntityImpl::readControlDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ControlIndex const controlIndex, ControlDescriptorHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readControlDescriptor(targetEntityID, configurationIndex, controlIndex, handler);
}

void ControllerEntityImpl::readClockDomainDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockDomainIndex const clockDomainIndex, ClockDomainDescriptorHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readClockDomainDescriptor(targetEntityID, configurationIndex, clockDomainIndex, handler);
}

void ControllerEntityImpl::readTimingDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::TimingIndex const timingIndex, TimingDescriptorHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readTimingDescriptor(targetEntityID, configurationIndex, timingIndex, handler);
}

void ControllerEntityImpl::readPtpInstanceDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::PtpInstanceIndex const ptpInstanceIndex, PtpInstanceDescriptorHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readPtpInstanceDescriptor(targetEntityID, configurationIndex, ptpInstanceIndex, handler);
}

void ControllerEntityImpl::readPtpPortDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::PtpPortIndex const ptpPortIndex, PtpPortDescriptorHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).readPtpPortDescriptor(targetEntityID, configurationIndex, ptpPortIndex, handler);
}

void ControllerEntityImpl::setConfiguration(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, SetConfigurationHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setConfiguration(targetEntityID, configurationIndex, handler);
}

void ControllerEntityImpl::getConfiguration(UniqueIdentifier const targetEntityID, GetConfigurationHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getConfiguration(targetEntityID, handler);
}

void ControllerEntityImpl::setStreamInputFormat(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, model::StreamFormat const streamFormat, SetStreamInputFormatHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setStreamInputFormat(targetEntityID, streamIndex, streamFormat, handler);
}

void ControllerEntityImpl::getStreamInputFormat(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, GetStreamInputFormatHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getStreamInputFormat(targetEntityID, streamIndex, handler);
}

void ControllerEntityImpl::setStreamOutputFormat(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, model::StreamFormat const streamFormat, SetStreamOutputFormatHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setStreamOutputFormat(targetEntityID, streamIndex, streamFormat, handler);
}

void ControllerEntityImpl::getStreamOutputFormat(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, GetStreamOutputFormatHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getStreamOutputFormat(targetEntityID, streamIndex, handler);
}

void ControllerEntityImpl::getStreamPortInputAudioMap(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::MapIndex const mapIndex, GetStreamPortInputAudioMapHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getStreamPortInputAudioMap(targetEntityID, streamPortIndex, mapIndex, handler);
}

void ControllerEntityImpl::getStreamPortOutputAudioMap(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::MapIndex const mapIndex, GetStreamPortOutputAudioMapHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getStreamPortOutputAudioMap(targetEntityID, streamPortIndex, mapIndex, handler);
}

void ControllerEntityImpl::addStreamPortInputAudioMappings(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::AudioMappings const& mappings, AddStreamPortInputAudioMappingsHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).addStreamPortInputAudioMappings(targetEntityID, streamPortIndex, mappings, handler);
}

void ControllerEntityImpl::addStreamPortOutputAudioMappings(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::AudioMappings const& mappings, AddStreamPortOutputAudioMappingsHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).addStreamPortOutputAudioMappings(targetEntityID, streamPortIndex, mappings, handler);
}

void ControllerEntityImpl::removeStreamPortInputAudioMappings(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::AudioMappings const& mappings, RemoveStreamPortInputAudioMappingsHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).removeStreamPortInputAudioMappings(targetEntityID, streamPortIndex, mappings, handler);
}

void ControllerEntityImpl::removeStreamPortOutputAudioMappings(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::AudioMappings const& mappings, RemoveStreamPortOutputAudioMappingsHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).removeStreamPortOutputAudioMappings(targetEntityID, streamPortIndex, mappings, handler);
}

void ControllerEntityImpl::setStreamInputInfo(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, model::StreamInfo const& info, SetStreamInputInfoHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setStreamInputInfo(targetEntityID, streamIndex, info, handler);
}

void ControllerEntityImpl::setStreamOutputInfo(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, model::StreamInfo const& info, SetStreamOutputInfoHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setStreamOutputInfo(targetEntityID, streamIndex, info, handler);
}

void ControllerEntityImpl::getStreamInputInfo(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, GetStreamInputInfoHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getStreamInputInfo(targetEntityID, streamIndex, handler);
}

void ControllerEntityImpl::getStreamOutputInfo(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, GetStreamOutputInfoHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getStreamOutputInfo(targetEntityID, streamIndex, handler);
}

void ControllerEntityImpl::setEntityName(UniqueIdentifier const targetEntityID, model::AvdeccFixedString const& entityName, SetEntityNameHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setEntityName(targetEntityID, entityName, handler);
}

void ControllerEntityImpl::getEntityName(UniqueIdentifier const targetEntityID, GetEntityNameHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getEntityName(targetEntityID, handler);
}

void ControllerEntityImpl::setEntityGroupName(UniqueIdentifier const targetEntityID, model::AvdeccFixedString const& entityGroupName, SetEntityGroupNameHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setEntityGroupName(targetEntityID, entityGroupName, handler);
}

void ControllerEntityImpl::getEntityGroupName(UniqueIdentifier const targetEntityID, GetEntityGroupNameHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getEntityGroupName(targetEntityID, handler);
}

void ControllerEntityImpl::setConfigurationName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AvdeccFixedString const& configurationName, SetConfigurationNameHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setConfigurationName(targetEntityID, configurationIndex, configurationName, handler);
}

void ControllerEntityImpl::getConfigurationName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, GetConfigurationNameHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getConfigurationName(targetEntityID, configurationIndex, handler);
}

void ControllerEntityImpl::setAudioUnitName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AudioUnitIndex const audioUnitIndex, model::AvdeccFixedString const& audioUnitName, SetAudioUnitNameHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setAudioUnitName(targetEntityID, configurationIndex, audioUnitIndex, audioUnitName, handler);
}

void ControllerEntityImpl::getAudioUnitName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AudioUnitIndex const audioUnitIndex, GetAudioUnitNameHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getAudioUnitName(targetEntityID, configurationIndex, audioUnitIndex, handler);
}

void ControllerEntityImpl::setStreamInputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, model::AvdeccFixedString const& streamInputName, SetStreamInputNameHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setStreamInputName(targetEntityID, configurationIndex, streamIndex, streamInputName, handler);
}

void ControllerEntityImpl::getStreamInputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, GetStreamInputNameHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getStreamInputName(targetEntityID, configurationIndex, streamIndex, handler);
}

void ControllerEntityImpl::setStreamOutputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, model::AvdeccFixedString const& streamOutputName, SetStreamOutputNameHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setStreamOutputName(targetEntityID, configurationIndex, streamIndex, streamOutputName, handler);
}

void ControllerEntityImpl::getStreamOutputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, GetStreamOutputNameHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getStreamOutputName(targetEntityID, configurationIndex, streamIndex, handler);
}

void ControllerEntityImpl::setJackInputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::JackIndex const jackIndex, model::AvdeccFixedString const& jackInputName, SetJackInputNameHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setJackInputName(targetEntityID, configurationIndex, jackIndex, jackInputName, handler);
}

void ControllerEntityImpl::getJackInputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::JackIndex const jackIndex, GetJackInputNameHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getJackInputName(targetEntityID, configurationIndex, jackIndex, handler);
}

void ControllerEntityImpl::setJackOutputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::JackIndex const jackIndex, model::AvdeccFixedString const& jackOutputName, SetJackOutputNameHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setJackOutputName(targetEntityID, configurationIndex, jackIndex, jackOutputName, handler);
}

void ControllerEntityImpl::getJackOutputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::JackIndex const jackIndex, GetJackOutputNameHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getJackOutputName(targetEntityID, configurationIndex, jackIndex, handler);
}

void ControllerEntityImpl::setAvbInterfaceName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AvbInterfaceIndex const avbInterfaceIndex, model::AvdeccFixedString const& avbInterfaceName, SetAvbInterfaceNameHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setAvbInterfaceName(targetEntityID, configurationIndex, avbInterfaceIndex, avbInterfaceName, handler);
}

void ControllerEntityImpl::getAvbInterfaceName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AvbInterfaceIndex const avbInterfaceIndex, GetAvbInterfaceNameHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getAvbInterfaceName(targetEntityID, configurationIndex, avbInterfaceIndex, handler);
}

void ControllerEntityImpl::setClockSourceName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockSourceIndex const clockSourceIndex, model::AvdeccFixedString const& clockSourceName, SetClockSourceNameHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setClockSourceName(targetEntityID, configurationIndex, clockSourceIndex, clockSourceName, handler);
}

void ControllerEntityImpl::getClockSourceName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockSourceIndex const clockSourceIndex, GetClockSourceNameHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getClockSourceName(targetEntityID, configurationIndex, clockSourceIndex, handler);
}

void ControllerEntityImpl::setMemoryObjectName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MemoryObjectIndex const memoryObjectIndex, model::AvdeccFixedString const& memoryObjectName, SetMemoryObjectNameHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setMemoryObjectName(targetEntityID, configurationIndex, memoryObjectIndex, memoryObjectName, handler);
}

void ControllerEntityImpl::getMemoryObjectName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MemoryObjectIndex const memoryObjectIndex, GetMemoryObjectNameHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getMemoryObjectName(targetEntityID, configurationIndex, memoryObjectIndex, handler);
}

void ControllerEntityImpl::setAudioClusterName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClusterIndex const audioClusterIndex, model::AvdeccFixedString const& audioClusterName, SetAudioClusterNameHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setAudioClusterName(targetEntityID, configurationIndex, audioClusterIndex, audioClusterName, handler);
}

void ControllerEntityImpl::getAudioClusterName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClusterIndex const audioClusterIndex, GetAudioClusterNameHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getAudioClusterName(targetEntityID, configurationIndex, audioClusterIndex, handler);
}

void ControllerEntityImpl::setControlName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ControlIndex const controlIndex, model::AvdeccFixedString const& controlName, SetControlNameHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setControlName(targetEntityID, configurationIndex, controlIndex, controlName, handler);
}

void ControllerEntityImpl::getControlName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ControlIndex const controlIndex, GetControlNameHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getControlName(targetEntityID, configurationIndex, controlIndex, handler);
}

void ControllerEntityImpl::setClockDomainName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockDomainIndex const clockDomainIndex, model::AvdeccFixedString const& clockDomainName, SetClockDomainNameHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setClockDomainName(targetEntityID, configurationIndex, clockDomainIndex, clockDomainName, handler);
}

void ControllerEntityImpl::getClockDomainName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockDomainIndex const clockDomainIndex, GetClockDomainNameHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getClockDomainName(targetEntityID, configurationIndex, clockDomainIndex, handler);
}

void ControllerEntityImpl::setTimingName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::TimingIndex const timingIndex, model::AvdeccFixedString const& timingName, SetTimingNameHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setTimingName(targetEntityID, configurationIndex, timingIndex, timingName, handler);
}

void ControllerEntityImpl::getTimingName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::TimingIndex const timingIndex, GetTimingNameHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getTimingName(targetEntityID, configurationIndex, timingIndex, handler);
}

void ControllerEntityImpl::setPtpInstanceName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::PtpInstanceIndex const ptpInstanceIndex, model::AvdeccFixedString const& ptpInstanceName, SetPtpInstanceNameHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setPtpInstanceName(targetEntityID, configurationIndex, ptpInstanceIndex, ptpInstanceName, handler);
}

void ControllerEntityImpl::getPtpInstanceName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::PtpInstanceIndex const ptpInstanceIndex, GetPtpInstanceNameHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getPtpInstanceName(targetEntityID, configurationIndex, ptpInstanceIndex, handler);
}

void ControllerEntityImpl::setPtpPortName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::PtpPortIndex const ptpPortIndex, model::AvdeccFixedString const& ptpPortName, SetPtpPortNameHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setPtpPortName(targetEntityID, configurationIndex, ptpPortIndex, ptpPortName, handler);
}

void ControllerEntityImpl::getPtpPortName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::PtpPortIndex const ptpPortIndex, GetPtpPortNameHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getPtpPortName(targetEntityID, configurationIndex, ptpPortIndex, handler);
}

void ControllerEntityImpl::setAssociation(UniqueIdentifier const targetEntityID, UniqueIdentifier const associationID, SetAssociationHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setAssociationID(targetEntityID, associationID, handler);
}

void ControllerEntityImpl::getAssociation(UniqueIdentifier const targetEntityID, GetAssociationHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getAssociationID(targetEntityID, handler);
}

void ControllerEntityImpl::setAudioUnitSamplingRate(UniqueIdentifier const targetEntityID, model::AudioUnitIndex const audioUnitIndex, model::SamplingRate const samplingRate, SetAudioUnitSamplingRateHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setAudioUnitSamplingRate(targetEntityID, audioUnitIndex, samplingRate, handler);
}

void ControllerEntityImpl::getAudioUnitSamplingRate(UniqueIdentifier const targetEntityID, model::AudioUnitIndex const audioUnitIndex, GetAudioUnitSamplingRateHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getAudioUnitSamplingRate(targetEntityID, audioUnitIndex, handler);
}

void ControllerEntityImpl::setVideoClusterSamplingRate(UniqueIdentifier const targetEntityID, model::ClusterIndex const videoClusterIndex, model::SamplingRate const samplingRate, SetVideoClusterSamplingRateHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setVideoClusterSamplingRate(targetEntityID, videoClusterIndex, samplingRate, handler);
}

void ControllerEntityImpl::getVideoClusterSamplingRate(UniqueIdentifier const targetEntityID, model::ClusterIndex const videoClusterIndex, GetVideoClusterSamplingRateHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getVideoClusterSamplingRate(targetEntityID, videoClusterIndex, handler);
}

void ControllerEntityImpl::setSensorClusterSamplingRate(UniqueIdentifier const targetEntityID, model::ClusterIndex const sensorClusterIndex, model::SamplingRate const samplingRate, SetSensorClusterSamplingRateHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setSensorClusterSamplingRate(targetEntityID, sensorClusterIndex, samplingRate, handler);
}

void ControllerEntityImpl::getSensorClusterSamplingRate(UniqueIdentifier const targetEntityID, model::ClusterIndex const sensorClusterIndex, GetSensorClusterSamplingRateHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getSensorClusterSamplingRate(targetEntityID, sensorClusterIndex, handler);
}

void ControllerEntityImpl::setClockSource(UniqueIdentifier const targetEntityID, model::ClockDomainIndex const clockDomainIndex, model::ClockSourceIndex const clockSourceIndex, SetClockSourceHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setClockSource(targetEntityID, clockDomainIndex, clockSourceIndex, handler);
}

void ControllerEntityImpl::getClockSource(UniqueIdentifier const targetEntityID, model::ClockDomainIndex const clockDomainIndex, GetClockSourceHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getClockSource(targetEntityID, clockDomainIndex, handler);
}

void ControllerEntityImpl::setControlValues(UniqueIdentifier const targetEntityID, model::ControlIndex const controlIndex, model::ControlValues const& controlValues, SetControlValuesHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setControlValues(targetEntityID, controlIndex, controlValues, handler);
}

void ControllerEntityImpl::getControlValues(UniqueIdentifier const targetEntityID, model::ControlIndex const controlIndex, GetControlValuesHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getControlValues(targetEntityID, controlIndex, handler);
}

void ControllerEntityImpl::startStreamInput(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, StartStreamInputHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).startStreamInput(targetEntityID, streamIndex, handler);
}

void ControllerEntityImpl::startStreamOutput(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, StartStreamOutputHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).startStreamOutput(targetEntityID, streamIndex, handler);
}

void ControllerEntityImpl::stopStreamInput(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, StopStreamInputHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).stopStreamInput(targetEntityID, streamIndex, handler);
}

void ControllerEntityImpl::stopStreamOutput(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, StopStreamOutputHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).stopStreamOutput(targetEntityID, streamIndex, handler);
}

void ControllerEntityImpl::getAvbInfo(UniqueIdentifier const targetEntityID, model::AvbInterfaceIndex const avbInterfaceIndex, GetAvbInfoHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getAvbInfo(targetEntityID, avbInterfaceIndex, handler);
}

void ControllerEntityImpl::getAsPath(UniqueIdentifier const targetEntityID, model::AvbInterfaceIndex const avbInterfaceIndex, GetAsPathHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getAsPath(targetEntityID, avbInterfaceIndex, handler);
}

void ControllerEntityImpl::getEntityCounters(UniqueIdentifier const targetEntityID, GetEntityCountersHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getEntityCounters(targetEntityID, handler);
}

void ControllerEntityImpl::getAvbInterfaceCounters(UniqueIdentifier const targetEntityID, model::AvbInterfaceIndex const avbInterfaceIndex, GetAvbInterfaceCountersHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getAvbInterfaceCounters(targetEntityID, avbInterfaceIndex, handler);
}

void ControllerEntityImpl::getClockDomainCounters(UniqueIdentifier const targetEntityID, model::ClockDomainIndex const clockDomainIndex, GetClockDomainCountersHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getClockDomainCounters(targetEntityID, clockDomainIndex, handler);
}

void ControllerEntityImpl::getStreamInputCounters(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, GetStreamInputCountersHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getStreamInputCounters(targetEntityID, streamIndex, handler);
}

void ControllerEntityImpl::getStreamOutputCounters(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, GetStreamOutputCountersHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getStreamOutputCounters(targetEntityID, streamIndex, handler);
}

void ControllerEntityImpl::reboot(UniqueIdentifier const targetEntityID, RebootHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).reboot(targetEntityID, handler);
}

void ControllerEntityImpl::rebootToFirmware(UniqueIdentifier const targetEntityID, model::MemoryObjectIndex const memoryObjectIndex, RebootToFirmwareHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).rebootToFirmware(targetEntityID, memoryObjectIndex, handler);
}

void ControllerEntityImpl::startOperation(UniqueIdentifier const targetEntityID, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, model::MemoryObjectOperationType const operationType, MemoryBuffer const& memoryBuffer, StartOperationHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).startOperation(targetEntityID, descriptorType, descriptorIndex, operationType, memoryBuffer, handler);
}

void ControllerEntityImpl::abortOperation(UniqueIdentifier const targetEntityID, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, model::OperationID const operationID, AbortOperationHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).abortOperation(targetEntityID, descriptorType, descriptorIndex, operationID, handler);
}

void ControllerEntityImpl::setMemoryObjectLength(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length, SetMemoryObjectLengthHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setMemoryObjectLength(targetEntityID, configurationIndex, memoryObjectIndex, length, handler);
}

void ControllerEntityImpl::getMemoryObjectLength(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MemoryObjectIndex const memoryObjectIndex, GetMemoryObjectLengthHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getMemoryObjectLength(targetEntityID, configurationIndex, memoryObjectIndex, handler);
}

/* Enumeration and Control Protocol (AECP) AA */
void ControllerEntityImpl::addressAccess(UniqueIdentifier const targetEntityID, addressAccess::Tlvs const& tlvs, AddressAccessHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).addressAccess(targetEntityID, tlvs, handler);
}

/* Enumeration and Control Protocol (AECP) MVU (Milan Vendor Unique) */
void ControllerEntityImpl::getMilanInfo(UniqueIdentifier const targetEntityID, GetMilanInfoHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getMilanInfo(targetEntityID, handler);
}

/* Connection Management Protocol (ACMP) */
void ControllerEntityImpl::connectStream(model::StreamIdentification const& talkerStream, model::StreamIdentification const& listenerStream, ConnectStreamHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).connectStream(talkerStream, listenerStream, handler);
}

void ControllerEntityImpl::disconnectStream(model::StreamIdentification const& talkerStream, model::StreamIdentification const& listenerStream, DisconnectStreamHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).disconnectStream(talkerStream, listenerStream, handler);
}

void ControllerEntityImpl::disconnectTalkerStream(model::StreamIdentification const& talkerStream, model::StreamIdentification const& listenerStream, DisconnectTalkerStreamHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).disconnectTalkerStream(talkerStream, listenerStream, handler);
}

void ControllerEntityImpl::getTalkerStreamState(model::StreamIdentification const& talkerStream, GetTalkerStreamStateHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getTalkerStreamState(talkerStream, handler);
}

void ControllerEntityImpl::getListenerStreamState(model::StreamIdentification const& listenerStream, GetListenerStreamStateHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getListenerStreamState(listenerStream, handler);
}

void ControllerEntityImpl::getTalkerStreamConnection(model::StreamIdentification const& talkerStream, std::uint16_t const connectionIndex, GetTalkerStreamConnectionHandler const& handler) const noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).getTalkerStreamConnection(talkerStream, connectionIndex, handler);
}

/* ************************************************************************** */
/* ControllerEntity overrides                                                  */
/* ************************************************************************** */
void ControllerEntityImpl::setControllerDelegate(controller::Delegate* const delegate) noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).setControllerDelegate(delegate);
}

/* ************************************************************************** */
/* protocol::ProtocolInterface::Observer overrides                            */
/* ************************************************************************** */
/* **** Global notifications **** */
void ControllerEntityImpl::onTransportError(protocol::ProtocolInterface* const pi) noexcept
{
	_controllerCapabilityDelegate->onTransportError(pi);
}

/* **** Discovery notifications **** */
void ControllerEntityImpl::onLocalEntityOnline(protocol::ProtocolInterface* const pi, Entity const& entity) noexcept
{
	_controllerCapabilityDelegate->onLocalEntityOnline(pi, entity);
}

void ControllerEntityImpl::onLocalEntityOffline(protocol::ProtocolInterface* const pi, UniqueIdentifier const entityID) noexcept
{
	_controllerCapabilityDelegate->onLocalEntityOffline(pi, entityID);
}

void ControllerEntityImpl::onLocalEntityUpdated(protocol::ProtocolInterface* const pi, Entity const& entity) noexcept
{
	_controllerCapabilityDelegate->onLocalEntityUpdated(pi, entity);
}

void ControllerEntityImpl::onRemoteEntityOnline(protocol::ProtocolInterface* const pi, Entity const& entity) noexcept
{
	_controllerCapabilityDelegate->onRemoteEntityOnline(pi, entity);
}

void ControllerEntityImpl::onRemoteEntityOffline(protocol::ProtocolInterface* const pi, UniqueIdentifier const entityID) noexcept
{
	_controllerCapabilityDelegate->onRemoteEntityOffline(pi, entityID);
}

void ControllerEntityImpl::onRemoteEntityUpdated(protocol::ProtocolInterface* const pi, Entity const& entity) noexcept
{
	_controllerCapabilityDelegate->onRemoteEntityUpdated(pi, entity);
}

/* **** AECP notifications **** */
void ControllerEntityImpl::onAecpAemUnsolicitedResponse(protocol::ProtocolInterface* const pi, protocol::AemAecpdu const& aecpdu) noexcept
{
	_controllerCapabilityDelegate->onAecpAemUnsolicitedResponse(pi, aecpdu);
}

void ControllerEntityImpl::onAecpAemIdentifyNotification(protocol::ProtocolInterface* const pi, protocol::AemAecpdu const& aecpdu) noexcept
{
	_controllerCapabilityDelegate->onAecpAemIdentifyNotification(pi, aecpdu);
}

/* **** ACMP notifications **** */
void ControllerEntityImpl::onAcmpCommand(protocol::ProtocolInterface* const pi, protocol::Acmpdu const& acmpdu) noexcept
{
	_controllerCapabilityDelegate->onAcmpCommand(pi, acmpdu);
}

void ControllerEntityImpl::onAcmpResponse(protocol::ProtocolInterface* const pi, protocol::Acmpdu const& acmpdu) noexcept
{
	_controllerCapabilityDelegate->onAcmpResponse(pi, acmpdu);
}

/* **** Statistics **** */
void ControllerEntityImpl::onAecpRetry(protocol::ProtocolInterface* const pi, UniqueIdentifier const& entityID) noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).onAecpRetry(pi, entityID);
}

void ControllerEntityImpl::onAecpTimeout(protocol::ProtocolInterface* const pi, UniqueIdentifier const& entityID) noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).onAecpTimeout(pi, entityID);
}

void ControllerEntityImpl::onAecpUnexpectedResponse(protocol::ProtocolInterface* const pi, UniqueIdentifier const& entityID) noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).onAecpUnexpectedResponse(pi, entityID);
}

void ControllerEntityImpl::onAecpResponseTime(protocol::ProtocolInterface* const pi, UniqueIdentifier const& entityID, std::chrono::milliseconds const& responseTime) noexcept
{
	static_cast<controller::CapabilityDelegate&>(*_controllerCapabilityDelegate).onAecpResponseTime(pi, entityID, responseTime);
}

/* ************************************************************************** */
/* LocalEntityImpl overrides                                                  */
/* ************************************************************************** */
bool ControllerEntityImpl::onUnhandledAecpCommand(protocol::ProtocolInterface* const pi, protocol::Aecpdu const& aecpdu) noexcept
{
	return _controllerCapabilityDelegate->onUnhandledAecpCommand(pi, aecpdu);
}

bool ControllerEntityImpl::onUnhandledAecpVuCommand(protocol::ProtocolInterface* const pi, protocol::VuAecpdu::ProtocolIdentifier const& protocolIdentifier, protocol::Aecpdu const& aecpdu) noexcept
{
	return _controllerCapabilityDelegate->onUnhandledAecpVuCommand(pi, protocolIdentifier, aecpdu);
}

/* ************************************************************************** */
/* ControllerEntity methods                                                   */
/* ************************************************************************** */
/** Entry point */
ControllerEntity* LA_AVDECC_CALL_CONVENTION ControllerEntity::createRawControllerEntity(protocol::ProtocolInterface* const protocolInterface, CommonInformation const& commonInformation, InterfacesInformation const& interfacesInformation, model::EntityTree const* const entityModelTree, controller::Delegate* const delegate)
{
	return new LocalEntityGuard<ControllerEntityImpl>(protocolInterface, commonInformation, interfacesInformation, entityModelTree, delegate);
}

/** Constructor */
ControllerEntity::ControllerEntity(CommonInformation const& commonInformation, InterfacesInformation const& interfacesInformation)
	: LocalEntity(commonInformation, interfacesInformation)
{
}

} // namespace entity
} // namespace avdecc
} // namespace la
