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
* @file protocolAemPayloads.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "la/avdecc/memoryBuffer.hpp"
#include "la/avdecc/internals/serialization.hpp"
#include "la/avdecc/internals/entityModel.hpp"
#include "la/avdecc/internals/entity.hpp"
#include "la/avdecc/internals/protocolAemPayloadSizes.hpp"

#include <cstdint>
#include <tuple>

namespace la
{
namespace avdecc
{
namespace protocol
{
namespace aemPayload
{
/** Exception when receiving a payload that has an invalid size */
class IncorrectPayloadSizeException final : public Exception
{
public:
	IncorrectPayloadSizeException()
		: Exception("Incorrect payload size")
	{
	}
};

/** Exception when receiving a payload the library does not handle yet */
class UnsupportedValueException final : public Exception
{
public:
	UnsupportedValueException()
		: Exception("Unsupported value")
	{
	}
};
// All serialization methods might throw a std::invalid_argument if serialization goes wrong
// All deserialization methods might throw a la::avdecc:Exception if deserialization goes wrong

/** ACQUIRE_ENTITY Command - IEEE1722.1-2013 Clause 7.4.1.1 */
Serializer<AecpAemAcquireEntityCommandPayloadSize> serializeAcquireEntityCommand(AemAcquireEntityFlags const flags, UniqueIdentifier const ownerID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex);
std::tuple<AemAcquireEntityFlags, UniqueIdentifier, entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeAcquireEntityCommand(AemAecpdu::Payload const& payload);

/** ACQUIRE_ENTITY Response - IEEE1722.1-2013 Clause 7.4.1.1 */
Serializer<AecpAemAcquireEntityResponsePayloadSize> serializeAcquireEntityResponse(AemAcquireEntityFlags const flags, UniqueIdentifier const ownerID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex);
std::tuple<AemAcquireEntityFlags, UniqueIdentifier, entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeAcquireEntityResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload);

/** LOCK_ENTITY Command - IEEE1722.1-2013 Clause 7.4.2.1 */
Serializer<AecpAemLockEntityCommandPayloadSize> serializeLockEntityCommand(AemLockEntityFlags const flags, UniqueIdentifier const lockedID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex);
std::tuple<AemLockEntityFlags, UniqueIdentifier, entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeLockEntityCommand(AemAecpdu::Payload const& payload);

/** LOCK_ENTITY Response - IEEE1722.1-2013 Clause 7.4.2.1 */
Serializer<AecpAemLockEntityResponsePayloadSize> serializeLockEntityResponse(AemLockEntityFlags const flags, UniqueIdentifier const lockedID, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex);
std::tuple<AemLockEntityFlags, UniqueIdentifier, entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeLockEntityResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload);

/** ENTITY_AVAILABLE Command - IEEE1722.1-2013 Clause 7.4.3.1 */
// No payload

/** ENTITY_AVAILABLE Response - IEEE1722.1-2013 Clause 7.4.3.1 */
// No payload

/** CONTROLLER_AVAILABLE Command - IEEE1722.1-2013 Clause 7.4.4.1 */
// No payload

/** CONTROLLER_AVAILABLE Response - IEEE1722.1-2013 Clause 7.4.4.1 */
// No payload

/** READ_DESCRIPTOR Command - IEEE1722.1-2013 Clause 7.4.5.1 */
Serializer<AecpAemReadDescriptorCommandPayloadSize> serializeReadDescriptorCommand(entity::model::ConfigurationIndex const configurationIndex, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex);
std::tuple<entity::model::ConfigurationIndex, entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeReadDescriptorCommand(AemAecpdu::Payload const& payload);

/** READ_DESCRIPTOR Response - IEEE1722.1-2013 Clause 7.4.5.2 */
Serializer<AemAecpdu::MaximumSendPayloadBufferLength> serializeReadDescriptorCommonResponse(entity::model::ConfigurationIndex const configurationIndex, entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex);
void serializeReadEntityDescriptorResponse(Serializer<AemAecpdu::MaximumSendPayloadBufferLength>& ser, entity::model::EntityDescriptor const& entityDescriptor);
void serializeReadConfigurationDescriptorResponse(Serializer<AemAecpdu::MaximumSendPayloadBufferLength>& ser, entity::model::ConfigurationDescriptor const& configurationDescriptor);
std::tuple<size_t, entity::model::ConfigurationIndex, entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeReadDescriptorCommonResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload);
entity::model::EntityDescriptor deserializeReadEntityDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status);
entity::model::ConfigurationDescriptor deserializeReadConfigurationDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status);
entity::model::AudioUnitDescriptor deserializeReadAudioUnitDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status);
entity::model::StreamDescriptor deserializeReadStreamDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status);
entity::model::JackDescriptor deserializeReadJackDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status);
entity::model::AvbInterfaceDescriptor deserializeReadAvbInterfaceDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status);
entity::model::ClockSourceDescriptor deserializeReadClockSourceDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status);
entity::model::MemoryObjectDescriptor deserializeReadMemoryObjectDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status);
entity::model::LocaleDescriptor deserializeReadLocaleDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status);
entity::model::StringsDescriptor deserializeReadStringsDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status);
entity::model::StreamPortDescriptor deserializeReadStreamPortDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status);
entity::model::ExternalPortDescriptor deserializeReadExternalPortDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status);
entity::model::InternalPortDescriptor deserializeReadInternalPortDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status);
entity::model::AudioClusterDescriptor deserializeReadAudioClusterDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status);
entity::model::AudioMapDescriptor deserializeReadAudioMapDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status);
entity::model::ControlDescriptor deserializeReadControlDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status);
entity::model::ClockDomainDescriptor deserializeReadClockDomainDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status);
entity::model::TimingDescriptor deserializeReadTimingDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status);
entity::model::PtpInstanceDescriptor deserializeReadPtpInstanceDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status);
entity::model::PtpPortDescriptor deserializeReadPtpPortDescriptorResponse(AemAecpdu::Payload const& payload, size_t const commonSize, AemAecpStatus const status);

