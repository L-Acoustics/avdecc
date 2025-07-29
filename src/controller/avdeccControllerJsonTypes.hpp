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
#include "la/avdecc/controller/internals/avdeccControlledEntity.hpp"

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
constexpr auto ControlledEntityStatistics_MvuAecpUnsolicitedCounter = "mvu_aecp_unsolicited_counter";
constexpr auto ControlledEntityStatistics_MvuAecpUnsolicitedLossCounter = "mvu_aecp_unsolicited_loss_counter";
constexpr auto ControlledEntityStatistics_EnumerationTime = "enumeration_time";

/* ControlledEntityDiagnostics */
constexpr auto ControlledEntityDiagnostics_RedundancyWarning = "redundancy_warning";
constexpr auto ControlledEntityDiagnostics_StreamInputLatencyErrors = "stream_input_latency_errors";

/* ControlledEntityCompatibilityChangedEvent */
constexpr auto ControlledEntityCompatibilityChangedEvent_PreviousFlags = "previous_flags";
constexpr auto ControlledEntityCompatibilityChangedEvent_NewFlags = "new_flags";
constexpr auto ControlledEntityCompatibilityChangedEvent_PreviousMilanVersion = "previous_milan_version";
constexpr auto ControlledEntityCompatibilityChangedEvent_NewMilanVersion = "new_milan_version";
constexpr auto ControlledEntityCompatibilityChangedEvent_SpecClause = "spec_clause";
constexpr auto ControlledEntityCompatibilityChangedEvent_Message = "message";
constexpr auto ControlledEntityCompatibilityChangedEvent_Timestamp = "timestamp";

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

/* CompatibilityChangedEvent conversion */
inline void to_json(json& j, ControlledEntity::CompatibilityChangedEvent const& event)
{
	j[keyName::ControlledEntityCompatibilityChangedEvent_PreviousFlags] = event.previousFlags;
	j[keyName::ControlledEntityCompatibilityChangedEvent_NewFlags] = event.newFlags;
	j[keyName::ControlledEntityCompatibilityChangedEvent_PreviousMilanVersion] = static_cast<std::string>(event.previousMilanVersion);
	j[keyName::ControlledEntityCompatibilityChangedEvent_NewMilanVersion] = static_cast<std::string>(event.newMilanVersion);
	j[keyName::ControlledEntityCompatibilityChangedEvent_SpecClause] = event.specClause;
	j[keyName::ControlledEntityCompatibilityChangedEvent_Message] = event.message;
	j[keyName::ControlledEntityCompatibilityChangedEvent_Timestamp] = std::chrono::duration_cast<std::chrono::milliseconds>(event.timestamp.time_since_epoch()).count(); // Force as milliseconds, as system_clock::time_point is implementation defined
}
inline void from_json(json const& j, ControlledEntity::CompatibilityChangedEvent& event)
{
	j.at(keyName::ControlledEntityCompatibilityChangedEvent_PreviousFlags).get_to(event.previousFlags);
	j.at(keyName::ControlledEntityCompatibilityChangedEvent_NewFlags).get_to(event.newFlags);
	{
		auto const str = j.at(keyName::ControlledEntityCompatibilityChangedEvent_PreviousMilanVersion).get<std::string>();
		event.previousMilanVersion = entity::model::MilanVersion{ str };
	}
	{
		auto const str = j.at(keyName::ControlledEntityCompatibilityChangedEvent_NewMilanVersion).get<std::string>();
		event.newMilanVersion = entity::model::MilanVersion{ str };
	}
	j.at(keyName::ControlledEntityCompatibilityChangedEvent_SpecClause).get_to(event.specClause);
	j.at(keyName::ControlledEntityCompatibilityChangedEvent_Message).get_to(event.message);
	{
		auto const durationAsMs = j.at(keyName::ControlledEntityCompatibilityChangedEvent_Timestamp).get<std::chrono::milliseconds::rep>(); // Get as milliseconds
		// Convert to system_clock::time_point (which is implementation defined)
		event.timestamp = std::chrono::system_clock::time_point{ std::chrono::system_clock::duration{ durationAsMs } };
	}
}


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
constexpr auto ControlledEntity_MilanCompatibilityVersion = "milan_compatibility_version";
constexpr auto ControlledEntity_CompatibilityEvents = "compatibility_events";
constexpr auto ControlledEntity_AdpInformation = "adp_information";
constexpr auto ControlledEntity_EntityModel = "entity_model";
constexpr auto ControlledEntity_EntityModelID = "entity_model_id";
constexpr auto ControlledEntity_MilanInformation = "milan_information";
constexpr auto ControlledEntity_MilanDynamicState = "milan_dynamic_state";
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
