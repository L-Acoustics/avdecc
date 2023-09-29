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
* @file controllerEntity.hpp
* @author Christophe Calmejane
* @brief Avdecc controller entity.
*/

#pragma once

#include "la/avdecc/memoryBuffer.hpp"

#include "protocolInterface.hpp"
#include "entity.hpp"
#include "entityModel.hpp"
#include "entityModelTree.hpp"
#include "entityAddressAccessTypes.hpp"
#include "exports.hpp"

#include <thread>
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>

namespace la
{
namespace avdecc
{
namespace entity
{
namespace controller
{
class Interface
{
public:
	/* Enumeration and Control Protocol (AECP) AEM handlers */
	using AcquireEntityHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const owningEntity, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex)>;
	using ReleaseEntityHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const owningEntity, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex)>;
	using LockEntityHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const lockingEntity, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex)>;
	using UnlockEntityHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const lockingEntity, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex)>;
	using QueryEntityAvailableHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status)>;
	using QueryControllerAvailableHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status)>;
	using RegisterUnsolicitedNotificationsHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status)>;
	using UnregisterUnsolicitedNotificationsHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status)>;
	using EntityDescriptorHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::EntityDescriptor const& descriptor)>;
	using ConfigurationDescriptorHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ConfigurationDescriptor const& descriptor)>;
	using AudioUnitDescriptorHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AudioUnitIndex const audioUnitIndex, la::avdecc::entity::model::AudioUnitDescriptor const& descriptor)>;
	using StreamInputDescriptorHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamDescriptor const& descriptor)>;
	using StreamOutputDescriptorHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamDescriptor const& descriptor)>;
	using JackInputDescriptorHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::JackIndex const jackIndex, la::avdecc::entity::model::JackDescriptor const& descriptor)>;
	using JackOutputDescriptorHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::JackIndex const jackIndex, la::avdecc::entity::model::JackDescriptor const& descriptor)>;
	using AvbInterfaceDescriptorHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::model::AvbInterfaceDescriptor const& descriptor)>;
	using ClockSourceDescriptorHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockSourceIndex const clockSourceIndex, la::avdecc::entity::model::ClockSourceDescriptor const& descriptor)>;
	using MemoryObjectDescriptorHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::MemoryObjectIndex const memoryObjectIndex, la::avdecc::entity::model::MemoryObjectDescriptor const& descriptor)>;
	using LocaleDescriptorHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::LocaleIndex const localeIndex, la::avdecc::entity::model::LocaleDescriptor const& descriptor)>;
	using StringsDescriptorHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StringsIndex const stringsIndex, la::avdecc::entity::model::StringsDescriptor const& descriptor)>;
	using StreamPortInputDescriptorHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::StreamPortDescriptor const& descriptor)>;
	using StreamPortOutputDescriptorHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::StreamPortDescriptor const& descriptor)>;
	using ExternalPortInputDescriptorHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ExternalPortIndex const externalPortIndex, la::avdecc::entity::model::ExternalPortDescriptor const& descriptor)>;
	using ExternalPortOutputDescriptorHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ExternalPortIndex const externalPortIndex, la::avdecc::entity::model::ExternalPortDescriptor const& descriptor)>;
	using InternalPortInputDescriptorHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::InternalPortIndex const internalPortIndex, la::avdecc::entity::model::InternalPortDescriptor const& descriptor)>;
	using InternalPortOutputDescriptorHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::InternalPortIndex const internalPortIndex, la::avdecc::entity::model::InternalPortDescriptor const& descriptor)>;
	using AudioClusterDescriptorHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClusterIndex const clusterIndex, la::avdecc::entity::model::AudioClusterDescriptor const& descriptor)>;
	using AudioMapDescriptorHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::MapIndex const mapIndex, la::avdecc::entity::model::AudioMapDescriptor const& descriptor)>;
	using ControlDescriptorHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ControlIndex const controlIndex, la::avdecc::entity::model::ControlDescriptor const& descriptor)>;
	using ClockDomainDescriptorHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::entity::model::ClockDomainDescriptor const& descriptor)>;
	using TimingDescriptorHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::TimingIndex const timingIndex, la::avdecc::entity::model::TimingDescriptor const& descriptor)>;
	using PtpInstanceDescriptorHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::PtpInstanceIndex const ptpInstanceIndex, la::avdecc::entity::model::PtpInstanceDescriptor const& descriptor)>;
	using PtpPortDescriptorHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::PtpPortIndex const ptpPortIndex, la::avdecc::entity::model::PtpPortDescriptor const& descriptor)>;
	using SetConfigurationHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex)>;
	using GetConfigurationHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex)>;
	using SetStreamInputFormatHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamFormat const streamFormat)>;
	using GetStreamInputFormatHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamFormat const streamFormat)>;
	using SetStreamOutputFormatHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamFormat const streamFormat)>;
	using GetStreamOutputFormatHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamFormat const streamFormat)>;
	using GetStreamPortInputAudioMapHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::MapIndex const numberOfMaps, la::avdecc::entity::model::MapIndex const mapIndex, la::avdecc::entity::model::AudioMappings const& mappings)>;
	using GetStreamPortOutputAudioMapHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::MapIndex const numberOfMaps, la::avdecc::entity::model::MapIndex const mapIndex, la::avdecc::entity::model::AudioMappings const& mappings)>;
	using AddStreamPortInputAudioMappingsHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::AudioMappings const& mappings)>;
	using AddStreamPortOutputAudioMappingsHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::AudioMappings const& mappings)>;
	using RemoveStreamPortInputAudioMappingsHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::AudioMappings const& mappings)>;
	using RemoveStreamPortOutputAudioMappingsHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::AudioMappings const& mappings)>;
	using SetStreamInputInfoHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamInfo const& info)>;
	using SetStreamOutputInfoHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamInfo const& info)>;
	using GetStreamInputInfoHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamInfo const& info)>;
	using GetStreamOutputInfoHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamInfo const& info)>;
	using SetEntityNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::AvdeccFixedString const& entityName)>;
	using GetEntityNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::AvdeccFixedString const& entityName)>;
	using SetEntityGroupNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::AvdeccFixedString const& entityGroupName)>;
	using GetEntityGroupNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::AvdeccFixedString const& entityGroupName)>;
	using SetConfigurationNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AvdeccFixedString const& configurationName)>;
	using GetConfigurationNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AvdeccFixedString const& configurationName)>;
	using SetAudioUnitNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AudioUnitIndex const audioUnitIndex, la::avdecc::entity::model::AvdeccFixedString const& audioUnitName)>;
	using GetAudioUnitNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AudioUnitIndex const audioUnitIndex, la::avdecc::entity::model::AvdeccFixedString const& audioUnitName)>;
	using SetStreamInputNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::AvdeccFixedString const& streamInputName)>;
	using GetStreamInputNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::AvdeccFixedString const& streamInputName)>;
	using SetStreamOutputNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::AvdeccFixedString const& streamOutputName)>;
	using GetStreamOutputNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::AvdeccFixedString const& streamOutputName)>;
	using SetJackInputNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::JackIndex const jackIndex, la::avdecc::entity::model::AvdeccFixedString const& jackInputName)>;
	using GetJackInputNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::JackIndex const jackIndex, la::avdecc::entity::model::AvdeccFixedString const& jackInputName)>;
	using SetJackOutputNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::JackIndex const jackIndex, la::avdecc::entity::model::AvdeccFixedString const& jackOutputName)>;
	using GetJackOutputNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::JackIndex const jackIndex, la::avdecc::entity::model::AvdeccFixedString const& jackOutputName)>;
	using SetAvbInterfaceNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::model::AvdeccFixedString const& avbInterfaceName)>;
	using GetAvbInterfaceNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::model::AvdeccFixedString const& avbInterfaceName)>;
	using SetClockSourceNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockSourceIndex const clockSourceIndex, la::avdecc::entity::model::AvdeccFixedString const& clockSourceName)>;
	using GetClockSourceNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockSourceIndex const clockSourceIndex, la::avdecc::entity::model::AvdeccFixedString const& clockSourceName)>;
	using SetMemoryObjectNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::MemoryObjectIndex const memoryObjectIndex, la::avdecc::entity::model::AvdeccFixedString const& memoryObjectName)>;
	using GetMemoryObjectNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::MemoryObjectIndex const memoryObjectIndex, la::avdecc::entity::model::AvdeccFixedString const& memoryObjectName)>;
	using SetAudioClusterNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClusterIndex const audioClusterIndex, la::avdecc::entity::model::AvdeccFixedString const& audioClusterName)>;
	using GetAudioClusterNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClusterIndex const audioClusterIndex, la::avdecc::entity::model::AvdeccFixedString const& audioClusterName)>;
	using SetControlNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ControlIndex const controlIndex, la::avdecc::entity::model::AvdeccFixedString const& controlName)>;
	using GetControlNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ControlIndex const controlIndex, la::avdecc::entity::model::AvdeccFixedString const& controlName)>;
	using SetClockDomainNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::entity::model::AvdeccFixedString const& clockDomainName)>;
	using GetClockDomainNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::entity::model::AvdeccFixedString const& clockDomainName)>;
	using SetTimingNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::TimingIndex const timingIndex, la::avdecc::entity::model::AvdeccFixedString const& timingName)>;
	using GetTimingNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::TimingIndex const timingIndex, la::avdecc::entity::model::AvdeccFixedString const& timingName)>;
	using SetPtpInstanceNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::PtpInstanceIndex const ptpInstanceIndex, la::avdecc::entity::model::AvdeccFixedString const& ptpInstanceName)>;
	using GetPtpInstanceNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::PtpInstanceIndex const ptpInstanceIndex, la::avdecc::entity::model::AvdeccFixedString const& ptpInstanceName)>;
	using SetPtpPortNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::PtpPortIndex const ptpPortIndex, la::avdecc::entity::model::AvdeccFixedString const& ptpPortName)>;
	using GetPtpPortNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::PtpPortIndex const ptpPortIndex, la::avdecc::entity::model::AvdeccFixedString const& ptpPortName)>;
	using SetAssociationHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const associationID)>;
	using GetAssociationHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const associationID)>;
	using SetAudioUnitSamplingRateHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::AudioUnitIndex const audioUnitIndex, la::avdecc::entity::model::SamplingRate const samplingRate)>;
	using GetAudioUnitSamplingRateHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::AudioUnitIndex const audioUnitIndex, la::avdecc::entity::model::SamplingRate const samplingRate)>;
	using SetVideoClusterSamplingRateHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ClusterIndex const videoClusterIndex, la::avdecc::entity::model::SamplingRate const samplingRate)>;
	using GetVideoClusterSamplingRateHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ClusterIndex const videoClusterIndex, la::avdecc::entity::model::SamplingRate const samplingRate)>;
	using SetSensorClusterSamplingRateHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ClusterIndex const sensorClusterIndex, la::avdecc::entity::model::SamplingRate const samplingRate)>;
	using GetSensorClusterSamplingRateHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ClusterIndex const sensorClusterIndex, la::avdecc::entity::model::SamplingRate const samplingRate)>;
	using SetClockSourceHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::entity::model::ClockSourceIndex const clockSourceIndex)>;
	using GetClockSourceHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::entity::model::ClockSourceIndex const clockSourceIndex)>;
	using SetControlValuesHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ControlIndex const controlIndex, la::avdecc::MemoryBuffer const& packedControlValues)>;
	using GetControlValuesHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ControlIndex const controlIndex, la::avdecc::MemoryBuffer const& packedControlValues)>;
	using StartStreamInputHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex)>;
	using StartStreamOutputHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex)>;
	using StopStreamInputHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex)>;
	using StopStreamOutputHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex)>;
	using GetAvbInfoHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::model::AvbInfo const& info)>;
	using GetAsPathHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::model::AsPath const& asPath)>;
	using GetEntityCountersHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::EntityCounterValidFlags const validCounters, la::avdecc::entity::model::DescriptorCounters const& counters)>;
	using GetAvbInterfaceCountersHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::AvbInterfaceCounterValidFlags const validCounters, la::avdecc::entity::model::DescriptorCounters const& counters)>;
	using GetClockDomainCountersHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::entity::ClockDomainCounterValidFlags const validCounters, la::avdecc::entity::model::DescriptorCounters const& counters)>;
	using GetStreamInputCountersHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::StreamInputCounterValidFlags const validCounters, la::avdecc::entity::model::DescriptorCounters const& counters)>;
	using GetStreamOutputCountersHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::StreamOutputCounterValidFlags const validCounters, la::avdecc::entity::model::DescriptorCounters const& counters)>;
	using RebootHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status)>;
	using RebootToFirmwareHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::MemoryObjectIndex const memoryObjectIndex)>;
	using StartOperationHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex, la::avdecc::entity::model::OperationID const operationID, la::avdecc::entity::model::MemoryObjectOperationType const operationType, la::avdecc::MemoryBuffer const& memoryBuffer)>;
	using AbortOperationHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex, la::avdecc::entity::model::OperationID const operationID)>;
	using SetMemoryObjectLengthHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length)>;
	using GetMemoryObjectLengthHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length)>;
	/* Enumeration and Control Protocol (AECP) AA handlers */
	using AddressAccessHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AaCommandStatus const status, la::avdecc::entity::addressAccess::Tlvs const& tlvs)>;
	/* Enumeration and Control Protocol (AECP) MVU handlers (Milan Vendor Unique) */
	using GetMilanInfoHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::MvuCommandStatus const status, la::avdecc::entity::model::MilanInfo const& info)>;
	/* Connection Management Protocol (ACMP) handlers */
	using ConnectStreamHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status)>;
	using DisconnectStreamHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status)>;
	using DisconnectTalkerStreamHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status)>;
	using GetTalkerStreamStateHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status)>;
	using GetListenerStreamStateHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status)>;
	using GetTalkerStreamConnectionHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status)>;

	/* Enumeration and Control Protocol (AECP) AEM */
	virtual void acquireEntity(la::avdecc::UniqueIdentifier const targetEntityID, bool const isPersistent, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex, AcquireEntityHandler const& handler) const noexcept = 0;
	virtual void releaseEntity(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex, ReleaseEntityHandler const& handler) const noexcept = 0;
	virtual void lockEntity(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex, LockEntityHandler const& handler) const noexcept = 0;
	virtual void unlockEntity(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex, UnlockEntityHandler const& handler) const noexcept = 0;
	virtual void queryEntityAvailable(la::avdecc::UniqueIdentifier const targetEntityID, QueryEntityAvailableHandler const& handler) const noexcept = 0;
	virtual void queryControllerAvailable(la::avdecc::UniqueIdentifier const targetEntityID, QueryControllerAvailableHandler const& handler) const noexcept = 0;
	virtual void registerUnsolicitedNotifications(la::avdecc::UniqueIdentifier const targetEntityID, RegisterUnsolicitedNotificationsHandler const& handler) const noexcept = 0;
	virtual void unregisterUnsolicitedNotifications(la::avdecc::UniqueIdentifier const targetEntityID, UnregisterUnsolicitedNotificationsHandler const& handler) const noexcept = 0;
	virtual void readEntityDescriptor(la::avdecc::UniqueIdentifier const targetEntityID, EntityDescriptorHandler const& handler) const noexcept = 0;
	virtual void readConfigurationDescriptor(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, ConfigurationDescriptorHandler const& handler) const noexcept = 0;
	virtual void readAudioUnitDescriptor(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AudioUnitIndex const audioUnitIndex, AudioUnitDescriptorHandler const& handler) const noexcept = 0;
	virtual void readStreamInputDescriptor(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, StreamInputDescriptorHandler const& handler) const noexcept = 0;
	virtual void readStreamOutputDescriptor(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, StreamOutputDescriptorHandler const& handler) const noexcept = 0;
	virtual void readJackInputDescriptor(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::JackIndex const jackIndex, JackInputDescriptorHandler const& handler) const noexcept = 0;
	virtual void readJackOutputDescriptor(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::JackIndex const jackIndex, JackOutputDescriptorHandler const& handler) const noexcept = 0;
	virtual void readAvbInterfaceDescriptor(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, AvbInterfaceDescriptorHandler const& handler) const noexcept = 0;
	virtual void readClockSourceDescriptor(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockSourceIndex const clockSourceIndex, ClockSourceDescriptorHandler const& handler) const noexcept = 0;
	virtual void readMemoryObjectDescriptor(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::MemoryObjectIndex const memoryObjectIndex, MemoryObjectDescriptorHandler const& handler) const noexcept = 0;
	virtual void readLocaleDescriptor(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::LocaleIndex const localeIndex, LocaleDescriptorHandler const& handler) const noexcept = 0;
	virtual void readStringsDescriptor(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StringsIndex const stringsIndex, StringsDescriptorHandler const& handler) const noexcept = 0;
	virtual void readStreamPortInputDescriptor(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, StreamPortInputDescriptorHandler const& handler) const noexcept = 0;
	virtual void readStreamPortOutputDescriptor(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, StreamPortOutputDescriptorHandler const& handler) const noexcept = 0;
	virtual void readExternalPortInputDescriptor(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ExternalPortIndex const externalPortIndex, ExternalPortInputDescriptorHandler const& handler) const noexcept = 0;
	virtual void readExternalPortOutputDescriptor(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ExternalPortIndex const externalPortIndex, ExternalPortOutputDescriptorHandler const& handler) const noexcept = 0;
	virtual void readInternalPortInputDescriptor(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::InternalPortIndex const internalPortIndex, InternalPortInputDescriptorHandler const& handler) const noexcept = 0;
	virtual void readInternalPortOutputDescriptor(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::InternalPortIndex const internalPortIndex, InternalPortOutputDescriptorHandler const& handler) const noexcept = 0;
	virtual void readAudioClusterDescriptor(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClusterIndex const clusterIndex, AudioClusterDescriptorHandler const& handler) const noexcept = 0;
	virtual void readAudioMapDescriptor(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::MapIndex const mapIndex, AudioMapDescriptorHandler const& handler) const noexcept = 0;
	virtual void readControlDescriptor(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ControlIndex const controlIndex, ControlDescriptorHandler const& handler) const noexcept = 0;
	virtual void readClockDomainDescriptor(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, ClockDomainDescriptorHandler const& handler) const noexcept = 0;
	virtual void readTimingDescriptor(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::TimingIndex const timingIndex, TimingDescriptorHandler const& handler) const noexcept = 0;
	virtual void readPtpInstanceDescriptor(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::PtpInstanceIndex const ptpInstanceIndex, PtpInstanceDescriptorHandler const& handler) const noexcept = 0;
	virtual void readPtpPortDescriptor(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::PtpPortIndex const ptpPortIndex, PtpPortDescriptorHandler const& handler) const noexcept = 0;
	virtual void setConfiguration(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, SetConfigurationHandler const& handler) const noexcept = 0;
	virtual void getConfiguration(la::avdecc::UniqueIdentifier const targetEntityID, GetConfigurationHandler const& handler) const noexcept = 0;
	virtual void setStreamInputFormat(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamFormat const streamFormat, SetStreamInputFormatHandler const& handler) const noexcept = 0;
	virtual void getStreamInputFormat(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, GetStreamInputFormatHandler const& handler) const noexcept = 0;
	virtual void setStreamOutputFormat(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamFormat const streamFormat, SetStreamOutputFormatHandler const& handler) const noexcept = 0;
	virtual void getStreamOutputFormat(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, GetStreamOutputFormatHandler const& handler) const noexcept = 0;
	virtual void getStreamPortInputAudioMap(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::MapIndex const mapIndex, GetStreamPortInputAudioMapHandler const& handler) const noexcept = 0;
	virtual void getStreamPortOutputAudioMap(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::MapIndex const mapIndex, GetStreamPortOutputAudioMapHandler const& handler) const noexcept = 0;
	virtual void addStreamPortInputAudioMappings(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::AudioMappings const& mappings, AddStreamPortInputAudioMappingsHandler const& handler) const noexcept = 0;
	virtual void addStreamPortOutputAudioMappings(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::AudioMappings const& mappings, AddStreamPortOutputAudioMappingsHandler const& handler) const noexcept = 0;
	virtual void removeStreamPortInputAudioMappings(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::AudioMappings const& mappings, RemoveStreamPortInputAudioMappingsHandler const& handler) const noexcept = 0;
	virtual void removeStreamPortOutputAudioMappings(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::AudioMappings const& mappings, RemoveStreamPortOutputAudioMappingsHandler const& handler) const noexcept = 0;
	virtual void setStreamInputInfo(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamInfo const& info, SetStreamInputInfoHandler const& handler) const noexcept = 0;
	virtual void setStreamOutputInfo(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamInfo const& info, SetStreamOutputInfoHandler const& handler) const noexcept = 0;
	virtual void getStreamInputInfo(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, GetStreamInputInfoHandler const& handler) const noexcept = 0;
	virtual void getStreamOutputInfo(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, GetStreamOutputInfoHandler const& handler) const noexcept = 0;
	virtual void setEntityName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::AvdeccFixedString const& entityName, SetEntityNameHandler const& handler) const noexcept = 0;
	virtual void getEntityName(la::avdecc::UniqueIdentifier const targetEntityID, GetEntityNameHandler const& handler) const noexcept = 0;
	virtual void setEntityGroupName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::AvdeccFixedString const& entityGroupName, SetEntityGroupNameHandler const& handler) const noexcept = 0;
	virtual void getEntityGroupName(la::avdecc::UniqueIdentifier const targetEntityID, GetEntityGroupNameHandler const& handler) const noexcept = 0;
	virtual void setConfigurationName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AvdeccFixedString const& configurationName, SetConfigurationNameHandler const& handler) const noexcept = 0;
	virtual void getConfigurationName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, GetConfigurationNameHandler const& handler) const noexcept = 0;
	virtual void setAudioUnitName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AudioUnitIndex const audioUnitIndex, la::avdecc::entity::model::AvdeccFixedString const& audioUnitName, SetAudioUnitNameHandler const& handler) const noexcept = 0;
	virtual void getAudioUnitName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AudioUnitIndex const audioUnitIndex, GetAudioUnitNameHandler const& handler) const noexcept = 0;
	virtual void setStreamInputName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::AvdeccFixedString const& streamInputName, SetStreamInputNameHandler const& handler) const noexcept = 0;
	virtual void getStreamInputName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, GetStreamInputNameHandler const& handler) const noexcept = 0;
	virtual void setStreamOutputName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::AvdeccFixedString const& streamOutputName, SetStreamOutputNameHandler const& handler) const noexcept = 0;
	virtual void getStreamOutputName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, GetStreamOutputNameHandler const& handler) const noexcept = 0;
	virtual void setJackInputName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::JackIndex const jackIndex, la::avdecc::entity::model::AvdeccFixedString const& jackInputName, SetJackInputNameHandler const& handler) const noexcept = 0;
	virtual void getJackInputName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::JackIndex const jackIndex, GetJackInputNameHandler const& handler) const noexcept = 0;
	virtual void setJackOutputName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::JackIndex const jackIndex, la::avdecc::entity::model::AvdeccFixedString const& jackOutputName, SetJackOutputNameHandler const& handler) const noexcept = 0;
	virtual void getJackOutputName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::JackIndex const jackIndex, GetJackOutputNameHandler const& handler) const noexcept = 0;
	virtual void setAvbInterfaceName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::model::AvdeccFixedString const& avbInterfaceName, SetAvbInterfaceNameHandler const& handler) const noexcept = 0;
	virtual void getAvbInterfaceName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, GetAvbInterfaceNameHandler const& handler) const noexcept = 0;
	virtual void setClockSourceName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockSourceIndex const clockSourceIndex, la::avdecc::entity::model::AvdeccFixedString const& clockSourceName, SetClockSourceNameHandler const& handler) const noexcept = 0;
	virtual void getClockSourceName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockSourceIndex const clockSourceIndex, GetClockSourceNameHandler const& handler) const noexcept = 0;
	virtual void setMemoryObjectName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::MemoryObjectIndex const memoryObjectIndex, la::avdecc::entity::model::AvdeccFixedString const& memoryObjectName, SetMemoryObjectNameHandler const& handler) const noexcept = 0;
	virtual void getMemoryObjectName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::MemoryObjectIndex const memoryObjectIndex, GetMemoryObjectNameHandler const& handler) const noexcept = 0;
	virtual void setAudioClusterName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClusterIndex const audioClusterIndex, la::avdecc::entity::model::AvdeccFixedString const& audioClusterName, SetAudioClusterNameHandler const& handler) const noexcept = 0;
	virtual void getAudioClusterName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClusterIndex const audioClusterIndex, GetAudioClusterNameHandler const& handler) const noexcept = 0;
	virtual void setControlName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ControlIndex const controlIndex, la::avdecc::entity::model::AvdeccFixedString const& controlName, SetControlNameHandler const& handler) const noexcept = 0;
	virtual void getControlName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ControlIndex const controlIndex, GetControlNameHandler const& handler) const noexcept = 0;
	virtual void setClockDomainName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::entity::model::AvdeccFixedString const& clockDomainName, SetClockDomainNameHandler const& handler) const noexcept = 0;
	virtual void getClockDomainName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, GetClockDomainNameHandler const& handler) const noexcept = 0;
	virtual void setTimingName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::TimingIndex const timingIndex, la::avdecc::entity::model::AvdeccFixedString const& timingName, SetTimingNameHandler const& handler) const noexcept = 0;
	virtual void getTimingName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::TimingIndex const timingIndex, GetTimingNameHandler const& handler) const noexcept = 0;
	virtual void setPtpInstanceName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::PtpInstanceIndex const ptpInstanceIndex, la::avdecc::entity::model::AvdeccFixedString const& ptpInstanceName, SetPtpInstanceNameHandler const& handler) const noexcept = 0;
	virtual void getPtpInstanceName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::PtpInstanceIndex const ptpInstanceIndex, GetPtpInstanceNameHandler const& handler) const noexcept = 0;
	virtual void setPtpPortName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::PtpPortIndex const ptpPortIndex, la::avdecc::entity::model::AvdeccFixedString const& ptpPortName, SetPtpPortNameHandler const& handler) const noexcept = 0;
	virtual void getPtpPortName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::PtpPortIndex const ptpPortIndex, GetPtpPortNameHandler const& handler) const noexcept = 0;
	virtual void setAssociation(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::UniqueIdentifier const associationID, SetAssociationHandler const& handler) const noexcept = 0;
	virtual void getAssociation(la::avdecc::UniqueIdentifier const targetEntityID, GetAssociationHandler const& handler) const noexcept = 0;
	virtual void setAudioUnitSamplingRate(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::AudioUnitIndex const audioUnitIndex, la::avdecc::entity::model::SamplingRate const samplingRate, SetAudioUnitSamplingRateHandler const& handler) const noexcept = 0;
	virtual void getAudioUnitSamplingRate(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::AudioUnitIndex const audioUnitIndex, GetAudioUnitSamplingRateHandler const& handler) const noexcept = 0;
	virtual void setVideoClusterSamplingRate(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ClusterIndex const videoClusterIndex, la::avdecc::entity::model::SamplingRate const samplingRate, SetVideoClusterSamplingRateHandler const& handler) const noexcept = 0;
	virtual void getVideoClusterSamplingRate(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ClusterIndex const videoClusterIndex, GetVideoClusterSamplingRateHandler const& handler) const noexcept = 0;
	virtual void setSensorClusterSamplingRate(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ClusterIndex const sensorClusterIndex, la::avdecc::entity::model::SamplingRate const samplingRate, SetSensorClusterSamplingRateHandler const& handler) const noexcept = 0;
	virtual void getSensorClusterSamplingRate(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ClusterIndex const sensorClusterIndex, GetSensorClusterSamplingRateHandler const& handler) const noexcept = 0;
	virtual void setClockSource(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::entity::model::ClockSourceIndex const clockSourceIndex, SetClockSourceHandler const& handler) const noexcept = 0;
	virtual void getClockSource(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, GetClockSourceHandler const& handler) const noexcept = 0;
	virtual void setControlValues(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ControlIndex const controlIndex, la::avdecc::entity::model::ControlValues const& controlValues, SetControlValuesHandler const& handler) const noexcept = 0;
	virtual void getControlValues(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ControlIndex const controlIndex, GetControlValuesHandler const& handler) const noexcept = 0;
	virtual void startStreamInput(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, StartStreamInputHandler const& handler) const noexcept = 0;
	virtual void startStreamOutput(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, StartStreamOutputHandler const& handler) const noexcept = 0;
	virtual void stopStreamInput(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, StopStreamInputHandler const& handler) const noexcept = 0;
	virtual void stopStreamOutput(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, StopStreamOutputHandler const& handler) const noexcept = 0;
	virtual void getAvbInfo(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, GetAvbInfoHandler const& handler) const noexcept = 0;
	virtual void getAsPath(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, GetAsPathHandler const& handler) const noexcept = 0;
	virtual void getEntityCounters(la::avdecc::UniqueIdentifier const targetEntityID, GetEntityCountersHandler const& handler) const noexcept = 0;
	virtual void getAvbInterfaceCounters(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, GetAvbInterfaceCountersHandler const& handler) const noexcept = 0;
	virtual void getClockDomainCounters(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, GetClockDomainCountersHandler const& handler) const noexcept = 0;
	virtual void getStreamInputCounters(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, GetStreamInputCountersHandler const& handler) const noexcept = 0;
	virtual void getStreamOutputCounters(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, GetStreamOutputCountersHandler const& handler) const noexcept = 0;
	virtual void reboot(la::avdecc::UniqueIdentifier const targetEntityID, RebootHandler const& handler) const noexcept = 0;
	virtual void rebootToFirmware(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::MemoryObjectIndex const memoryObjectIndex, RebootToFirmwareHandler const& handler) const noexcept = 0;
	virtual void startOperation(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex, la::avdecc::entity::model::MemoryObjectOperationType const operationType, la::avdecc::MemoryBuffer const& memoryBuffer, StartOperationHandler const& handler) const noexcept = 0;
	virtual void abortOperation(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex, la::avdecc::entity::model::OperationID const operationID, AbortOperationHandler const& handler) const noexcept = 0;
	virtual void setMemoryObjectLength(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length, SetMemoryObjectLengthHandler const& handler) const noexcept = 0;
	virtual void getMemoryObjectLength(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::MemoryObjectIndex const memoryObjectIndex, GetMemoryObjectLengthHandler const& handler) const noexcept = 0;

	/* Enumeration and Control Protocol (AECP) AA */
	virtual void addressAccess(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::addressAccess::Tlvs const& tlvs, AddressAccessHandler const& handler) const noexcept = 0;

	/* Enumeration and Control Protocol (AECP) MVU (Milan Vendor Unique) */
	virtual void getMilanInfo(la::avdecc::UniqueIdentifier const targetEntityID, GetMilanInfoHandler const& handler) const noexcept = 0;

	/* Connection Management Protocol (ACMP) */
	virtual void connectStream(la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, ConnectStreamHandler const& handler) const noexcept = 0;
	virtual void disconnectStream(la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, DisconnectStreamHandler const& handler) const noexcept = 0;
	virtual void disconnectTalkerStream(la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, DisconnectTalkerStreamHandler const& handler) const noexcept = 0;
	virtual void getTalkerStreamState(la::avdecc::entity::model::StreamIdentification const& talkerStream, GetTalkerStreamStateHandler const& handler) const noexcept = 0;
	virtual void getListenerStreamState(la::avdecc::entity::model::StreamIdentification const& listenerStream, GetListenerStreamStateHandler const& handler) const noexcept = 0;
	virtual void getTalkerStreamConnection(la::avdecc::entity::model::StreamIdentification const& talkerStream, std::uint16_t const connectionIndex, GetTalkerStreamConnectionHandler const& handler) const noexcept = 0;

	// Defaulted compiler auto-generated methods
	Interface() noexcept = default;
	virtual ~Interface() noexcept = default;
	Interface(Interface&&) = default;
	Interface(Interface const&) = default;
	Interface& operator=(Interface const&) = default;
	Interface& operator=(Interface&&) = default;
};

