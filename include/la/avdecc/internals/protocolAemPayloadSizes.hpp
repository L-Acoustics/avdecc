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
* @file protocolAemPayloadSizes.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "la/avdecc/internals/protocolAemAecpdu.hpp"

namespace la
{
namespace avdecc
{
namespace protocol
{
namespace aemPayload
{
/** ACQUIRE_ENTITY Command and Response - IEEE1722.1-2013 Clause 7.4.1.1 */
constexpr size_t AecpAemAcquireEntityCommandPayloadSize = 16u;
constexpr size_t AecpAemAcquireEntityResponsePayloadSize = 16u;

/** LOCK_ENTITY Command and Response - IEEE1722.1-2013 Clause 7.4.2.1 */
constexpr size_t AecpAemLockEntityCommandPayloadSize = 16u;
constexpr size_t AecpAemLockEntityResponsePayloadSize = 16u;

/** ENTITY_AVAILABLE Command and Response - IEEE1722.1-2013 Clause 7.4.3.1 */
constexpr size_t AecpAemEntityAvailableCommandPayloadSize = 0u;
constexpr size_t AecpAemEntityAvailableResponsePayloadSize = 0u;

/** CONTROLLER_AVAILABLE Command and Response - IEEE1722.1-2013 Clause 7.4.4.1 */
constexpr size_t AecpAemControllerAvailableCommandPayloadSize = 0u;
constexpr size_t AecpAemControllerAvailableResponsePayloadSize = 0u;

/** READ_DESCRIPTOR Command - IEEE1722.1-2013 Clause 7.4.5.1 */
constexpr size_t AecpAemReadDescriptorCommandPayloadSize = 8u;

/** READ_DESCRIPTOR Response - IEEE1722.1-2013 Clause 7.4.5.2 */
constexpr size_t AecpAemReadCommonDescriptorResponsePayloadSize = 8u; // configuration_index (2), reserved (2), descriptor_type (2), descriptor_index (2)
constexpr size_t AecpAemReadEntityDescriptorResponsePayloadSize = AecpAemReadCommonDescriptorResponsePayloadSize + 308u;
constexpr size_t AecpAemReadConfigurationDescriptorResponsePayloadMinSize = AecpAemReadCommonDescriptorResponsePayloadSize + 70u;
constexpr size_t AecpAemReadAudioUnitDescriptorResponsePayloadMinSize = AecpAemReadCommonDescriptorResponsePayloadSize + 140u;
constexpr size_t AecpAemReadStreamDescriptorResponsePayloadMinSize = AecpAemReadCommonDescriptorResponsePayloadSize + 128u;
constexpr size_t AecpAemReadJackDescriptorResponsePayloadSize = AecpAemReadCommonDescriptorResponsePayloadSize + 74u;
constexpr size_t AecpAemReadAvbInterfaceDescriptorResponsePayloadSize = AecpAemReadCommonDescriptorResponsePayloadSize + 94u;
constexpr size_t AecpAemReadClockSourceDescriptorResponsePayloadSize = AecpAemReadCommonDescriptorResponsePayloadSize + 82u;
constexpr size_t AecpAemReadMemoryObjectDescriptorResponsePayloadSize = AecpAemReadCommonDescriptorResponsePayloadSize + 96u;
constexpr size_t AecpAemReadLocaleDescriptorResponsePayloadSize = AecpAemReadCommonDescriptorResponsePayloadSize + 68u;
constexpr size_t AecpAemReadStringsDescriptorResponsePayloadSize = AecpAemReadCommonDescriptorResponsePayloadSize + 448u;
constexpr size_t AecpAemReadStreamPortDescriptorResponsePayloadSize = AecpAemReadCommonDescriptorResponsePayloadSize + 16u;
constexpr size_t AecpAemReadExternalPortDescriptorResponsePayloadSize = AecpAemReadCommonDescriptorResponsePayloadSize + 20u;
constexpr size_t AecpAemReadInternalPortDescriptorResponsePayloadSize = AecpAemReadCommonDescriptorResponsePayloadSize + 20u;
constexpr size_t AecpAemReadAudioClusterDescriptorResponsePayloadSize = AecpAemReadCommonDescriptorResponsePayloadSize + 83u;
constexpr size_t AecpAemReadAudioMapDescriptorResponsePayloadMinSize = AecpAemReadCommonDescriptorResponsePayloadSize + 4u;
constexpr size_t AecpAemReadControlDescriptorResponsePayloadMinSize = AecpAemReadCommonDescriptorResponsePayloadSize + 100u;
constexpr size_t AecpAemReadSignalSelectorDescriptorResponsePayloadMinSize = AecpAemReadCommonDescriptorResponsePayloadSize + 92u;
constexpr size_t AecpAemReadMixerDescriptorResponsePayloadMinSize = AecpAemReadCommonDescriptorResponsePayloadSize + 84u;
constexpr size_t AecpAemReadMatrixDescriptorResponsePayloadMinSize = AecpAemReadCommonDescriptorResponsePayloadSize + 98u;
constexpr size_t AecpAemReadMatrixSignalDescriptorResponsePayloadMinSize = AecpAemReadCommonDescriptorResponsePayloadSize + 4u;
constexpr size_t AecpAemReadSignalSplitterDescriptorResponsePayloadMinSize = AecpAemReadCommonDescriptorResponsePayloadSize + 88u;
constexpr size_t AecpAemReadSignalCombinerDescriptorResponsePayloadMinSize = AecpAemReadCommonDescriptorResponsePayloadSize + 84u;
constexpr size_t AecpAemReadSignalDemultiplexerDescriptorResponsePayloadMinSize = AecpAemReadCommonDescriptorResponsePayloadSize + 88u;
constexpr size_t AecpAemReadSignalMultiplexerDescriptorResponsePayloadMinSize = AecpAemReadCommonDescriptorResponsePayloadSize + 84u;
constexpr size_t AecpAemReadSignalTranscoderDescriptorResponsePayloadMinSize = AecpAemReadCommonDescriptorResponsePayloadSize + 88u;
constexpr size_t AecpAemReadClockDomainDescriptorResponsePayloadMinSize = AecpAemReadCommonDescriptorResponsePayloadSize + 72u;
constexpr size_t AecpAemReadControlBlockDescriptorResponsePayloadSize = AecpAemReadCommonDescriptorResponsePayloadSize + 72u;
constexpr size_t AecpAemReadTimingDescriptorResponsePayloadMinSize = AecpAemReadCommonDescriptorResponsePayloadSize + 72u;
constexpr size_t AecpAemReadPtpInstanceDescriptorResponsePayloadSize = AecpAemReadCommonDescriptorResponsePayloadSize + 86u;
constexpr size_t AecpAemReadPtpPortDescriptorResponsePayloadSize = AecpAemReadCommonDescriptorResponsePayloadSize + 82u;

/** SET_CONFIGURATION Command and Response - IEEE1722.1-2013 Clause 7.4.7.1 */
constexpr size_t AecpAemSetConfigurationCommandPayloadSize = 4u;
constexpr size_t AecpAemSetConfigurationResponsePayloadSize = 4u;

/** GET_CONFIGURATION Command - IEEE1722.1-2013 Clause 7.4.8.1 */
constexpr size_t AecpAemGetConfigurationCommandPayloadSize = 0u;

/** GET_CONFIGURATION Response - IEEE1722.1-2013 Clause 7.4.8.2 */
constexpr size_t AecpAemGetConfigurationResponsePayloadSize = 4u;

/** SET_STREAM_FORMAT Command and Response - IEEE1722.1-2013 Clause 7.4.9.1 */
constexpr size_t AecpAemSetStreamFormatCommandPayloadSize = 12u;
constexpr size_t AecpAemSetStreamFormatResponsePayloadSize = 12u;

/** GET_STREAM_FORMAT Command - IEEE1722.1-2013 Clause 7.4.10.1 */
constexpr size_t AecpAemGetStreamFormatCommandPayloadSize = 4u;

/** GET_STREAM_FORMAT Respones - IEEE1722.1-2013 Clause 7.4.10.2 */
constexpr size_t AecpAemGetStreamFormatResponsePayloadSize = 12u;

/** SET_STREAM_INFO Command and Response - IEEE1722.1-2013 Clause 7.4.15.1 */
constexpr size_t AecpAemSetStreamInfoCommandPayloadSize = 48u;
constexpr size_t AecpAemSetStreamInfoResponsePayloadSize = 48u;

/** GET_STREAM_INFO Command - IEEE1722.1-2013 Clause 7.4.16.1 */
constexpr size_t AecpAemGetStreamInfoCommandPayloadSize = 4u;

/** GET_STREAM_INFO Response - IEEE1722.1-2013 Clause 7.4.16.2 */
constexpr size_t AecpAemGetStreamInfoResponsePayloadSize = 48u;

/** GET_STREAM_INFO Response - Milan-2019 Clause 7.3.10 */
constexpr size_t AecpAemMilanGetStreamInfoResponsePayloadSize = 56u;

/** SET_NAME Command and Response - IEEE1722.1-2013 Clause 7.4.17.1 */
constexpr size_t AecpAemSetNameCommandPayloadSize = 72u;
constexpr size_t AecpAemSetNameResponsePayloadSize = 72u;

/** GET_NAME Command - IEEE1722.1-2013 Clause 7.4.18.1 */
constexpr size_t AecpAemGetNameCommandPayloadSize = 8u;

/** GET_NAME Response - IEEE1722.1-2013 Clause 7.4.18.2 */
constexpr size_t AecpAemGetNameResponsePayloadSize = 72u;

/** SET_ASSOCIATION_ID Command and Response - IEEE1722.1-2013 Clause 7.4.19.1 */
constexpr size_t AecpAemSetAssociationIDCommandPayloadSize = 8u;
constexpr size_t AecpAemSetAssociationIDResponsePayloadSize = 8u;

/** GET_ASSOCIATION_ID Command - IEEE1722.1-2013 Clause 7.4.20.1 */
constexpr size_t AecpAemGetAssociationIDCommandPayloadSize = 0u;

/** GET_ASSOCIATION_ID Response - IEEE1722.1-2013 Clause 7.4.20.2 */
constexpr size_t AecpAemGetAssociationIDResponsePayloadSize = 8u;

/** SET_SAMPLING_RATE Command and Response - IEEE1722.1-2013 Clause 7.4.21.1 */
constexpr size_t AecpAemSetSamplingRateCommandPayloadSize = 8u;
constexpr size_t AecpAemSetSamplingRateResponsePayloadSize = 8u;

/** GET_SAMPLING_RATE Command - IEEE1722.1-2013 Clause 7.4.22.1 */
constexpr size_t AecpAemGetSamplingRateCommandPayloadSize = 4u;

/** GET_SAMPLING_RATE Response - IEEE1722.1-2013 Clause 7.4.22.2 */
constexpr size_t AecpAemGetSamplingRateResponsePayloadSize = 8u;

/** SET_CLOCK_SOURCE Command and Response - IEEE1722.1-2013 Clause 7.4.23.1 */
constexpr size_t AecpAemSetClockSourceCommandPayloadSize = 8u;
constexpr size_t AecpAemSetClockSourceResponsePayloadSize = 8u;

/** GET_CLOCK_SOURCE Command - IEEE1722.1-2013 Clause 7.4.24.1 */
constexpr size_t AecpAemGetClockSourceCommandPayloadSize = 4u;

/** GET_CLOCK_SOURCE Response - IEEE1722.1-2013 Clause 7.4.24.2 */
constexpr size_t AecpAemGetClockSourceResponsePayloadSize = 8u;

/** SET_CONTROL Command and Response - IEEE1722.1-2013 Clause 7.4.25.1 */
constexpr size_t AecpAemSetControlCommandPayloadMinSize = 4u;
constexpr size_t AecpAemSetControlResponsePayloadMinSize = 4u;

/** GET_CONTROL Command - IEEE1722.1-2013 Clause 7.4.26.1 */
constexpr size_t AecpAemGetControlCommandPayloadSize = 4u;

/** GET_CONTROL Response - IEEE1722.1-2013 Clause 7.4.26.2 */
constexpr size_t AecpAemGetControlResponsePayloadMinSize = 4u;

/** START_STREAMING Command and Response - IEEE1722.1-2013 Clause 7.4.35.1 */
constexpr size_t AecpAemStartStreamingCommandPayloadSize = 4u;
constexpr size_t AecpAemStartStreamingResponsePayloadSize = 4u;

/** STOP_STREAMING Command and Response - IEEE1722.1-2013 Clause 7.4.36.1 */
constexpr size_t AecpAemStopStreamingCommandPayloadSize = 4u;
constexpr size_t AecpAemStopStreamingResponsePayloadSize = 4u;

/** REGISTER_UNSOLICITED_NOTIFICATION Command and Response - IEEE1722.1-2013 Clause 7.4.37.1 */
constexpr size_t AecpAemRegisterUnsolicitedNotificationCommandPayloadSize = 0u;
constexpr size_t AecpAemRegisterUnsolicitedNotificationResponsePayloadSize = 0u;

/** DEREGISTER_UNSOLICITED_NOTIFICATION Command and Response - IEEE1722.1-2013 Clause 7.4.38.1 */
constexpr size_t AecpAemDeregisterUnsolicitedNotificationCommandPayloadSize = 0u;
constexpr size_t AecpAemDeregisterUnsolicitedNotificationResponsePayloadSize = 0u;

/** GET_AVB_INFO Command - IEEE1722.1-2013 Clause 7.4.40.1 */
constexpr size_t AecpAemGetAvbInfoCommandPayloadSize = 4u;

/** GET_AVB_INFO Response - IEEE1722.1-2013 Clause 7.4.40.2 */
constexpr size_t AecpAemGetAvbInfoResponsePayloadMinSize = 20u;

/** GET_AS_PATH Command - IEEE1722.1-2013 Clause 7.4.41.1 */
constexpr size_t AecpAemGetAsPathCommandPayloadSize = 4u;

/** GET_AS_PATH Response - IEEE1722.1-2013 Clause 7.4.41.2 */
constexpr size_t AecpAemGetAsPathResponsePayloadMinSize = 4u;

/** GET_COUNTERS Command - IEEE1722.1-2013 Clause 7.4.42.1 */
constexpr size_t AecpAemGetCountersCommandPayloadSize = 4u;

/** GET_COUNTERS Response - IEEE1722.1-2013 Clause 7.4.42.2 */
constexpr size_t AecpAemGetCountersResponsePayloadSize = 136u;

/** REBOOT Command - IEEE1722.1-2013 Clause 7.4.43.1 */
constexpr size_t AecpAemRebootCommandPayloadSize = 4u;

/** REBOOT Response - IEEE1722.1-2013 Clause 7.4.43.2 */
constexpr size_t AecpAemRebootResponsePayloadSize = 4u;

/** GET_AUDIO_MAP Command - IEEE1722.1-2013 Clause 7.4.44.1 */
constexpr size_t AecpAemGetAudioMapCommandPayloadSize = 8u;

/** GET_AUDIO_MAP Response - IEEE1722.1-2013 Clause 7.4.44.2 */
constexpr size_t AecpAemGetAudioMapResponsePayloadMinSize = 12u;

/** ADD_AUDIO_MAPPINGS Command - IEEE1722.1-2013 Clause 7.4.45.1 */
constexpr size_t AecpAemAddAudioMappingsCommandPayloadMinSize = 8u;

/** ADD_AUDIO_MAPPINGS Response - IEEE1722.1-2013 Clause 7.4.45.1 */
constexpr size_t AecpAemAddAudioMappingsResponsePayloadMinSize = 8u;

/** REMOVE_AUDIO_MAPPINGS Command - IEEE1722.1-2013 Clause 7.4.46.1 */
constexpr size_t AecpAemRemoveAudioMappingsCommandPayloadMinSize = 8u;

/** REMOVE_AUDIO_MAPPINGS Response - IEEE1722.1-2013 Clause 7.4.46.1 */
constexpr size_t AecpAemRemoveAudioMappingsResponsePayloadMinSize = 8u;

/** START_OPERATION Command - IEEE1722.1-2013 Clause 7.4.53.1 */
constexpr size_t AecpAemStartOperationCommandPayloadMinSize = 8u;

/** START_OPERATION Response - IEEE1722.1-2013 Clause 7.4.53.1 */
constexpr size_t AecpAemStartOperationResponsePayloadMinSize = 8u;

/** ABORT_OPERATION Command - IEEE1722.1-2013 Clause 7.4.54.1 */
constexpr size_t AecpAemAbortOperationCommandPayloadSize = 8u;

/** ABORT_OPERATION Response - IEEE1722.1-2013 Clause 7.4.54.1 */
constexpr size_t AecpAemAbortOperationResponsePayloadSize = 8u;

/** OPERATION_STATUS Response - IEEE1722.1-2013 Clause 7.4.55.1 */
constexpr size_t AecpAemOperationStatusResponsePayloadSize = 8u;

/** SET_MEMORY_OBJECT_LENGTH Command - IEEE1722.1-2013 Clause 7.4.72.1 */
constexpr size_t AecpAemSetMemoryObjectLengthCommandPayloadSize = 12u;

/** SET_MEMORY_OBJECT_LENGTH Response - IEEE1722.1-2013 Clause 7.4.72.1 */
constexpr size_t AecpAemSetMemoryObjectLengthResponsePayloadSize = 12u;

/** GET_MEMORY_OBJECT_LENGTH Command - IEEE1722.1-2013 Clause 7.4.73.1 */
constexpr size_t AecpAemGetMemoryObjectLengthCommandPayloadSize = 4u;

/** GET_MEMORY_OBJECT_LENGTH Response - IEEE1722.1-2013 Clause 7.4.73.2 */
constexpr size_t AecpAemGetMemoryObjectLengthResponsePayloadSize = 12u;

static_assert(AecpAemAddAudioMappingsCommandPayloadMinSize == AecpAemRemoveAudioMappingsCommandPayloadMinSize, "Add and Remove no longer the same size, we should split AecpAemMaxAddRemoveAudioMappings in 2");
constexpr size_t AecpAemMaxAddRemoveAudioMappings = (AemAecpdu::MaximumSendPayloadBufferLength - AecpAemRemoveAudioMappingsCommandPayloadMinSize) / 8;

} // namespace aemPayload
} // namespace protocol
} // namespace avdecc
} // namespace la
