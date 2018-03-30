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

#include <thread>
#include <unordered_map>
#include <string>
#include <vector>
#include <functional>
#include "entity.hpp"
#include "exports.hpp"

namespace la
{
namespace avdecc
{
namespace entity
{

class ControllerEntity : public LocalEntity
{
public:
	/** Status code returned by all AEM (AECP) command methods. */
	enum class AemCommandStatus : std::uint16_t
	{
		// AVDECC Protocol Error Codes
		Success = 0,
		NotImplemented = 1,
		NoSuchDescriptor = 2,
		LockedByOther = 3,
		AcquiredByOther = 4,
		NotAuthenticated = 5,
		AuthenticationDisabled = 6,
		BadArguments = 7,
		NoResources = 8,
		InProgress = 9,
		EntityMisbehaving = 10,
		NotSupported = 11,
		StreamIsRunning = 12,
		// Library Error Codes
		NetworkError = 995,
		ProtocolError = 996,
		TimedOut = 997,
		UnknownEntity = 998,
		InternalError = 999,
	};

	/** Status code returned by all ACMP control methods. */
	enum class ControlStatus : std::uint16_t
	{
		// AVDECC Protocol Error Codes
		Success = 0,
		ListenerUnknownID = 1,
		TalkerUnknownID = 2,
		TalkerDestMacFail = 3,
		TalkerNoStreamIndex = 4,
		TalkerNoBandwidth = 5,
		TalkerExclusive = 6,
		ListenerTalkerTimeout = 7,
		ListenerExclusive = 8,
		StateUnavailable = 9,
		NotConnected = 10,
		NoSuchConnection = 11,
		CouldNotSendMessage = 12,
		TalkerMisbehaving = 13,
		ListenerMisbehaving = 14,
		// Reserved
		ControllerNotAuthorized = 16,
		IncompatibleRequest = 17,
		// Reserved
		NotSupported = 31,
		// Library Error Codes
		NetworkError = 995,
		ProtocolError = 996,
		TimedOut = 997,
		UnknownEntity = 998,
		InternalError = 999,
	};

	/** Delegate for all controller related notifications. */
	class Delegate
	{
	public:
		Delegate() = default;
		virtual ~Delegate() = default;

		/* Global notifications */
		/** Called when a fatal error on the transport layer occured. */
		virtual void onTransportError(la::avdecc::entity::ControllerEntity const* const /*controller*/) noexcept {}

		/* Discovery Protocol (ADP) */
		/** Called when a new entity was discovered on the network (either local or remote). */
		virtual void onEntityOnline(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::Entity const& /*entity*/) noexcept {}
		/** Called when an already discovered entity updated its discovery (ADP) information. */
		virtual void onEntityUpdate(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::Entity const& /*entity*/) noexcept {} // When GpgpGrandMasterID, GpgpDomainNumber or EntityCapabilities changed
		/** Called when an already discovered entity went offline or timed out (either local or remote). */
		virtual void onEntityOffline(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/) noexcept {}