/** Delegate for all controller related notifications. */
class Delegate
{
public:
	/* Global notifications */
	/** Called when a fatal error on the transport layer occured. */
	virtual void onTransportError(la::avdecc::entity::controller::Interface const* const controller) noexcept = 0;

	/* Discovery Protocol (ADP) */
	/** Called when a new entity was discovered on the network (either local or remote). */
	virtual void onEntityOnline(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::Entity const& entity) noexcept = 0;
	/** Called when an already discovered entity updated its discovery (ADP) information. */
	virtual void onEntityUpdate(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::Entity const& entity) noexcept = 0; // When GpgpGrandMasterID, GpgpDomainNumber or EntityCapabilities changed
	/** Called when an already discovered entity went offline or timed out (either local or remote). */
	virtual void onEntityOffline(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID) noexcept = 0;

	/* Connection Management Protocol sniffed messages (ACMP) (not triggered for our own commands even though ACMP messages are broadcasted, the command's 'result' method will be called in that case) */
	/** Called when a controller connect request has been sniffed on the network. */
	virtual void onControllerConnectResponseSniffed(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status) noexcept = 0;
	/** Called when a controller disconnect request has been sniffed on the network. */
	virtual void onControllerDisconnectResponseSniffed(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status) noexcept = 0;
	/** Called when a listener connect request has been sniffed on the network (either due to a another controller connect, or a fast connect). */
	virtual void onListenerConnectResponseSniffed(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status) noexcept = 0;
	/** Called when a listener disconnect request has been sniffed on the network (either due to a another controller disconnect, or a fast disconnect). */
	virtual void onListenerDisconnectResponseSniffed(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status) noexcept = 0;
	/** Called when a stream state query has been sniffed on the network. */
	virtual void onGetTalkerStreamStateResponseSniffed(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status) noexcept = 0;
	/** Called when a stream state query has been sniffed on the network */
	virtual void onGetListenerStreamStateResponseSniffed(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status) noexcept = 0;

