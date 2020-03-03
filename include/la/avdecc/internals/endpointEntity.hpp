/*
* Copyright (C) 2016-2020, L-Acoustics and its contributors

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
* @file endpointEntity.hpp
* @author Christophe Calmejane
* @brief Avdecc endpoint entity.
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
namespace endpoint
{
class Interface
{
public:
	/* Enumeration and Control Protocol (AECP) AEM handlers */
	using QueryEntityAvailableHandler = std::function<void(la::avdecc::entity::endpoint::Interface const* const endpoint, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status)>;
	using QueryControllerAvailableHandler = std::function<void(la::avdecc::entity::endpoint::Interface const* const endpoint, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status)>;
	/* Enumeration and Control Protocol (AECP) AA handlers */
	//using AddressAccessHandler = std::function<void(la::avdecc::entity::endpoint::Interface const* const endpoint, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AaCommandStatus const status, la::avdecc::entity::addressAccess::Tlvs const& tlvs)>;
	/* Connection Management Protocol (ACMP) handlers */
	//using ConnectStreamHandler = std::function<void(la::avdecc::entity::endpoint::Interface const* const endpoint, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status)>;
	//using DisconnectStreamHandler = std::function<void(la::avdecc::entity::endpoint::Interface const* const endpoint, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status)>;
	//using DisconnectTalkerStreamHandler = std::function<void(la::avdecc::entity::endpoint::Interface const* const endpoint, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status)>;
	//using GetTalkerStreamConnectionHandler = std::function<void(la::avdecc::entity::endpoint::Interface const* const endpoint, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status)>;

	/* Enumeration and Control Protocol (AECP) AEM */
	virtual void queryEntityAvailable(UniqueIdentifier const targetEntityID, networkInterface::MacAddress const& targetMacAddress, QueryEntityAvailableHandler const& handler) const noexcept = 0;
	virtual void queryControllerAvailable(UniqueIdentifier const targetEntityID, networkInterface::MacAddress const& targetMacAddress, QueryControllerAvailableHandler const& handler) const noexcept = 0;

	/* Enumeration and Control Protocol (AECP) AA */
	//virtual void addressAccess(UniqueIdentifier const targetEntityID, addressAccess::Tlvs const& tlvs, AddressAccessHandler const& handler) const noexcept = 0;

	/* Connection Management Protocol (ACMP) */
	//virtual void connectStream(model::StreamIdentification const& talkerStream, model::StreamIdentification const& listenerStream, ConnectStreamHandler const& handler) const noexcept = 0;
	//virtual void disconnectStream(model::StreamIdentification const& talkerStream, model::StreamIdentification const& listenerStream, DisconnectStreamHandler const& handler) const noexcept = 0;
	//virtual void disconnectTalkerStream(model::StreamIdentification const& talkerStream, model::StreamIdentification const& listenerStream, DisconnectTalkerStreamHandler const& handler) const noexcept = 0;
	//virtual void getTalkerStreamConnection(model::StreamIdentification const& talkerStream, std::uint16_t const connectionIndex, GetTalkerStreamConnectionHandler const& handler) const noexcept = 0;

	virtual void sendAemAecpResponse(protocol::AemAecpdu const& aemAecpduCommand, protocol::AecpStatus const status, protocol::AemAecpdu::Payload const& payload) const noexcept = 0;

	// To Implement
	// Notes:
	//  - sequenceID automatically handled internaly
	//  - Either be passed AemAecpdu&& (and const& overload) or AemAecpdu::UniquePointer&&
	//virtual void sendAemAecpUnsolicitedResponse() const noexcept = 0;

	// Defaulted compiler auto-generated methods
	Interface() noexcept = default;
	virtual ~Interface() noexcept = default;
	Interface(Interface&&) = default;
	Interface(Interface const&) = default;
	Interface& operator=(Interface const&) = default;
	Interface& operator=(Interface&&) = default;
};

