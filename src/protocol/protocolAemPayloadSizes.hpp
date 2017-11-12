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
* @file protocolAemPayloadSizes.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "protocolAemAecpdu.hpp"

namespace la
{
namespace avdecc
{
namespace protocol
{
namespace aemPayload
{

/** ACQUIRE_ENTITY Command - Clause 7.4.1.1 */
constexpr size_t AecpAemAcquireEntityCommandPayloadSize = 16u;

/** ACQUIRE_ENTITY Response - Clause 7.4.1.1 */
constexpr size_t AecpAemAcquireEntityResponsePayloadSize = 16u;

/** LOCK_ENTITY Command - Clause 7.4.2.1 */
constexpr size_t AecpAemLockEntityCommandPayloadSize = 16u;

/** LOCK_ENTITY Response - Clause 7.4.2.1 */
constexpr size_t AecpAemLockEntityResponsePayloadSize = 16u;

/** READ_DESCRIPTOR Command - Clause 7.4.5.1 */
constexpr size_t AecpAemReadDescriptorCommandPayloadSize = 8u;

/** READ_DESCRIPTOR Response - Clause 7.4.5.2 */
constexpr size_t AecpAemReadCommonDescriptorResponsePayloadSize = 8u;
constexpr size_t AecpAemReadDescriptorResponsePayloadMinSize = AecpAemReadCommonDescriptorResponsePayloadSize;
constexpr size_t AecpAemReadEntityDescriptorResponsePayloadSize = AecpAemReadCommonDescriptorResponsePayloadSize + 308u;
constexpr size_t AecpAemReadConfigurationDescriptorResponsePayloadMinSize = AecpAemReadCommonDescriptorResponsePayloadSize + 70u;
constexpr size_t AecpAemReadLocaleDescriptorResponsePayloadSize = AecpAemReadCommonDescriptorResponsePayloadSize + 68u;
constexpr size_t AecpAemReadStringsDescriptorResponsePayloadSize = AecpAemReadCommonDescriptorResponsePayloadSize + 448u;
constexpr size_t AecpAemReadStreamDescriptorResponsePayloadMinSize = AecpAemReadCommonDescriptorResponsePayloadSize + 128u;

/** SET_STREAM_FORMAT Command - Clause 7.4.9.1 */
constexpr size_t AecpAemSetStreamFormatCommandPayloadSize = 12u;

/** SET_STREAM_FORMAT Response - Clause 7.4.9.1 */
constexpr size_t AecpAemSetStreamFormatResponsePayloadSize = 12u;

/** GET_AUDIO_MAP Command  - Clause 7.4.44.1 */
constexpr size_t AecpAemGetAudioMapCommandPayloadSize = 8u;

/** GET_AUDIO_MAP Response  - Clause 7.4.44.2 */
constexpr size_t AecpAemGetAudioMapResponsePayloadMinSize = 12u;

/** ADD_AUDIO_MAPPINGS Command  - Clause 7.4.45.1 */
constexpr size_t AecpAemAddAudioMappingsCommandPayloadMaxSize = Aecpdu::MaximumLength - Aecpdu::HeaderLength - AemAecpdu::HeaderLength;

/** ADD_AUDIO_MAPPINGS Response  - Clause 7.4.45.1 */
constexpr size_t AecpAemAddAudioMappingsResponsePayloadMinSize = 8u;

/** REMOVE_AUDIO_MAPPINGS Command  - Clause 7.4.46.1 */
constexpr size_t AecpAemRemoveAudioMappingsCommandPayloadMaxSize = Aecpdu::MaximumLength - Aecpdu::HeaderLength - AemAecpdu::HeaderLength;

/** REMOVE_AUDIO_MAPPINGS Response  - Clause 7.4.46.1 */
constexpr size_t AecpAemRemoveAudioMappingsResponsePayloadMinSize = 8u;

/** GET_STREAM_INFO Command - Clause 7.4.16.1 */
constexpr size_t AecpAemGetStreamInfoCommandPayloadSize = 4u;

/** GET_STREAM_INFO Response - Clause 7.4.16.2 */
constexpr size_t AecpAemGetStreamInfoResponsePayloadSize = 48u;

/** SET_NAME Command - Clause 7.4.17.1 */
constexpr size_t AecpAemSetNameCommandPayloadSize = 72u;

/** SET_NAME Response - Clause 7.4.17.1 */
constexpr size_t AecpAemSetNameResponsePayloadSize = 72u;

/** GET_NAME Command - Clause 7.4.18.1 */
constexpr size_t AecpAemGetNameCommandPayloadSize = 8u;

/** GET_NAME Response - Clause 7.4.17.1 */
constexpr size_t AecpAemGetNameResponsePayloadSize = 72u;

/** START_STREAMING Command - Clause 7.4.35.1 */
constexpr size_t AecpAemStartStreamingCommandPayloadSize = 4u;

/** START_STREAMING Response - Clause 7.4.35.1 */
constexpr size_t AecpAemStartStreamingResponsePayloadSize = 4u;

/** STOP_STREAMING Command - Clause 7.4.36.1 */
constexpr size_t AecpAemStopStreamingCommandPayloadSize = 4u;

/** STOP_STREAMING Response - Clause 7.4.36.1 */
constexpr size_t AecpAemStopStreamingResponsePayloadSize = 4u;

} // namespace aemPayload
} // namespace protocol
} // namespace avdecc
} // namespace la