	/* Unsolicited notifications (not triggered for our own commands, the command's 'result' method will be called in that case). Only successfull commands can cause an unsolicited notification. */
	/** Called when an entity has been deregistered from unsolicited notifications. */
	virtual void onDeregisteredFromUnsolicitedNotifications(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID) noexcept = 0;
	/** Called when an entity has been acquired by another controller. */
	virtual void onEntityAcquired(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::UniqueIdentifier const owningEntity, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex) noexcept = 0;
	/** Called when an entity has been released by another controller. */
	virtual void onEntityReleased(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::UniqueIdentifier const owningEntity, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex) noexcept = 0;
	/** Called when an entity has been locked by another controller. */
	virtual void onEntityLocked(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::UniqueIdentifier const lockingEntity, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex) noexcept = 0;
	/** Called when an entity has been unlocked by another controller (or because of the lock timeout). */
	virtual void onEntityUnlocked(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::UniqueIdentifier const lockingEntity, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex) noexcept = 0;
	/** Called when the current configuration was changed by another controller. */
	virtual void onConfigurationChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex) noexcept = 0;
	/** Called when the format of an input stream was changed by another controller. */
	virtual void onStreamInputFormatChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamFormat const streamFormat) noexcept = 0;
	/** Called when the format of an output stream was changed by another controller. */
	virtual void onStreamOutputFormatChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamFormat const streamFormat) noexcept = 0;
	/** Called when the audio mappings of a stream port input was changed by another controller. */
	virtual void onStreamPortInputAudioMappingsChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::MapIndex const numberOfMaps, la::avdecc::entity::model::MapIndex const mapIndex, la::avdecc::entity::model::AudioMappings const& mappings) noexcept = 0;
	/** Called when the audio mappings of a stream port output was changed by another controller. */
	virtual void onStreamPortOutputAudioMappingsChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::MapIndex const numberOfMaps, la::avdecc::entity::model::MapIndex const mapIndex, la::avdecc::entity::model::AudioMappings const& mappings) noexcept = 0;
	/** Called when the information of an input stream was changed by another controller. */
	virtual void onStreamInputInfoChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamInfo const& info, bool const fromGetStreamInfoResponse) noexcept = 0;
	/** Called when the information of an output stream was changed by another controller. */
	virtual void onStreamOutputInfoChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamInfo const& info, bool const fromGetStreamInfoResponse) noexcept = 0;
	/** Called when the entity's name was changed by another controller. */
	virtual void onEntityNameChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::AvdeccFixedString const& entityName) noexcept = 0;
	/** Called when the entity's group name was changed by another controller. */
	virtual void onEntityGroupNameChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::AvdeccFixedString const& entityGroupName) noexcept = 0;
	/** Called when a configuration name was changed by another controller. */
	virtual void onConfigurationNameChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AvdeccFixedString const& configurationName) noexcept = 0;
	/** Called when an audio unit name was changed by another controller. */
	virtual void onAudioUnitNameChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AudioUnitIndex const audioUnitIndex, la::avdecc::entity::model::AvdeccFixedString const& audioUnitName) noexcept = 0;
	/** Called when an input stream name was changed by another controller. */
	virtual void onStreamInputNameChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::AvdeccFixedString const& streamName) noexcept = 0;
	/** Called when an output stream name was changed by another controller. */
	virtual void onStreamOutputNameChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::AvdeccFixedString const& streamName) noexcept = 0;
	/** Called when an input jack name was changed by another controller. */
	virtual void onJackInputNameChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::JackIndex const jackIndex, la::avdecc::entity::model::AvdeccFixedString const& jackName) noexcept = 0;
	/** Called when an output jack name was changed by another controller. */
	virtual void onJackOutputNameChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::JackIndex const jackIndex, la::avdecc::entity::model::AvdeccFixedString const& jackName) noexcept = 0;
	/** Called when an avb interface name was changed by another controller. */
	virtual void onAvbInterfaceNameChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::model::AvdeccFixedString const& avbInterfaceName) noexcept = 0;
	/** Called when a clock source name was changed by another controller. */
	virtual void onClockSourceNameChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockSourceIndex const clockSourceIndex, la::avdecc::entity::model::AvdeccFixedString const& clockSourceName) noexcept = 0;
	/** Called when a memory object name was changed by another controller. */
	virtual void onMemoryObjectNameChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::MemoryObjectIndex const memoryObjectIndex, la::avdecc::entity::model::AvdeccFixedString const& memoryObjectName) noexcept = 0;
	/** Called when an audio cluster name was changed by another controller. */
	virtual void onAudioClusterNameChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClusterIndex const audioClusterIndex, la::avdecc::entity::model::AvdeccFixedString const& audioClusterName) noexcept = 0;
	/** Called when a control name was changed by another controller. */
	virtual void onControlNameChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ControlIndex const controlIndex, la::avdecc::entity::model::AvdeccFixedString const& controlName) noexcept = 0;
	/** Called when a clock domain name was changed by another controller. */
	virtual void onClockDomainNameChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::entity::model::AvdeccFixedString const& clockDomainName) noexcept = 0;
	/** Called when a timing name was changed by another controller. */
	virtual void onTimingNameChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::TimingIndex const timingIndex, la::avdecc::entity::model::AvdeccFixedString const& timingName) noexcept = 0;
	/** Called when a ptp instance name was changed by another controller. */
	virtual void onPtpInstanceNameChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::PtpInstanceIndex const ptpInstanceIndex, la::avdecc::entity::model::AvdeccFixedString const& ptpInstanceName) noexcept = 0;
	/** Called when a ptp port name was changed by another controller. */
	virtual void onPtpPortNameChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::PtpPortIndex const ptpPortIndex, la::avdecc::entity::model::AvdeccFixedString const& ptpPortName) noexcept = 0;
	/** Called when then associationID was changed by another controller. */
	virtual void onAssociationIDChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::UniqueIdentifier const associationID) noexcept = 0;
	/** Called when an AudioUnit sampling rate was changed by another controller. */
	virtual void onAudioUnitSamplingRateChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::AudioUnitIndex const audioUnitIndex, la::avdecc::entity::model::SamplingRate const samplingRate) noexcept = 0;
	/** Called when a VideoCluster sampling rate was changed by another controller. */
	virtual void onVideoClusterSamplingRateChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::ClusterIndex const videoClusterIndex, la::avdecc::entity::model::SamplingRate const samplingRate) noexcept = 0;
	/** Called when a SensorCluster sampling rate was changed by another controller. */
	virtual void onSensorClusterSamplingRateChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::ClusterIndex const sensorClusterIndex, la::avdecc::entity::model::SamplingRate const samplingRate) noexcept = 0;
	/** Called when a clock source was changed by another controller. */
	virtual void onClockSourceChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::entity::model::ClockSourceIndex const clockSourceIndex) noexcept = 0;
	/** Called when a control's values was changed by another controller. */
	virtual void onControlValuesChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::ControlIndex const controlIndex, la::avdecc::MemoryBuffer const& packedControlValues) noexcept = 0;
	/** Called when an input stream was started by another controller. */
	virtual void onStreamInputStarted(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::StreamIndex const streamIndex) noexcept = 0;
	/** Called when an output stream was started by another controller. */
	virtual void onStreamOutputStarted(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::StreamIndex const streamIndex) noexcept = 0;
	/** Called when an input stream was stopped by another controller. */
	virtual void onStreamInputStopped(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::StreamIndex const streamIndex) noexcept = 0;
	/** Called when an output stream was stopped by another controller. */
	virtual void onStreamOutputStopped(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::StreamIndex const streamIndex) noexcept = 0;
	/** Called when the Avb Info of an Avb Interface changed. */
	virtual void onAvbInfoChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::model::AvbInfo const& info) noexcept = 0;
	/** Called when the AS Path of an Avb Interface changed. */
	virtual void onAsPathChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::model::AsPath const& asPath) noexcept = 0;
	/** Called when the counters of Entity changed. */
	virtual void onEntityCountersChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::EntityCounterValidFlags const validCounters, la::avdecc::entity::model::DescriptorCounters const& counters) noexcept = 0;
	/** Called when the counters of an Avb Interface changed. */
	virtual void onAvbInterfaceCountersChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::AvbInterfaceCounterValidFlags const validCounters, la::avdecc::entity::model::DescriptorCounters const& counters) noexcept = 0;
	/** Called when the counters of a Clock Domain changed. */
	virtual void onClockDomainCountersChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::entity::ClockDomainCounterValidFlags const validCounters, la::avdecc::entity::model::DescriptorCounters const& counters) noexcept = 0;
	/** Called when the counters of a Stream Input changed. */
	virtual void onStreamInputCountersChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::StreamInputCounterValidFlags const validCounters, la::avdecc::entity::model::DescriptorCounters const& counters) noexcept = 0;
	/** Called when the counters of a Stream Output changed. */
	virtual void onStreamOutputCountersChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::StreamOutputCounterValidFlags const validCounters, la::avdecc::entity::model::DescriptorCounters const& counters) noexcept = 0;
	/** Called when (some or all) audio mappings of a stream port input were added by another controller. */
	virtual void onStreamPortInputAudioMappingsAdded(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::AudioMappings const& mappings) noexcept = 0;
	/** Called when (some or all) audio mappings of a stream port output were added by another controller. */
	virtual void onStreamPortOutputAudioMappingsAdded(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::AudioMappings const& mappings) noexcept = 0;
	/** Called when (some or all) audio mappings of a stream port input were removed by another controller. */
	virtual void onStreamPortInputAudioMappingsRemoved(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::AudioMappings const& mappings) noexcept = 0;
	/** Called when (some or all) audio mappings of a stream port output were removed by another controller. */
	virtual void onStreamPortOutputAudioMappingsRemoved(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::AudioMappings const& mappings) noexcept = 0;
	/** Called when the length of a MemoryObject changed. */
	virtual void onMemoryObjectLengthChanged(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length) noexcept = 0;
	/** Called when there is a status update on an ongoing Operation */
	virtual void onOperationStatus(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex, la::avdecc::entity::model::OperationID const operationID, std::uint16_t const percentComplete) noexcept = 0;

	/* Identification notifications */
	/** Called when an entity emits an identify notification. */
	virtual void onEntityIdentifyNotification(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID) noexcept = 0;

	/* **** Statistics **** */
	/** Notification for when an AECP Command was resent due to a timeout. If the retry time out again, then onAecpTimeout will be called. */
	virtual void onAecpRetry(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const& entityID) noexcept = 0;
	/** Notification for when an AECP Command timed out (not called when onAecpRetry is called). */
	virtual void onAecpTimeout(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const& entityID) noexcept = 0;
	/** Notification for when an AECP Response is received but is not expected (might have already timed out). */
	virtual void onAecpUnexpectedResponse(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const& entityID) noexcept = 0;
	/** Notification for when an AECP Response is received (not an Unsolicited one) along with the time elapsed between the send and the receive. */
	virtual void onAecpResponseTime(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const& entityID, std::chrono::milliseconds const& responseTime) noexcept = 0;
	/** Notification for when an AEM-AECP Unsolicited Response was received. */
	virtual void onAemAecpUnsolicitedReceived(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const& entityID, la::avdecc::protocol::AecpSequenceID const sequenceID) noexcept = 0;

	// Defaulted compiler auto-generated methods
	Delegate() noexcept = default;
	virtual ~Delegate() noexcept = default;
	Delegate(Delegate&&) = default;
	Delegate(Delegate const&) = default;
	Delegate& operator=(Delegate const&) = default;
	Delegate& operator=(Delegate&&) = default;
};