/** WRITE_DESCRIPTOR Command - IEEE1722.1-2013 Clause 7.4.6.1 */

/** WRITE_DESCRIPTOR Response - IEEE1722.1-2013 Clause 7.4.6.1 */

/** SET_CONFIGURATION Command - IEEE1722.1-2013 Clause 7.4.7.1 */
Serializer<AecpAemSetConfigurationCommandPayloadSize> serializeSetConfigurationCommand(entity::model::ConfigurationIndex const configurationIndex);
std::tuple<entity::model::ConfigurationIndex> deserializeSetConfigurationCommand(AemAecpdu::Payload const& payload);

/** SET_CONFIGURATION Response - IEEE1722.1-2013 Clause 7.4.7.1 */
Serializer<AecpAemSetConfigurationResponsePayloadSize> serializeSetConfigurationResponse(entity::model::ConfigurationIndex const configurationIndex);
std::tuple<entity::model::ConfigurationIndex> deserializeSetConfigurationResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload);

/** GET_CONFIGURATION Command - IEEE1722.1-2013 Clause 7.4.8.1 */
// No payload

/** GET_CONFIGURATION Response - IEEE1722.1-2013 Clause 7.4.8.2 */
Serializer<AecpAemGetConfigurationResponsePayloadSize> serializeGetConfigurationResponse(entity::model::ConfigurationIndex const configurationIndex);
std::tuple<entity::model::ConfigurationIndex> deserializeGetConfigurationResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload);

/** SET_STREAM_FORMAT Command - IEEE1722.1-2013 Clause 7.4.9.1 */
Serializer<AecpAemSetStreamFormatCommandPayloadSize> serializeSetStreamFormatCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::StreamFormat const streamFormat);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::StreamFormat> deserializeSetStreamFormatCommand(AemAecpdu::Payload const& payload);

/** SET_STREAM_FORMAT Response - IEEE1722.1-2013 Clause 7.4.9.1 */
Serializer<AecpAemSetStreamFormatResponsePayloadSize> serializeSetStreamFormatResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::StreamFormat const streamFormat);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::StreamFormat> deserializeSetStreamFormatResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload);

/** GET_STREAM_FORMAT Command - IEEE1722.1-2013 Clause 7.4.10.1 */
Serializer<AecpAemGetStreamFormatCommandPayloadSize> serializeGetStreamFormatCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeGetStreamFormatCommand(AemAecpdu::Payload const& payload);

/** GET_STREAM_FORMAT Response - IEEE1722.1-2013 Clause 7.4.10.2 */
Serializer<AecpAemGetStreamFormatResponsePayloadSize> serializeGetStreamFormatResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::StreamFormat const streamFormat);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::StreamFormat> deserializeGetStreamFormatResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload);

/** SET_STREAM_INFO Command - IEEE1722.1-2013 Clause 7.4.15.1 */
Serializer<AecpAemSetStreamInfoCommandPayloadSize> serializeSetStreamInfoCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::StreamInfo const& streamInfo);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::StreamInfo> deserializeSetStreamInfoCommand(AemAecpdu::Payload const& payload);