/** Delegate for all endpoint related notifications. */
class Delegate
{
public:
	/* Global notifications */
	/** Called when a fatal error on the transport layer occured. */
	virtual void onTransportError(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/) noexcept {}

	/* Discovery Protocol (ADP) */
	/** Called when a new entity was discovered on the network (either local or remote). */
	virtual void onEntityOnline(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::Entity const& /*entity*/) noexcept {}
	/** Called when an already discovered entity updated its discovery (ADP) information. */
	virtual void onEntityUpdate(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::Entity const& /*entity*/) noexcept {} // When GpgpGrandMasterID, GpgpDomainNumber or EntityCapabilities changed
	/** Called when an already discovered entity went offline or timed out (either local or remote). */
	virtual void onEntityOffline(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/) noexcept {}

	//	/* Connection Management Protocol sniffed messages (ACMP) (not triggered for our own commands even though ACMP messages are broadcasted, the command's 'result' method will be called in that case) */
	//	/** Called when a controller connect request has been sniffed on the network. */
	//	virtual void onControllerConnectResponseSniffed(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::entity::model::StreamIdentification const& /*talkerStream*/, la::avdecc::entity::model::StreamIdentification const& /*listenerStream*/, std::uint16_t const /*connectionCount*/, la::avdecc::entity::ConnectionFlags const /*flags*/, la::avdecc::entity::LocalEntity::ControlStatus const /*status*/) noexcept {}
	//	/** Called when a controller disconnect request has been sniffed on the network. */
	//	virtual void onControllerDisconnectResponseSniffed(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::entity::model::StreamIdentification const& /*talkerStream*/, la::avdecc::entity::model::StreamIdentification const& /*listenerStream*/, std::uint16_t const /*connectionCount*/, la::avdecc::entity::ConnectionFlags const /*flags*/, la::avdecc::entity::LocalEntity::ControlStatus const /*status*/) noexcept {}
	//	/** Called when a listener connect request has been sniffed on the network (either due to a another controller connect, or a fast connect). */
	//	virtual void onListenerConnectResponseSniffed(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::entity::model::StreamIdentification const& /*talkerStream*/, la::avdecc::entity::model::StreamIdentification const& /*listenerStream*/, std::uint16_t const /*connectionCount*/, la::avdecc::entity::ConnectionFlags const /*flags*/, la::avdecc::entity::LocalEntity::ControlStatus const /*status*/) noexcept {}
	//	/** Called when a listener disconnect request has been sniffed on the network (either due to a another controller disconnect, or a fast disconnect). */
	//	virtual void onListenerDisconnectResponseSniffed(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::entity::model::StreamIdentification const& /*talkerStream*/, la::avdecc::entity::model::StreamIdentification const& /*listenerStream*/, std::uint16_t const /*connectionCount*/, la::avdecc::entity::ConnectionFlags const /*flags*/, la::avdecc::entity::LocalEntity::ControlStatus const /*status*/) noexcept {}
	//	/** Called when a stream state query has been sniffed on the network. */
	//	virtual void onGetTalkerStreamStateResponseSniffed(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::entity::model::StreamIdentification const& /*talkerStream*/, la::avdecc::entity::model::StreamIdentification const& /*listenerStream*/, std::uint16_t const /*connectionCount*/, la::avdecc::entity::ConnectionFlags const /*flags*/, la::avdecc::entity::LocalEntity::ControlStatus const /*status*/) noexcept {}
	//	/** Called when a stream state query has been sniffed on the network */
	//	virtual void onGetListenerStreamStateResponseSniffed(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::entity::model::StreamIdentification const& /*talkerStream*/, la::avdecc::entity::model::StreamIdentification const& /*listenerStream*/, std::uint16_t const /*connectionCount*/, la::avdecc::entity::ConnectionFlags const /*flags*/, la::avdecc::entity::LocalEntity::ControlStatus const /*status*/) noexcept {}
	//
	/* Query received from a Controller. */
	/** Called when a controller wants to register to unsolicited notifications. */
	virtual bool onQueryRegisterToUnsolicitedNotifications(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*controllerID*/, la::avdecc::protocol::AemAecpdu const& /*command*/) noexcept
	{
		return false;
	}
	/** Called when a controller wants to deregister from unsolicited notifications. */
	virtual bool onQueryDeregisteredFromUnsolicitedNotifications(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*controllerID*/, la::avdecc::protocol::AemAecpdu const& /*command*/) noexcept
	{
		return false;
	}
	/** Called when a controller wants to acquire the endpoint. */
	virtual bool onQueryAcquireEntity(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*controllerID*/, la::avdecc::protocol::AemAecpdu const& /*command*/, la::avdecc::entity::model::DescriptorType const /*descriptorType*/, la::avdecc::entity::model::DescriptorIndex const /*descriptorIndex*/) noexcept
	{
		return false;
	}
	/** Called when a controller wants to release the endpoint. */
	virtual bool onQueryReleaseEntity(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*controllerID*/, la::avdecc::protocol::AemAecpdu const& /*command*/, la::avdecc::entity::model::DescriptorType const /*descriptorType*/, la::avdecc::entity::model::DescriptorIndex const /*descriptorIndex*/) noexcept
	{
		return false;
	}
	//	/** Called when an entity has been locked by another controller. */
	//	virtual void onEntityLocked(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::UniqueIdentifier const /*lockingEntity*/, la::avdecc::entity::model::DescriptorType const /*descriptorType*/, la::avdecc::entity::model::DescriptorIndex const /*descriptorIndex*/) noexcept {}
	//	/** Called when an entity has been unlocked by another controller (or because of the lock timeout). */
	//	virtual void onEntityUnlocked(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::UniqueIdentifier const /*lockingEntity*/, la::avdecc::entity::model::DescriptorType const /*descriptorType*/, la::avdecc::entity::model::DescriptorIndex const /*descriptorIndex*/) noexcept {}
	//	/** Called when the current configuration was changed by another controller. */
	//	virtual void onConfigurationChanged(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/) noexcept {}
	//	/** Called when the format of an input stream was changed by another controller. */
	//	virtual void onStreamInputFormatChanged(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::StreamFormat const /*streamFormat*/) noexcept {}
	//	/** Called when the format of an output stream was changed by another controller. */
	//	virtual void onStreamOutputFormatChanged(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::StreamFormat const /*streamFormat*/) noexcept {}
	//	/** Called when the audio mappings of a stream port input was changed by another controller. */
	//	virtual void onStreamPortInputAudioMappingsChanged(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamPortIndex const /*streamPortIndex*/, la::avdecc::entity::model::MapIndex const /*numberOfMaps*/, la::avdecc::entity::model::MapIndex const /*mapIndex*/, la::avdecc::entity::model::AudioMappings const& /*mappings*/) noexcept {}
	//	/** Called when the audio mappings of a stream port output was changed by another controller. */
	//	virtual void onStreamPortOutputAudioMappingsChanged(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamPortIndex const /*streamPortIndex*/, la::avdecc::entity::model::MapIndex const /*numberOfMaps*/, la::avdecc::entity::model::MapIndex const /*mapIndex*/, la::avdecc::entity::model::AudioMappings const& /*mappings*/) noexcept {}
	//	/** Called when the information of an input stream was changed by another controller. */
	//	virtual void onStreamInputInfoChanged(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::StreamInfo const& /*info*/, bool const /*fromGetStreamInfoResponse*/) noexcept {}
	//	/** Called when the information of an output stream was changed by another controller. */
	//	virtual void onStreamOutputInfoChanged(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::StreamInfo const& /*info*/, bool const /*fromGetStreamInfoResponse*/) noexcept {}
	//	/** Called when the entity's name was changed by another controller. */
	//	virtual void onEntityNameChanged(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::AvdeccFixedString const& /*entityName*/) noexcept {}
	//	/** Called when the entity's group name was changed by another controller. */
	//	virtual void onEntityGroupNameChanged(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::AvdeccFixedString const& /*entityGroupName*/) noexcept {}
	//	/** Called when a configuration name was changed by another controller. */
	//	virtual void onConfigurationNameChanged(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*configurationName*/) noexcept {}
	//	/** Called when an audio unit name was changed by another controller. */
	//	virtual void onAudioUnitNameChanged(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::AudioUnitIndex const /*audioUnitIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*audioUnitName*/) noexcept {}
	//	/** Called when an input stream name was changed by another controller. */
	//	virtual void onStreamInputNameChanged(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*streamName*/) noexcept {}
	//	/** Called when an output stream name was changed by another controller. */
	//	virtual void onStreamOutputNameChanged(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*streamName*/) noexcept {}
	//	/** Called when an avb interface name was changed by another controller. */
	//	virtual void onAvbInterfaceNameChanged(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::AvbInterfaceIndex const /*avbInterfaceIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*avbInterfaceName*/) noexcept {}
	//	/** Called when a clock source name was changed by another controller. */
	//	virtual void onClockSourceNameChanged(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::ClockSourceIndex const /*clockSourceIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*clockSourceName*/) noexcept {}
	//	/** Called when a memory object name was changed by another controller. */
	//	virtual void onMemoryObjectNameChanged(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::MemoryObjectIndex const /*memoryObjectIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*memoryObjectName*/) noexcept {}
	//	/** Called when an audio cluster name was changed by another controller. */
	//	virtual void onAudioClusterNameChanged(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::ClusterIndex const /*audioClusterIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*audioClusterName*/) noexcept {}
	//	/** Called when a clock domain name was changed by another controller. */
	//	virtual void onClockDomainNameChanged(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::ClockDomainIndex const /*clockDomainIndex*/, la::avdecc::entity::model::AvdeccFixedString const& /*clockDomainName*/) noexcept {}
	//	/** Called when an AudioUnit sampling rate was changed by another controller. */
	//	virtual void onAudioUnitSamplingRateChanged(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::AudioUnitIndex const /*audioUnitIndex*/, la::avdecc::entity::model::SamplingRate const /*samplingRate*/) noexcept {}
	//	/** Called when a VideoCluster sampling rate was changed by another controller. */
	//	virtual void onVideoClusterSamplingRateChanged(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ClusterIndex const /*videoClusterIndex*/, la::avdecc::entity::model::SamplingRate const /*samplingRate*/) noexcept {}
	//	/** Called when a SensorCluster sampling rate was changed by another controller. */
	//	virtual void onSensorClusterSamplingRateChanged(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ClusterIndex const /*sensorClusterIndex*/, la::avdecc::entity::model::SamplingRate const /*samplingRate*/) noexcept {}
	//	/** Called when a clock source was changed by another controller. */
	//	virtual void onClockSourceChanged(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ClockDomainIndex const /*clockDomainIndex*/, la::avdecc::entity::model::ClockSourceIndex const /*clockSourceIndex*/) noexcept {}
	//	/** Called when an input stream was started by another controller. */
	//	virtual void onStreamInputStarted(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/) noexcept {}
	//	/** Called when an output stream was started by another controller. */
	//	virtual void onStreamOutputStarted(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/) noexcept {}
	//	/** Called when an input stream was stopped by another controller. */
	//	virtual void onStreamInputStopped(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/) noexcept {}
	//	/** Called when an output stream was stopped by another controller. */
	//	virtual void onStreamOutputStopped(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/) noexcept {}
	//	/** Called when the Avb Info of an Avb Interface changed. */
	//	virtual void onAvbInfoChanged(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::AvbInterfaceIndex const /*avbInterfaceIndex*/, la::avdecc::entity::model::AvbInfo const& /*info*/) noexcept {}
	//	/** Called when the AS Path of an Avb Interface changed. */
	//	virtual void onAsPathChanged(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::AvbInterfaceIndex const /*avbInterfaceIndex*/, la::avdecc::entity::model::AsPath const& /*asPath*/) noexcept {}
	//	/** Called when the counters of Entity changed. */
	//	virtual void onEntityCountersChanged(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::EntityCounterValidFlags const /*validCounters*/, la::avdecc::entity::model::DescriptorCounters const& /*counters*/) noexcept {}
	//	/** Called when the counters of an Avb Interface changed. */
	//	virtual void onAvbInterfaceCountersChanged(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::AvbInterfaceIndex const /*avbInterfaceIndex*/, la::avdecc::entity::AvbInterfaceCounterValidFlags const /*validCounters*/, la::avdecc::entity::model::DescriptorCounters const& /*counters*/) noexcept {}
	//	/** Called when the counters of a Clock Domain changed. */
	//	virtual void onClockDomainCountersChanged(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ClockDomainIndex const /*clockDomainIndex*/, la::avdecc::entity::ClockDomainCounterValidFlags const /*validCounters*/, la::avdecc::entity::model::DescriptorCounters const& /*counters*/) noexcept {}
	//	/** Called when the counters of a Stream Input changed. */
	//	virtual void onStreamInputCountersChanged(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::StreamInputCounterValidFlags const /*validCounters*/, la::avdecc::entity::model::DescriptorCounters const& /*counters*/) noexcept {}
	//	/** Called when the counters of a Stream Output changed. */
	//	virtual void onStreamOutputCountersChanged(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamIndex const /*streamIndex*/, la::avdecc::entity::StreamOutputCounterValidFlags const /*validCounters*/, la::avdecc::entity::model::DescriptorCounters const& /*counters*/) noexcept {}
	//	/** Called when (some or all) audio mappings of a stream port input were added by another controller. */
	//	virtual void onStreamPortInputAudioMappingsAdded(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamPortIndex const /*streamPortIndex*/, la::avdecc::entity::model::AudioMappings const& /*mappings*/) noexcept {}
	//	/** Called when (some or all) audio mappings of a stream port output were added by another controller. */
	//	virtual void onStreamPortOutputAudioMappingsAdded(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamPortIndex const /*streamPortIndex*/, la::avdecc::entity::model::AudioMappings const& /*mappings*/) noexcept {}
	//	/** Called when (some or all) audio mappings of a stream port input were removed by another controller. */
	//	virtual void onStreamPortInputAudioMappingsRemoved(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamPortIndex const /*streamPortIndex*/, la::avdecc::entity::model::AudioMappings const& /*mappings*/) noexcept {}
	//	/** Called when (some or all) audio mappings of a stream port output were removed by another controller. */
	//	virtual void onStreamPortOutputAudioMappingsRemoved(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::StreamPortIndex const /*streamPortIndex*/, la::avdecc::entity::model::AudioMappings const& /*mappings*/) noexcept {}
	//	/** Called when the length of a MemoryObject changed. */
	//	virtual void onMemoryObjectLengthChanged(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::ConfigurationIndex const /*configurationIndex*/, la::avdecc::entity::model::MemoryObjectIndex const /*memoryObjectIndex*/, std::uint64_t const /*length*/) noexcept {}
	//	/** Called when there is a status update on an ongoing Operation */
	//	virtual void onOperationStatus(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/, la::avdecc::entity::model::DescriptorType const /*descriptorType*/, la::avdecc::entity::model::DescriptorIndex const /*descriptorIndex*/, la::avdecc::entity::model::OperationID const /*operationID*/, std::uint16_t const /*percentComplete*/) noexcept {}

