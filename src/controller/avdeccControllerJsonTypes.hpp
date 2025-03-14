/*
* Copyright (C) 2016-2025, L-Acoustics and its contributors

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
constexpr auto ControlledEntityState_UnsolSupported = "unsol_supported";
constexpr auto ControlledEntityState_ActiveConfiguration = "active_configuration";

/* ControlledEntityStatistics */
constexpr auto ControlledEntityStatistics_AecpRetryCounter = "aecp_retry_counter";
constexpr auto ControlledEntityStatistics_AecpTimeoutCounter = "aecp_timeout_counter";
constexpr auto ControlledEntityStatistics_AecpUnexpectedResponseCounter = "aecp_unexpected_response_counter";
constexpr auto ControlledEntityStatistics_AecpResponseAverageTime = "aecp_response_average_time";
constexpr auto ControlledEntityStatistics_AemAecpUnsolicitedCounter = "aem_aecp_unsolicited_counter";
constexpr auto ControlledEntityStatistics_AemAecpUnsolicitedLossCounter = "aem_aecp_unsolicited_loss_counter";
constexpr auto ControlledEntityStatistics_EnumerationTime = "enumeration_time";

/* ControlledEntityDiagnostics */
constexpr auto ControlledEntityDiagnostics_RedundancyWarning = "redundancy_warning";
constexpr auto ControlledEntityDiagnostics_StreamInputLatencyErrors = "stream_input_latency_errors";

} // namespace keyName

/* ControlledEntity::CompatibilityFlag conversion */
NLOHMANN_JSON_SERIALIZE_ENUM(ControlledEntity::CompatibilityFlag, {
																																		{ ControlledEntity::CompatibilityFlag::None, "UNKNOWN" },
																																		{ ControlledEntity::CompatibilityFlag::IEEE17221, "IEEE17221" },
																																		{ ControlledEntity::CompatibilityFlag::Milan, "MILAN" },
																																		{ ControlledEntity::CompatibilityFlag::IEEE17221Warning, "IEEE17221WARNING" },
																																		{ ControlledEntity::CompatibilityFlag::MilanWarning, "MILANWARNING" },
																																		{ ControlledEntity::CompatibilityFlag::Misbehaving, "MISBEHAVING" },
																																	});

namespace jsonSerializer
{
namespace keyName
{
/* Controller nodes */
constexpr auto Controller_DumpVersion = "dump_version";
constexpr auto Controller_Entities = "entities";
constexpr auto Controller_Informative_DumpSource = "_dump_source (informative)";

/* ControlledEntity nodes */
constexpr auto ControlledEntity_DumpVersion = "dump_version";
constexpr auto ControlledEntity_Schema = "$schema";
constexpr auto ControlledEntity_CompatibilityFlags = "compatibility_flags";
constexpr auto ControlledEntity_AdpInformation = "adp_information";
constexpr auto ControlledEntity_EntityModel = "entity_model";
constexpr auto ControlledEntity_EntityModelID = "entity_model_id";
constexpr auto ControlledEntity_MilanInformation = "milan_information";
constexpr auto ControlledEntity_EntityState = "state";
constexpr auto ControlledEntity_Statistics = "statistics";
constexpr auto ControlledEntity_Diagnostics = "diagnostics";

} // namespace keyName

namespace keyValue
{
/* Controller nodes */
constexpr auto Controller_DumpVersion = std::uint32_t{ 1 };

/* ControlledEntity nodes */
constexpr auto ControlledEntity_DumpVersion = std::uint32_t{ 2 };
constexpr auto ControlledEntity_SchemaBaseURL = "https://raw.githubusercontent.com/L-Acoustics/avdecc/refs/heads/main/resources/schemas/AVE/";

} // namespace keyValue
} // namespace jsonSerializer
} // namespace controller
} // namespace avdecc
} // namespace la