/** SET_STREAM_INFO Response - IEEE1722.1-2013 Clause 7.4.15.1 */
Serializer<AecpAemSetStreamInfoResponsePayloadSize> serializeSetStreamInfoResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::StreamInfo const& streamInfo);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::StreamInfo> deserializeSetStreamInfoResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload);

/** GET_STREAM_INFO Command - IEEE1722.1-2013 Clause 7.4.16.1 */
Serializer<AecpAemGetStreamInfoCommandPayloadSize> serializeGetStreamInfoCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeGetStreamInfoCommand(AemAecpdu::Payload const& payload);

/** GET_STREAM_INFO Response - IEEE1722.1-2013 Clause 7.4.16.2 */
Serializer<AecpAemMilanGetStreamInfoResponsePayloadSize> serializeGetStreamInfoResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::StreamInfo const& streamInfo);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::StreamInfo> deserializeGetStreamInfoResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload);

/** SET_NAME Command - IEEE1722.1-2013 Clause 7.4.17.1 */
Serializer<AecpAemSetNameCommandPayloadSize> serializeSetNameCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const nameIndex, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& name);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, std::uint16_t, entity::model::ConfigurationIndex, entity::model::AvdeccFixedString> deserializeSetNameCommand(AemAecpdu::Payload const& payload);

/** SET_NAME Response - IEEE1722.1-2013 Clause 7.4.17.1 */
Serializer<AecpAemSetNameResponsePayloadSize> serializeSetNameResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const nameIndex, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& name);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, std::uint16_t, entity::model::ConfigurationIndex, entity::model::AvdeccFixedString> deserializeSetNameResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload);

/** GET_NAME Command - IEEE1722.1-2013 Clause 7.4.18.1 */
Serializer<AecpAemGetNameCommandPayloadSize> serializeGetNameCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const nameIndex, entity::model::ConfigurationIndex const configurationIndex);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, std::uint16_t, entity::model::ConfigurationIndex> deserializeGetNameCommand(AemAecpdu::Payload const& payload);

/** GET_NAME Response - IEEE1722.1-2013 Clause 7.4.18.2 */
Serializer<AecpAemGetNameResponsePayloadSize> serializeGetNameResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, std::uint16_t const nameIndex, entity::model::ConfigurationIndex const configurationIndex, entity::model::AvdeccFixedString const& name);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, std::uint16_t, entity::model::ConfigurationIndex, entity::model::AvdeccFixedString> deserializeGetNameResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload);

/** SET_ASSOCIATION_ID Command - IEEE1722.1-2013 Clause 7.4.19.1 */
Serializer<AecpAemSetAssociationIDCommandPayloadSize> serializeSetAssociationIDCommand(UniqueIdentifier const associationID);
std::tuple<UniqueIdentifier> deserializeSetAssociationIDCommand(AemAecpdu::Payload const& payload);

/** SET_ASSOCIATION_ID Response - IEEE1722.1-2013 Clause 7.4.19.1 */
Serializer<AecpAemSetAssociationIDResponsePayloadSize> serializeSetAssociationIDResponse(UniqueIdentifier const associationID);
std::tuple<UniqueIdentifier> deserializeSetAssociationIDResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload);

/** GET_ASSOCIATION_ID Command - IEEE1722.1-2013 Clause 7.4.20.1 */
// No payload

/** GET_ASSOCIATION_ID Response - IEEE1722.1-2013 Clause 7.4.20.2 */
Serializer<AecpAemGetAssociationIDResponsePayloadSize> serializeGetAssociationIDResponse(UniqueIdentifier const associationID);
std::tuple<UniqueIdentifier> deserializeGetAssociationIDResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload);

/** SET_SAMPLING_RATE Command - IEEE1722.1-2013 Clause 7.4.21.1 */
Serializer<AecpAemSetSamplingRateCommandPayloadSize> serializeSetSamplingRateCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::SamplingRate const samplingRate);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::SamplingRate> deserializeSetSamplingRateCommand(AemAecpdu::Payload const& payload);

/** SET_SAMPLING_RATE Response - IEEE1722.1-2013 Clause 7.4.21.1 */
Serializer<AecpAemSetSamplingRateResponsePayloadSize> serializeSetSamplingRateResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::SamplingRate const samplingRate);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::SamplingRate> deserializeSetSamplingRateResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload);