	//	/* Identification notifications */
	//	/** Called when an entity emits an identify notification. */
	//	virtual void onEntityIdentifyNotification(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const /*entityID*/) noexcept {}

	//	/* **** Statistics **** */
	//	/** Notification for when an AECP Command was resent due to a timeout. If the retry time out again, then onAecpTimeout will be called. */
	//	virtual void onAecpRetry(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const& /*entityID*/) noexcept {}
	//	/** Notification for when an AECP Command timed out (not called when onAecpRetry is called). */
	//	virtual void onAecpTimeout(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const& /*entityID*/) noexcept {}
	//	/** Notification for when an AECP Response is received but is not expected (might have already timed out). */
	//	virtual void onAecpUnexpectedResponse(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const& /*entityID*/) noexcept {}
	//	/** Notification for when an AECP Response is received (not an Unsolicited one) along with the time elapsed between the send and the receive. */
	//	virtual void onAecpResponseTime(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const& /*entityID*/, std::chrono::milliseconds const& /*responseTime*/) noexcept {}
	//	/** Notification for when an AEM-AECP Unsolicited Response was received. */
	//	virtual void onAemAecpUnsolicitedReceived(la::avdecc::entity::endpoint::Interface const* const /*endpoint*/, la::avdecc::UniqueIdentifier const& /*entityID*/) noexcept {}

