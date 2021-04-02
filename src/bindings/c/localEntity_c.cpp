/*
* Copyright (C) 2016-2021, L-Acoustics and its contributors

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
* @file localEntity_c.cpp
* @author Christophe Calmejane
* @brief C bindings for la::avdecc::LocalEntity.
*/

#include <la/avdecc/internals/entity.hpp>
#include <la/avdecc/internals/aggregateEntity.hpp>
#include "la/avdecc/avdecc.h"
#include "protocolInterface_c.hpp"
#include "utils.hpp"

/* ************************************************************************** */
/* Controller::Delegate Bindings                                              */
/* ************************************************************************** */
class Delegate final : public la::avdecc::entity::controller::Delegate
{
public:
	Delegate(avdecc_local_entity_controller_delegate_p const delegate, LA_AVDECC_LOCAL_ENTITY_HANDLE const handle)
		: _delegate(delegate)
		, _handle(handle)
	{
	}

	LA_AVDECC_LOCAL_ENTITY_HANDLE getHandle() const noexcept
	{
		return _handle;
	}

private:
	/* Global notifications */
	virtual void onTransportError(la::avdecc::entity::controller::Interface const* const /*controller*/) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onTransportError, _handle);
	}

	/* Discovery Protocol (ADP) */
	virtual void onEntityOnline(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::Entity const& entity) noexcept override
	{
		auto const e = la::avdecc::bindings::fromCppToC::make_entity(entity);
		la::avdecc::utils::invokeProtectedHandler(_delegate->onEntityOnline, _handle, entityID, &e.entity);
	}
	virtual void onEntityUpdate(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::Entity const& entity) noexcept override
	{
		auto const e = la::avdecc::bindings::fromCppToC::make_entity(entity);
		la::avdecc::utils::invokeProtectedHandler(_delegate->onEntityUpdate, _handle, entityID, &e.entity);
	}
	virtual void onEntityOffline(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onEntityOffline, _handle, entityID);
	}

	/* Connection Management Protocol sniffed messages (ACMP) (not triggered for our own commands even though ACMP messages are broadcasted, the command's 'result' method will be called in that case) */
	virtual void onControllerConnectResponseSniffed(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status) noexcept override
	{
		auto ts = la::avdecc::bindings::fromCppToC::make_stream_identification(talkerStream);
		auto ls = la::avdecc::bindings::fromCppToC::make_stream_identification(listenerStream);
		la::avdecc::utils::invokeProtectedHandler(_delegate->onControllerConnectResponseSniffed, _handle, &ts, &ls, connectionCount, static_cast<avdecc_entity_connection_flags_t>(flags.value()), static_cast<avdecc_local_entity_control_status_t>(status));
	}
	virtual void onControllerDisconnectResponseSniffed(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status) noexcept override
	{
		auto ts = la::avdecc::bindings::fromCppToC::make_stream_identification(talkerStream);
		auto ls = la::avdecc::bindings::fromCppToC::make_stream_identification(listenerStream);
		la::avdecc::utils::invokeProtectedHandler(_delegate->onControllerDisconnectResponseSniffed, _handle, &ts, &ls, connectionCount, static_cast<avdecc_entity_connection_flags_t>(flags.value()), static_cast<avdecc_local_entity_control_status_t>(status));
	}
	virtual void onListenerConnectResponseSniffed(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status) noexcept override
	{
		auto ts = la::avdecc::bindings::fromCppToC::make_stream_identification(talkerStream);
		auto ls = la::avdecc::bindings::fromCppToC::make_stream_identification(listenerStream);
		la::avdecc::utils::invokeProtectedHandler(_delegate->onListenerConnectResponseSniffed, _handle, &ts, &ls, connectionCount, static_cast<avdecc_entity_connection_flags_t>(flags.value()), static_cast<avdecc_local_entity_control_status_t>(status));
	}
	virtual void onListenerDisconnectResponseSniffed(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status) noexcept override
	{
		auto ts = la::avdecc::bindings::fromCppToC::make_stream_identification(talkerStream);
		auto ls = la::avdecc::bindings::fromCppToC::make_stream_identification(listenerStream);
		la::avdecc::utils::invokeProtectedHandler(_delegate->onListenerDisconnectResponseSniffed, _handle, &ts, &ls, connectionCount, static_cast<avdecc_entity_connection_flags_t>(flags.value()), static_cast<avdecc_local_entity_control_status_t>(status));
	}
	virtual void onGetTalkerStreamStateResponseSniffed(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status) noexcept override
	{
		auto ts = la::avdecc::bindings::fromCppToC::make_stream_identification(talkerStream);
		auto ls = la::avdecc::bindings::fromCppToC::make_stream_identification(listenerStream);
		la::avdecc::utils::invokeProtectedHandler(_delegate->onGetTalkerStreamStateResponseSniffed, _handle, &ts, &ls, connectionCount, static_cast<avdecc_entity_connection_flags_t>(flags.value()), static_cast<avdecc_local_entity_control_status_t>(status));
	}
	virtual void onGetListenerStreamStateResponseSniffed(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status) noexcept override
	{
		auto ts = la::avdecc::bindings::fromCppToC::make_stream_identification(talkerStream);
		auto ls = la::avdecc::bindings::fromCppToC::make_stream_identification(listenerStream);
		la::avdecc::utils::invokeProtectedHandler(_delegate->onGetListenerStreamStateResponseSniffed, _handle, &ts, &ls, connectionCount, static_cast<avdecc_entity_connection_flags_t>(flags.value()), static_cast<avdecc_local_entity_control_status_t>(status));
	}

	/* Unsolicited notifications (not triggered for our own commands, the command's 'result' method will be called in that case). Only successfull commands can cause an unsolicited notification. */
	virtual void onDeregisteredFromUnsolicitedNotifications(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onDeregisteredFromUnsolicitedNotifications, _handle, entityID);
	}
	virtual void onEntityAcquired(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::UniqueIdentifier const owningEntity, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onEntityAcquired, _handle, entityID, owningEntity, static_cast<avdecc_entity_model_descriptor_type_t>(descriptorType), descriptorIndex);
	}
	virtual void onEntityReleased(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::UniqueIdentifier const owningEntity, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onEntityReleased, _handle, entityID, owningEntity, static_cast<avdecc_entity_model_descriptor_type_t>(descriptorType), descriptorIndex);
	}
	virtual void onEntityLocked(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::UniqueIdentifier const lockingEntity, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onEntityLocked, _handle, entityID, lockingEntity, static_cast<avdecc_entity_model_descriptor_type_t>(descriptorType), descriptorIndex);
	}
	virtual void onEntityUnlocked(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::UniqueIdentifier const lockingEntity, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onEntityUnlocked, _handle, entityID, lockingEntity, static_cast<avdecc_entity_model_descriptor_type_t>(descriptorType), descriptorIndex);
	}
	virtual void onConfigurationChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onConfigurationChanged, _handle, entityID, configurationIndex);
	}
	virtual void onStreamInputFormatChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamFormat const streamFormat) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onStreamInputFormatChanged, _handle, entityID, streamIndex, streamFormat);
	}
	virtual void onStreamOutputFormatChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamFormat const streamFormat) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onStreamOutputFormatChanged, _handle, entityID, streamIndex, streamFormat);
	}
	virtual void onStreamPortInputAudioMappingsChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::MapIndex const numberOfMaps, la::avdecc::entity::model::MapIndex const mapIndex, la::avdecc::entity::model::AudioMappings const& mappings) noexcept override
	{
		auto m = la::avdecc::bindings::fromCppToC::make_audio_mappings(mappings);
		auto const mp = la::avdecc::bindings::fromCppToC::make_audio_mappings_pointer(m);
		la::avdecc::utils::invokeProtectedHandler(_delegate->onStreamPortInputAudioMappingsChanged, _handle, entityID, streamPortIndex, numberOfMaps, mapIndex, mp.data());
	}
	virtual void onStreamPortOutputAudioMappingsChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::MapIndex const numberOfMaps, la::avdecc::entity::model::MapIndex const mapIndex, la::avdecc::entity::model::AudioMappings const& mappings) noexcept override
	{
		auto m = la::avdecc::bindings::fromCppToC::make_audio_mappings(mappings);
		auto const mp = la::avdecc::bindings::fromCppToC::make_audio_mappings_pointer(m);
		la::avdecc::utils::invokeProtectedHandler(_delegate->onStreamPortOutputAudioMappingsChanged, _handle, entityID, streamPortIndex, numberOfMaps, mapIndex, mp.data());
	}
	virtual void onStreamInputInfoChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamInfo const& info, bool const fromGetStreamInfoResponse) noexcept override
	{
		auto const i = la::avdecc::bindings::fromCppToC::make_stream_info(info);
		la::avdecc::utils::invokeProtectedHandler(_delegate->onStreamInputInfoChanged, _handle, entityID, streamIndex, &i, fromGetStreamInfoResponse);
	}
	virtual void onStreamOutputInfoChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamInfo const& info, bool const fromGetStreamInfoResponse) noexcept override
	{
		auto const i = la::avdecc::bindings::fromCppToC::make_stream_info(info);
		la::avdecc::utils::invokeProtectedHandler(_delegate->onStreamOutputInfoChanged, _handle, entityID, streamIndex, &i, fromGetStreamInfoResponse);
	}
	virtual void onEntityNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::AvdeccFixedString const& entityName) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onEntityNameChanged, _handle, entityID, entityName.data());
	}
	virtual void onEntityGroupNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::AvdeccFixedString const& entityGroupName) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onEntityGroupNameChanged, _handle, entityID, entityGroupName.data());
	}
	virtual void onConfigurationNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AvdeccFixedString const& configurationName) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onConfigurationNameChanged, _handle, entityID, configurationIndex, configurationName.data());
	}
	virtual void onAudioUnitNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AudioUnitIndex const audioUnitIndex, la::avdecc::entity::model::AvdeccFixedString const& audioUnitName) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onAudioUnitNameChanged, _handle, entityID, configurationIndex, audioUnitIndex, audioUnitName.data());
	}
	virtual void onStreamInputNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::AvdeccFixedString const& streamName) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onStreamInputNameChanged, _handle, entityID, configurationIndex, streamIndex, streamName.data());
	}
	virtual void onStreamOutputNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::AvdeccFixedString const& streamName) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onStreamOutputNameChanged, _handle, entityID, configurationIndex, streamIndex, streamName.data());
	}
	virtual void onAvbInterfaceNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::model::AvdeccFixedString const& avbInterfaceName) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onAvbInterfaceNameChanged, _handle, entityID, configurationIndex, avbInterfaceIndex, avbInterfaceName.data());
	}
	virtual void onClockSourceNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockSourceIndex const clockSourceIndex, la::avdecc::entity::model::AvdeccFixedString const& clockSourceName) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onClockSourceNameChanged, _handle, entityID, configurationIndex, clockSourceIndex, clockSourceName.data());
	}
	virtual void onMemoryObjectNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::MemoryObjectIndex const memoryObjectIndex, la::avdecc::entity::model::AvdeccFixedString const& memoryObjectName) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onMemoryObjectNameChanged, _handle, entityID, configurationIndex, memoryObjectIndex, memoryObjectName.data());
	}
	virtual void onAudioClusterNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClusterIndex const audioClusterIndex, la::avdecc::entity::model::AvdeccFixedString const& audioClusterName) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onAudioClusterNameChanged, _handle, entityID, configurationIndex, audioClusterIndex, audioClusterName.data());
	}
	virtual void onClockDomainNameChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::entity::model::AvdeccFixedString const& clockDomainName) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onClockDomainNameChanged, _handle, entityID, configurationIndex, clockDomainIndex, clockDomainName.data());
	}
	virtual void onAudioUnitSamplingRateChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::AudioUnitIndex const audioUnitIndex, la::avdecc::entity::model::SamplingRate const samplingRate) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onAudioUnitSamplingRateChanged, _handle, entityID, audioUnitIndex, samplingRate);
	}
	virtual void onVideoClusterSamplingRateChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::ClusterIndex const videoClusterIndex, la::avdecc::entity::model::SamplingRate const samplingRate) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onVideoClusterSamplingRateChanged, _handle, entityID, videoClusterIndex, samplingRate);
	}
	virtual void onSensorClusterSamplingRateChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::ClusterIndex const sensorClusterIndex, la::avdecc::entity::model::SamplingRate const samplingRate) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onSensorClusterSamplingRateChanged, _handle, entityID, sensorClusterIndex, samplingRate);
	}
	virtual void onClockSourceChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::entity::model::ClockSourceIndex const clockSourceIndex) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onAudioUnitSamplingRateChanged, _handle, entityID, clockDomainIndex, clockSourceIndex);
	}
	virtual void onStreamInputStarted(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::StreamIndex const streamIndex) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onStreamInputStarted, _handle, entityID, streamIndex);
	}
	virtual void onStreamOutputStarted(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::StreamIndex const streamIndex) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onStreamOutputStarted, _handle, entityID, streamIndex);
	}
	virtual void onStreamInputStopped(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::StreamIndex const streamIndex) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onStreamInputStopped, _handle, entityID, streamIndex);
	}
	virtual void onStreamOutputStopped(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::StreamIndex const streamIndex) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onStreamOutputStopped, _handle, entityID, streamIndex);
	}
	virtual void onAvbInfoChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::model::AvbInfo const& info) noexcept override
	{
		auto i = la::avdecc::bindings::fromCppToC::make_avb_info(info);
		auto m = la::avdecc::bindings::fromCppToC::make_msrp_mappings(info.mappings);
		auto mp = la::avdecc::bindings::fromCppToC::make_msrp_mappings_pointer(m);
		i.mappings = mp.data();
		la::avdecc::utils::invokeProtectedHandler(_delegate->onAvbInfoChanged, _handle, entityID, avbInterfaceIndex, &i);
	}
	virtual void onAsPathChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::model::AsPath const& asPath) noexcept override
	{
		auto path = la::avdecc::bindings::fromCppToC::make_as_path(asPath);
		auto p = la::avdecc::bindings::fromCppToC::make_path_sequence(asPath.sequence);
		auto pp = la::avdecc::bindings::fromCppToC::make_path_sequence_pointer(p);
		path.sequence = pp.data();
		la::avdecc::utils::invokeProtectedHandler(_delegate->onAsPathChanged, _handle, entityID, avbInterfaceIndex, &path);
	}
	virtual void onEntityCountersChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::EntityCounterValidFlags const validCounters, la::avdecc::entity::model::DescriptorCounters const& counters) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onEntityCountersChanged, _handle, entityID, validCounters.value(), counters.data());
	}
	virtual void onAvbInterfaceCountersChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::AvbInterfaceCounterValidFlags const validCounters, la::avdecc::entity::model::DescriptorCounters const& counters) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onAvbInterfaceCountersChanged, _handle, entityID, avbInterfaceIndex, validCounters.value(), counters.data());
	}
	virtual void onClockDomainCountersChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::entity::ClockDomainCounterValidFlags const validCounters, la::avdecc::entity::model::DescriptorCounters const& counters) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onClockDomainCountersChanged, _handle, entityID, clockDomainIndex, validCounters.value(), counters.data());
	}
	virtual void onStreamInputCountersChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::StreamInputCounterValidFlags const validCounters, la::avdecc::entity::model::DescriptorCounters const& counters) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onStreamInputCountersChanged, _handle, entityID, streamIndex, validCounters.value(), counters.data());
	}
	virtual void onStreamOutputCountersChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::StreamOutputCounterValidFlags const validCounters, la::avdecc::entity::model::DescriptorCounters const& counters) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onStreamOutputCountersChanged, _handle, entityID, streamIndex, validCounters.value(), counters.data());
	}
	virtual void onStreamPortInputAudioMappingsAdded(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::AudioMappings const& mappings) noexcept override
	{
		auto m = la::avdecc::bindings::fromCppToC::make_audio_mappings(mappings);
		auto const mp = la::avdecc::bindings::fromCppToC::make_audio_mappings_pointer(m);
		la::avdecc::utils::invokeProtectedHandler(_delegate->onStreamPortInputAudioMappingsAdded, _handle, entityID, streamPortIndex, mp.data());
	}
	virtual void onStreamPortOutputAudioMappingsAdded(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::AudioMappings const& mappings) noexcept override
	{
		auto m = la::avdecc::bindings::fromCppToC::make_audio_mappings(mappings);
		auto const mp = la::avdecc::bindings::fromCppToC::make_audio_mappings_pointer(m);
		la::avdecc::utils::invokeProtectedHandler(_delegate->onStreamPortOutputAudioMappingsAdded, _handle, entityID, streamPortIndex, mp.data());
	}
	virtual void onStreamPortInputAudioMappingsRemoved(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::AudioMappings const& mappings) noexcept override
	{
		auto m = la::avdecc::bindings::fromCppToC::make_audio_mappings(mappings);
		auto const mp = la::avdecc::bindings::fromCppToC::make_audio_mappings_pointer(m);
		la::avdecc::utils::invokeProtectedHandler(_delegate->onStreamPortInputAudioMappingsRemoved, _handle, entityID, streamPortIndex, mp.data());
	}
	virtual void onStreamPortOutputAudioMappingsRemoved(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::AudioMappings const& mappings) noexcept override
	{
		auto m = la::avdecc::bindings::fromCppToC::make_audio_mappings(mappings);
		auto const mp = la::avdecc::bindings::fromCppToC::make_audio_mappings_pointer(m);
		la::avdecc::utils::invokeProtectedHandler(_delegate->onStreamPortOutputAudioMappingsRemoved, _handle, entityID, streamPortIndex, mp.data());
	}
	virtual void onMemoryObjectLengthChanged(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onMemoryObjectLengthChanged, _handle, entityID, configurationIndex, memoryObjectIndex, length);
	}
	virtual void onOperationStatus(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex, la::avdecc::entity::model::OperationID const operationID, std::uint16_t const percentComplete) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onOperationStatus, _handle, entityID, static_cast<avdecc_entity_model_descriptor_type_t>(descriptorType), descriptorIndex, operationID, percentComplete);
	}

	/* Identification notifications */
	virtual void onEntityIdentifyNotification(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onEntityIdentifyNotification, _handle, entityID);
	}

	/* **** Statistics **** */
	virtual void onAecpRetry(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const& entityID) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onAecpRetry, _handle, entityID);
	}
	virtual void onAecpTimeout(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const& entityID) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onAecpTimeout, _handle, entityID);
	}
	virtual void onAecpUnexpectedResponse(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const& entityID) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onAecpUnexpectedResponse, _handle, entityID);
	}
	virtual void onAecpResponseTime(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const& entityID, std::chrono::milliseconds const& responseTime) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onAecpResponseTime, _handle, entityID, responseTime.count());
	}
	virtual void onAemAecpUnsolicitedReceived(la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const& entityID) noexcept override
	{
		la::avdecc::utils::invokeProtectedHandler(_delegate->onAemAecpUnsolicitedReceived, _handle, entityID);
	}

	// Private members
	avdecc_local_entity_controller_delegate_p _delegate{ nullptr };
	LA_AVDECC_LOCAL_ENTITY_HANDLE _handle{ LA_AVDECC_INVALID_HANDLE };
};