/** GET_SAMPLING_RATE Command - IEEE1722.1-2013 Clause 7.4.22.1 */
Serializer<AecpAemGetSamplingRateCommandPayloadSize> serializeGetSamplingRateCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeGetSamplingRateCommand(AemAecpdu::Payload const& payload);

/** GET_SAMPLING_RATE Response - IEEE1722.1-2013 Clause 7.4.22.2 */
Serializer<AecpAemGetSamplingRateResponsePayloadSize> serializeGetSamplingRateResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::SamplingRate const samplingRate);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::SamplingRate> deserializeGetSamplingRateResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload);

/** SET_CLOCK_SOURCE Command - IEEE1722.1-2013 Clause 7.4.23.1 */
Serializer<AecpAemSetClockSourceCommandPayloadSize> serializeSetClockSourceCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::ClockSourceIndex const clockSourceIndex);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::ClockSourceIndex> deserializeSetClockSourceCommand(AemAecpdu::Payload const& payload);

/** SET_CLOCK_SOURCE Response - IEEE1722.1-2013 Clause 7.4.23.1 */
Serializer<AecpAemSetClockSourceResponsePayloadSize> serializeSetClockSourceResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::ClockSourceIndex const clockSourceIndex);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::ClockSourceIndex> deserializeSetClockSourceResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload);

/** GET_CLOCK_SOURCE Command - IEEE1722.1-2013 Clause 7.4.24.1 */
Serializer<AecpAemGetClockSourceCommandPayloadSize> serializeGetClockSourceCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeGetClockSourceCommand(AemAecpdu::Payload const& payload);

/** GET_CLOCK_SOURCE Response - IEEE1722.1-2013 Clause 7.4.24.2 */
Serializer<AecpAemGetClockSourceResponsePayloadSize> serializeGetClockSourceResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::ClockSourceIndex const clockSourceIndex);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::ClockSourceIndex> deserializeGetClockSourceResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload);

/** SET_CONTROL Command - IEEE1722.1-2013 Clause 7.4.25.1 */
Serializer<AemAecpdu::MaximumSendPayloadBufferLength> serializeSetControlCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::ControlValues const& controlValues);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, MemoryBuffer> deserializeSetControlCommand(AemAecpdu::Payload const& payload);

/** SET_CONTROL Response - IEEE1722.1-2013 Clause 7.4.25.1 */
Serializer<AemAecpdu::MaximumSendPayloadBufferLength> serializeSetControlResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::ControlValues const& controlValues);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, MemoryBuffer> deserializeSetControlResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload);

/** GET_CONTROL Command - IEEE1722.1-2013 Clause 7.4.26.1 */
Serializer<AecpAemGetControlCommandPayloadSize> serializeGetControlCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeGetControlCommand(AemAecpdu::Payload const& payload);

/** GET_CONTROL Response - IEEE1722.1-2013 Clause 7.4.26.2 */
Serializer<AemAecpdu::MaximumSendPayloadBufferLength> serializeGetControlResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::ControlValues const& controlValues);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, MemoryBuffer> deserializeGetControlResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload);

/** START_STREAMING Command - IEEE1722.1-2013 Clause 7.4.35.1 */
Serializer<AecpAemStartStreamingCommandPayloadSize> serializeStartStreamingCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeStartStreamingCommand(AemAecpdu::Payload const& payload);

/** START_STREAMING Response - IEEE1722.1-2013 Clause 7.4.35.1 */
Serializer<AecpAemStartStreamingResponsePayloadSize> serializeStartStreamingResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeStartStreamingResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload);

/** STOP_STREAMING Command - IEEE1722.1-2013 Clause 7.4.36.1 */
Serializer<AecpAemStopStreamingCommandPayloadSize> serializeStopStreamingCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeStopStreamingCommand(AemAecpdu::Payload const& payload);

/** STOP_STREAMING Response - IEEE1722.1-2013 Clause 7.4.36.1 */
Serializer<AecpAemStopStreamingResponsePayloadSize> serializeStopStreamingResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeStopStreamingResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload);

/** REGISTER_UNSOLICITED_NOTIFICATION Command - IEEE1722.1-2013 Clause 7.4.37.1 */
// No payload

/** REGISTER_UNSOLICITED_NOTIFICATION Response - IEEE1722.1-2013 Clause 7.4.37.1 */
// No payload

/** DEREGISTER_UNSOLICITED_NOTIFICATION Command - IEEE1722.1-2013 Clause 7.4.38.1 */
// No payload