		/* Connection Management Protocol sniffed messages (ACMP) (not triggered for our own commands even though ACMP messages are broadcasted, the command's 'result' method will be called in that case) */
		/** Called when a controller connect request has been sniffed on the network. */
		virtual void onControllerConnectResponseSniffed(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::entity::model::StreamIdentification const& /*talkerStream*/, la::avdecc::entity::model::StreamIdentification const& /*listenerStream*/, uint16_t const /*connectionCount*/, la::avdecc::entity::ConnectionFlags const /*flags*/, la::avdecc::entity::ControllerEntity::ControlStatus const /*status*/) noexcept {}
		/** Called when a controller disconnect request has been sniffed on the network. */
		virtual void onControllerDisconnectResponseSniffed(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::entity::model::StreamIdentification const& /*talkerStream*/, la::avdecc::entity::model::StreamIdentification const& /*listenerStream*/, uint16_t const /*connectionCount*/, la::avdecc::entity::ConnectionFlags const /*flags*/, la::avdecc::entity::ControllerEntity::ControlStatus const /*status*/) noexcept {}
		/** Called when a listener connect request has been sniffed on the network (either due to a another controller connect, or a fast connect). */
		virtual void onListenerConnectResponseSniffed(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::entity::model::StreamIdentification const& /*talkerStream*/, la::avdecc::entity::model::StreamIdentification const& /*listenerStream*/, uint16_t const /*connectionCount*/, la::avdecc::entity::ConnectionFlags const /*flags*/, la::avdecc::entity::ControllerEntity::ControlStatus const /*status*/) noexcept {}
		/** Called when a listener disconnect request has been sniffed on the network (either due to a another controller disconnect, or a fast disconnect). */
		virtual void onListenerDisconnectResponseSniffed(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::entity::model::StreamIdentification const& /*talkerStream*/, la::avdecc::entity::model::StreamIdentification const& /*listenerStream*/, uint16_t const /*connectionCount*/, la::avdecc::entity::ConnectionFlags const /*flags*/, la::avdecc::entity::ControllerEntity::ControlStatus const /*status*/) noexcept {}
		/** Called when a stream state query has been sniffed on the network. */
		virtual void onGetTalkerStreamStateResponseSniffed(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::entity::model::StreamIdentification const& /*talkerStream*/, la::avdecc::entity::model::StreamIdentification const& /*listenerStream*/, uint16_t const /*connectionCount*/, la::avdecc::entity::ConnectionFlags const /*flags*/, la::avdecc::entity::ControllerEntity::ControlStatus const /*status*/) noexcept {}
		/** Called when a stream state query has been sniffed on the network */
		virtual void onGetListenerStreamStateResponseSniffed(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::entity::model::StreamIdentification const& /*talkerStream*/, la::avdecc::entity::model::StreamIdentification const& /*listenerStream*/, uint16_t const /*connectionCount*/, la::avdecc::entity::ConnectionFlags const /*flags*/, la::avdecc::entity::ControllerEntity::ControlStatus const /*status*/) noexcept {}