/** Defaulted version of Delegate. */
class DefaultedDelegate : public Delegate
{
public:
	/* Global notifications */
	/** Called when a fatal error on the transport layer occured. */
	virtual void onTransportError(la::avdecc::entity::controller::Interface const* const /*controller*/) noexcept override {}

	/* Discovery Protocol (ADP) */
	/** Called when a new entity was discovered on the network (either local or remote). */
	virtual void onEntityOnline(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::Entity const& /*entity*/) noexcept override {}
	/** Called when an already discovered entity updated its discovery (ADP) information. */
	virtual void onEntityUpdate(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::Entity const& /*entity*/) noexcept override {} // When GpgpGrandMasterID, GpgpDomainNumber or EntityCapabilities changed
	/** Called when an already discovered entity went offline or timed out (either local or remote). */
	virtual void onEntityOffline(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/) noexcept override {}

	/* Connection Management Protocol sniffed messages (ACMP) (not triggered for our own commands even though ACMP messages are broadcasted, the command's 'result' method will be called in that case) */
	/** Called when a controller connect request has been sniffed on the network. */
	virtual void onControllerConnectResponseSniffed(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::entity::model::StreamIdentification const& /*talkerStream*/, la::avdecc::entity::model::StreamIdentification const& /*listenerStream*/, std::uint16_t const /*connectionCount*/, la::avdecc::entity::ConnectionFlags const /*flags*/, la::avdecc::entity::LocalEntity::ControlStatus const /*status*/) noexcept override {}
	/** Called when a controller disconnect request has been sniffed on the network. */
	virtual void onControllerDisconnectResponseSniffed(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::entity::model::StreamIdentification const& /*talkerStream*/, la::avdecc::entity::model::StreamIdentification const& /*listenerStream*/, std::uint16_t const /*connectionCount*/, la::avdecc::entity::ConnectionFlags const /*flags*/, la::avdecc::entity::LocalEntity::ControlStatus const /*status*/) noexcept override {}
	/** Called when a listener connect request has been sniffed on the network (either due to a another controller connect, or a fast connect). */
	virtual void onListenerConnectResponseSniffed(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::entity::model::StreamIdentification const& /*talkerStream*/, la::avdecc::entity::model::StreamIdentification const& /*listenerStream*/, std::uint16_t const /*connectionCount*/, la::avdecc::entity::ConnectionFlags const /*flags*/, la::avdecc::entity::LocalEntity::ControlStatus const /*status*/) noexcept override {}
	/** Called when a listener disconnect request has been sniffed on the network (either due to a another controller disconnect, or a fast disconnect). */
	virtual void onListenerDisconnectResponseSniffed(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::entity::model::StreamIdentification const& /*talkerStream*/, la::avdecc::entity::model::StreamIdentification const& /*listenerStream*/, std::uint16_t const /*connectionCount*/, la::avdecc::entity::ConnectionFlags const /*flags*/, la::avdecc::entity::LocalEntity::ControlStatus const /*status*/) noexcept override {}
	/** Called when a stream state query has been sniffed on the network. */
	virtual void onGetTalkerStreamStateResponseSniffed(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::entity::model::StreamIdentification const& /*talkerStream*/, la::avdecc::entity::model::StreamIdentification const& /*listenerStream*/, std::uint16_t const /*connectionCount*/, la::avdecc::entity::ConnectionFlags const /*flags*/, la::avdecc::entity::LocalEntity::ControlStatus const /*status*/) noexcept override {}
	/** Called when a stream state query has been sniffed on the network */
	virtual void onGetListenerStreamStateResponseSniffed(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::entity::model::StreamIdentification const& /*talkerStream*/, la::avdecc::entity::model::StreamIdentification const& /*listenerStream*/, std::uint16_t const /*connectionCount*/, la::avdecc::entity::ConnectionFlags const /*flags*/, la::avdecc::entity::LocalEntity::ControlStatus const /*status*/) noexcept override {}