/** DEREGISTER_UNSOLICITED_NOTIFICATION Response - IEEE1722.1-2013 Clause 7.4.38.1 */
// No payload

/** GET_AVB_INFO Command - IEEE1722.1-2013 Clause 7.4.40.1 */
Serializer<AecpAemGetAvbInfoCommandPayloadSize> serializeGetAvbInfoCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeGetAvbInfoCommand(AemAecpdu::Payload const& payload);

/** GET_AVB_INFO Response - IEEE1722.1-2013 Clause 7.4.40.2 */
Serializer<AemAecpdu::MaximumSendPayloadBufferLength> serializeGetAvbInfoResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::AvbInfo const& avbInfo);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::AvbInfo> deserializeGetAvbInfoResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload);

/** GET_AS_PATH Command - IEEE1722.1-2013 Clause 7.4.41.1 */
Serializer<AecpAemGetAsPathCommandPayloadSize> serializeGetAsPathCommand(entity::model::DescriptorIndex const descriptorIndex);
std::tuple<entity::model::DescriptorIndex> deserializeGetAsPathCommand(AemAecpdu::Payload const& payload);

/** GET_AS_PATH Response - IEEE1722.1-2013 Clause 7.4.41.2 */
Serializer<AemAecpdu::MaximumSendPayloadBufferLength> serializeGetAsPathResponse(entity::model::DescriptorIndex const descriptorIndex, entity::model::AsPath const& asPath);
std::tuple<entity::model::DescriptorIndex, entity::model::AsPath> deserializeGetAsPathResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload);

/** GET_COUNTERS Command - IEEE1722.1-2013 Clause 7.4.42.1 */
Serializer<AecpAemGetCountersCommandPayloadSize> serializeGetCountersCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeGetCountersCommand(AemAecpdu::Payload const& payload);

/** GET_COUNTERS Response - IEEE1722.1-2013 Clause 7.4.42.2 */
Serializer<AecpAemGetCountersResponsePayloadSize> serializeGetCountersResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::DescriptorCounterValidFlag const validCounters, entity::model::DescriptorCounters const& counters);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::DescriptorCounterValidFlag, entity::model::DescriptorCounters> deserializeGetCountersResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload);

/** REBOOT Command - IEEE1722.1-2013 Clause 7.4.43.1 */
Serializer<AecpAemRebootCommandPayloadSize> serializeRebootCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeRebootCommand(AemAecpdu::Payload const& payload);

/** GET_COUNTERS Response - IEEE1722.1-2013 Clause 7.4.43.1 */
Serializer<AecpAemRebootResponsePayloadSize> serializeRebootResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex> deserializeRebootResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload);

/** GET_AUDIO_MAP Command - IEEE1722.1-2013 Clause 7.4.44.1 */
Serializer<AecpAemGetAudioMapCommandPayloadSize> serializeGetAudioMapCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::MapIndex const mapIndex);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::MapIndex> deserializeGetAudioMapCommand(AemAecpdu::Payload const& payload);

/** GET_AUDIO_MAP Response - IEEE1722.1-2013 Clause 7.4.44.2 */
Serializer<AemAecpdu::MaximumSendPayloadBufferLength> serializeGetAudioMapResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::MapIndex const mapIndex, entity::model::MapIndex const numberOfMaps, entity::model::AudioMappings const& mappings);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::MapIndex, entity::model::MapIndex, entity::model::AudioMappings> deserializeGetAudioMapResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload);

/** ADD_AUDIO_MAPPINGS Command - IEEE1722.1-2013 Clause 7.4.45.1 */
Serializer<AemAecpdu::MaximumSendPayloadBufferLength> serializeAddAudioMappingsCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::AudioMappings const& mappings);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::AudioMappings> deserializeAddAudioMappingsCommand(AemAecpdu::Payload const& payload);

/** ADD_AUDIO_MAPPINGS Response - IEEE1722.1-2013 Clause 7.4.45.1 */
Serializer<AemAecpdu::MaximumSendPayloadBufferLength> serializeAddAudioMappingsResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::AudioMappings const& mappings);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::AudioMappings> deserializeAddAudioMappingsResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload);

/** REMOVE_AUDIO_MAPPINGS Command - IEEE1722.1-2013 Clause 7.4.46.1 */
Serializer<AemAecpdu::MaximumSendPayloadBufferLength> serializeRemoveAudioMappingsCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::AudioMappings const& mappings);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::AudioMappings> deserializeRemoveAudioMappingsCommand(AemAecpdu::Payload const& payload);