	// Defaulted compiler auto-generated methods
	Delegate() noexcept = default;
	virtual ~Delegate() noexcept = default;
	Delegate(Delegate&&) = default;
	Delegate(Delegate const&) = default;
	Delegate& operator=(Delegate const&) = default;
	Delegate& operator=(Delegate&&) = default;
};
} // namespace endpoint

class EndpointEntity : public LocalEntity, public endpoint::Interface
{
public:
	using UniquePointer = std::unique_ptr<EndpointEntity, void (*)(EndpointEntity*)>;

	/**
	* @brief Factory method to create a new EndpointEntity.
	* @details Creates a new EndpointEntity as a unique pointer.
	* @param[in] protocolInterface The protocol interface to bind the entity to.
	* @param[in] commonInformation Common information for this endpoint entity.
	* @param[in] interfacesInformation All interfaces information for this endpoint entity.
	* @param[in] delegate The Delegate to be called whenever a endpoint related notification occurs.
	* @return A new EndpointEntity as a Entity::UniquePointer.
	* @note Might throw an Exception.
	*/
	static UniquePointer create(protocol::ProtocolInterface* const protocolInterface, CommonInformation const& commonInformation, InterfacesInformation const& interfacesInformation, endpoint::Delegate* const delegate)
	{
		auto deleter = [](EndpointEntity* self)
		{
			self->destroy();
		};
		return UniquePointer(createRawEndpointEntity(protocolInterface, commonInformation, interfacesInformation, delegate), deleter);
	}