	/* Unsolicited notifications (not triggered for our own commands, the command's 'result' method will be called in that case). Only successfull commands can cause an unsolicited notification. */
	/** Called when an entity has been deregistered from unsolicited notifications. */
	virtual void onDeregisteredFromUnsolicitedNotifications(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/) noexcept override {}
	/** Called when an entity has been acquired by another controller. */
	virtual void onEntityAcquired(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::UniqueIdentifier const /*owningEntity*/, la::avdecc::entity::model::DescriptorType const /*descriptorType*/, la::avdecc::entity::model::DescriptorIndex const /*descriptorIndex*/) noexcept override {}
	/** Called when an entity has been released by another controller. */
	virtual void onEntityReleased(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::UniqueIdentifier const /*owningEntity*/, la::avdecc::entity::model::DescriptorType const /*descriptorType*/, la::avdecc::entity::model::DescriptorIndex const /*descriptorIndex*/) noexcept override {}
	/** Called when an entity has been locked by another controller. */
	virtual void onEntityLocked(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::UniqueIdentifier const /*lockingEntity*/, la::avdecc::entity::model::DescriptorType const /*descriptorType*/, la::avdecc::entity::model::DescriptorIndex const /*descriptorIndex*/) noexcept override {}
	/** Called when an entity has been unlocked by another controller (or because of the lock timeout). */
	virtual void onEntityUnlocked(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::UniqueIdentifier const /*lockingEntity*/, la::avdecc::entity::model::DescriptorType const /*descriptorType*/, la::avdecc::entity::model::DescriptorIndex const /*descriptorIndex*/) noexcept override {}
	/** Called when the current configuration was changed by another controller. */
	virtual void onConfigurationChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/) noexcept override {}
	/** Called when the format of an input stream was changed by another controller. */
	virtual void onStreamInputFormatChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::StreamFormat const /*streamFormat*/) noexcept override {}
	/** Called when the format of an output stream was changed by another controller. */
	virtual void onStreamOutputFormatChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::StreamFormat const /*streamFormat*/) noexcept override {}
	/** Called when the audio mappings of a stream port input was changed by another controller. */
	virtual void onStreamPortInputAudioMappingsChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamPortIndex const /*streamPortIndex*/, la::avdecc::entity::model::MapIndex const /*numberOfMaps*/, la::avdecc::entity::model::MapIndex const /*mapIndex*/, la::avdecc::entity::model::AudioMappings const& /*mappings*/) noexcept override {}
	/** Called when the audio mappings of a stream port output was changed by another controller. */
	virtual void onStreamPortOutputAudioMappingsChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamPortIndex const /*streamPortIndex*/, la::avdecc::entity::model::MapIndex const /*numberOfMaps*/, la::avdecc::entity::model::MapIndex const /*mapIndex*/, la::avdecc::entity::model::AudioMappings const& /*mappings*/) noexcept override {}
	/** Called when the information of an input stream was changed by another controller. */
	virtual void onStreamInputInfoChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::StreamInfo const& /*info*/, bool const /*fromGetStreamInfoResponse*/) noexcept override {}
	/** Called when the information of an output stream was changed by another controller. */
	virtual void onStreamOutputInfoChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::StreamInfo const& /*info*/, bool const /*fromGetStreamInfoResponse*/) noexcept override {}
	/** Called when the entity's name was changed by another controller. */
	virtual void onEntityNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::AvdeccFixedString const& /*entityName*/) noexcept override {}
	/** Called when the entity's group name was changed by another controller. */
	virtual void onEntityGroupNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::AvdeccFixedString const& /*entityGroupName*/) noexcept override {}
	/** Called when a configuration name was changed by another controller. */
	virtual void onConfigurationNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*configurationName*/) noexcept override {}
	/** Called when an audio unit name was changed by another controller. */
	virtual void onAudioUnitNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::AudioUnitIndex const /*audioUnitIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*audioUnitName*/) noexcept override {}
	/** Called when an input stream name was changed by another controller. */
	virtual void onStreamInputNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*streamName*/) noexcept override {}
	/** Called when an output stream name was changed by another controller. */
	virtual void onStreamOutputNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*streamName*/) noexcept override {}
	/** Called when an input jack name was changed by another controller. */
	virtual void onJackInputNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::JackIndex const /*jackIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*jackName*/) noexcept override {}
	/** Called when an output jack name was changed by another controller. */
	virtual void onJackOutputNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::JackIndex const /*jackIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*jackName*/) noexcept override {}
	/** Called when an avb interface name was changed by another controller. */
	virtual void onAvbInterfaceNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::AvbInterfaceIndex const /*avbInterfaceIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*avbInterfaceName*/) noexcept override {}
	/** Called when a clock source name was changed by another controller. */
	virtual void onClockSourceNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::ClockSourceIndex const /*clockSourceIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*clockSourceName*/) noexcept override {}
	/** Called when a memory object name was changed by another controller. */
	virtual void onMemoryObjectNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::MemoryObjectIndex const /*memoryObjectIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*memoryObjectName*/) noexcept override {}
	/** Called when an audio cluster name was changed by another controller. */
	virtual void onAudioClusterNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::ClusterIndex const /*audioClusterIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*audioClusterName*/) noexcept override {}
	/** Called when a control name was changed by another controller. */
	virtual void onControlNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::ControlIndex const /*controlIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*controlName*/) noexcept override {}
	/** Called when a clock domain name was changed by another controller. */
	virtual void onClockDomainNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::ClockDomainIndex const /*clockDomainIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*clockDomainName*/) noexcept override {}
	/** Called when a timing name was changed by another controller. */
	virtual void onTimingNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::TimingIndex const /*timingIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*timingName*/) noexcept override {}
	/** Called when a ptp instance name was changed by another controller. */
	virtual void onPtpInstanceNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::PtpInstanceIndex const /*ptpInstanceIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*ptpInstanceName*/) noexcept override {}
	/** Called when a ptp port name was changed by another controller. */
	virtual void onPtpPortNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::PtpPortIndex const /*ptpPortIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*ptpPortName*/) noexcept override {}
	/** Called when then associationID was changed by another controller. */
	virtual void onAssociationIDChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::UniqueIdentifier const /*associationID*/) noexcept override {}
	/** Called when an AudioUnit sampling rate was changed by another controller. */
	virtual void onAudioUnitSamplingRateChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::AudioUnitIndex const /*audioUnitIndex*/, la::avdecc::entity::model::SamplingRate const /*samplingRate*/) noexcept override {}
	/** Called when a VideoCluster sampling rate was changed by another controller. */
	virtual void onVideoClusterSamplingRateChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ClusterIndex const /*videoClusterIndex*/, la::avdecc::entity::model::SamplingRate const /*samplingRate*/) noexcept override {}
	/** Called when a SensorCluster sampling rate was changed by another controller. */
	virtual void onSensorClusterSamplingRateChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ClusterIndex const /*sensorClusterIndex*/, la::avdecc::entity::model::SamplingRate const /*samplingRate*/) noexcept override {}
	/** Called when a clock source was changed by another controller. */
	virtual void onClockSourceChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ClockDomainIndex const /*clockDomainIndex*/, la::avdecc::entity::model::ClockSourceIndex const /*clockSourceIndex*/) noexcept override {}
	/** Called when a control's values was changed by another controller. */
	virtual void onControlValuesChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ControlIndex const /*controlIndex*/, la::avdecc::MemoryBuffer const& /*packedControlValues*/) noexcept override {}
	/** Called when an input stream was started by another controller. */
	virtual void onStreamInputStarted(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/) noexcept override {}
	/** Called when an output stream was started by another controller. */
	virtual void onStreamOutputStarted(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/) noexcept override {}
	/** Called when an input stream was stopped by another controller. */
	virtual void onStreamInputStopped(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/) noexcept override {}
	/** Called when an output stream was stopped by another controller. */
	virtual void onStreamOutputStopped(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/) noexcept override {}
	/** Called when the Avb Info of an Avb Interface changed. */
	virtual void onAvbInfoChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::AvbInterfaceIndex const /*avbInterfaceIndex*/, la::avdecc::entity::model::AvbInfo const& /*info*/) noexcept override {}
	/** Called when the AS Path of an Avb Interface changed. */
	virtual void onAsPathChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::AvbInterfaceIndex const /*avbInterfaceIndex*/, la::avdecc::entity::model::AsPath const& /*asPath*/) noexcept override {}
	/** Called when the counters of Entity changed. */
	virtual void onEntityCountersChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::EntityCounterValidFlags const /*validCounters*/, la::avdecc::entity::model::DescriptorCounters const& /*counters*/) noexcept override {}
	/** Called when the counters of an Avb Interface changed. */
	virtual void onAvbInterfaceCountersChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::AvbInterfaceIndex const /*avbInterfaceIndex*/, la::avdecc::entity::AvbInterfaceCounterValidFlags const /*validCounters*/, la::avdecc::entity::model::DescriptorCounters const& /*counters*/) noexcept override {}
	/** Called when the counters of a Clock Domain changed. */
	virtual void onClockDomainCountersChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ClockDomainIndex const /*clockDomainIndex*/, la::avdecc::entity::ClockDomainCounterValidFlags const /*validCounters*/, la::avdecc::entity::model::DescriptorCounters const& /*counters*/) noexcept override {}
	/** Called when the counters of a Stream Input changed. */
	virtual void onStreamInputCountersChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::StreamInputCounterValidFlags const /*validCounters*/, la::avdecc::entity::model::DescriptorCounters const& /*counters*/) noexcept override {}
	/** Called when the counters of a Stream Output changed. */
	virtual void onStreamOutputCountersChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::StreamOutputCounterValidFlags const /*validCounters*/, la::avdecc::entity::model::DescriptorCounters const& /*counters*/) noexcept override {}
	/** Called when (some or all) audio mappings of a stream port input were added by another controller. */
	virtual void onStreamPortInputAudioMappingsAdded(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamPortIndex const /*streamPortIndex*/, la::avdecc::entity::model::AudioMappings const& /*mappings*/) noexcept override {}
	/** Called when (some or all) audio mappings of a stream port output were added by another controller. */
	virtual void onStreamPortOutputAudioMappingsAdded(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamPortIndex const /*streamPortIndex*/, la::avdecc::entity::model::AudioMappings const& /*mappings*/) noexcept override {}
	/** Called when (some or all) audio mappings of a stream port input were removed by another controller. */
	virtual void onStreamPortInputAudioMappingsRemoved(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamPortIndex const /*streamPortIndex*/, la::avdecc::entity::model::AudioMappings const& /*mappings*/) noexcept override {}
	/** Called when (some or all) audio mappings of a stream port output were removed by another controller. */
	virtual void onStreamPortOutputAudioMappingsRemoved(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamPortIndex const /*streamPortIndex*/, la::avdecc::entity::model::AudioMappings const& /*mappings*/) noexcept override {}
	/** Called when the length of a MemoryObject changed. */
	virtual void onMemoryObjectLengthChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::MemoryObjectIndex const /*memoryObjectIndex*/, std::uint64_t const /*length*/) noexcept override {}
	/** Called when there is a status update on an ongoing Operation */
	virtual void onOperationStatus(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::DescriptorType const /*descriptorType*/, la::avdecc::entity::model::DescriptorIndex const /*descriptorIndex*/, la::avdecc::entity::model::OperationID const /*operationID*/, std::uint16_t const /*percentComplete*/) noexcept override {}

	/* Identification notifications */
	/** Called when an entity emits an identify notification. */
	virtual void onEntityIdentifyNotification(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/) noexcept override {}

	/* **** Statistics **** */
	/** Notification for when an AECP Command was resent due to a timeout. If the retry time out again, then onAecpTimeout will be called. */
	virtual void onAecpRetry(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const& /*entityID*/) noexcept override {}
	/** Notification for when an AECP Command timed out (not called when onAecpRetry is called). */
	virtual void onAecpTimeout(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const& /*entityID*/) noexcept override {}
	/** Notification for when an AECP Response is received but is not expected (might have already timed out). */
	virtual void onAecpUnexpectedResponse(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const& /*entityID*/) noexcept override {}
	/** Notification for when an AECP Response is received (not an Unsolicited one) along with the time elapsed between the send and the receive. */
	virtual void onAecpResponseTime(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const& /*entityID*/, std::chrono::milliseconds const& /*responseTime*/) noexcept override {}
	/** Notification for when an AEM-AECP Unsolicited Response was received. */
	virtual void onAemAecpUnsolicitedReceived(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const& /*entityID*/, la::avdecc::protocol::AecpSequenceID const /*sequenceID*/) noexcept override {}

	// Defaulted compiler auto-generated methods
	DefaultedDelegate() noexcept = default;
	virtual ~DefaultedDelegate() noexcept = default;
	DefaultedDelegate(DefaultedDelegate&&) = default;
	DefaultedDelegate(DefaultedDelegate const&) = default;
	DefaultedDelegate& operator=(DefaultedDelegate const&) = default;
	DefaultedDelegate& operator=(DefaultedDelegate&&) = default;
};
} // namespace controller