		/* Unsolicited notifications (not triggered for our own commands, the command's 'result' method will be called in that case). Only successfull commands can cause an unsolicited notification. */
		/** Called when an entity has been acquired by another controller. */
		virtual void onEntityAcquired(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const /*acquiredEntity*/, la::avdecc::UniqueIdentifier const /*owningEntity*/, la::avdecc::entity::model::DescriptorType const /*descriptorType*/, la::avdecc::entity::model::DescriptorIndex const /*descriptorIndex*/) noexcept {}
		/** Called when an entity has been released by another controller. */
		virtual void onEntityReleased(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const /*releasedEntity*/, la::avdecc::UniqueIdentifier const /*owningEntity*/, la::avdecc::entity::model::DescriptorType const /*descriptorType*/, la::avdecc::entity::model::DescriptorIndex const /*descriptorIndex*/) noexcept {}
		/** Called when the current configuration was changed by another controller. */
		virtual void onConfigurationChanged(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/) noexcept {}
		/** Called when the format of an input stream was changed by another controller. */
		virtual void onStreamInputFormatChanged(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::StreamFormat const /*streamFormat*/) noexcept {}
		/** Called when the format of an output stream was changed by another controller. */
		virtual void onStreamOutputFormatChanged(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::StreamFormat const /*streamFormat*/) noexcept {}
		/** Called when the audio mappings of a stream port input was changed by another controller. */
		virtual void onStreamPortInputAudioMappingsChanged(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamPortIndex const /*streamPortIndex*/, entity::model::MapIndex const /*numberOfMaps*/, entity::model::MapIndex const /*mapIndex*/, la::avdecc::entity::model::AudioMappings const& /*mappings*/) noexcept {}
		/** Called when the audio mappings of a stream port output was changed by another controller. */
		virtual void onStreamPortOutputAudioMappingsChanged(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamPortIndex const /*streamPortIndex*/, entity::model::MapIndex const /*numberOfMaps*/, entity::model::MapIndex const /*mapIndex*/, la::avdecc::entity::model::AudioMappings const& /*mappings*/) noexcept {}
		/** Called when the information of an input stream was changed by another controller. */
		virtual void onStreamInputInfoChanged(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::StreamInfo const& /*info*/) noexcept {}
		/** Called when the information of an output stream was changed by another controller. */
		virtual void onStreamOutputInfoChanged(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::StreamInfo const& /*info*/) noexcept {}
		/** Called when the entity's name was changed by another controller. */
		virtual void onEntityNameChanged(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::AvdeccFixedString const& /*entityName*/) noexcept {}
		/** Called when the entity's group name was changed by another controller. */
		virtual void onEntityGroupNameChanged(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::AvdeccFixedString const& /*entityGroupName*/) noexcept {}
		/** Called when a configuration name was changed by another controller. */
		virtual void onConfigurationNameChanged(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*configurationName*/) noexcept {}
		/** Called when an input stream name was changed by another controller. */
		virtual void onStreamInputNameChanged(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*streamName*/) noexcept {}
		/** Called when an output stream name was changed by another controller. */
		virtual void onStreamOutputNameChanged(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*streamName*/) noexcept {}
		/** Called when an AudioUnit sampling rate was changed by another controller. */
		virtual void onAudioUnitSamplingRateChanged(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::AudioUnitIndex const /*audioUnitIndex*/, la::avdecc::entity::model::SamplingRate const /*samplingRate*/) noexcept {}
		/** Called when a VideoCluster sampling rate was changed by another controller. */
		virtual void onVideoClusterSamplingRateChanged(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ClusterIndex const /*videoClusterIndex*/, la::avdecc::entity::model::SamplingRate const /*samplingRate*/) noexcept {}
		/** Called when a SensorCluster sampling rate was changed by another controller. */
		virtual void onSensorClusterSamplingRateChanged(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ClusterIndex const /*sensorClusterIndex*/, la::avdecc::entity::model::SamplingRate const /*samplingRate*/) noexcept {}
		/** Called when a clock source was changed by another controller. */
		virtual void onClockSourceChanged(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ClockDomainIndex const /*clockDomainIndex*/, la::avdecc::entity::model::ClockSourceIndex const /*clockSourceIndex*/) noexcept {}
		/** Called when an input stream was started by another controller. */
		virtual void onStreamInputStarted(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/) noexcept {}
		/** Called when an output stream was started by another controller. */
		virtual void onStreamOutputStarted(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/) noexcept {}
		/** Called when an input stream was stopped by another controller. */
		virtual void onStreamInputStopped(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/) noexcept {}
		/** Called when an output stream was stopped by another controller. */
		virtual void onStreamOutputStopped(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/) noexcept {}
		/** Called when the Avb Info of an Avb Interface changed. */
		virtual void onAvbInfoChanged(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::AvbInterfaceIndex const /*avbInterfaceIndex*/, la::avdecc::entity::model::AvbInfo const& /*info*/) noexcept {}
		// TODO: AddAudioMappings
		// TODO: RemoveAudioMappings
	};