/** REMOVE_AUDIO_MAPPINGS Response - IEEE1722.1-2013 Clause 7.4.46.1 */
Serializer<AemAecpdu::MaximumSendPayloadBufferLength> serializeRemoveAudioMappingsResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::AudioMappings const& mappings);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::AudioMappings> deserializeRemoveAudioMappingsResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload);

/** START_OPERATION Command - IEEE1722.1-2013 Clause 7.4.53.1 */
Serializer<AemAecpdu::MaximumSendPayloadBufferLength> serializeStartOperationCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::OperationID const operationID, entity::model::MemoryObjectOperationType const operationType, MemoryBuffer const& memoryBuffer);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::OperationID, entity::model::MemoryObjectOperationType, MemoryBuffer> deserializeStartOperationCommand(AemAecpdu::Payload const& payload);

/** START_OPERATION Response - IEEE1722.1-2013 Clause 7.4.53.1 */
Serializer<AemAecpdu::MaximumSendPayloadBufferLength> serializeStartOperationResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::OperationID const operationID, entity::model::MemoryObjectOperationType const operationType, MemoryBuffer const& memoryBuffer);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::OperationID, entity::model::MemoryObjectOperationType, MemoryBuffer> deserializeStartOperationResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload);

/** ABORT_OPERATION Command - IEEE1722.1-2013 Clause 7.4.55.1 */
Serializer<AecpAemAbortOperationCommandPayloadSize> serializeAbortOperationCommand(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::OperationID const operationID);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::OperationID> deserializeAbortOperationCommand(AemAecpdu::Payload const& payload);

/** ABORT_OPERATION Response - IEEE1722.1-2013 Clause 7.4.55.1 */
Serializer<AecpAemAbortOperationResponsePayloadSize> serializeAbortOperationResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::OperationID const operationID);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::OperationID> deserializeAbortOperationResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload);

/** OPERATION_STATUS Unsolicited Response - IEEE1722.1-2013 Clause 7.4.55.1 */
Serializer<AecpAemOperationStatusResponsePayloadSize> serializeOperationStatusResponse(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex, entity::model::OperationID const operationID, std::uint16_t const percentComplete);
std::tuple<entity::model::DescriptorType, entity::model::DescriptorIndex, entity::model::OperationID, std::uint16_t> deserializeOperationStatusResponse(AemAecpdu::Payload const& payload);

/** SET_MEMORY_OBJECT_LENGTH Command - IEEE1722.1-2013 Clause 7.4.72.1 */
Serializer<AecpAemSetMemoryObjectLengthCommandPayloadSize> serializeSetMemoryObjectLengthCommand(entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length);
std::tuple<entity::model::ConfigurationIndex, entity::model::MemoryObjectIndex, std::uint64_t> deserializeSetMemoryObjectLengthCommand(AemAecpdu::Payload const& payload);

/** SET_MEMORY_OBJECT_LENGTH Response - IEEE1722.1-2013 Clause 7.4.72.1 */
Serializer<AecpAemSetMemoryObjectLengthResponsePayloadSize> serializeSetMemoryObjectLengthResponse(entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length);
std::tuple<entity::model::ConfigurationIndex, entity::model::MemoryObjectIndex, std::uint64_t> deserializeSetMemoryObjectLengthResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload);

/** GET_MEMORY_OBJECT_LENGTH Command - IEEE1722.1-2013 Clause 7.4.73.1 */
Serializer<AecpAemGetMemoryObjectLengthCommandPayloadSize> serializeGetMemoryObjectLengthCommand(entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex);
std::tuple<entity::model::ConfigurationIndex, entity::model::MemoryObjectIndex> deserializeGetMemoryObjectLengthCommand(AemAecpdu::Payload const& payload);

/** GET_MEMORY_OBJECT_LENGTH Response - IEEE1722.1-2013 Clause 7.4.73.2 */
Serializer<AecpAemGetMemoryObjectLengthResponsePayloadSize> serializeGetMemoryObjectLengthResponse(entity::model::ConfigurationIndex const configurationIndex, entity::model::MemoryObjectIndex const memoryObjectIndex, std::uint64_t const length);
std::tuple<entity::model::ConfigurationIndex, entity::model::MemoryObjectIndex, std::uint64_t> deserializeGetMemoryObjectLengthResponse(entity::LocalEntity::AemCommandStatus const status, AemAecpdu::Payload const& payload);

} // namespace aemPayload
} // namespace protocol
} // namespace avdecc
} // namespace la