class ControllerEntity : public LocalEntity, public controller::Interface
{
public:
	using UniquePointer = std::unique_ptr<ControllerEntity, void (*)(ControllerEntity*)>;

	/**
	* @brief Factory method to create a new ControllerEntity.
	* @details Creates a new ControllerEntity as a unique pointer.
	* @param[in] protocolInterface The protocol interface to bind the entity to.
	* @param[in] commonInformation Common information for this controller entity.
	* @param[in] interfacesInformation All interfaces information for this controller entity.
	* @param[in] entityModelTree The entity model tree to use for this controller entity, or null to not expose a model.
	* @param[in] delegate The Delegate to be called whenever a controller related notification occurs.
	* @return A new ControllerEntity as a Entity::UniquePointer.
	* @note Might throw an Exception.
	*/
	static UniquePointer create(protocol::ProtocolInterface* const protocolInterface, CommonInformation const& commonInformation, InterfacesInformation const& interfacesInformation, model::EntityTree const* const entityModelTree, controller::Delegate* const delegate)
	{
		auto deleter = [](ControllerEntity* self)
		{
			self->destroy();
		};
		return UniquePointer(createRawControllerEntity(protocolInterface, commonInformation, interfacesInformation, entityModelTree, delegate), deleter);
	}

	/* Discovery Protocol (ADP) */
	/** Enables entity advertising with available duration included between 2-62 seconds on the specified interfaceIndex if set, otherwise on all interfaces. Returns false if EntityID is already in use on the local computer, true otherwise. */
	using LocalEntity::enableEntityAdvertising;
	/** Disables entity advertising on the specified interfaceIndex if set, otherwise on all interfaces. */
	using LocalEntity::disableEntityAdvertising;
	/** Requests a remote entities discovery. */
	using LocalEntity::discoverRemoteEntities;
	/** Requests a targetted remote entity discovery. */
	using LocalEntity::discoverRemoteEntity;
	/** Sets automatic discovery delay. 0 (default) for no automatic discovery. */
	using LocalEntity::setAutomaticDiscoveryDelay;

	/* Other methods */
	virtual void setControllerDelegate(controller::Delegate* const delegate) noexcept = 0;

	// Deleted compiler auto-generated methods
	ControllerEntity(ControllerEntity&&) = delete;
	ControllerEntity(ControllerEntity const&) = delete;
	ControllerEntity& operator=(ControllerEntity const&) = delete;
	ControllerEntity& operator=(ControllerEntity&&) = delete;

protected:
	/** Constructor */
	ControllerEntity(CommonInformation const& commonInformation, InterfacesInformation const& interfacesInformation);

	/** Destructor */
	virtual ~ControllerEntity() noexcept = default;

private:
	/** Entry point */
	static LA_AVDECC_API ControllerEntity* LA_AVDECC_CALL_CONVENTION createRawControllerEntity(protocol::ProtocolInterface* const protocolInterface, CommonInformation const& commonInformation, InterfacesInformation const& interfacesInformation, model::EntityTree const* const entityModelTree, controller::Delegate* const delegate);

	/** Destroy method for COM-like interface */
	virtual void destroy() noexcept = 0;
};

} // namespace entity
} // namespace avdecc
} // namespace la
