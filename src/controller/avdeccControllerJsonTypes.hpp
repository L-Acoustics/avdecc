/*
* Copyright (C) 2016-2019, L-Acoustics and its contributors

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
* @file jsonTypes.hpp
* @author Christophe Calmejane
* @brief Conversion to/from our classes and JSON.
* @note For JSON library: https://github.com/nlohmann/json
*/

#pragma once

#include "la/avdecc/utils.hpp"
#include "la/avdecc/controller/avdeccController.hpp"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace la
{
namespace avdecc
{
namespace controller
{
namespace model
{
namespace keyName
{
/* Globals */
constexpr auto Node_Informative_Index = "_index (informative)";

/* EntityNode */
constexpr auto EntityNode_Static_VendorNameString = "vendor_name_string";
constexpr auto EntityNode_Static_ModelNameString = "model_name_string";
constexpr auto EntityNode_Dynamic_EntityName = "entity_name";
constexpr auto EntityNode_Dynamic_GroupName = "group_name";
constexpr auto EntityNode_Dynamic_FirmwareVersion = "firmware_version";
constexpr auto EntityNode_Dynamic_SerialNumber = "serial_number";
constexpr auto EntityNode_Dynamic_CurrentConfiguration = "current_configuration";
constexpr auto EntityNode_Dynamic_Counters = "counters";

/* ConfigurationNode */
constexpr auto ConfigurationNode_Static_LocalizedDescription = "localized_description";
constexpr auto ConfigurationNode_Dynamic_ObjectName = "object_name";

/* AudioUnitNode */
constexpr auto AudioUnitNode_Static_LocalizedDescription = "localized_description";
constexpr auto AudioUnitNode_Static_ClockDomainIndex = "clock_domain_index";
constexpr auto AudioUnitNode_Static_SamplingRates = "sampling_rates";
constexpr auto AudioUnitNode_Dynamic_ObjectName = "object_name";
constexpr auto AudioUnitNode_Dynamic_CurrentSamplingRate = "current_sampling_rate";

/* StreamNode */
constexpr auto StreamNode_Static_LocalizedDescription = "localized_description";
constexpr auto StreamNode_Static_ClockDomainIndex = "clock_domain_index";
constexpr auto StreamNode_Static_StreamFlags = "stream_flags";
constexpr auto StreamNode_Static_BackupTalkerEntityID0 = "backup_talker_entity_id_0";
constexpr auto StreamNode_Static_BackupTalkerUniqueID0 = "backup_talker_unique_id_0";
constexpr auto StreamNode_Static_BackupTalkerEntityID1 = "backup_talker_entity_id_1";
constexpr auto StreamNode_Static_BackupTalkerUniqueID1 = "backup_talker_unique_id_1";
constexpr auto StreamNode_Static_BackupTalkerEntityID2 = "backup_talker_entity_id_2";
constexpr auto StreamNode_Static_BackupTalkerUniqueID2 = "backup_talker_unique_id_2";
constexpr auto StreamNode_Static_BackedupTalkerEntityID = "backedup_talker_entity_id";
constexpr auto StreamNode_Static_BackedupTalkerUnique = "backedup_talker_unique";
constexpr auto StreamNode_Static_AvbInterfaceIndex = "avb_interface_index";
constexpr auto StreamNode_Static_BufferLength = "buffer_length";
constexpr auto StreamNode_Static_Formats = "formats";
constexpr auto StreamNode_Static_RedundantStreams = "redundant_streams";

/* StreamInputNode */
constexpr auto StreamInputNode_Dynamic_ObjectName = "object_name";
constexpr auto StreamInputNode_Dynamic_StreamInfo = "stream_info";
constexpr auto StreamInputNode_Dynamic_ConnectedTalker = "connected_talker";
constexpr auto StreamInputNode_Dynamic_Counters = "counters";

/* StreamOutputNode */
constexpr auto StreamOutputNode_Dynamic_ObjectName = "object_name";
constexpr auto StreamOutputNode_Dynamic_StreamInfo = "stream_info";
constexpr auto StreamOutputNode_Dynamic_Counters = "counters";

/* AvbInterfaceNode */
constexpr auto AvbInterfaceNode_Static_LocalizedDescription = "localized_description";
constexpr auto AvbInterfaceNode_Static_MacAddress = "mac_address";
constexpr auto AvbInterfaceNode_Static_Flags = "flags";
constexpr auto AvbInterfaceNode_Static_ClockIdentity = "clock_identity";
constexpr auto AvbInterfaceNode_Static_Priority1 = "priority1";
constexpr auto AvbInterfaceNode_Static_ClockClass = "clock_class";
constexpr auto AvbInterfaceNode_Static_OffsetScaledLogVariance = "offset_scaled_log_variance";
constexpr auto AvbInterfaceNode_Static_ClockAccuracy = "clock_accuracy";
constexpr auto AvbInterfaceNode_Static_Priority2 = "priority2";
constexpr auto AvbInterfaceNode_Static_DomainNumber = "domain_number";
constexpr auto AvbInterfaceNode_Static_LogSyncInterval = "log_sync_interval";
constexpr auto AvbInterfaceNode_Static_LogAnnounceInterval = "log_announce_interval";
constexpr auto AvbInterfaceNode_Static_LogPdelayInterval = "log_pdelay_interval";
constexpr auto AvbInterfaceNode_Static_PortNumber = "port_number";
constexpr auto AvbInterfaceNode_Dynamic_ObjectName = "object_name";
constexpr auto AvbInterfaceNode_Dynamic_AvbInfo = "avb_info";
constexpr auto AvbInterfaceNode_Dynamic_AsPath = "as_path";
constexpr auto AvbInterfaceNode_Dynamic_Counters = "counters";

/* ClockSourceNode */
constexpr auto ClockSourceNode_Static_LocalizedDescription = "localized_description";
constexpr auto ClockSourceNode_Static_ClockSourceType = "clock_source_type";
constexpr auto ClockSourceNode_Static_ClockSourceLocationType = "clock_source_location_type";
constexpr auto ClockSourceNode_Static_ClockSourceLocationIndex = "clock_source_location_index";
constexpr auto ClockSourceNode_Dynamic_ObjectName = "object_name";
constexpr auto ClockSourceNode_Dynamic_ClockSourceFlags = "clock_source_flags";
constexpr auto ClockSourceNode_Dynamic_ClockSourceIdentifier = "clock_source_identifier";

/* MemoryObjectNode */
constexpr auto MemoryObjectNode_Static_LocalizedDescription = "localized_description";
constexpr auto MemoryObjectNode_Static_MemoryObjectType = "memory_object_type";
constexpr auto MemoryObjectNode_Static_TargetDescriptorType = "target_descriptor_type";
constexpr auto MemoryObjectNode_Static_TargetDescriptorIndex = "target_descriptor_index";
constexpr auto MemoryObjectNode_Static_StartAddress = "start_address";
constexpr auto MemoryObjectNode_Static_MaximumLength = "maximum_length";
constexpr auto MemoryObjectNode_Dynamic_ObjectName = "object_name";
constexpr auto MemoryObjectNode_Dynamic_Length = "length";

/* LocaleNode */
constexpr auto LocaleNode_Static_LocaleID = "locale_id";
constexpr auto LocaleNode_Static_BaseStringDescriptor = "base_string_descriptor";

/* StringsNode */
constexpr auto StringsNode_Static_Strings = "strings";

/* StreamPortNode */
constexpr auto StreamPortNode_Static_ClockDomainIndex = "clock_domain_index";
constexpr auto StreamPortNode_Static_Flags = "flags";
constexpr auto StreamPortNode_Dynamic_DynamicMappings = "dynamic_mappings";

/* AudioClusterNode */
constexpr auto AudioClusterNode_Static_LocalizedDescription = "localized_description";
constexpr auto AudioClusterNode_Static_SignalType = "signal_type";
constexpr auto AudioClusterNode_Static_SignalIndex = "signal_index";
constexpr auto AudioClusterNode_Static_SignalOutput = "signal_output";
constexpr auto AudioClusterNode_Static_PathLatency = "path_latency";
constexpr auto AudioClusterNode_Static_BlockLatency = "block_latency";
constexpr auto AudioClusterNode_Static_ChannelCount = "channel_count";
constexpr auto AudioClusterNode_Static_Format = "format";
constexpr auto AudioClusterNode_Dynamic_ObjectName = "object_name";

/* AudioMapNode */
constexpr auto AudioMapNode_Static_Mappings = "mappings";

/* ClockDomainNode */
constexpr auto ClockDomainNode_Static_LocalizedDescription = "localized_description";
constexpr auto ClockDomainNode_Static_ClockSources = "clock_sources";
constexpr auto ClockDomainNode_Dynamic_ObjectName = "object_name";
constexpr auto ClockDomainNode_Dynamic_ClockSourceIndex = "clock_source_index";
constexpr auto ClockDomainNode_Dynamic_Counters = "counters";

} // namespace keyName

/* AcquireState conversion */
NLOHMANN_JSON_SERIALIZE_ENUM(AcquireState, {
																						 { AcquireState::Undefined, "UNKNOWN" },
																						 { AcquireState::NotSupported, "NOT_SUPPORTED" },
																						 { AcquireState::NotAcquired, "NOT_ACQUIRED" },
																						 { AcquireState::AcquireInProgress, "ACQUIRE_IN_PROGRESS" },
																						 { AcquireState::Acquired, "ACQUIRED" },
																						 { AcquireState::AcquiredByOther, "ACQUIRED_BY_OTHER" },
																						 { AcquireState::ReleaseInProgress, "RELEASE_IN_PROGRESS" },
																					 });

/* LockState conversion */
NLOHMANN_JSON_SERIALIZE_ENUM(LockState, {
																					{ LockState::Undefined, "UNKNOWN" },
																					{ LockState::NotSupported, "NOT_SUPPORTED" },
																					{ LockState::NotLocked, "NOT_LOCKED" },
																					{ LockState::LockInProgress, "LOCK_IN_PROGRESS" },
																					{ LockState::Locked, "LOCKED" },
																					{ LockState::LockedByOther, "LOCKED_BY_OTHER" },
																					{ LockState::UnlockInProgress, "UNLOCK_IN_PROGRESS" },
																				});

} // namespace model

namespace keyName
{
/* ControlledEntityState */
constexpr auto ControlledEntityState_AcquireState = "acquire_state";
constexpr auto ControlledEntityState_OwningControllerID = "owning_controller_id";
constexpr auto ControlledEntityState_LockState = "lock_state";
constexpr auto ControlledEntityState_LockingControllerID = "locking_controller_id";
constexpr auto ControlledEntityState_SubscribedUnsol = "subscribed_unsol";
constexpr auto ControlledEntityState_ActiveConfiguration = "active_configuration";

/* ControlledEntityStatistics */
constexpr auto ControlledEntityStatistics_AecpRetryCounter = "aecp_retry_counter";
constexpr auto ControlledEntityStatistics_AecpTimeoutCounter = "aecp_timeout_counter";
constexpr auto ControlledEntityStatistics_AecpUnexpectedResponseCounter = "aecp_unexpected_response_counter";
constexpr auto ControlledEntityStatistics_AecpResponseAverageTime = "aecp_response_average_time";
constexpr auto ControlledEntityStatistics_AemAecpUnsolicitedCounter = "aem_aecp_unsolicited_counter";
constexpr auto ControlledEntityStatistics_EnumerationTime = "enumeration_time";

} // namespace keyName

/* ControlledEntity::CompatibilityFlag conversion */
NLOHMANN_JSON_SERIALIZE_ENUM(ControlledEntity::CompatibilityFlag, {
																																		{ ControlledEntity::CompatibilityFlag::None, "UNKNOWN" },
																																		{ ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE17221" },
																																		{ ControlledEntity::CompatibilityFlag::Milan, "MILAN" },
																																		{ ControlledEntity::CompatibilityFlag::Misbehaving, "MISBEHAVING" },
																																	});

namespace jsonSerializer
{
namespace keyName
{
/* Controller nodes */
constexpr auto Controller_DumpVersion = "dump_version";
constexpr auto Controller_Entities = "entities";

/* ControlledEntity nodes */
constexpr auto ControlledEntity_DumpVersion = "dump_version";
constexpr auto ControlledEntity_CompatibilityFlags = "compatibility_flags";
constexpr auto ControlledEntity_AdpInformation = "adp_information";
constexpr auto ControlledEntity_EntityModel = "entity_model";
constexpr auto ControlledEntity_MilanInformation = "milan_information";
constexpr auto ControlledEntity_EntityState = "state";
constexpr auto ControlledEntity_Statistics = "statistics";

} // namespace keyName

namespace keyValue
{
/* Controller nodes */
constexpr auto Controller_DumpVersion = std::uint32_t{ 1 };

/* ControlledEntity nodes */
constexpr auto ControlledEntity_DumpVersion = std::uint32_t{ 1 };

} // namespace keyValue
} // namespace jsonSerializer
} // namespace controller
} // namespace avdecc
} // namespace la