	/* Enumeration and Control Protocol (AECP) handlers */
	using AcquireEntityHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const owningEntity, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex) noexcept>;
	using ReleaseEntityHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const owningEntity, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex) noexcept>;
	using LockEntityHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const owningEntity) noexcept>;
	using UnlockEntityHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status) noexcept>;
	using QueryEntityAvailableHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status) noexcept>;
	using QueryControllerAvailableHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status) noexcept>;
	using RegisterUnsolicitedNotificationsHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status) noexcept>;
	using UnregisterUnsolicitedNotificationsHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status) noexcept>;
	using EntityDescriptorHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::EntityDescriptor const& descriptor) noexcept>;
	using ConfigurationDescriptorHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ConfigurationDescriptor const& descriptor) noexcept>;
	using AudioUnitDescriptorHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AudioUnitIndex const audioUnitIndex, la::avdecc::entity::model::AudioUnitDescriptor const& descriptor) noexcept>;
	using StreamInputDescriptorHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamDescriptor const& descriptor) noexcept>;
	using StreamOutputDescriptorHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamDescriptor const& descriptor) noexcept>;
	using JackInputDescriptorHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::JackIndex const jackIndex, la::avdecc::entity::model::JackDescriptor const& descriptor) noexcept>;
	using JackOutputDescriptorHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::JackIndex const jackIndex, la::avdecc::entity::model::JackDescriptor const& descriptor) noexcept>;
	using AvbInterfaceDescriptorHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::model::AvbInterfaceDescriptor const& descriptor) noexcept>;
	using ClockSourceDescriptorHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockSourceIndex const clockSourceIndex, la::avdecc::entity::model::ClockSourceDescriptor const& descriptor) noexcept>;
	using MemoryObjectDescriptorHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::MemoryObjectIndex const memoryObjectIndex, la::avdecc::entity::model::MemoryObjectDescriptor const& descriptor) noexcept>;
	using LocaleDescriptorHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::LocaleIndex const localeIndex, la::avdecc::entity::model::LocaleDescriptor const& descriptor) noexcept>;
	using StringsDescriptorHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StringsIndex const stringsIndex, la::avdecc::entity::model::StringsDescriptor const& descriptor) noexcept>;
	using StreamPortInputDescriptorHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::StreamPortDescriptor const& descriptor) noexcept>;
	using StreamPortOutputDescriptorHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::StreamPortDescriptor const& descriptor) noexcept>;
	using ExternalPortInputDescriptorHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ExternalPortIndex const externalPortIndex, la::avdecc::entity::model::ExternalPortDescriptor const& descriptor) noexcept>;
	using ExternalPortOutputDescriptorHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ExternalPortIndex const externalPortIndex, la::avdecc::entity::model::ExternalPortDescriptor const& descriptor) noexcept>;
	using InternalPortInputDescriptorHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::InternalPortIndex const internalPortIndex, la::avdecc::entity::model::InternalPortDescriptor const& descriptor) noexcept>;
	using InternalPortOutputDescriptorHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::InternalPortIndex const internalPortIndex, la::avdecc::entity::model::InternalPortDescriptor const& descriptor) noexcept>;
	using AudioClusterDescriptorHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClusterIndex const clusterIndex, la::avdecc::entity::model::AudioClusterDescriptor const& descriptor) noexcept>;
	using AudioMapDescriptorHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::MapIndex const mapIndex, la::avdecc::entity::model::AudioMapDescriptor const& descriptor) noexcept>;
	using ClockDomainDescriptorHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::entity::model::ClockDomainDescriptor const& descriptor) noexcept>;
	using SetConfigurationHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex) noexcept>;
	using SetStreamInputFormatHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamFormat const streamFormat) noexcept>;
	using SetStreamOutputFormatHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamFormat const streamFormat) noexcept>;
	using GetStreamPortInputAudioMapHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, la::avdecc::entity::model::AudioMappings const& mappings) noexcept>;
	using GetStreamPortOutputAudioMapHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, la::avdecc::entity::model::AudioMappings const& mappings) noexcept>;
	using AddStreamPortInputAudioMappingsHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::AudioMappings const& mappings) noexcept>;
	using AddStreamPortOutputAudioMappingsHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::AudioMappings const& mappings) noexcept>;
	using RemoveStreamPortInputAudioMappingsHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::AudioMappings const& mappings) noexcept>;
	using RemoveStreamPortOutputAudioMappingsHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::AudioMappings const& mappings) noexcept>;
	using GetStreamInputInfoHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamInfo const& info) noexcept>;
	using GetStreamOutputInfoHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamInfo const& info) noexcept>;
	using SetEntityNameHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status) noexcept>;
	using GetEntityNameHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::AvdeccFixedString const& entityName) noexcept>;
	using SetEntityGroupNameHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status) noexcept>;
	using GetEntityGroupNameHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::AvdeccFixedString const& entityGroupName) noexcept>;
	using SetConfigurationNameHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex) noexcept>;
	using GetConfigurationNameHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AvdeccFixedString const& entityGroupName) noexcept>;
	using SetStreamInputNameHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex) noexcept>;
	using GetStreamInputNameHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::AvdeccFixedString const& streamInputName) noexcept>;
	using SetStreamOutputNameHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex) noexcept>;
	using GetStreamOutputNameHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::AvdeccFixedString const& streamOutputName) noexcept>;
	using SetAudioUnitSamplingRateHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::AudioUnitIndex const audioUnitIndex, la::avdecc::entity::model::SamplingRate const samplingRate) noexcept>;
	using SetVideoClusterSamplingRateHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ClusterIndex const videoClusterIndex, la::avdecc::entity::model::SamplingRate const samplingRate) noexcept>;
	using SetSensorClusterSamplingRateHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ClusterIndex const sensorClusterIndex, la::avdecc::entity::model::SamplingRate const samplingRate) noexcept>;
	using SetClockSourceHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::entity::model::ClockSourceIndex const clockSourceIndex) noexcept>;
	using StartStreamInputHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex) noexcept>;
	using StartStreamOutputHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex) noexcept>;
	using StopStreamInputHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex) noexcept>;
	using StopStreamOutputHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex) noexcept>;
	using GetAvbInfoHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::model::AvbInfo const& info) noexcept>;
	/* Connection Management Protocol (ACMP) handlers */
	using ConnectStreamHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::ControllerEntity::ControlStatus const status) noexcept>;
	using DisconnectStreamHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::ControllerEntity::ControlStatus const status) noexcept>;
	using DisconnectTalkerStreamHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::ControllerEntity::ControlStatus const status) noexcept>;
	using GetTalkerStreamStateHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::ControllerEntity::ControlStatus const status) noexcept>;
	using GetListenerStreamStateHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::ControllerEntity::ControlStatus const status) noexcept>;
	using GetTalkerStreamConnectionHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::ControllerEntity::ControlStatus const status) noexcept>;

	/* Discovery Protocol (ADP) */
	/** Enables entity advertising with available duration included between 2-62 seconds, defaulting to 62. */
	using LocalEntity::enableEntityAdvertising; // From LocalEntity
	/** Disables entity advertising. */
	using LocalEntity::disableEntityAdvertising; // From LocalEntity

	/* Enumeration and Control Protocol (AECP) */
	virtual void acquireEntity(la::avdecc::UniqueIdentifier const targetEntityID, bool const isPersistent, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex, AcquireEntityHandler const& handler) const noexcept = 0;
	virtual void releaseEntity(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex, ReleaseEntityHandler const& handler) const noexcept = 0;
	virtual void lockEntity(la::avdecc::UniqueIdentifier const targetEntityID, LockEntityHandler const& handler) const noexcept = 0;
	virtual void unlockEntity(la::avdecc::UniqueIdentifier const targetEntityID, UnlockEntityHandler const& handler) const noexcept = 0;
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
	virtual void readClockDomainDescriptor(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, ClockDomainDescriptorHandler const& handler) const noexcept = 0;
	virtual void setConfiguration(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, SetConfigurationHandler const& handler) const noexcept = 0;
	virtual void setStreamInputFormat(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamFormat const streamFormat, SetStreamInputFormatHandler const& handler) const noexcept = 0;
	virtual void setStreamOutputFormat(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamFormat const streamFormat, SetStreamOutputFormatHandler const& handler) const noexcept = 0;
	virtual void getStreamPortInputAudioMap(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, entity::model::MapIndex const mapIndex, GetStreamPortInputAudioMapHandler const& handler) const noexcept = 0;
	virtual void getStreamPortOutputAudioMap(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, entity::model::MapIndex const mapIndex, GetStreamPortOutputAudioMapHandler const& handler) const noexcept = 0;
	virtual void addStreamPortInputAudioMappings(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, std::vector<la::avdecc::entity::model::AudioMapping> const& mappings, AddStreamPortInputAudioMappingsHandler const& handler) const noexcept = 0;
	virtual void addStreamPortOutputAudioMappings(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::AudioMappings const& mappings, AddStreamPortOutputAudioMappingsHandler const& handler) const noexcept = 0;
	virtual void removeStreamPortInputAudioMappings(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, std::vector<la::avdecc::entity::model::AudioMapping> const& mappings, RemoveStreamPortInputAudioMappingsHandler const& handler) const noexcept = 0;
	virtual void removeStreamPortOutputAudioMappings(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::AudioMappings const& mappings, RemoveStreamPortOutputAudioMappingsHandler const& handler) const noexcept = 0;
	virtual void getStreamInputInfo(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, GetStreamInputInfoHandler const& handler) const noexcept = 0;
	virtual void getStreamOutputInfo(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, GetStreamOutputInfoHandler const& handler) const noexcept = 0;
	virtual void setEntityName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::AvdeccFixedString const& entityName, SetEntityNameHandler const& handler) const noexcept = 0;
	virtual void getEntityName(la::avdecc::UniqueIdentifier const targetEntityID, GetEntityNameHandler const& handler) const noexcept = 0;
	virtual void setEntityGroupName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::AvdeccFixedString const& entityGroupName, SetEntityGroupNameHandler const& handler) const noexcept = 0;
	virtual void getEntityGroupName(la::avdecc::UniqueIdentifier const targetEntityID, GetEntityGroupNameHandler const& handler) const noexcept = 0;
	virtual void setConfigurationName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AvdeccFixedString const& entityGroupName, SetConfigurationNameHandler const& handler) const noexcept = 0;
	virtual void getConfigurationName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, GetConfigurationNameHandler const& handler) const noexcept = 0;
	virtual void setStreamInputName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::AvdeccFixedString const& streamInputName, SetStreamInputNameHandler const& handler) const noexcept = 0;
	virtual void getStreamInputName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, GetStreamInputNameHandler const& handler) const noexcept = 0;
	virtual void setStreamOutputName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::AvdeccFixedString const& streamOutputName, SetStreamOutputNameHandler const& handler) const noexcept = 0;
	virtual void getStreamOutputName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, GetStreamOutputNameHandler const& handler) const noexcept = 0;
	virtual void setAudioUnitSamplingRate(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::AudioUnitIndex const audioUnitIndex, la::avdecc::entity::model::SamplingRate const samplingRate, SetAudioUnitSamplingRateHandler const& handler) const noexcept = 0;
	virtual void setVideoClusterSamplingRate(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ClusterIndex const videoClusterIndex, la::avdecc::entity::model::SamplingRate const samplingRate, SetVideoClusterSamplingRateHandler const& handler) const noexcept = 0;
	virtual void setSensorClusterSamplingRate(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ClusterIndex const sensorClusterIndex, la::avdecc::entity::model::SamplingRate const samplingRate, SetSensorClusterSamplingRateHandler const& handler) const noexcept = 0;
	virtual void setClockSource(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::entity::model::ClockSourceIndex const clockSourceIndex, SetClockSourceHandler const& handler) const noexcept = 0;
	virtual void startStreamInput(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, StartStreamInputHandler const& handler) const noexcept = 0;
	virtual void startStreamOutput(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, StartStreamOutputHandler const& handler) const noexcept = 0;
	virtual void stopStreamInput(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, StopStreamInputHandler const& handler) const noexcept = 0;
	virtual void stopStreamOutput(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, StopStreamOutputHandler const& handler) const noexcept = 0;
	virtual void getAvbInfo(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, GetAvbInfoHandler const& handler) const noexcept = 0;

	/* Connection Management Protocol (ACMP) */
	virtual void connectStream(la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, ConnectStreamHandler const& handler) const noexcept = 0;
	virtual void disconnectStream(la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, DisconnectStreamHandler const& handler) const noexcept = 0;
	virtual void disconnectTalkerStream(la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, DisconnectTalkerStreamHandler const& handler) const noexcept = 0;
	virtual void getTalkerStreamState(la::avdecc::entity::model::StreamIdentification const& talkerStream, GetTalkerStreamStateHandler const& handler) const noexcept = 0;
	virtual void getListenerStreamState(la::avdecc::entity::model::StreamIdentification const& listenerStream, GetListenerStreamStateHandler const& handler) const noexcept = 0;
	virtual void getTalkerStreamConnection(la::avdecc::entity::model::StreamIdentification const& talkerStream, uint16_t const connectionIndex, GetTalkerStreamConnectionHandler const& handler) const noexcept = 0;

	/* Other methods */
	virtual void setDelegate(Delegate* const delegate) noexcept = 0;

	/* Utility methods */
	static LA_AVDECC_API std::string LA_AVDECC_CALL_CONVENTION statusToString(la::avdecc::entity::ControllerEntity::AemCommandStatus const status);
	static LA_AVDECC_API std::string LA_AVDECC_CALL_CONVENTION statusToString(la::avdecc::entity::ControllerEntity::ControlStatus const status);

	// Deleted compiler auto-generated methods
	ControllerEntity(ControllerEntity&&) = delete;
	ControllerEntity(ControllerEntity const&) = delete;
	ControllerEntity& operator=(ControllerEntity const&) = delete;
	ControllerEntity& operator=(ControllerEntity&&) = delete;

