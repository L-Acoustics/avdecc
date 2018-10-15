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
* @file controllerEntity.hpp
* @author Christophe Calmejane
* @brief Avdecc controller entity.
*/

#pragma once

#include "la/avdecc/memoryBuffer.hpp"
#include "protocolInterface.hpp"
#include "entity.hpp"
#include "entityModel.hpp"
#include "entityAddressAccessTypes.hpp"
#include "exports.hpp"
#include <thread>
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>
#include <functional>

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
	using LockEntityHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const owningEntity)>;
	using UnlockEntityHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status)>;
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
	using ClockDomainDescriptorHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::entity::model::ClockDomainDescriptor const& descriptor)>;
	using SetConfigurationHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex)>;
	using SetStreamInputFormatHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamFormat const streamFormat)>;
	using GetStreamInputFormatHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamFormat const streamFormat)>;
	using SetStreamOutputFormatHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamFormat const streamFormat)>;
	using GetStreamOutputFormatHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamFormat const streamFormat)>;
	using GetStreamPortInputAudioMapHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, la::avdecc::entity::model::AudioMappings const& mappings)>;
	using GetStreamPortOutputAudioMapHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, la::avdecc::entity::model::AudioMappings const& mappings)>;
	using AddStreamPortInputAudioMappingsHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::AudioMappings const& mappings)>;
	using AddStreamPortOutputAudioMappingsHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::AudioMappings const& mappings)>;
	using RemoveStreamPortInputAudioMappingsHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::AudioMappings const& mappings)>;
	using RemoveStreamPortOutputAudioMappingsHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::AudioMappings const& mappings)>;
	using GetStreamInputInfoHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamInfo const& info)>;
	using GetStreamOutputInfoHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamInfo const& info)>;
	using SetEntityNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status)>;
	using GetEntityNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::AvdeccFixedString const& entityName)>;
	using SetEntityGroupNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status)>;
	using GetEntityGroupNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::AvdeccFixedString const& entityGroupName)>;
	using SetConfigurationNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex)>;
	using GetConfigurationNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AvdeccFixedString const& configurationName)>;
	using SetAudioUnitNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AudioUnitIndex const audioUnitIndex)>;
	using GetAudioUnitNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AudioUnitIndex const audioUnitIndex, la::avdecc::entity::model::AvdeccFixedString const& audioUnitName)>;
	using SetStreamInputNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex)>;
	using GetStreamInputNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::AvdeccFixedString const& streamInputName)>;
	using SetStreamOutputNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex)>;
	using GetStreamOutputNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::AvdeccFixedString const& streamOutputName)>;
	using SetAvbInterfaceNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex)>;
	using GetAvbInterfaceNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::model::AvdeccFixedString const& avbInterfaceName)>;
	using SetClockSourceNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockSourceIndex const clockSourceIndex)>;
	using GetClockSourceNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockSourceIndex const clockSourceIndex, la::avdecc::entity::model::AvdeccFixedString const& clockSourceName)>;
	using SetMemoryObjectNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::MemoryObjectIndex const memoryObjectIndex)>;
	using GetMemoryObjectNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::MemoryObjectIndex const memoryObjectIndex, la::avdecc::entity::model::AvdeccFixedString const& memoryObjectName)>;
	using SetAudioClusterNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClusterIndex const audioClusterIndex)>;
	using GetAudioClusterNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClusterIndex const audioClusterIndex, la::avdecc::entity::model::AvdeccFixedString const& audioClusterName)>;
	using SetClockDomainNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex)>;
	using GetClockDomainNameHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::entity::model::AvdeccFixedString const& clockDomainName)>;
	using SetAudioUnitSamplingRateHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::AudioUnitIndex const audioUnitIndex, la::avdecc::entity::model::SamplingRate const samplingRate)>;
	using GetAudioUnitSamplingRateHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::AudioUnitIndex const audioUnitIndex, la::avdecc::entity::model::SamplingRate const samplingRate)>;
	using SetVideoClusterSamplingRateHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ClusterIndex const videoClusterIndex, la::avdecc::entity::model::SamplingRate const samplingRate)>;
	using GetVideoClusterSamplingRateHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ClusterIndex const videoClusterIndex, la::avdecc::entity::model::SamplingRate const samplingRate)>;
	using SetSensorClusterSamplingRateHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ClusterIndex const sensorClusterIndex, la::avdecc::entity::model::SamplingRate const samplingRate)>;
	using GetSensorClusterSamplingRateHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ClusterIndex const sensorClusterIndex, la::avdecc::entity::model::SamplingRate const samplingRate)>;
	using SetClockSourceHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::entity::model::ClockSourceIndex const clockSourceIndex)>;
	using GetClockSourceHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::entity::model::ClockSourceIndex const clockSourceIndex)>;
	using StartStreamInputHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex)>;
	using StartStreamOutputHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex)>;
	using StopStreamInputHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex)>;
	using StopStreamOutputHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex)>;
	using GetAvbInfoHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::model::AvbInfo const& info)>;
	using GetAvbInterfaceCountersHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::AvbInterfaceCounterValidFlags const validCounters, la::avdecc::entity::model::DescriptorCounters const& counters)>;
	using GetClockDomainCountersHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::entity::ClockDomainCounterValidFlags const validCounters, la::avdecc::entity::model::DescriptorCounters const& counters)>;
	using GetStreamInputCountersHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::StreamInputCounterValidFlags const validCounters, la::avdecc::entity::model::DescriptorCounters const& counters)>;
	using StartOperationHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex, la::avdecc::entity::model::OperationID const operationID, la::avdecc::entity::model::MemoryObjectOperationType const operationType, la::avdecc::MemoryBuffer const& memoryBuffer)>;
	using AbortOperationHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex, la::avdecc::entity::model::OperationID const operationID)>;
	using SetMemoryObjectLengthHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length)>;
	using GetMemoryObjectLengthHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length)>;
	/* Enumeration and Control Protocol (AECP) AA handlers */
	using AddressAccessHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AaCommandStatus const status, la::avdecc::entity::addressAccess::Tlvs const& tlvs)>;
	/* Enumeration and Control Protocol (AECP) MVU handlers (Milan Vendor Unique) */
	using GetMilanInfoHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::MvuCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, std::uint32_t const protocolVersion, la::avdecc::protocol::MvuFeaturesFlags const featuresFlags, std::uint32_t const certificationVersion)>;
	/* Connection Management Protocol (ACMP) handlers */
	using ConnectStreamHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status)>;
	using DisconnectStreamHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status)>;
	using DisconnectTalkerStreamHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status)>;
	using GetTalkerStreamStateHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status)>;
	using GetListenerStreamStateHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status)>;
	using GetTalkerStreamConnectionHandler = std::function<void(la::avdecc::entity::controller::Interface const* const controller, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status)>;

	/* Enumeration and Control Protocol (AECP) AEM */
	virtual void acquireEntity(UniqueIdentifier const targetEntityID, bool const isPersistent, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, AcquireEntityHandler const& handler) const noexcept = 0;
	virtual void releaseEntity(UniqueIdentifier const targetEntityID, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, ReleaseEntityHandler const& handler) const noexcept = 0;
	virtual void lockEntity(UniqueIdentifier const targetEntityID, LockEntityHandler const& handler) const noexcept = 0;
	virtual void unlockEntity(UniqueIdentifier const targetEntityID, UnlockEntityHandler const& handler) const noexcept = 0;
	virtual void queryEntityAvailable(UniqueIdentifier const targetEntityID, QueryEntityAvailableHandler const& handler) const noexcept = 0;
	virtual void queryControllerAvailable(UniqueIdentifier const targetEntityID, QueryControllerAvailableHandler const& handler) const noexcept = 0;
	virtual void registerUnsolicitedNotifications(UniqueIdentifier const targetEntityID, RegisterUnsolicitedNotificationsHandler const& handler) const noexcept = 0;
	virtual void unregisterUnsolicitedNotifications(UniqueIdentifier const targetEntityID, UnregisterUnsolicitedNotificationsHandler const& handler) const noexcept = 0;
	virtual void readEntityDescriptor(UniqueIdentifier const targetEntityID, EntityDescriptorHandler const& handler) const noexcept = 0;
	virtual void readConfigurationDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, ConfigurationDescriptorHandler const& handler) const noexcept = 0;
	virtual void readAudioUnitDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AudioUnitIndex const audioUnitIndex, AudioUnitDescriptorHandler const& handler) const noexcept = 0;
	virtual void readStreamInputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, StreamInputDescriptorHandler const& handler) const noexcept = 0;
	virtual void readStreamOutputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, StreamOutputDescriptorHandler const& handler) const noexcept = 0;
	virtual void readJackInputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::JackIndex const jackIndex, JackInputDescriptorHandler const& handler) const noexcept = 0;
	virtual void readJackOutputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::JackIndex const jackIndex, JackOutputDescriptorHandler const& handler) const noexcept = 0;
	virtual void readAvbInterfaceDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AvbInterfaceIndex const avbInterfaceIndex, AvbInterfaceDescriptorHandler const& handler) const noexcept = 0;
	virtual void readClockSourceDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockSourceIndex const clockSourceIndex, ClockSourceDescriptorHandler const& handler) const noexcept = 0;
	virtual void readMemoryObjectDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MemoryObjectIndex const memoryObjectIndex, MemoryObjectDescriptorHandler const& handler) const noexcept = 0;
	virtual void readLocaleDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::LocaleIndex const localeIndex, LocaleDescriptorHandler const& handler) const noexcept = 0;
	virtual void readStringsDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StringsIndex const stringsIndex, StringsDescriptorHandler const& handler) const noexcept = 0;
	virtual void readStreamPortInputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamPortIndex const streamPortIndex, StreamPortInputDescriptorHandler const& handler) const noexcept = 0;
	virtual void readStreamPortOutputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamPortIndex const streamPortIndex, StreamPortOutputDescriptorHandler const& handler) const noexcept = 0;
	virtual void readExternalPortInputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ExternalPortIndex const externalPortIndex, ExternalPortInputDescriptorHandler const& handler) const noexcept = 0;
	virtual void readExternalPortOutputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ExternalPortIndex const externalPortIndex, ExternalPortOutputDescriptorHandler const& handler) const noexcept = 0;
	virtual void readInternalPortInputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::InternalPortIndex const internalPortIndex, InternalPortInputDescriptorHandler const& handler) const noexcept = 0;
	virtual void readInternalPortOutputDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::InternalPortIndex const internalPortIndex, InternalPortOutputDescriptorHandler const& handler) const noexcept = 0;
	virtual void readAudioClusterDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClusterIndex const clusterIndex, AudioClusterDescriptorHandler const& handler) const noexcept = 0;
	virtual void readAudioMapDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MapIndex const mapIndex, AudioMapDescriptorHandler const& handler) const noexcept = 0;
	virtual void readClockDomainDescriptor(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockDomainIndex const clockDomainIndex, ClockDomainDescriptorHandler const& handler) const noexcept = 0;
	virtual void setConfiguration(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, SetConfigurationHandler const& handler) const noexcept = 0;
	virtual void setStreamInputFormat(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, model::StreamFormat const streamFormat, SetStreamInputFormatHandler const& handler) const noexcept = 0;
	virtual void getStreamInputFormat(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, GetStreamInputFormatHandler const& handler) const noexcept = 0;
	virtual void setStreamOutputFormat(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, model::StreamFormat const streamFormat, SetStreamOutputFormatHandler const& handler) const noexcept = 0;
	virtual void getStreamOutputFormat(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, GetStreamOutputFormatHandler const& handler) const noexcept = 0;
	virtual void getStreamPortInputAudioMap(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, entity::model::MapIndex const mapIndex, GetStreamPortInputAudioMapHandler const& handler) const noexcept = 0;
	virtual void getStreamPortOutputAudioMap(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, entity::model::MapIndex const mapIndex, GetStreamPortOutputAudioMapHandler const& handler) const noexcept = 0;
	virtual void addStreamPortInputAudioMappings(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::AudioMappings const& mappings, AddStreamPortInputAudioMappingsHandler const& handler) const noexcept = 0;
	virtual void addStreamPortOutputAudioMappings(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::AudioMappings const& mappings, AddStreamPortOutputAudioMappingsHandler const& handler) const noexcept = 0;
	virtual void removeStreamPortInputAudioMappings(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::AudioMappings const& mappings, RemoveStreamPortInputAudioMappingsHandler const& handler) const noexcept = 0;
	virtual void removeStreamPortOutputAudioMappings(UniqueIdentifier const targetEntityID, model::StreamPortIndex const streamPortIndex, model::AudioMappings const& mappings, RemoveStreamPortOutputAudioMappingsHandler const& handler) const noexcept = 0;
	virtual void getStreamInputInfo(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, GetStreamInputInfoHandler const& handler) const noexcept = 0;
	virtual void getStreamOutputInfo(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, GetStreamOutputInfoHandler const& handler) const noexcept = 0;
	virtual void setEntityName(UniqueIdentifier const targetEntityID, model::AvdeccFixedString const& entityName, SetEntityNameHandler const& handler) const noexcept = 0;
	virtual void getEntityName(UniqueIdentifier const targetEntityID, GetEntityNameHandler const& handler) const noexcept = 0;
	virtual void setEntityGroupName(UniqueIdentifier const targetEntityID, model::AvdeccFixedString const& entityGroupName, SetEntityGroupNameHandler const& handler) const noexcept = 0;
	virtual void getEntityGroupName(UniqueIdentifier const targetEntityID, GetEntityGroupNameHandler const& handler) const noexcept = 0;
	virtual void setConfigurationName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AvdeccFixedString const& entityGroupName, SetConfigurationNameHandler const& handler) const noexcept = 0;
	virtual void getConfigurationName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, GetConfigurationNameHandler const& handler) const noexcept = 0;
	virtual void setAudioUnitName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AudioUnitIndex const audioUnitIndex, model::AvdeccFixedString const& audioUnitName, SetAudioUnitNameHandler const& handler) const noexcept = 0;
	virtual void getAudioUnitName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const audioUnitIndex, GetAudioUnitNameHandler const& handler) const noexcept = 0;
	virtual void setStreamInputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, model::AvdeccFixedString const& streamInputName, SetStreamInputNameHandler const& handler) const noexcept = 0;
	virtual void getStreamInputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, GetStreamInputNameHandler const& handler) const noexcept = 0;
	virtual void setStreamOutputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, model::AvdeccFixedString const& streamOutputName, SetStreamOutputNameHandler const& handler) const noexcept = 0;
	virtual void getStreamOutputName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const streamIndex, GetStreamOutputNameHandler const& handler) const noexcept = 0;
	virtual void setAvbInterfaceName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::AvbInterfaceIndex const avbInterfaceIndex, model::AvdeccFixedString const& avbInterfaceName, SetAvbInterfaceNameHandler const& handler) const noexcept = 0;
	virtual void getAvbInterfaceName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const avbInterfaceIndex, GetAvbInterfaceNameHandler const& handler) const noexcept = 0;
	virtual void setClockSourceName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockSourceIndex const clockSourceIndex, model::AvdeccFixedString const& clockSourceName, SetClockSourceNameHandler const& handler) const noexcept = 0;
	virtual void getClockSourceName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const clockSourceIndex, GetClockSourceNameHandler const& handler) const noexcept = 0;
	virtual void setMemoryObjectName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MemoryObjectIndex const memoryObjectIndex, model::AvdeccFixedString const& memoryObjectName, SetMemoryObjectNameHandler const& handler) const noexcept = 0;
	virtual void getMemoryObjectName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const memoryObjectIndex, GetMemoryObjectNameHandler const& handler) const noexcept = 0;
	virtual void setAudioClusterName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClusterIndex const audioClusterIndex, model::AvdeccFixedString const& audioClusterName, SetAudioClusterNameHandler const& handler) const noexcept = 0;
	virtual void getAudioClusterName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const audioClusterIndex, GetAudioClusterNameHandler const& handler) const noexcept = 0;
	virtual void setClockDomainName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::ClockDomainIndex const clockDomainIndex, model::AvdeccFixedString const& clockDomainName, SetClockDomainNameHandler const& handler) const noexcept = 0;
	virtual void getClockDomainName(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::StreamIndex const clockDomainIndex, GetClockDomainNameHandler const& handler) const noexcept = 0;
	virtual void setAudioUnitSamplingRate(UniqueIdentifier const targetEntityID, model::AudioUnitIndex const audioUnitIndex, model::SamplingRate const samplingRate, SetAudioUnitSamplingRateHandler const& handler) const noexcept = 0;
	virtual void getAudioUnitSamplingRate(UniqueIdentifier const targetEntityID, model::AudioUnitIndex const audioUnitIndex, GetAudioUnitSamplingRateHandler const& handler) const noexcept = 0;
	virtual void setVideoClusterSamplingRate(UniqueIdentifier const targetEntityID, model::ClusterIndex const videoClusterIndex, model::SamplingRate const samplingRate, SetVideoClusterSamplingRateHandler const& handler) const noexcept = 0;
	virtual void getVideoClusterSamplingRate(UniqueIdentifier const targetEntityID, model::ClusterIndex const videoClusterIndex, GetVideoClusterSamplingRateHandler const& handler) const noexcept = 0;
	virtual void setSensorClusterSamplingRate(UniqueIdentifier const targetEntityID, model::ClusterIndex const sensorClusterIndex, model::SamplingRate const samplingRate, SetSensorClusterSamplingRateHandler const& handler) const noexcept = 0;
	virtual void getSensorClusterSamplingRate(UniqueIdentifier const targetEntityID, model::ClusterIndex const sensorClusterIndex, GetSensorClusterSamplingRateHandler const& handler) const noexcept = 0;
	virtual void setClockSource(UniqueIdentifier const targetEntityID, model::ClockDomainIndex const clockDomainIndex, model::ClockSourceIndex const clockSourceIndex, SetClockSourceHandler const& handler) const noexcept = 0;
	virtual void getClockSource(UniqueIdentifier const targetEntityID, model::ClockDomainIndex const clockDomainIndex, GetClockSourceHandler const& handler) const noexcept = 0;
	virtual void startStreamInput(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, StartStreamInputHandler const& handler) const noexcept = 0;
	virtual void startStreamOutput(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, StartStreamOutputHandler const& handler) const noexcept = 0;
	virtual void stopStreamInput(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, StopStreamInputHandler const& handler) const noexcept = 0;
	virtual void stopStreamOutput(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, StopStreamOutputHandler const& handler) const noexcept = 0;
	virtual void getAvbInfo(UniqueIdentifier const targetEntityID, model::AvbInterfaceIndex const avbInterfaceIndex, GetAvbInfoHandler const& handler) const noexcept = 0;
	virtual void getAvbInterfaceCounters(UniqueIdentifier const targetEntityID, model::AvbInterfaceIndex const avbInterfaceIndex, GetAvbInterfaceCountersHandler const& handler) const noexcept = 0;
	virtual void getClockDomainCounters(UniqueIdentifier const targetEntityID, model::ClockDomainIndex const clockDomainIndex, GetClockDomainCountersHandler const& handler) const noexcept = 0;
	virtual void getStreamInputCounters(UniqueIdentifier const targetEntityID, model::StreamIndex const streamIndex, GetStreamInputCountersHandler const& handler) const noexcept = 0;
	virtual void startOperation(UniqueIdentifier const targetEntityID, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, model::MemoryObjectOperationType const operationType, MemoryBuffer const& memoryBuffer, StartOperationHandler const& handler) const noexcept = 0;
	virtual void abortOperation(UniqueIdentifier const targetEntityID, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, model::OperationID const operationID, AbortOperationHandler const& handler) const noexcept = 0;
	virtual void setMemoryObjectLength(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length, SetMemoryObjectLengthHandler const& handler) const noexcept = 0;
	virtual void getMemoryObjectLength(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, model::MemoryObjectIndex const memoryObjectIndex, GetMemoryObjectLengthHandler const& handler) const noexcept = 0;

	/* Enumeration and Control Protocol (AECP) AA */
	virtual void addressAccess(UniqueIdentifier const targetEntityID, addressAccess::Tlvs const& tlvs, AddressAccessHandler const& handler) const noexcept = 0;

	/* Enumeration and Control Protocol (AECP) MVU (Milan Vendor Unique) */
	virtual void getMilanInfo(UniqueIdentifier const targetEntityID, model::ConfigurationIndex const configurationIndex, GetMilanInfoHandler const& handler) const noexcept = 0;

	/* Connection Management Protocol (ACMP) */
	virtual void connectStream(model::StreamIdentification const& talkerStream, model::StreamIdentification const& listenerStream, ConnectStreamHandler const& handler) const noexcept = 0;
	virtual void disconnectStream(model::StreamIdentification const& talkerStream, model::StreamIdentification const& listenerStream, DisconnectStreamHandler const& handler) const noexcept = 0;
	virtual void disconnectTalkerStream(model::StreamIdentification const& talkerStream, model::StreamIdentification const& listenerStream, DisconnectTalkerStreamHandler const& handler) const noexcept = 0;
	virtual void getTalkerStreamState(model::StreamIdentification const& talkerStream, GetTalkerStreamStateHandler const& handler) const noexcept = 0;
	virtual void getListenerStreamState(model::StreamIdentification const& listenerStream, GetListenerStreamStateHandler const& handler) const noexcept = 0;
	virtual void getTalkerStreamConnection(model::StreamIdentification const& talkerStream, uint16_t const connectionIndex, GetTalkerStreamConnectionHandler const& handler) const noexcept = 0;

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
	virtual void onTransportError(la::avdecc::entity::controller::Interface const* const /*controller*/) noexcept {}

	/* Discovery Protocol (ADP) */
	/** Called when a new entity was discovered on the network (either local or remote). */
	virtual void onEntityOnline(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::Entity const& /*entity*/) noexcept {}
	/** Called when an already discovered entity updated its discovery (ADP) information. */
	virtual void onEntityUpdate(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::Entity const& /*entity*/) noexcept {} // When GpgpGrandMasterID, GpgpDomainNumber or EntityCapabilities changed
	/** Called when an already discovered entity went offline or timed out (either local or remote). */
	virtual void onEntityOffline(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/) noexcept {}

	/* Connection Management Protocol sniffed messages (ACMP) (not triggered for our own commands even though ACMP messages are broadcasted, the command's 'result' method will be called in that case) */
	/** Called when a controller connect request has been sniffed on the network. */
	virtual void onControllerConnectResponseSniffed(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::entity::model::StreamIdentification const& /*talkerStream*/, la::avdecc::entity::model::StreamIdentification const& /*listenerStream*/, uint16_t const /*connectionCount*/, la::avdecc::entity::ConnectionFlags const /*flags*/, la::avdecc::entity::LocalEntity::ControlStatus const /*status*/) noexcept {}
	/** Called when a controller disconnect request has been sniffed on the network. */
	virtual void onControllerDisconnectResponseSniffed(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::entity::model::StreamIdentification const& /*talkerStream*/, la::avdecc::entity::model::StreamIdentification const& /*listenerStream*/, uint16_t const /*connectionCount*/, la::avdecc::entity::ConnectionFlags const /*flags*/, la::avdecc::entity::LocalEntity::ControlStatus const /*status*/) noexcept {}
	/** Called when a listener connect request has been sniffed on the network (either due to a another controller connect, or a fast connect). */
	virtual void onListenerConnectResponseSniffed(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::entity::model::StreamIdentification const& /*talkerStream*/, la::avdecc::entity::model::StreamIdentification const& /*listenerStream*/, uint16_t const /*connectionCount*/, la::avdecc::entity::ConnectionFlags const /*flags*/, la::avdecc::entity::LocalEntity::ControlStatus const /*status*/) noexcept {}
	/** Called when a listener disconnect request has been sniffed on the network (either due to a another controller disconnect, or a fast disconnect). */
	virtual void onListenerDisconnectResponseSniffed(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::entity::model::StreamIdentification const& /*talkerStream*/, la::avdecc::entity::model::StreamIdentification const& /*listenerStream*/, uint16_t const /*connectionCount*/, la::avdecc::entity::ConnectionFlags const /*flags*/, la::avdecc::entity::LocalEntity::ControlStatus const /*status*/) noexcept {}
	/** Called when a stream state query has been sniffed on the network. */
	virtual void onGetTalkerStreamStateResponseSniffed(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::entity::model::StreamIdentification const& /*talkerStream*/, la::avdecc::entity::model::StreamIdentification const& /*listenerStream*/, uint16_t const /*connectionCount*/, la::avdecc::entity::ConnectionFlags const /*flags*/, la::avdecc::entity::LocalEntity::ControlStatus const /*status*/) noexcept {}
	/** Called when a stream state query has been sniffed on the network */
	virtual void onGetListenerStreamStateResponseSniffed(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::entity::model::StreamIdentification const& /*talkerStream*/, la::avdecc::entity::model::StreamIdentification const& /*listenerStream*/, uint16_t const /*connectionCount*/, la::avdecc::entity::ConnectionFlags const /*flags*/, la::avdecc::entity::LocalEntity::ControlStatus const /*status*/) noexcept {}

	/* Unsolicited notifications (not triggered for our own commands, the command's 'result' method will be called in that case). Only successfull commands can cause an unsolicited notification. */
	/** Called when an entity has been acquired by another controller. */
	virtual void onEntityAcquired(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*acquiredEntity*/, la::avdecc::UniqueIdentifier const /*owningEntity*/, la::avdecc::entity::model::DescriptorType const /*descriptorType*/, la::avdecc::entity::model::DescriptorIndex const /*descriptorIndex*/) noexcept {}
	/** Called when an entity has been released by another controller. */
	virtual void onEntityReleased(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*releasedEntity*/, la::avdecc::UniqueIdentifier const /*owningEntity*/, la::avdecc::entity::model::DescriptorType const /*descriptorType*/, la::avdecc::entity::model::DescriptorIndex const /*descriptorIndex*/) noexcept {}
	/** Called when the current configuration was changed by another controller. */
	virtual void onConfigurationChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/) noexcept {}
	/** Called when the format of an input stream was changed by another controller. */
	virtual void onStreamInputFormatChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::StreamFormat const /*streamFormat*/) noexcept {}
	/** Called when the format of an output stream was changed by another controller. */
	virtual void onStreamOutputFormatChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::StreamFormat const /*streamFormat*/) noexcept {}
	/** Called when the audio mappings of a stream port input was changed by another controller. */
	virtual void onStreamPortInputAudioMappingsChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamPortIndex const /*streamPortIndex*/, la::avdecc::entity::model::MapIndex const /*numberOfMaps*/, la::avdecc::entity::model::MapIndex const /*mapIndex*/, la::avdecc::entity::model::AudioMappings const& /*mappings*/) noexcept {}
	/** Called when the audio mappings of a stream port output was changed by another controller. */
	virtual void onStreamPortOutputAudioMappingsChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamPortIndex const /*streamPortIndex*/, la::avdecc::entity::model::MapIndex const /*numberOfMaps*/, la::avdecc::entity::model::MapIndex const /*mapIndex*/, la::avdecc::entity::model::AudioMappings const& /*mappings*/) noexcept {}
	/** Called when the information of an input stream was changed by another controller. */
	virtual void onStreamInputInfoChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::StreamInfo const& /*info*/) noexcept {}
	/** Called when the information of an output stream was changed by another controller. */
	virtual void onStreamOutputInfoChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::StreamInfo const& /*info*/) noexcept {}
	/** Called when the entity's name was changed by another controller. */
	virtual void onEntityNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::AvdeccFixedString const& /*entityName*/) noexcept {}
	/** Called when the entity's group name was changed by another controller. */
	virtual void onEntityGroupNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::AvdeccFixedString const& /*entityGroupName*/) noexcept {}
	/** Called when a configuration name was changed by another controller. */
	virtual void onConfigurationNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*configurationName*/) noexcept {}
	/** Called when an audio unit name was changed by another controller. */
	virtual void onAudioUnitNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::AudioUnitIndex const /*audioUnitIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*audioUnitName*/) noexcept {}
	/** Called when an input stream name was changed by another controller. */
	virtual void onStreamInputNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*streamName*/) noexcept {}
	/** Called when an output stream name was changed by another controller. */
	virtual void onStreamOutputNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*streamName*/) noexcept {}
	/** Called when an audio unit name was changed by another controller. */
	virtual void onAvbInterfaceNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::AvbInterfaceIndex const /*avbInterfaceIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*avbInterfaceName*/) noexcept {}
	/** Called when an audio unit name was changed by another controller. */
	virtual void onClockSourceNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::ClockSourceIndex const /*clockSourceIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*clockSourceName*/) noexcept {}
	/** Called when an audio unit name was changed by another controller. */
	virtual void onMemoryObjectNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::MemoryObjectIndex const /*memoryObjectIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*memoryObjectName*/) noexcept {}
	/** Called when an audio unit name was changed by another controller. */
	virtual void onAudioClusterNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::ClusterIndex const /*audioClusterIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*audioClusterName*/) noexcept {}
	/** Called when an audio unit name was changed by another controller. */
	virtual void onClockDomainNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::ClockDomainIndex const /*clockDomainIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*clockDomainName*/) noexcept {}
	/** Called when an AudioUnit sampling rate was changed by another controller. */
	virtual void onAudioUnitSamplingRateChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::AudioUnitIndex const /*audioUnitIndex*/, la::avdecc::entity::model::SamplingRate const /*samplingRate*/) noexcept {}
	/** Called when a VideoCluster sampling rate was changed by another controller. */
	virtual void onVideoClusterSamplingRateChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ClusterIndex const /*videoClusterIndex*/, la::avdecc::entity::model::SamplingRate const /*samplingRate*/) noexcept {}
	/** Called when a SensorCluster sampling rate was changed by another controller. */
	virtual void onSensorClusterSamplingRateChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ClusterIndex const /*sensorClusterIndex*/, la::avdecc::entity::model::SamplingRate const /*samplingRate*/) noexcept {}
	/** Called when a clock source was changed by another controller. */
	virtual void onClockSourceChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ClockDomainIndex const /*clockDomainIndex*/, la::avdecc::entity::model::ClockSourceIndex const /*clockSourceIndex*/) noexcept {}
	/** Called when an input stream was started by another controller. */
	virtual void onStreamInputStarted(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/) noexcept {}
	/** Called when an output stream was started by another controller. */
	virtual void onStreamOutputStarted(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/) noexcept {}
	/** Called when an input stream was stopped by another controller. */
	virtual void onStreamInputStopped(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/) noexcept {}
	/** Called when an output stream was stopped by another controller. */
	virtual void onStreamOutputStopped(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/) noexcept {}
	/** Called when the Avb Info of an Avb Interface changed. */
	virtual void onAvbInfoChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::AvbInterfaceIndex const /*avbInterfaceIndex*/, la::avdecc::entity::model::AvbInfo const& /*info*/) noexcept {}
	virtual void onAvbInterfaceCountersChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::AvbInterfaceIndex const /*avbInterfaceIndex*/, la::avdecc::entity::AvbInterfaceCounterValidFlags const /*validCounters*/, la::avdecc::entity::model::DescriptorCounters const& /*counters*/) noexcept {}
	virtual void onClockDomainCountersChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ClockDomainIndex const /*clockDomainIndex*/, la::avdecc::entity::ClockDomainCounterValidFlags const /*validCounters*/, la::avdecc::entity::model::DescriptorCounters const& /*counters*/) noexcept {}
	virtual void onStreamInputCountersChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::StreamInputCounterValidFlags const /*validCounters*/, la::avdecc::entity::model::DescriptorCounters const& /*counters*/) noexcept {}
	// TODO: AddAudioMappings
	// TODO: RemoveAudioMappings
	/** Called when the length of a MemoryObject changed. */
	virtual void onMemoryObjectLengthChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::MemoryObjectIndex const /*memoryObjectIndex*/, std::uint64_t const /*length*/) noexcept {}
	/** Called when there is a status update on an ongoing Operation */
	virtual void onOperationStatus(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::DescriptorType const /*descriptorType*/, la::avdecc::entity::model::DescriptorIndex const /*descriptorIndex*/, la::avdecc::entity::model::OperationID const /*operationID*/, std::uint16_t const /*percentComplete*/) noexcept {};

	// Defaulted compiler auto-generated methods
	Delegate() noexcept = default;
	virtual ~Delegate() noexcept = default;
	Delegate(Delegate&&) = default;
	Delegate(Delegate const&) = default;
	Delegate& operator=(Delegate const&) = default;
	Delegate& operator=(Delegate&&) = default;
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
	* @param[in] progID ID that will be used to generate the #UniqueIdentifier for the controller.
	* @param[in] entityModelID The EntityModelID value for the controller. You can use entity::model::makeEntityModelID to create this value.
	* @param[in] delegate The Delegate to be called whenever a controller related notification occurs.
	* @return A new ControllerEntity as a Entity::UniquePointer.
	* @note Might throw an Exception.
	*/
	static UniquePointer create(protocol::ProtocolInterface* const protocolInterface, std::uint16_t const progID, UniqueIdentifier const entityModelID, entity::controller::Delegate* const delegate)
	{
		auto deleter = [](ControllerEntity* self)
		{
			self->destroy();
		};
		return UniquePointer(createRawControllerEntity(protocolInterface, progID, entityModelID, delegate), deleter);
	}

	/* Discovery Protocol (ADP) */
	/** Enables entity advertising with available duration included between 2-62 seconds, defaulting to 62. */
	using LocalEntity::enableEntityAdvertising;
	/** Disables entity advertising. */
	using LocalEntity::disableEntityAdvertising;

	/* Other methods */
	virtual void setControllerDelegate(controller::Delegate* const delegate) noexcept = 0;

	// Deleted compiler auto-generated methods
	ControllerEntity(ControllerEntity&&) = delete;
	ControllerEntity(ControllerEntity const&) = delete;
	ControllerEntity& operator=(ControllerEntity const&) = delete;
	ControllerEntity& operator=(ControllerEntity&&) = delete;

protected:
	/** Constructor */
	ControllerEntity(UniqueIdentifier const entityID, networkInterface::MacAddress const& macAddress, UniqueIdentifier const entityModelID, EntityCapabilities const entityCapabilities, std::uint16_t const talkerStreamSources, TalkerCapabilities const talkerCapabilities, std::uint16_t const listenerStreamSinks, ListenerCapabilities const listenerCapabilities, ControllerCapabilities const controllerCapabilities, std::uint16_t const identifyControlIndex, std::uint16_t const interfaceIndex, UniqueIdentifier const associationID) noexcept;

	/** Destructor */
	virtual ~ControllerEntity() noexcept = default;

private:
	/** Entry point */
	static LA_AVDECC_API ControllerEntity* LA_AVDECC_CALL_CONVENTION createRawControllerEntity(protocol::ProtocolInterface* const protocolInterface, std::uint16_t const progID, UniqueIdentifier const entityModelID, entity::controller::Delegate* const delegate);

	/** Destroy method for COM-like interface */
	virtual void destroy() noexcept = 0;
};

} // namespace entity
} // namespace avdecc
} // namespace la