	/* Discovery Protocol (ADP) */
	/** Enables entity advertising with available duration included between 2-62 seconds on the specified interfaceIndex if set, otherwise on all interfaces. Returns false if EntityID is already in use on the local computer, true otherwise. */
	using LocalEntity::enableEntityAdvertising;
	/** Disables entity advertising on the specified interfaceIndex if set, otherwise on all interfaces. */
	using LocalEntity::disableEntityAdvertising;

	/* Other methods */
	virtual void setEndpointDelegate(endpoint::Delegate* const delegate) noexcept = 0;

	// Deleted compiler auto-generated methods
	EndpointEntity(EndpointEntity&&) = delete;
	EndpointEntity(EndpointEntity const&) = delete;
	EndpointEntity& operator=(EndpointEntity const&) = delete;
	EndpointEntity& operator=(EndpointEntity&&) = delete;

protected:
	/** Constructor */
	EndpointEntity(CommonInformation const& commonInformation, InterfacesInformation const& interfacesInformation);

	/** Destructor */
	virtual ~EndpointEntity() noexcept = default;

private:
	/** Entry point */
	static LA_AVDECC_API EndpointEntity* LA_AVDECC_CALL_CONVENTION createRawEndpointEntity(protocol::ProtocolInterface* const protocolInterface, CommonInformation const& commonInformation, InterfacesInformation const& interfacesInformation, endpoint::Delegate* const delegate);

	/** Destroy method for COM-like interface */
	virtual void destroy() noexcept = 0;
};

} // namespace entity
} // namespace avdecc
} // namespace la