protected:
	/** Constructor */
	ControllerEntity(UniqueIdentifier const entityID, networkInterface::MacAddress const& macAddress, model::VendorEntityModel const vendorEntityModelID, EntityCapabilities const entityCapabilities,
									 std::uint16_t const talkerStreamSources, TalkerCapabilities const talkerCapabilities,
									 std::uint16_t const listenerStreamSinks, ListenerCapabilities const listenerCapabilities,
									 ControllerCapabilities const controllerCapabilities,
									 std::uint16_t const identifyControlIndex, std::uint16_t const interfaceIndex, UniqueIdentifier const associationID) noexcept;


	/** Destructor */
	virtual ~ControllerEntity() noexcept = default;
};

/* Operator overloads */
constexpr bool operator!(la::avdecc::entity::ControllerEntity::AemCommandStatus const status)
{
	return status != ControllerEntity::AemCommandStatus::Success;
}
constexpr la::avdecc::entity::ControllerEntity::AemCommandStatus operator|(la::avdecc::entity::ControllerEntity::AemCommandStatus const lhs, la::avdecc::entity::ControllerEntity::AemCommandStatus const rhs)
{
	if (!!lhs)
		return rhs;
	return lhs;
}
constexpr la::avdecc::entity::ControllerEntity::AemCommandStatus& operator|=(la::avdecc::entity::ControllerEntity::AemCommandStatus& lhs, la::avdecc::entity::ControllerEntity::AemCommandStatus const rhs)
{
	if (!!lhs)
		lhs = rhs;
	return lhs;
}
constexpr bool operator!(la::avdecc::entity::ControllerEntity::ControlStatus const status)
{
	return status != ControllerEntity::ControlStatus::Success;
}
constexpr la::avdecc::entity::ControllerEntity::ControlStatus& operator|=(la::avdecc::entity::ControllerEntity::ControlStatus& lhs, la::avdecc::entity::ControllerEntity::ControlStatus const rhs)
{
	if (!!lhs)
		lhs = rhs;
	return lhs;
}
constexpr la::avdecc::entity::ControllerEntity::ControlStatus operator|=(la::avdecc::entity::ControllerEntity::ControlStatus const lhs, la::avdecc::entity::ControllerEntity::ControlStatus const rhs)
{
	if (!!lhs)
		return rhs;
	return lhs;
}

} // namespace entity
} // namespace avdecc
} // namespace la