/* ************************************************************************** */
/* LocalEntity APIs                                                           */
/* ************************************************************************** */
static la::avdecc::bindings::HandleManager<la::avdecc::entity::AggregateEntity::UniquePointer> s_AggregateEntityManager{};
static la::avdecc::bindings::HandleManager<Delegate*> s_ControllerDelegateManager{};

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_create(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, avdecc_entity_cp const entity, avdecc_local_entity_controller_delegate_p const delegate, LA_AVDECC_LOCAL_ENTITY_HANDLE* const createdLocalEntityHandle)
{
	try
	{
		auto const commonInfo = la::avdecc::bindings::fromCToCpp::make_entity_common_information(entity->common_information);
		auto interfacesInfo = la::avdecc::entity::Entity::InterfacesInformation{ { entity->interfaces_information.interface_index, la::avdecc::bindings::fromCToCpp::make_entity_interface_information(entity->interfaces_information) } };

		if (entity->interfaces_information.next)
		{
			for (auto* interfaceInfo = entity->interfaces_information.next; interfaceInfo == nullptr; interfaceInfo = interfaceInfo->next)
			{
				interfacesInfo.insert({ interfaceInfo->interface_index, la::avdecc::bindings::fromCToCpp::make_entity_interface_information(*interfaceInfo) });
			}
		}
		auto& protocolInterface = getProtocolInterface(handle);
		*createdLocalEntityHandle = s_AggregateEntityManager.createObject(&protocolInterface, commonInfo, interfacesInfo, nullptr);

		// Set delegate
		if (delegate)
		{
			auto const h = *createdLocalEntityHandle;
			auto& obj = s_AggregateEntityManager.getObject(h);

			obj.setControllerDelegate(&s_ControllerDelegateManager.getObject(s_ControllerDelegateManager.createObject(delegate, h)));
		}
	}
	catch (la::avdecc::Exception const&) // Because la::avdecc::entity::AggregateEntity::create might throw if entityID is already locally registered
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_duplicate_local_entity_id);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_destroy(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle)
{
	try
	{
		// Destroy object, which will sync until async operations are complete
		s_AggregateEntityManager.destroyObject(handle);

		// Destroy delegate
		{
			// Search the delegate matching our LocalEntity handle
			for (auto const& delegateKV : s_ControllerDelegateManager.getObjects())
			{
				auto const& delegate = delegateKV.second;
				if (delegate->getHandle() == handle)
				{
					s_ControllerDelegateManager.destroyObject(delegateKV.first);
					break;
				}
			}
		}
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_enableEntityAdvertising(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, unsigned int const availableDuration)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.enableEntityAdvertising(availableDuration);
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_disableEntityAdvertising(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.disableEntityAdvertising();
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_discoverRemoteEntities(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		if (!obj.discoverRemoteEntities())
		{
			return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_parameters);
		}
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_discoverRemoteEntity(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		if (!obj.discoverRemoteEntity(la::avdecc::UniqueIdentifier{ entityID }))
		{
			return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_parameters);
		}
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setAutomaticDiscoveryDelay(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, unsigned int const millisecondsDelay)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.setAutomaticDiscoveryDelay(std::chrono::milliseconds{ millisecondsDelay });
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

/* Enumeration and Control Protocol (AECP) AEM */

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_acquireEntity(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_bool_t const isPersistent, avdecc_entity_model_descriptor_type_t const descriptorType, avdecc_entity_model_descriptor_index_t const descriptorIndex, avdecc_local_entity_acquire_entity_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.acquireEntity(la::avdecc::UniqueIdentifier{ entityID }, isPersistent, la::avdecc::entity::model::DescriptorType{ descriptorType }, descriptorIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const owningEntity, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), owningEntity, static_cast<avdecc_entity_model_descriptor_type_t>(descriptorType), descriptorIndex);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_releaseEntity(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const descriptorType, avdecc_entity_model_descriptor_index_t const descriptorIndex, avdecc_local_entity_release_entity_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.releaseEntity(la::avdecc::UniqueIdentifier{ entityID }, la::avdecc::entity::model::DescriptorType{ descriptorType }, descriptorIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const owningEntity, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), owningEntity, static_cast<avdecc_entity_model_descriptor_type_t>(descriptorType), descriptorIndex);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_lockEntity(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const descriptorType, avdecc_entity_model_descriptor_index_t const descriptorIndex, avdecc_local_entity_lock_entity_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.lockEntity(la::avdecc::UniqueIdentifier{ entityID }, la::avdecc::entity::model::DescriptorType{ descriptorType }, descriptorIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const lockingEntity, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), lockingEntity, static_cast<avdecc_entity_model_descriptor_type_t>(descriptorType), descriptorIndex);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_unlockEntity(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const descriptorType, avdecc_entity_model_descriptor_index_t const descriptorIndex, avdecc_local_entity_unlock_entity_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.unlockEntity(la::avdecc::UniqueIdentifier{ entityID }, la::avdecc::entity::model::DescriptorType{ descriptorType }, descriptorIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::UniqueIdentifier const lockingEntity, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), lockingEntity, static_cast<avdecc_entity_model_descriptor_type_t>(descriptorType), descriptorIndex);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_queryEntityAvailable(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_local_entity_query_entity_available_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.queryEntityAvailable(la::avdecc::UniqueIdentifier{ entityID },
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status));
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_queryControllerAvailable(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_local_entity_query_controller_available_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.queryControllerAvailable(la::avdecc::UniqueIdentifier{ entityID },
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status));
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_registerUnsolicitedNotifications(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_local_entity_register_unsolicited_notifications_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.registerUnsolicitedNotifications(la::avdecc::UniqueIdentifier{ entityID },
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status));
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_unregisterUnsolicitedNotifications(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_local_entity_unregister_unsolicited_notifications_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.unregisterUnsolicitedNotifications(la::avdecc::UniqueIdentifier{ entityID },
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status));
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readEntityDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_local_entity_read_entity_descriptor_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.readEntityDescriptor(la::avdecc::UniqueIdentifier{ entityID },
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::EntityDescriptor const& descriptor)
			{
				auto const d = la::avdecc::bindings::fromCppToC::make_entity_descriptor(descriptor);
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), &d);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readConfigurationDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_local_entity_read_configuration_descriptor_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.readConfigurationDescriptor(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ConfigurationDescriptor const& descriptor)
			{
				auto d = la::avdecc::bindings::fromCppToC::make_configuration_descriptor(descriptor);
				auto c = la::avdecc::bindings::fromCppToC::make_descriptors_count(descriptor.descriptorCounts);
				auto cp = la::avdecc::bindings::fromCppToC::make_descriptors_count_pointer(c);
				d.counts = cp.data();

				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, &d);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readAudioUnitDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const audioUnitIndex, avdecc_local_entity_read_audio_unit_descriptor_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.readAudioUnitDescriptor(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex, audioUnitIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AudioUnitIndex const audioUnitIndex, la::avdecc::entity::model::AudioUnitDescriptor const& descriptor)
			{
				auto d = la::avdecc::bindings::fromCppToC::make_audio_unit_descriptor(descriptor);
				auto r = la::avdecc::bindings::fromCppToC::make_sampling_rates(descriptor.samplingRates);
				auto rp = la::avdecc::bindings::fromCppToC::make_sampling_rates_pointer(r);
				d.sampling_rates = rp.data();

				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, audioUnitIndex, &d);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readStreamInputDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_local_entity_read_stream_input_descriptor_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.readStreamInputDescriptor(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex, streamIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamDescriptor const& descriptor)
			{
				auto d = la::avdecc::bindings::fromCppToC::make_stream_descriptor(descriptor);
				auto f = la::avdecc::bindings::fromCppToC::make_stream_formats(descriptor.formats);
				auto fp = la::avdecc::bindings::fromCppToC::make_stream_formats_pointer(f);
				d.formats = fp.data();
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
				auto r = la::avdecc::bindings::fromCppToC::make_redundant_stream_indexes(descriptor.redundantStreams);
				auto rp = la::avdecc::bindings::fromCppToC::make_redundant_stream_indexes_pointer(r);
				d.redundant_streams = rp.data();
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, streamIndex, &d);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readStreamOutputDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_local_entity_read_stream_output_descriptor_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.readStreamOutputDescriptor(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex, streamIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamDescriptor const& descriptor)
			{
				auto d = la::avdecc::bindings::fromCppToC::make_stream_descriptor(descriptor);
				auto f = la::avdecc::bindings::fromCppToC::make_stream_formats(descriptor.formats);
				auto fp = la::avdecc::bindings::fromCppToC::make_stream_formats_pointer(f);
				d.formats = fp.data();
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
				auto r = la::avdecc::bindings::fromCppToC::make_redundant_stream_indexes(descriptor.redundantStreams);
				auto rp = la::avdecc::bindings::fromCppToC::make_redundant_stream_indexes_pointer(r);
				d.redundant_streams = rp.data();
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, streamIndex, &d);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readJackInputDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const jackIndex, avdecc_local_entity_read_jack_input_descriptor_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.readJackInputDescriptor(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex, jackIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::JackIndex const jackIndex, la::avdecc::entity::model::JackDescriptor const& descriptor)
			{
				auto d = la::avdecc::bindings::fromCppToC::make_jack_descriptor(descriptor);
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, jackIndex, &d);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readJackOutputDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const jackIndex, avdecc_local_entity_read_jack_output_descriptor_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.readJackOutputDescriptor(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex, jackIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::JackIndex const jackIndex, la::avdecc::entity::model::JackDescriptor const& descriptor)
			{
				auto d = la::avdecc::bindings::fromCppToC::make_jack_descriptor(descriptor);
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, jackIndex, &d);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readAvbInterfaceDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const avbInterfaceIndex, avdecc_local_entity_read_avb_interface_descriptor_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.readAvbInterfaceDescriptor(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex, avbInterfaceIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::model::AvbInterfaceDescriptor const& descriptor)
			{
				auto d = la::avdecc::bindings::fromCppToC::make_avb_interface_descriptor(descriptor);
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, avbInterfaceIndex, &d);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readClockSourceDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const clockSourceIndex, avdecc_local_entity_read_clock_source_descriptor_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.readClockSourceDescriptor(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex, clockSourceIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockSourceIndex const clockSourceIndex, la::avdecc::entity::model::ClockSourceDescriptor const& descriptor)
			{
				auto d = la::avdecc::bindings::fromCppToC::make_clock_source_descriptor(descriptor);
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, clockSourceIndex, &d);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readMemoryObjectDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const memoryObjectIndex, avdecc_local_entity_read_memory_object_descriptor_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.readMemoryObjectDescriptor(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex, memoryObjectIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::MemoryObjectIndex const memoryObjectIndex, la::avdecc::entity::model::MemoryObjectDescriptor const& descriptor)
			{
				auto d = la::avdecc::bindings::fromCppToC::make_memory_object_descriptor(descriptor);
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, memoryObjectIndex, &d);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readLocaleDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const localeIndex, avdecc_local_entity_read_locale_descriptor_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.readLocaleDescriptor(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex, localeIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::LocaleIndex const localeIndex, la::avdecc::entity::model::LocaleDescriptor const& descriptor)
			{
				auto d = la::avdecc::bindings::fromCppToC::make_locale_descriptor(descriptor);
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, localeIndex, &d);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readStringsDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const stringsIndex, avdecc_local_entity_read_strings_descriptor_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.readStringsDescriptor(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex, stringsIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StringsIndex const stringsIndex, la::avdecc::entity::model::StringsDescriptor const& descriptor)
			{
				auto d = la::avdecc::bindings::fromCppToC::make_strings_descriptor(descriptor);
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, stringsIndex, &d);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readStreamPortInputDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const streamPortIndex, avdecc_local_entity_read_stream_port_input_descriptor_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.readStreamPortInputDescriptor(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex, streamPortIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::StreamPortDescriptor const& descriptor)
			{
				auto d = la::avdecc::bindings::fromCppToC::make_stream_port_descriptor(descriptor);
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, streamPortIndex, &d);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readStreamPortOutputDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const streamPortIndex, avdecc_local_entity_read_stream_port_output_descriptor_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.readStreamPortOutputDescriptor(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex, streamPortIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::StreamPortDescriptor const& descriptor)
			{
				auto d = la::avdecc::bindings::fromCppToC::make_stream_port_descriptor(descriptor);
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, streamPortIndex, &d);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readExternalPortInputDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const externalPortIndex, avdecc_local_entity_read_external_port_input_descriptor_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.readExternalPortInputDescriptor(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex, externalPortIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ExternalPortIndex const externalPortIndex, la::avdecc::entity::model::ExternalPortDescriptor const& descriptor)
			{
				auto d = la::avdecc::bindings::fromCppToC::make_external_port_descriptor(descriptor);
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, externalPortIndex, &d);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readExternalPortOutputDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const externalPortIndex, avdecc_local_entity_read_external_port_output_descriptor_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.readExternalPortOutputDescriptor(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex, externalPortIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ExternalPortIndex const externalPortIndex, la::avdecc::entity::model::ExternalPortDescriptor const& descriptor)
			{
				auto d = la::avdecc::bindings::fromCppToC::make_external_port_descriptor(descriptor);
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, externalPortIndex, &d);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readInternalPortInputDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const internalPortIndex, avdecc_local_entity_read_internal_port_input_descriptor_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.readInternalPortInputDescriptor(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex, internalPortIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::InternalPortIndex const internalPortIndex, la::avdecc::entity::model::InternalPortDescriptor const& descriptor)
			{
				auto d = la::avdecc::bindings::fromCppToC::make_internal_port_descriptor(descriptor);
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, internalPortIndex, &d);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readInternalPortOutputDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const internalPortIndex, avdecc_local_entity_read_internal_port_output_descriptor_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.readInternalPortOutputDescriptor(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex, internalPortIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::InternalPortIndex const internalPortIndex, la::avdecc::entity::model::InternalPortDescriptor const& descriptor)
			{
				auto d = la::avdecc::bindings::fromCppToC::make_internal_port_descriptor(descriptor);
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, internalPortIndex, &d);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readAudioClusterDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const clusterIndex, avdecc_local_entity_read_audio_cluster_descriptor_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.readAudioClusterDescriptor(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex, clusterIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClusterIndex const clusterIndex, la::avdecc::entity::model::AudioClusterDescriptor const& descriptor)
			{
				auto d = la::avdecc::bindings::fromCppToC::make_audio_cluster_descriptor(descriptor);
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, clusterIndex, &d);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readAudioMapDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const mapIndex, avdecc_local_entity_read_audio_map_descriptor_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.readAudioMapDescriptor(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex, mapIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::MapIndex const mapIndex, la::avdecc::entity::model::AudioMapDescriptor const& descriptor)
			{
				auto d = la::avdecc::bindings::fromCppToC::make_audio_map_descriptor(descriptor);
				auto m = la::avdecc::bindings::fromCppToC::make_audio_mappings(descriptor.mappings);
				auto mp = la::avdecc::bindings::fromCppToC::make_audio_mappings_pointer(m);
				d.mappings = mp.data();
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, mapIndex, &d);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readClockDomainDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const clockDomainIndex, avdecc_local_entity_read_clock_domain_descriptor_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.readClockDomainDescriptor(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex, clockDomainIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::entity::model::ClockDomainDescriptor const& descriptor)
			{
				auto d = la::avdecc::bindings::fromCppToC::make_clock_domain_descriptor(descriptor);
				auto s = la::avdecc::bindings::fromCppToC::make_clock_sources(descriptor.clockSources);
				auto sp = la::avdecc::bindings::fromCppToC::make_clock_sources_pointer(s);
				d.clock_sources = sp.data();
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, clockDomainIndex, &d);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setConfiguration(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_local_entity_set_configuration_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.setConfiguration(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getConfiguration(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_local_entity_get_configuration_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.getConfiguration(la::avdecc::UniqueIdentifier{ entityID },
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setStreamInputFormat(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_entity_model_stream_format_t const streamFormat, avdecc_local_entity_set_stream_input_format_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.setStreamInputFormat(la::avdecc::UniqueIdentifier{ entityID }, streamIndex, la::avdecc::entity::model::StreamFormat{ streamFormat },
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamFormat const streamFormat)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), streamIndex, streamFormat);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getStreamInputFormat(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_local_entity_get_stream_input_format_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.getStreamInputFormat(la::avdecc::UniqueIdentifier{ entityID }, streamIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamFormat const streamFormat)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), streamIndex, streamFormat);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setStreamOutputFormat(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_entity_model_stream_format_t const streamFormat, avdecc_local_entity_set_stream_output_format_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.setStreamOutputFormat(la::avdecc::UniqueIdentifier{ entityID }, streamIndex, la::avdecc::entity::model::StreamFormat{ streamFormat },
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamFormat const streamFormat)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), streamIndex, streamFormat);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getStreamOutputFormat(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_local_entity_get_stream_output_format_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.getStreamOutputFormat(la::avdecc::UniqueIdentifier{ entityID }, streamIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamFormat const streamFormat)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), streamIndex, streamFormat);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getStreamPortInputAudioMap(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamPortIndex, avdecc_entity_model_descriptor_index_t const mapIndex, avdecc_local_entity_get_stream_port_input_audio_map_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.getStreamPortInputAudioMap(la::avdecc::UniqueIdentifier{ entityID }, streamPortIndex, mapIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::MapIndex const numberOfMaps, la::avdecc::entity::model::MapIndex const mapIndex, la::avdecc::entity::model::AudioMappings const& mappings)
			{
				auto m = la::avdecc::bindings::fromCppToC::make_audio_mappings(mappings);
				auto const mp = la::avdecc::bindings::fromCppToC::make_audio_mappings_pointer(m);
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), streamPortIndex, numberOfMaps, mapIndex, mp.data());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getStreamPortOutputAudioMap(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamPortIndex, avdecc_entity_model_descriptor_index_t const mapIndex, avdecc_local_entity_get_stream_port_output_audio_map_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.getStreamPortOutputAudioMap(la::avdecc::UniqueIdentifier{ entityID }, streamPortIndex, mapIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::MapIndex const numberOfMaps, la::avdecc::entity::model::MapIndex const mapIndex, la::avdecc::entity::model::AudioMappings const& mappings)
			{
				auto m = la::avdecc::bindings::fromCppToC::make_audio_mappings(mappings);
				auto const mp = la::avdecc::bindings::fromCppToC::make_audio_mappings_pointer(m);
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), streamPortIndex, numberOfMaps, mapIndex, mp.data());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_addStreamPortInputAudioMappings(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamPortIndex, avdecc_entity_model_audio_mapping_cp const* const mappings, avdecc_local_entity_add_stream_port_input_audio_mappings_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		auto const m = la::avdecc::bindings::fromCToCpp::make_audio_mappings(mappings);
		obj.addStreamPortInputAudioMappings(la::avdecc::UniqueIdentifier{ entityID }, streamPortIndex, m,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::AudioMappings const& mappings)
			{
				auto m = la::avdecc::bindings::fromCppToC::make_audio_mappings(mappings);
				auto const mp = la::avdecc::bindings::fromCppToC::make_audio_mappings_pointer(m);
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), streamPortIndex, mp.data());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_addStreamPortOutputAudioMappings(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamPortIndex, avdecc_entity_model_audio_mapping_cp const* const mappings, avdecc_local_entity_add_stream_port_output_audio_mappings_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		auto const m = la::avdecc::bindings::fromCToCpp::make_audio_mappings(mappings);
		obj.addStreamPortOutputAudioMappings(la::avdecc::UniqueIdentifier{ entityID }, streamPortIndex, m,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::AudioMappings const& mappings)
			{
				auto m = la::avdecc::bindings::fromCppToC::make_audio_mappings(mappings);
				auto const mp = la::avdecc::bindings::fromCppToC::make_audio_mappings_pointer(m);
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), streamPortIndex, mp.data());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_removeStreamPortInputAudioMappings(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamPortIndex, avdecc_entity_model_audio_mapping_cp const* const mappings, avdecc_local_entity_remove_stream_port_input_audio_mappings_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		auto const m = la::avdecc::bindings::fromCToCpp::make_audio_mappings(mappings);
		obj.removeStreamPortInputAudioMappings(la::avdecc::UniqueIdentifier{ entityID }, streamPortIndex, m,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::AudioMappings const& mappings)
			{
				auto m = la::avdecc::bindings::fromCppToC::make_audio_mappings(mappings);
				auto const mp = la::avdecc::bindings::fromCppToC::make_audio_mappings_pointer(m);
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), streamPortIndex, mp.data());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_removeStreamPortOutputAudioMappings(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamPortIndex, avdecc_entity_model_audio_mapping_cp const* const mappings, avdecc_local_entity_remove_stream_port_output_audio_mappings_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		auto const m = la::avdecc::bindings::fromCToCpp::make_audio_mappings(mappings);
		obj.removeStreamPortOutputAudioMappings(la::avdecc::UniqueIdentifier{ entityID }, streamPortIndex, m,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamPortIndex const streamPortIndex, la::avdecc::entity::model::AudioMappings const& mappings)
			{
				auto m = la::avdecc::bindings::fromCppToC::make_audio_mappings(mappings);
				auto const mp = la::avdecc::bindings::fromCppToC::make_audio_mappings_pointer(m);
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), streamPortIndex, mp.data());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setStreamInputInfo(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_entity_model_stream_info_cp const info, avdecc_local_entity_set_stream_input_info_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		auto const i = la::avdecc::bindings::fromCToCpp::make_stream_info(info);
		obj.setStreamInputInfo(la::avdecc::UniqueIdentifier{ entityID }, streamIndex, i,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamInfo const& info)
			{
				auto const i = la::avdecc::bindings::fromCppToC::make_stream_info(info);
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), streamIndex, &i);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setStreamOutputInfo(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_entity_model_stream_info_cp const info, avdecc_local_entity_set_stream_output_info_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		auto const i = la::avdecc::bindings::fromCToCpp::make_stream_info(info);
		obj.setStreamOutputInfo(la::avdecc::UniqueIdentifier{ entityID }, streamIndex, i,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamInfo const& info)
			{
				auto const i = la::avdecc::bindings::fromCppToC::make_stream_info(info);
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), streamIndex, &i);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getStreamInputInfo(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_local_entity_get_stream_input_info_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.getStreamInputInfo(la::avdecc::UniqueIdentifier{ entityID }, streamIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamInfo const& info)
			{
				auto const i = la::avdecc::bindings::fromCppToC::make_stream_info(info);
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), streamIndex, &i);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getStreamOutputInfo(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_local_entity_get_stream_output_info_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.getStreamOutputInfo(la::avdecc::UniqueIdentifier{ entityID }, streamIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::StreamInfo const& info)
			{
				auto const i = la::avdecc::bindings::fromCppToC::make_stream_info(info);
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), streamIndex, &i);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setEntityName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_fixed_string_t const entityName, avdecc_local_entity_set_entity_name_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.setEntityName(la::avdecc::UniqueIdentifier{ entityID }, la::avdecc::entity::model::AvdeccFixedString{ entityName },
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::AvdeccFixedString const& entityName)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), entityName.data());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getEntityName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_local_entity_get_entity_name_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.getEntityName(la::avdecc::UniqueIdentifier{ entityID },
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::AvdeccFixedString const& entityName)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), entityName.data());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setEntityGroupName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_fixed_string_t const entityGroupName, avdecc_local_entity_set_entity_group_name_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.setEntityGroupName(la::avdecc::UniqueIdentifier{ entityID }, la::avdecc::entity::model::AvdeccFixedString{ entityGroupName },
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::AvdeccFixedString const& entityGroupName)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), entityGroupName.data());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getEntityGroupName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_local_entity_get_entity_group_name_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.getEntityGroupName(la::avdecc::UniqueIdentifier{ entityID },
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::AvdeccFixedString const& entityGroupName)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), entityGroupName.data());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setConfigurationName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_fixed_string_t const configurationName, avdecc_local_entity_set_configuration_name_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.setConfigurationName(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex, la::avdecc::entity::model::AvdeccFixedString{ configurationName },
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AvdeccFixedString const& configurationName)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, configurationName.data());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getConfigurationName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_local_entity_get_configuration_name_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.getConfigurationName(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AvdeccFixedString const& configurationName)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, configurationName.data());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setAudioUnitName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const audioUnitIndex, avdecc_fixed_string_t const audioUnitName, avdecc_local_entity_set_audio_unit_name_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.setAudioUnitName(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex, audioUnitIndex, la::avdecc::entity::model::AvdeccFixedString{ audioUnitName },
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AudioUnitIndex const audioUnitIndex, la::avdecc::entity::model::AvdeccFixedString const& audioUnitName)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, audioUnitIndex, audioUnitName.data());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getAudioUnitName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const audioUnitIndex, avdecc_local_entity_get_audio_unit_name_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.getAudioUnitName(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex, audioUnitIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AudioUnitIndex const audioUnitIndex, la::avdecc::entity::model::AvdeccFixedString const& audioUnitName)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, audioUnitIndex, audioUnitName.data());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setStreamInputName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_fixed_string_t const streamInputName, avdecc_local_entity_set_stream_input_name_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.setStreamInputName(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex, streamIndex, la::avdecc::entity::model::AvdeccFixedString{ streamInputName },
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::AvdeccFixedString const& streamInputName)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, streamIndex, streamInputName.data());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getStreamInputName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_local_entity_get_stream_input_name_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.getStreamInputName(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex, streamIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::AvdeccFixedString const& streamInputName)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, streamIndex, streamInputName.data());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setStreamOutputName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_fixed_string_t const streamOutputName, avdecc_local_entity_set_stream_output_name_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.setStreamOutputName(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex, streamIndex, la::avdecc::entity::model::AvdeccFixedString{ streamOutputName },
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::AvdeccFixedString const& streamOutputName)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, streamIndex, streamOutputName.data());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getStreamOutputName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_local_entity_get_stream_output_name_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.getStreamOutputName(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex, streamIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::model::AvdeccFixedString const& streamOutputName)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, streamIndex, streamOutputName.data());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setAvbInterfaceName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const avbInterfaceIndex, avdecc_fixed_string_t const avbInterfaceName, avdecc_local_entity_set_avb_interface_name_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.setAvbInterfaceName(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex, avbInterfaceIndex, la::avdecc::entity::model::AvdeccFixedString{ avbInterfaceName },
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::model::AvdeccFixedString const& avbInterfaceName)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, avbInterfaceIndex, avbInterfaceName.data());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getAvbInterfaceName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const avbInterfaceIndex, avdecc_local_entity_get_avb_interface_name_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.getAvbInterfaceName(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex, avbInterfaceIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::model::AvdeccFixedString const& avbInterfaceName)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, avbInterfaceIndex, avbInterfaceName.data());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setClockSourceName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const clockSourceIndex, avdecc_fixed_string_t const clockSourceName, avdecc_local_entity_set_clock_source_name_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.setClockSourceName(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex, clockSourceIndex, la::avdecc::entity::model::AvdeccFixedString{ clockSourceName },
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockSourceIndex const clockSourceIndex, la::avdecc::entity::model::AvdeccFixedString const& clockSourceName)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, clockSourceIndex, clockSourceName.data());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getClockSourceName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const clockSourceIndex, avdecc_local_entity_get_clock_source_name_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.getClockSourceName(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex, clockSourceIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockSourceIndex const clockSourceIndex, la::avdecc::entity::model::AvdeccFixedString const& clockSourceName)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, clockSourceIndex, clockSourceName.data());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setMemoryObjectName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const memoryObjectIndex, avdecc_fixed_string_t const memoryObjectName, avdecc_local_entity_set_memory_object_name_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.setMemoryObjectName(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex, memoryObjectIndex, la::avdecc::entity::model::AvdeccFixedString{ memoryObjectName },
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::MemoryObjectIndex const memoryObjectIndex, la::avdecc::entity::model::AvdeccFixedString const& memoryObjectName)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, memoryObjectIndex, memoryObjectName.data());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getMemoryObjectName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const memoryObjectIndex, avdecc_local_entity_get_memory_object_name_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.getMemoryObjectName(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex, memoryObjectIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::MemoryObjectIndex const memoryObjectIndex, la::avdecc::entity::model::AvdeccFixedString const& memoryObjectName)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, memoryObjectIndex, memoryObjectName.data());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setAudioClusterName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const audioClusterIndex, avdecc_fixed_string_t const audioClusterName, avdecc_local_entity_set_audio_cluster_name_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.setAudioClusterName(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex, audioClusterIndex, la::avdecc::entity::model::AvdeccFixedString{ audioClusterName },
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClusterIndex const audioClusterIndex, la::avdecc::entity::model::AvdeccFixedString const& audioClusterName)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, audioClusterIndex, audioClusterName.data());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getAudioClusterName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const audioClusterIndex, avdecc_local_entity_get_audio_cluster_name_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.getAudioClusterName(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex, audioClusterIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClusterIndex const audioClusterIndex, la::avdecc::entity::model::AvdeccFixedString const& audioClusterName)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, audioClusterIndex, audioClusterName.data());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setClockDomainName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const clockDomainIndex, avdecc_fixed_string_t const clockDomainName, avdecc_local_entity_set_clock_domain_name_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.setClockDomainName(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex, clockDomainIndex, la::avdecc::entity::model::AvdeccFixedString{ clockDomainName },
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::entity::model::AvdeccFixedString const& clockDomainName)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, clockDomainIndex, clockDomainName.data());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getClockDomainName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const clockDomainIndex, avdecc_local_entity_get_clock_domain_name_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.getClockDomainName(la::avdecc::UniqueIdentifier{ entityID }, configurationIndex, clockDomainIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ConfigurationIndex const configurationIndex, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::entity::model::AvdeccFixedString const& clockDomainName)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), configurationIndex, clockDomainIndex, clockDomainName.data());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setAudioUnitSamplingRate(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const audioUnitIndex, avdecc_entity_model_sampling_rate_t const samplingRate, avdecc_local_entity_set_audio_unit_sampling_rate_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.setAudioUnitSamplingRate(la::avdecc::UniqueIdentifier{ entityID }, audioUnitIndex, la::avdecc::entity::model::SamplingRate{ samplingRate },
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::AudioUnitIndex const audioUnitIndex, la::avdecc::entity::model::SamplingRate const samplingRate)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), audioUnitIndex, samplingRate.getValue());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getAudioUnitSamplingRate(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const audioUnitIndex, avdecc_local_entity_get_audio_unit_sampling_rate_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.getAudioUnitSamplingRate(la::avdecc::UniqueIdentifier{ entityID }, audioUnitIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::AudioUnitIndex const audioUnitIndex, la::avdecc::entity::model::SamplingRate const samplingRate)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), audioUnitIndex, samplingRate.getValue());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setVideoClusterSamplingRate(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const videoClusterIndex, avdecc_entity_model_sampling_rate_t const samplingRate, avdecc_local_entity_set_video_cluster_sampling_rate_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.setVideoClusterSamplingRate(la::avdecc::UniqueIdentifier{ entityID }, videoClusterIndex, la::avdecc::entity::model::SamplingRate{ samplingRate },
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ClusterIndex const videoClusterIndex, la::avdecc::entity::model::SamplingRate const samplingRate)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), videoClusterIndex, samplingRate.getValue());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getVideoClusterSamplingRate(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const videoClusterIndex, avdecc_local_entity_get_video_cluster_sampling_rate_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.getVideoClusterSamplingRate(la::avdecc::UniqueIdentifier{ entityID }, videoClusterIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ClusterIndex const videoClusterIndex, la::avdecc::entity::model::SamplingRate const samplingRate)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), videoClusterIndex, samplingRate.getValue());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setSensorClusterSamplingRate(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const sensorClusterIndex, avdecc_entity_model_sampling_rate_t const samplingRate, avdecc_local_entity_set_sensor_cluster_sampling_rate_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.setSensorClusterSamplingRate(la::avdecc::UniqueIdentifier{ entityID }, sensorClusterIndex, la::avdecc::entity::model::SamplingRate{ samplingRate },
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ClusterIndex const sensorClusterIndex, la::avdecc::entity::model::SamplingRate const samplingRate)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), sensorClusterIndex, samplingRate.getValue());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getSensorClusterSamplingRate(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const sensorClusterIndex, avdecc_local_entity_get_sensor_cluster_sampling_rate_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.getSensorClusterSamplingRate(la::avdecc::UniqueIdentifier{ entityID }, sensorClusterIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ClusterIndex const sensorClusterIndex, la::avdecc::entity::model::SamplingRate const samplingRate)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), sensorClusterIndex, samplingRate.getValue());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setClockSource(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const clockDomainIndex, avdecc_entity_model_descriptor_type_t const clockSourceIndex, avdecc_local_entity_set_clock_source_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.setClockSource(la::avdecc::UniqueIdentifier{ entityID }, clockDomainIndex, clockSourceIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::entity::model::ClockSourceIndex const clockSourceIndex)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), clockDomainIndex, clockSourceIndex);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getClockSource(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const clockDomainIndex, avdecc_local_entity_get_clock_source_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.getClockSource(la::avdecc::UniqueIdentifier{ entityID }, clockDomainIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::entity::model::ClockSourceIndex const clockSourceIndex)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), clockDomainIndex, clockSourceIndex);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_startStreamInput(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_local_entity_start_stream_input_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.startStreamInput(la::avdecc::UniqueIdentifier{ entityID }, streamIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), streamIndex);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_startStreamOutput(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_local_entity_start_stream_output_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.startStreamOutput(la::avdecc::UniqueIdentifier{ entityID }, streamIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), streamIndex);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_stopStreamInput(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_local_entity_stop_stream_input_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.stopStreamInput(la::avdecc::UniqueIdentifier{ entityID }, streamIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), streamIndex);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_stopStreamOutput(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_local_entity_stop_stream_output_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.stopStreamOutput(la::avdecc::UniqueIdentifier{ entityID }, streamIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), streamIndex);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getAvbInfo(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const avbInterfaceIndex, avdecc_local_entity_get_avb_info_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.getAvbInfo(la::avdecc::UniqueIdentifier{ entityID }, avbInterfaceIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::model::AvbInfo const& info)
			{
				auto i = la::avdecc::bindings::fromCppToC::make_avb_info(info);
				auto m = la::avdecc::bindings::fromCppToC::make_msrp_mappings(info.mappings);
				auto mp = la::avdecc::bindings::fromCppToC::make_msrp_mappings_pointer(m);
				i.mappings = mp.data();
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), avbInterfaceIndex, &i);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getAsPath(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const avbInterfaceIndex, avdecc_local_entity_get_as_path_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.getAsPath(la::avdecc::UniqueIdentifier{ entityID }, avbInterfaceIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::model::AsPath const& asPath)
			{
				auto path = la::avdecc::bindings::fromCppToC::make_as_path(asPath);
				auto p = la::avdecc::bindings::fromCppToC::make_path_sequence(asPath.sequence);
				auto pp = la::avdecc::bindings::fromCppToC::make_path_sequence_pointer(p);
				path.sequence = pp.data();
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), avbInterfaceIndex, &path);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getEntityCounters(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_local_entity_get_entity_counters_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.getEntityCounters(la::avdecc::UniqueIdentifier{ entityID },
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::EntityCounterValidFlags const validCounters, la::avdecc::entity::model::DescriptorCounters const& counters)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), validCounters.value(), counters.data());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getAvbInterfaceCounters(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const avbInterfaceIndex, avdecc_local_entity_get_avb_interface_counters_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.getAvbInterfaceCounters(la::avdecc::UniqueIdentifier{ entityID }, avbInterfaceIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::AvbInterfaceIndex const avbInterfaceIndex, la::avdecc::entity::AvbInterfaceCounterValidFlags const validCounters, la::avdecc::entity::model::DescriptorCounters const& counters)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), avbInterfaceIndex, validCounters.value(), counters.data());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getClockDomainCounters(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const clockDomainIndex, avdecc_local_entity_get_clock_domain_counters_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.getClockDomainCounters(la::avdecc::UniqueIdentifier{ entityID }, clockDomainIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::ClockDomainIndex const clockDomainIndex, la::avdecc::entity::ClockDomainCounterValidFlags const validCounters, la::avdecc::entity::model::DescriptorCounters const& counters)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), clockDomainIndex, validCounters.value(), counters.data());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getStreamInputCounters(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_local_entity_get_stream_input_counters_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.getStreamInputCounters(la::avdecc::UniqueIdentifier{ entityID }, streamIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::StreamInputCounterValidFlags const validCounters, la::avdecc::entity::model::DescriptorCounters const& counters)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), streamIndex, validCounters.value(), counters.data());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getStreamOutputCounters(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_local_entity_get_stream_output_counters_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.getStreamOutputCounters(la::avdecc::UniqueIdentifier{ entityID }, streamIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::AemCommandStatus const status, la::avdecc::entity::model::StreamIndex const streamIndex, la::avdecc::entity::StreamOutputCounterValidFlags const validCounters, la::avdecc::entity::model::DescriptorCounters const& counters)
			{
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_aem_command_status_t>(status), streamIndex, validCounters.value(), counters.data());
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

/* Enumeration and Control Protocol (AECP) MVU (Milan Vendor Unique) */

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getMilanInfo(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_local_entity_get_milan_info_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.getMilanInfo(la::avdecc::UniqueIdentifier{ entityID },
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::UniqueIdentifier const entityID, la::avdecc::entity::LocalEntity::MvuCommandStatus const status, la::avdecc::entity::model::MilanInfo const& info)
			{
				auto i = la::avdecc::bindings::fromCppToC::make_milan_info(info);
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, entityID, static_cast<avdecc_local_entity_mvu_command_status_t>(status), &i);
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

/* Connection Management Protocol (ACMP) */

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_connectStream(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_entity_model_stream_identification_cp const talkerStream, avdecc_entity_model_stream_identification_cp const listenerStream, avdecc_local_entity_connect_stream_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.connectStream(la::avdecc::bindings::fromCToCpp::make_stream_identification(talkerStream), la::avdecc::bindings::fromCToCpp::make_stream_identification(listenerStream),
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status)
			{
				auto ts = la::avdecc::bindings::fromCppToC::make_stream_identification(talkerStream);
				auto ls = la::avdecc::bindings::fromCppToC::make_stream_identification(listenerStream);
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, &ts, &ls, connectionCount, static_cast<avdecc_entity_connection_flags_t>(flags.value()), static_cast<avdecc_local_entity_control_status_t>(status));
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_disconnectStream(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_entity_model_stream_identification_cp const talkerStream, avdecc_entity_model_stream_identification_cp const listenerStream, avdecc_local_entity_disconnect_stream_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.disconnectStream(la::avdecc::bindings::fromCToCpp::make_stream_identification(talkerStream), la::avdecc::bindings::fromCToCpp::make_stream_identification(listenerStream),
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status)
			{
				auto ts = la::avdecc::bindings::fromCppToC::make_stream_identification(talkerStream);
				auto ls = la::avdecc::bindings::fromCppToC::make_stream_identification(listenerStream);
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, &ts, &ls, connectionCount, static_cast<avdecc_entity_connection_flags_t>(flags.value()), static_cast<avdecc_local_entity_control_status_t>(status));
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_disconnectTalkerStream(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_entity_model_stream_identification_cp const talkerStream, avdecc_entity_model_stream_identification_cp const listenerStream, avdecc_local_entity_disconnect_talker_stream_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.disconnectTalkerStream(la::avdecc::bindings::fromCToCpp::make_stream_identification(talkerStream), la::avdecc::bindings::fromCToCpp::make_stream_identification(listenerStream),
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status)
			{
				auto ts = la::avdecc::bindings::fromCppToC::make_stream_identification(talkerStream);
				auto ls = la::avdecc::bindings::fromCppToC::make_stream_identification(listenerStream);
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, &ts, &ls, connectionCount, static_cast<avdecc_entity_connection_flags_t>(flags.value()), static_cast<avdecc_local_entity_control_status_t>(status));
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getTalkerStreamState(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_entity_model_stream_identification_cp const talkerStream, avdecc_local_entity_get_talker_stream_state_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.getTalkerStreamState(la::avdecc::bindings::fromCToCpp::make_stream_identification(talkerStream),
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status)
			{
				auto ts = la::avdecc::bindings::fromCppToC::make_stream_identification(talkerStream);
				auto ls = la::avdecc::bindings::fromCppToC::make_stream_identification(listenerStream);
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, &ts, &ls, connectionCount, static_cast<avdecc_entity_connection_flags_t>(flags.value()), static_cast<avdecc_local_entity_control_status_t>(status));
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getListenerStreamState(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_entity_model_stream_identification_cp const listenerStream, avdecc_local_entity_get_listener_stream_state_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.getListenerStreamState(la::avdecc::bindings::fromCToCpp::make_stream_identification(listenerStream),
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status)
			{
				auto ts = la::avdecc::bindings::fromCppToC::make_stream_identification(talkerStream);
				auto ls = la::avdecc::bindings::fromCppToC::make_stream_identification(listenerStream);
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, &ts, &ls, connectionCount, static_cast<avdecc_entity_connection_flags_t>(flags.value()), static_cast<avdecc_local_entity_control_status_t>(status));
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}

LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getTalkerStreamConnection(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_entity_model_stream_identification_cp const talkerStream, unsigned short const connectionIndex, avdecc_local_entity_get_talker_stream_connection_cb const onResult)
{
	try
	{
		auto& obj = s_AggregateEntityManager.getObject(handle);
		obj.getTalkerStreamConnection(la::avdecc::bindings::fromCToCpp::make_stream_identification(talkerStream), connectionIndex,
			[handle, onResult](la::avdecc::entity::controller::Interface const* const /*controller*/, la::avdecc::entity::model::StreamIdentification const& talkerStream, la::avdecc::entity::model::StreamIdentification const& listenerStream, std::uint16_t const connectionCount, la::avdecc::entity::ConnectionFlags const flags, la::avdecc::entity::LocalEntity::ControlStatus const status)
			{
				auto ts = la::avdecc::bindings::fromCppToC::make_stream_identification(talkerStream);
				auto ls = la::avdecc::bindings::fromCppToC::make_stream_identification(listenerStream);
				la::avdecc::utils::invokeProtectedHandler(onResult, handle, &ts, &ls, connectionCount, static_cast<avdecc_entity_connection_flags_t>(flags.value()), static_cast<avdecc_local_entity_control_status_t>(status));
			});
	}
	catch (...)
	{
		return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_invalid_entity_handle);
	}

	return static_cast<avdecc_local_entity_error_t>(avdecc_local_entity_error_no_error);
}


/* ************************************************************************** */
/* LocalEntity private APIs                                                   */
/* ************************************************************************** */
la::avdecc::entity::AggregateEntity& getAggregateEntity(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle)
{
	return s_AggregateEntityManager.getObject(handle);
}
