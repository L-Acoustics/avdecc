/*
* Copyright (C) 2016-2017, L-Acoustics and its contributors

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
		virtual void onTransportError() noexcept {}

		/* Discovery Protocol (ADP) */
		/** Called when a new entity was discovered on the network (either local or remote). */
		virtual void onEntityOnline(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::Entity const& /*entity*/) noexcept {}
		/** Called when an already discovered entity updated its discovery (ADP) information. */
		virtual void onEntityUpdate(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::Entity const& /*entity*/) noexcept {} // When GpgpGrandMasterID, GpgpDomainNumber or EntityCapabilities changed
		/** Called when an already discovered entity went offline or timed out (either local or remote). */
		virtual void onEntityOffline(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const /*entityID*/) noexcept {}

		/* Connection Management Protocol sniffed messages (ACMP) */
		/** Called when a stream connection has been sniffed on the network (not originating from this controller entity). */
		virtual void onConnectStreamSniffed(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const /*talkerEntityID*/, la::avdecc::entity::model::StreamIndex const /*talkerStreamIndex*/, la::avdecc::UniqueIdentifier const /*listenerEntityID*/, la::avdecc::entity::model::StreamIndex const /*listenerStreamIndex*/, uint16_t const /*connectionCount*/, la::avdecc::entity::ConnectionFlags const /*flags*/, la::avdecc::entity::ControllerEntity::ControlStatus const /*status*/) noexcept {}
		/** Called when a stream fast connection has been sniffed on the network (not originating from this controller entity). */
		virtual void onFastConnectStreamSniffed(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const /*talkerEntityID*/, la::avdecc::entity::model::StreamIndex const /*talkerStreamIndex*/, la::avdecc::UniqueIdentifier const /*listenerEntityID*/, la::avdecc::entity::model::StreamIndex const /*listenerStreamIndex*/, uint16_t const /*connectionCount*/, la::avdecc::entity::ConnectionFlags const /*flags*/, la::avdecc::entity::ControllerEntity::ControlStatus const /*status*/) noexcept {}
		/** Called when a stream disconnection has been sniffed on the network (not originating from this controller entity). */
		virtual void onDisconnectStreamSniffed(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const /*talkerEntityID*/, la::avdecc::entity::model::StreamIndex const /*talkerStreamIndex*/, la::avdecc::UniqueIdentifier const /*listenerEntityID*/, la::avdecc::entity::model::StreamIndex const /*listenerStreamIndex*/, uint16_t const /*connectionCount*/, la::avdecc::entity::ConnectionFlags const /*flags*/, la::avdecc::entity::ControllerEntity::ControlStatus const /*status*/) noexcept {}
		/** Called when a stream state query has been sniffed on the network (not originating from this controller entity). */
		virtual void onGetListenerStreamStateSniffed(la::avdecc::entity::ControllerEntity const* const /*controller*/, la::avdecc::UniqueIdentifier const /*listenerEntityID*/, la::avdecc::entity::model::StreamIndex const /*listenerStreamIndex*/, la::avdecc::UniqueIdentifier const /*talkerEntityID*/, la::avdecc::entity::model::StreamIndex const /*talkerStreamIndex*/, uint16_t const /*connectionCount*/, la::avdecc::entity::ConnectionFlags const /*flags*/, la::avdecc::entity::ControllerEntity::ControlStatus const /*status*/) noexcept {}

		/* Unsolicited notifications (not triggered for our own commands, the command's 'result' method will be called in that case). Only successfull commands can cause an unsolicited notification. */
		/** Called when an entity has been acquired by another controller. */
		virtual void onEntityAcquired(la::avdecc::UniqueIdentifier const /*acquiredEntity*/, la::avdecc::UniqueIdentifier const /*owningEntity*/) noexcept {}
		/** Called when an entity has been released by another controller. */
		virtual void onEntityReleased(la::avdecc::UniqueIdentifier const /*releasedEntity*/, la::avdecc::UniqueIdentifier const /*owningEntity*/) noexcept {}
		/** Called when the format of an input stream was changed by another controller. */
		virtual void onStreamInputFormatChanged(la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::StreamFormat const /*streamFormat*/) noexcept {}
		/** Called when the format of an output stream was changed by another controller. */
		virtual void onStreamOutputFormatChanged(la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::StreamFormat const /*streamFormat*/) noexcept {}
		/** Called when the audio mappings of an input stream was changed by another controller. */
		virtual void onStreamInputAudioMappingsChanged(la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, entity::model::MapIndex const /*numberOfMaps*/, entity::model::MapIndex const /*mapIndex*/, la::avdecc::entity::model::AudioMappings const& /*mappings*/) noexcept {}
		/** Called when the audio mappings of an output stream was changed by another controller. */
		virtual void onStreamOutputAudioMappingsChanged(la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, entity::model::MapIndex const /*numberOfMaps*/, entity::model::MapIndex const /*mapIndex*/, la::avdecc::entity::model::AudioMappings const& /*mappings*/) noexcept {}
		/** Called when the information of an input stream was changed by another controller. */
		virtual void onStreamInputInfoChanged(la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::StreamInfo const& /*info*/) noexcept {}
		/** Called when the information of an output stream was changed by another controller. */
		virtual void onStreamOutputInfoChanged(la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::StreamInfo const& /*info*/) noexcept {}
		// TBD: SetConfiguration
		// TBD: SetStreamInfo
		virtual void onEntityNameChanged(la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::AvdeccFixedString const& /*entityName*/) noexcept {}
		virtual void onEntityGroupNameChanged(la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::AvdeccFixedString const& /*entityGroupName*/) noexcept {}
		virtual void onConfigurationNameChanged(la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*configurationName*/) noexcept {}
		// TBD: SetSamplingRate
		// TBD: SetClockSource
		/** Called when an input stream was started by another controller. */
		virtual void onStreamInputStarted(la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/) noexcept {}
		/** Called when an output stream was started by another controller. */
		virtual void onStreamOutputStarted(la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/) noexcept {}
		/** Called when an input stream was stopped by another controller. */
		virtual void onStreamInputStopped(la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/) noexcept {}
		/** Called when an output stream was stopped by another controller. */
		virtual void onStreamOutputStopped(la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/) noexcept {}
		// TBD: AddAudioMappings
		// TBD: RemoveAudioMappings
	};

	/* Enumeration and Control Protocol (AECP) handlers */
	using QueryEntityAvailableHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status) noexcept>;
	using QueryControllerAvailableHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status) noexcept>;
	using LockEntityHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const owningEntity) noexcept>;
	using UnlockEntityHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status) noexcept>;
	using AcquireEntityHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const owningEntity) noexcept>;
	using ReleaseEntityHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const owningEntity) noexcept>;
	using RegisterUnsolicitedNotificationsHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status) noexcept>;
	using UnregisterUnsolicitedNotificationsHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status) noexcept>;
	using EntityDescriptorHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::EntityDescriptor const& descriptor) noexcept>;
	using ConfigurationDescriptorHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationDescriptor const& descriptor) noexcept>;
	using LocaleDescriptorHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::LocaleDescriptor const& descriptor) noexcept>;
	using StringsDescriptorHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::StringsDescriptor const& descriptor) noexcept>;
	using StreamInputDescriptorHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamDescriptor const& descriptor) noexcept>;
	using StreamOutputDescriptorHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamDescriptor const& descriptor) noexcept>;
	using SetStreamInputFormatHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamFormat const streamFormat) noexcept>;
	using SetStreamOutputFormatHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamFormat const streamFormat) noexcept>;
	using GetStreamInputAudioMapHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, la::avdecc::entity::model::AudioMappings const& mappings) noexcept>;
	using GetStreamOutputAudioMapHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, entity::model::MapIndex const numberOfMaps, entity::model::MapIndex const mapIndex, la::avdecc::entity::model::AudioMappings const& mappings) noexcept>;
	using AddStreamInputAudioMappingsHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::AudioMappings const& mappings) noexcept>;
	using AddStreamOutputAudioMappingsHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::AudioMappings const& mappings) noexcept>;
	using RemoveStreamInputAudioMappingsHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::AudioMappings const& mappings) noexcept>;
	using RemoveStreamOutputAudioMappingsHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::AudioMappings const& mappings) noexcept>;
	using GetStreamInputInfoHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamInfo const& info) noexcept>;
	using GetStreamOutputInfoHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamInfo const& info) noexcept>;
	using SetEntityNameHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status) noexcept>;
	using GetEntityNameHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::AvdeccFixedString const& entityName) noexcept>;
	using SetEntityGroupNameHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status) noexcept>;
	using GetEntityGroupNameHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::AvdeccFixedString const& entityGroupName) noexcept>;
	using SetConfigurationNameHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex) noexcept>;
	using GetConfigurationNameHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AvdeccFixedString const& entityGroupName) noexcept>;
	using StartStreamInputHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex) noexcept>;
	using StartStreamOutputHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex) noexcept>;
	using StopStreamInputHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex) noexcept>;
	using StopStreamOutputHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::ControllerEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex) noexcept>;
	/* Connection Management Protocol (ACMP) handlers */
	using ConnectStreamHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const talkerEntityID, la::avdecc::entity::model::StreamIndex const talkerStreamIndex, la::avdecc::UniqueIdentifier const listenerEntityID, la::avdecc::entity::model::StreamIndex const listenerStreamIndex, uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::ControllerEntity::ControlStatus const status) noexcept>;
	using DisconnectStreamHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const talkerEntityID, la::avdecc::entity::model::StreamIndex const talkerStreamIndex, la::avdecc::UniqueIdentifier const listenerEntityID, la::avdecc::entity::model::StreamIndex const listenerStreamIndex, uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::ControllerEntity::ControlStatus const status) noexcept>;
	using GetListenerStreamStateHandler = std::function<void(la::avdecc::entity::ControllerEntity const* const controller, la::avdecc::UniqueIdentifier const listenerEntityID, la::avdecc::entity::model::StreamIndex const listenerStreamIndex, la::avdecc::UniqueIdentifier const talkerEntityID, la::avdecc::entity::model::StreamIndex const talkerStreamIndex, uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::ControllerEntity::ControlStatus const status) noexcept>;

	/* Discovery Protocol (ADP) */
	/** Enables entity advertising with available duration included between 2-62 seconds, defaulting to 62. */
	using LocalEntity::enableEntityAdvertising; // From LocalEntity
	/** Disables entity advertising. */
	using LocalEntity::disableEntityAdvertising; // From LocalEntity

	/* Enumeration and Control Protocol (AECP) */
	virtual void queryEntityAvailable(la::avdecc::UniqueIdentifier const targetEntityID, QueryEntityAvailableHandler const& handler) const noexcept = 0;
	virtual void queryControllerAvailable(la::avdecc::UniqueIdentifier const targetEntityID, QueryControllerAvailableHandler const& handler) const noexcept = 0;
	virtual void lockEntity(la::avdecc::UniqueIdentifier const targetEntityID, LockEntityHandler const& handler) const noexcept = 0;
	virtual void unlockEntity(la::avdecc::UniqueIdentifier const targetEntityID, UnlockEntityHandler const& handler) const noexcept = 0;
	virtual void acquireEntity(la::avdecc::UniqueIdentifier const targetEntityID, bool const isPersistent, AcquireEntityHandler const& handler) const noexcept = 0;
	virtual void releaseEntity(la::avdecc::UniqueIdentifier const targetEntityID, ReleaseEntityHandler const& handler) const noexcept = 0;
	virtual void registerUnsolicitedNotifications(la::avdecc::UniqueIdentifier const targetEntityID, RegisterUnsolicitedNotificationsHandler const& handler) const noexcept = 0;
	virtual void unregisterUnsolicitedNotifications(la::avdecc::UniqueIdentifier const targetEntityID, UnregisterUnsolicitedNotificationsHandler const& handler) const noexcept = 0;
	virtual void readEntityDescriptor(la::avdecc::UniqueIdentifier const targetEntityID, EntityDescriptorHandler const& handler) const noexcept = 0;
	virtual void readConfigurationDescriptor(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, ConfigurationDescriptorHandler const& handler) const noexcept = 0;
	virtual void readLocaleDescriptor(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::LocaleIndex const localeIndex, LocaleDescriptorHandler const& handler) const noexcept = 0;
	virtual void readStringsDescriptor(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StringsIndex const stringsIndex, StringsDescriptorHandler const& handler) const noexcept = 0;
	virtual void readStreamInputDescriptor(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, StreamInputDescriptorHandler const& handler) const noexcept = 0;
	virtual void readStreamOutputDescriptor(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, StreamOutputDescriptorHandler const& handler) const noexcept = 0;
	virtual void setStreamInputFormat(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamFormat const streamFormat, SetStreamInputFormatHandler const& handler) const noexcept = 0;
	virtual void setStreamOutputFormat(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamFormat const streamFormat, SetStreamOutputFormatHandler const& handler) const noexcept = 0;
	virtual void getStreamInputAudioMap(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, entity::model::MapIndex const mapIndex, GetStreamInputAudioMapHandler const& handler) const noexcept = 0;
	virtual void getStreamOutputAudioMap(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, entity::model::MapIndex const mapIndex, GetStreamOutputAudioMapHandler const& handler) const noexcept = 0;
	virtual void addStreamInputAudioMappings(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, std::vector<la::avdecc::entity::model::AudioMapping> const& mappings, AddStreamInputAudioMappingsHandler const& handler) const noexcept = 0;
	virtual void addStreamOutputAudioMappings(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::AudioMappings const& mappings, AddStreamOutputAudioMappingsHandler const& handler) const noexcept = 0;
	virtual void removeStreamInputAudioMappings(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, std::vector<la::avdecc::entity::model::AudioMapping> const& mappings, RemoveStreamInputAudioMappingsHandler const& handler) const noexcept = 0;
	virtual void removeStreamOutputAudioMappings(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::AudioMappings const& mappings, RemoveStreamOutputAudioMappingsHandler const& handler) const noexcept = 0;
	virtual void getStreamInputInfo(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, GetStreamInputInfoHandler const& handler) const noexcept = 0;
	virtual void setEntityName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::AvdeccFixedString const& entityName, SetEntityNameHandler const& handler) const noexcept = 0;
	virtual void getEntityName(la::avdecc::UniqueIdentifier const targetEntityID, GetEntityNameHandler const& handler) const noexcept = 0;
	virtual void setEntityGroupName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::AvdeccFixedString const& entityGroupName, SetEntityGroupNameHandler const& handler) const noexcept = 0;
	virtual void getEntityGroupName(la::avdecc::UniqueIdentifier const targetEntityID, GetEntityGroupNameHandler const& handler) const noexcept = 0;
	virtual void setConfigurationName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AvdeccFixedString const& entityGroupName, SetConfigurationNameHandler const& handler) const noexcept = 0;
	virtual void getConfigurationName(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, GetConfigurationNameHandler const& handler) const noexcept = 0;
	virtual void startStreamInput(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, StartStreamInputHandler const& handler) const noexcept = 0;
	virtual void startStreamOutput(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, StartStreamOutputHandler const& handler) const noexcept = 0;
	virtual void stopStreamInput(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, StopStreamInputHandler const& handler) const noexcept = 0;
	virtual void stopStreamOutput(la::avdecc::UniqueIdentifier const targetEntityID, la::avdecc::entity::model::StreamIndex const streamIndex, StopStreamOutputHandler const& handler) const noexcept = 0;

	/* Connection Management Protocol (ACMP) */
	virtual void connectStream(la::avdecc::UniqueIdentifier const talkerEntityID, la::avdecc::entity::model::StreamIndex const talkerStreamIndex, la::avdecc::UniqueIdentifier const listenerEntityID, la::avdecc::entity::model::StreamIndex const listenerStreamIndex, ConnectStreamHandler const& handler) const noexcept = 0;
	virtual void disconnectStream(la::avdecc::UniqueIdentifier const talkerEntityID, la::avdecc::entity::model::StreamIndex const talkerStreamIndex, la::avdecc::UniqueIdentifier const listenerEntityID, la::avdecc::entity::model::StreamIndex const listenerStreamIndex, DisconnectStreamHandler const& handler) const noexcept = 0;
	virtual void getListenerStreamState(la::avdecc::UniqueIdentifier const listenerEntityID, la::avdecc::entity::model::StreamIndex const listenerStreamIndex, GetListenerStreamStateHandler const& handler) const noexcept = 0;

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
