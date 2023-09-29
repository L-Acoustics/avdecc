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
* @file jsonTypes.hpp
* @author Christophe Calmejane
* @brief Conversion to/from our classes and JSON.
* @note For JSON library: https://github.com/nlohmann/json
*/

#pragma once

#include "jsonSerialization.hpp"
#include "entityModelControlValues.hpp"
#include "entityModelControlValuesTraits.hpp"
#include "logItems.hpp"

#include "la/avdecc/utils.hpp"
#include "la/avdecc/avdecc.hpp"

#include <nlohmann/json.hpp>

#include <optional>
#include <chrono>
#include <unordered_map>
#include <stdexcept> // invalid_argument
#include <limits> // quiet_NaN // infinity

using json = nlohmann::json;

namespace la
{
namespace avdecc
{
namespace logger
{
inline void logJsonSerializer(Level const level, std::string const& message)
{
	auto const item = LogItemJsonSerializer{ message };
	Logger::getInstance().logItem(level, &item);
}

} // namespace logger
} // namespace avdecc
} // namespace la

namespace nlohmann
{
/** float converter that supports NaN and inf values*/
template<>
struct adl_serializer<float>
{
	static void to_json(json& j, float const& v)
	{
		// Handle NaN and inf values
		if (std::isnan(v))
		{
			j = json::object({ { "NaN", true } });
			return;
		}
		else if (std::isinf(v))
		{
			if (v > 0.0f)
			{
				j = json::object({ { "Inf", true } });
			}
			else
			{
				j = json::object({ { "-Inf", true } });
			}
			return;
		}

		// Default behavior
		::nlohmann::to_json(j, v);
	}
	static void from_json(json const& j, float& v)
	{
		if (j.is_object())
		{
			auto const& o = j.get<json::object_t>();
			if (o.size() == 1)
			{
				auto const& it = o.begin();
				// Check the only value is a boolean and is true
				auto const& bVal = it->second;
				if (bVal.is_boolean() && bVal.get<bool>())
				{
					if (it->first == "NaN")
					{
						v = std::numeric_limits<float>::quiet_NaN();
						return;
					}
					else if (it->first == "Inf")
					{
						v = std::numeric_limits<float>::infinity();
						return;
					}
					else if (it->first == "-Inf")
					{
						v = -std::numeric_limits<float>::infinity();
						return;
					}
				}
			}
		}

		// Default behavior
		::nlohmann::from_json(j, v);
	}
};

/** std::optional converter */
template<typename T>
struct adl_serializer<std::optional<T>>
{
	static void to_json(json& j, std::optional<T> const& opt)
	{
		if (opt)
		{
			j = *opt;
		}
		else
		{
			j = nullptr;
		}
	}
	static void from_json(json const& j, std::optional<T>& opt)
	{
		if (!j.is_null())
		{
			opt = j.get<T>();
		}
	}
};

/** std::chrono::milliseconds converter */
template<>
struct adl_serializer<std::chrono::milliseconds>
{
	static void to_json(json& j, std::chrono::milliseconds const& value)
	{
		j = value.count();
	}
	static void from_json(json const& j, std::chrono::milliseconds& value)
	{
		value = std::chrono::milliseconds{ j.get<std::chrono::milliseconds::rep>() };
	}
};

/** converter for an optionnaly defined value */
template<typename KeyT, typename ValueType>
inline void get_optional_value(json const& j, KeyT&& key, ValueType& v)
{
	auto const it = j.find(std::forward<KeyT>(key));
	if (it != j.end())
	{
		it->get_to(v);
	}
}

/** la::avdecc::entity::model::EntityCounters converter */
template<>
struct adl_serializer<la::avdecc::entity::model::EntityCounters>
{
	static void to_json(json& j, la::avdecc::entity::model::EntityCounters const& counters)
	{
		auto object = json::object();

		for (auto const& [name, value] : counters)
		{
			json const n = name; // Must use operator= instead of constructor to force usage of the to_json overload
			object[n.get<std::string>()] = value;
		}

		j = std::move(object);
	}
	static void from_json(json const& j, la::avdecc::entity::model::EntityCounters& counters)
	{
		for (auto const& [name, value] : j.items())
		{
			json const n = name; // Must use operator= instead of constructor to force usage of the to_json overload
			auto const key = n.get<la::avdecc::entity::model::EntityCounters::key_type>();
			// Check if key is a valid CounterValidFlag enum
			if (key == la::avdecc::entity::model::EntityCounters::key_type::None)
			{
				logJsonSerializer(la::avdecc::logger::Level::Warn, std::string("Unknown EntityCounterValidFlag name: ") + name);
				counters.insert(std::make_pair(static_cast<la::avdecc::entity::model::EntityCounters::key_type>(la::avdecc::utils::convertFromString<la::avdecc::entity::model::DescriptorCounterValidFlag>(name.c_str())), value.get<la::avdecc::entity::model::EntityCounters::mapped_type>()));
			}
			else
			{
				counters.insert(std::make_pair(key, value.get<la::avdecc::entity::model::EntityCounters::mapped_type>()));
			}
		}
	}
};

/** la::avdecc::entity::model::AvbInterfaceCounters converter */
template<>
struct adl_serializer<la::avdecc::entity::model::AvbInterfaceCounters>
{
	static void to_json(json& j, la::avdecc::entity::model::AvbInterfaceCounters const& counters)
	{
		auto object = json::object();

		for (auto const& [name, value] : counters)
		{
			json const n = name; // Must use operator= instead of constructor to force usage of the to_json overload
			auto const key = n.get<std::string>();
			if (key == "UNKNOWN")
			{
				object[la::avdecc::utils::toHexString(la::avdecc::utils::to_integral(name), true, true)] = value;
			}
			else
			{
				object[key] = value;
			}
		}

		j = std::move(object);
	}
	static void from_json(json const& j, la::avdecc::entity::model::AvbInterfaceCounters& counters)
	{
		for (auto const& [name, value] : j.items())
		{
			json const n = name; // Must use operator= instead of constructor to force usage of the to_json overload
			auto const key = n.get<la::avdecc::entity::model::AvbInterfaceCounters::key_type>();
			// Check if key is a valid CounterValidFlag enum
			if (key == la::avdecc::entity::model::AvbInterfaceCounters::key_type::None)
			{
				logJsonSerializer(la::avdecc::logger::Level::Warn, std::string("Unknown AvbInterfaceCounterValidFlag name: ") + name);
				counters.insert(std::make_pair(static_cast<la::avdecc::entity::model::AvbInterfaceCounters::key_type>(la::avdecc::utils::convertFromString<la::avdecc::entity::model::DescriptorCounterValidFlag>(name.c_str())), value.get<la::avdecc::entity::model::AvbInterfaceCounters::mapped_type>()));
			}
			else
			{
				counters.insert(std::make_pair(key, value.get<la::avdecc::entity::model::AvbInterfaceCounters::mapped_type>()));
			}
		}
	}
};

/** la::avdecc::entity::model::ClockDomainCounters converter */
template<>
struct adl_serializer<la::avdecc::entity::model::ClockDomainCounters>
{
	static void to_json(json& j, la::avdecc::entity::model::ClockDomainCounters const& counters)
	{
		auto object = json::object();

		for (auto const& [name, value] : counters)
		{
			json const n = name; // Must use operator= instead of constructor to force usage of the to_json overload
			auto const key = n.get<std::string>();
			if (key == "UNKNOWN")
			{
				object[la::avdecc::utils::toHexString(la::avdecc::utils::to_integral(name), true, true)] = value;
			}
			else
			{
				object[key] = value;
			}
		}

		j = std::move(object);
	}
	static void from_json(json const& j, la::avdecc::entity::model::ClockDomainCounters& counters)
	{
		for (auto const& [name, value] : j.items())
		{
			json const n = name; // Must use operator= instead of constructor to force usage of the to_json overload
			auto const key = n.get<la::avdecc::entity::model::ClockDomainCounters::key_type>();
			// Check if key is a valid CounterValidFlag enum
			if (key == la::avdecc::entity::model::ClockDomainCounters::key_type::None)
			{
				logJsonSerializer(la::avdecc::logger::Level::Warn, std::string("Unknown ClockDomainCounterValidFlag name: ") + name);
				counters.insert(std::make_pair(static_cast<la::avdecc::entity::model::ClockDomainCounters::key_type>(la::avdecc::utils::convertFromString<la::avdecc::entity::model::DescriptorCounterValidFlag>(name.c_str())), value.get<la::avdecc::entity::model::ClockDomainCounters::mapped_type>()));
			}
			else
			{
				counters.insert(std::make_pair(key, value.get<la::avdecc::entity::model::ClockDomainCounters::mapped_type>()));
			}
		}
	}
};

/** la::avdecc::entity::model::StreamInputCounters converter */
template<>
struct adl_serializer<la::avdecc::entity::model::StreamInputCounters>
{
	static void to_json(json& j, la::avdecc::entity::model::StreamInputCounters const& counters)
	{
		auto object = json::object();

		for (auto const& [name, value] : counters)
		{
			json const n = name; // Must use operator= instead of constructor to force usage of the to_json overload
			auto const key = n.get<std::string>();
			if (key == "UNKNOWN")
			{
				object[la::avdecc::utils::toHexString(la::avdecc::utils::to_integral(name), true, true)] = value;
			}
			else
			{
				object[key] = value;
			}
		}

		j = std::move(object);
	}
	static void from_json(json const& j, la::avdecc::entity::model::StreamInputCounters& counters)
	{
		for (auto const& [name, value] : j.items())
		{
			json const n = name; // Must use operator= instead of constructor to force usage of the to_json overload
			auto const key = n.get<la::avdecc::entity::model::StreamInputCounters::key_type>();
			// Check if key is a valid CounterValidFlag enum
			if (key == la::avdecc::entity::model::StreamInputCounters::key_type::None)
			{
				logJsonSerializer(la::avdecc::logger::Level::Warn, std::string("Unknown StreamInputCounterValidFlag name: ") + name);
				counters.insert(std::make_pair(static_cast<la::avdecc::entity::model::StreamInputCounters::key_type>(la::avdecc::utils::convertFromString<la::avdecc::entity::model::DescriptorCounterValidFlag>(name.c_str())), value.get<la::avdecc::entity::model::StreamInputCounters::mapped_type>()));
			}
			else
			{
				counters.insert(std::make_pair(key, value.get<la::avdecc::entity::model::StreamInputCounters::mapped_type>()));
			}
		}
	}
};

/** la::avdecc::entity::model::StreamOutputCounters converter */
template<>
struct adl_serializer<la::avdecc::entity::model::StreamOutputCounters>
{
	static void to_json(json& j, la::avdecc::entity::model::StreamOutputCounters const& counters)
	{
		auto object = json::object();

		for (auto const& [name, value] : counters)
		{
			json const n = name; // Must use operator= instead of constructor to force usage of the to_json overload
			auto const key = n.get<std::string>();
			if (key == "UNKNOWN")
			{
				object[la::avdecc::utils::toHexString(la::avdecc::utils::to_integral(name), true, true)] = value;
			}
			else
			{
				object[key] = value;
			}
		}

		j = std::move(object);
	}
	static void from_json(json const& j, la::avdecc::entity::model::StreamOutputCounters& counters)
	{
		for (auto const& [name, value] : j.items())
		{
			json const n = name; // Must use operator= instead of constructor to force usage of the to_json overload
			auto const key = n.get<la::avdecc::entity::model::StreamOutputCounters::key_type>();
			// Check if key is a valid CounterValidFlag enum
			if (key == la::avdecc::entity::model::StreamOutputCounters::key_type::None)
			{
				logJsonSerializer(la::avdecc::logger::Level::Warn, std::string("Unknown StreamOutputCounterValidFlag name: ") + name);
				counters.insert(std::make_pair(static_cast<la::avdecc::entity::model::StreamOutputCounters::key_type>(la::avdecc::utils::convertFromString<la::avdecc::entity::model::DescriptorCounterValidFlag>(name.c_str())), value.get<la::avdecc::entity::model::StreamOutputCounters::mapped_type>()));
			}
			else
			{
				counters.insert(std::make_pair(key, value.get<la::avdecc::entity::model::StreamOutputCounters::mapped_type>()));
			}
		}
	}
};

} // namespace nlohmann

namespace la
{
namespace avdecc
{
namespace utils
{
/* TypedDefined conversion */
template<typename Define, typename DerivedType = typename Define::derived_type, typename ValueType = typename Define::value_type, typename = std::enable_if_t<std::is_base_of_v<TypedDefine<DerivedType, ValueType>, Define>>>
void from_json(json const& j, Define& value)
{
	value.fromString(j.get<std::string>());
}

/* EnumBitfield conversion */
template<typename Enum, typename EnumValue = typename Enum::value_type, EnumValue DefaultValue = EnumValue::None, typename = std::enable_if_t<std::is_same_v<Enum, EnumBitfield<EnumValue>>>>
void to_json(json& j, Enum const& flags)
{
	for (auto const flag : flags)
	{
		json const conv = flag; // Must use operator= instead of constructor to force usage of the to_json overload
		AVDECC_ASSERT(conv.is_string(), "Converted Enum should be a string (forgot to define the enum using NLOHMANN_JSON_SERIALIZE_ENUM?");
		if (conv.get<EnumValue>() != DefaultValue)
		{
			j.push_back(flag);
		}
		else
		{
			auto const value = toHexString(utils::to_integral(flag), true, true);
			logJsonSerializer(la::avdecc::logger::Level::Warn, std::string("Unknown Enum value: ") + value);
			j.push_back(value);
		}
	}
}
template<typename Enum, typename EnumValue = typename Enum::value_type, EnumValue DefaultValue = EnumValue::None, typename = std::enable_if_t<std::is_same_v<Enum, EnumBitfield<EnumValue>>>>
void from_json(json const& j, Enum& flags)
{
	for (auto const& o : j)
	{
		auto const v = o.get<EnumValue>();
		if (v == DefaultValue)
		{
			auto const value = o.get<std::string>();
			logJsonSerializer(la::avdecc::logger::Level::Warn, std::string("Unknown Enum value: ") + value);
			using UnderlyingEnumType = typename Enum::underlying_value_type;
			if constexpr (sizeof(UnderlyingEnumType) == 1)
			{
				flags.set(static_cast<EnumValue>(la::avdecc::utils::convertFromString<std::uint16_t>(value.c_str())));
			}
			else
			{
				flags.set(static_cast<EnumValue>(la::avdecc::utils::convertFromString<UnderlyingEnumType>(value.c_str())));
			}
		}
		else
		{
			flags.set(v);
		}
	}
}

} // namespace utils

/* UniqueIdentifier conversion */
inline void to_json(json& j, UniqueIdentifier const& eid)
{
	j = utils::toHexString(eid.getValue(), true, true);
}
inline void from_json(json const& j, UniqueIdentifier& eid)
{
	eid.setValue(utils::convertFromString<UniqueIdentifier::value_type>(j.get<std::string>().c_str()));
}

namespace entity
{
namespace keyName
{
/* Entity::CommonInformation */
constexpr auto Entity_CommonInformation_Node = "common";
constexpr auto Entity_CommonInformation_EntityID = "entity_id";
constexpr auto Entity_CommonInformation_EntityModelID = "entity_model_id";
constexpr auto Entity_CommonInformation_EntityCapabilities = "entity_capabilities";
constexpr auto Entity_CommonInformation_TalkerStreamSources = "talker_stream_sources";
constexpr auto Entity_CommonInformation_TalkerCapabilities = "talker_capabilities";
constexpr auto Entity_CommonInformation_ListenerStreamSinks = "listener_stream_sinks";
constexpr auto Entity_CommonInformation_ListenerCapabilities = "listener_capabilities";
constexpr auto Entity_CommonInformation_ControllerCapabilities = "controller_capabilities";
constexpr auto Entity_CommonInformation_IdentifyControlIndex = "identify_control_index";
constexpr auto Entity_CommonInformation_AssociationID = "association_id";

/* Entity::InterfaceInformation */
constexpr auto Entity_InterfaceInformation_Node = "interfaces";
constexpr auto Entity_InterfaceInformation_AvbInterfaceIndex = "avb_interface_index";
constexpr auto Entity_InterfaceInformation_MacAddress = "mac_address";
constexpr auto Entity_InterfaceInformation_ValidTime = "valid_time";
constexpr auto Entity_InterfaceInformation_AvailableIndex = "available_index";
constexpr auto Entity_InterfaceInformation_GptpGrandmasterID = "gptp_grandmaster_id";
constexpr auto Entity_InterfaceInformation_GptpDomainNumber = "gptp_domain_number";

} // namespace keyName

/* EntityCapability conversion */
NLOHMANN_JSON_SERIALIZE_ENUM(EntityCapability, {
																								 { EntityCapability::None, "UNKNOWN" },
																								 { EntityCapability::EfuMode, "EFU_MODE" },
																								 { EntityCapability::AddressAccessSupported, "ADDRESS_ACCESS_SUPPORTED" },
																								 { EntityCapability::GatewayEntity, "GATEWAY_ENTITY" },
																								 { EntityCapability::AemSupported, "AEM_SUPPORTED" },
																								 { EntityCapability::LegacyAvc, "LEGACY_AVC" },
																								 { EntityCapability::AssociationIDSupported, "ASSOCIATION_ID_SUPPORTED" },
																								 { EntityCapability::AssociationIDValid, "ASSOCIATION_ID_VALID" },
																								 { EntityCapability::VendorUniqueSupported, "VENDOR_UNIQUE_SUPPORTED" },
																								 { EntityCapability::ClassASupported, "CLASS_A_SUPPORTED" },
																								 { EntityCapability::ClassBSupported, "CLASS_B_SUPPORTED" },
																								 { EntityCapability::GptpSupported, "GPTP_SUPPORTED" },
																								 { EntityCapability::AemAuthenticationSupported, "AEM_AUTHENTICATION_SUPPORTED" },
																								 { EntityCapability::AemAuthenticationRequired, "AEM_AUTHENTICATION_REQUIRED" },
																								 { EntityCapability::AemPersistentAcquireSupported, "AEM_PERSISTENT_ACQUIRE_SUPPORTED" },
																								 { EntityCapability::AemIdentifyControlIndexValid, "AEM_IDENTIFY_CONTROL_INDEX_VALID" },
																								 { EntityCapability::AemInterfaceIndexValid, "AEM_INTERFACE_INDEX_VALID" },
																								 { EntityCapability::GeneralControllerIgnore, "GENERAL_CONTROLLER_IGNORE" },
																								 { EntityCapability::EntityNotReady, "ENTITY_NOT_READY" },
																							 });

/* TalkerCapability conversion */
NLOHMANN_JSON_SERIALIZE_ENUM(TalkerCapability, {
																								 { TalkerCapability::None, "UNKNOWN" },
																								 { TalkerCapability::Implemented, "IMPLEMENTED" },
																								 { TalkerCapability::OtherSource, "OTHER_SOURCE" },
																								 { TalkerCapability::ControlSource, "CONTROL_SOURCE" },
																								 { TalkerCapability::MediaClockSource, "MEDIA_CLOCK_SOURCE" },
																								 { TalkerCapability::SmpteSource, "SMPTE_SOURCE" },
																								 { TalkerCapability::MidiSource, "MIDI_SOURCE" },
																								 { TalkerCapability::AudioSource, "AUDIO_SOURCE" },
																								 { TalkerCapability::VideoSource, "VIDEO_SOURCE" },
																							 });

/* ListenerCapability conversion */
NLOHMANN_JSON_SERIALIZE_ENUM(ListenerCapability, {
																									 { ListenerCapability::None, "UNKNOWN" },
																									 { ListenerCapability::Implemented, "IMPLEMENTED" },
																									 { ListenerCapability::OtherSink, "OTHER_SINK" },
																									 { ListenerCapability::ControlSink, "CONTROL_SINK" },
																									 { ListenerCapability::MediaClockSink, "MEDIA_CLOCK_SINK" },
																									 { ListenerCapability::SmpteSink, "SMPTE_SINK" },
																									 { ListenerCapability::MidiSink, "MIDI_SINK" },
																									 { ListenerCapability::AudioSink, "AUDIO_SINK" },
																									 { ListenerCapability::VideoSink, "VIDEO_SINK" },
																								 });

/* ControllerCapability conversion */
NLOHMANN_JSON_SERIALIZE_ENUM(ControllerCapability, {
																										 { ControllerCapability::None, "UNKNOWN" },
																										 { ControllerCapability::Implemented, "IMPLEMENTED" },
																									 });

/* ConnectionFlag conversion */
NLOHMANN_JSON_SERIALIZE_ENUM(ConnectionFlag, {
																							 { ConnectionFlag::None, "UNKNOWN" },
																							 { ConnectionFlag::ClassB, "CLASS_B" },
																							 { ConnectionFlag::FastConnect, "FAST_CONNECT" },
																							 { ConnectionFlag::SavedState, "SAVED_STATE" },
																							 { ConnectionFlag::StreamingWait, "STREAMING_WAIT" },
																							 { ConnectionFlag::SupportsEncrypted, "SUPPORTS_ENCRYPTED" },
																							 { ConnectionFlag::EncryptedPdu, "ENCRYPTED_PDU" },
																							 { ConnectionFlag::TalkerFailed, "TALKER_FAILED" },
																						 });

/* StreamFlag conversion */
NLOHMANN_JSON_SERIALIZE_ENUM(StreamFlag, {
																					 { StreamFlag::None, "UNKNOWN" },
																					 { StreamFlag::ClockSyncSource, "CLOCK_SYNC_SOURCE" },
																					 { StreamFlag::ClassA, "CLASS_A" },
																					 { StreamFlag::ClassB, "CLASS_B" },
																					 { StreamFlag::SupportsEncrypted, "SUPPORTS_ENCRYPTED" },
																					 { StreamFlag::PrimaryBackupSupported, "PRIMARY_BACKUP_SUPPORTED" },
																					 { StreamFlag::PrimaryBackupValid, "PRIMARY_BACKUP_VALID" },
																					 { StreamFlag::SecondaryBackupSupported, "SECONDARY_BACKUP_SUPPORTED" },
																					 { StreamFlag::SecondaryBackupValid, "SECONDARY_BACKUP_VALID" },
																					 { StreamFlag::TertiaryBackupSupported, "TERTIARY_BACKUP_SUPPORTED" },
																					 { StreamFlag::TertiaryBackupValid, "TERTIARY_BACKUP_VALID" },
																				 });

/* JackFlag conversion */
NLOHMANN_JSON_SERIALIZE_ENUM(JackFlag, {
																				 { JackFlag::None, "UNKNOWN" },
																				 { JackFlag::ClockSyncSource, "CLOCK_SYNC_SOURCE" },
																				 { JackFlag::Captive, "CAPTIVE" },
																			 });

/* AvbInterfaceFlag conversion */
NLOHMANN_JSON_SERIALIZE_ENUM(AvbInterfaceFlag, {
																								 { AvbInterfaceFlag::None, "UNKNOWN" },
																								 { AvbInterfaceFlag::GptpGrandmasterSupported, "GPTP_GRANDMASTER_SUPPORTED" },
																								 { AvbInterfaceFlag::GptpSupported, "GPTP_SUPPORTED" },
																								 { AvbInterfaceFlag::SrpSupported, "SRP_SUPPORTED" },
																							 });

/* ClockSourceFlag conversion */
NLOHMANN_JSON_SERIALIZE_ENUM(ClockSourceFlag, {
																								{ ClockSourceFlag::None, "UNKNOWN" },
																								{ ClockSourceFlag::StreamID, "STREAM_ID" },
																								{ ClockSourceFlag::LocalID, "LOCAL_ID" },
																							});

/* PortFlag conversion */
NLOHMANN_JSON_SERIALIZE_ENUM(PortFlag, {
																				 { PortFlag::None, "UNKNOWN" },
																				 { PortFlag::ClockSyncSource, "CLOCK_SYNC_SOURCE" },
																				 { PortFlag::AsyncSampleRateConv, "ASYNC_SAMPLE_RATE_CONV" },
																				 { PortFlag::SyncSampleRateConv, "SYNC_SAMPLE_RATE_CONV" },
																			 });

/* PtpInstanceFlag conversion */
NLOHMANN_JSON_SERIALIZE_ENUM(PtpInstanceFlag, {
																								{ PtpInstanceFlag::None, "UNKNOWN" },
																								{ PtpInstanceFlag::CanSetInstanceEnable, "CAN_SET_INSTANCE_ENABLE" },
																								{ PtpInstanceFlag::CanSetPriority1, "CAN_SET_PRIORITY_1" },
																								{ PtpInstanceFlag::CanSetPriority2, "CAN_SET_PRIORITY_2" },
																								{ PtpInstanceFlag::CanSetDomainNumber, "CAN_SET_DOMAIN_NUMBER" },
																								{ PtpInstanceFlag::CanSetExternalPortConfiguration, "CAN_SET_EXTERNAL_PORT_CONFIGURATION" },
																								{ PtpInstanceFlag::CanSetSlaveOnly, "CAN_SET_SLAVE_ONLY" },
																								{ PtpInstanceFlag::CanEnablePerformance, "CAN_ENABLE_PERFORMANCE" },
																								{ PtpInstanceFlag::PerformanceMonitoring, "PERFORMANCE_MONITORING" },
																								{ PtpInstanceFlag::GrandmasterCapable, "GRANDMASTER_CAPABLE" },
																							});

/* PtpPortFlag conversion */
NLOHMANN_JSON_SERIALIZE_ENUM(PtpPortFlag, {
																						{ PtpPortFlag::None, "UNKNOWN" },
																						{ PtpPortFlag::CanSetEnable, "CAN_SET_ENABLE" },
																						{ PtpPortFlag::CanSetLinkDelayThreshold, "CAN_SET_LINK_DELAY_THRESHOLD" },
																						{ PtpPortFlag::CanSetDelayMechanism, "CAN_SET_DELAY_MECHANISM" },
																						{ PtpPortFlag::CanSetDelayAsymmetry, "CAN_SET_DELAY_ASYMMETRY" },
																						{ PtpPortFlag::CanSetInitialMessageIntervals, "CAN_SET_INITIAL_MESSAGE_INTERVALS" },
																						{ PtpPortFlag::CanSetTimeouts, "CAN_SET_TIMEOUTS" },
																						{ PtpPortFlag::CanOverrideAnnounceInterval, "CAN_OVERRIDE_ANNOUNCE_INTERVAL" },
																						{ PtpPortFlag::CanOverrideSyncInterval, "CAN_OVERRIDE_SYNC_INTERVAL" },
																						{ PtpPortFlag::CanOverridePDelayInterval, "CAN_OVERRIDE_PDELAY_INTERVAL" },
																						{ PtpPortFlag::CanOverrideGptpCapableInterval, "CAN_OVERRIDE_GPTP_CAPABLE_INTERVAL" },
																						{ PtpPortFlag::CanOverrideComputeNeighbor, "CAN_OVERRIDE_COMPUTE_NEIGHBOR" },
																						{ PtpPortFlag::CanOverrideComputeLinkDelay, "CAN_OVERRIDE_COMPUTE_LINK_DELAY" },
																						{ PtpPortFlag::CanOverrideOnestep, "CAN_OVERRIDE_ONESTEP" },
																						{ PtpPortFlag::SupportsRemoteIntervalSignal, "SUPPORTS_REMOTE_INTERVAL_SIGNAL" },
																						{ PtpPortFlag::SupportsOnestepTransmit, "SUPPORTS_ONESTEP_TRANSMIT" },
																						{ PtpPortFlag::SupportsOnestepReceive, "SUPPORTS_ONESTEP_RECEIVE" },
																						{ PtpPortFlag::SupportsUnicastNegotiate, "SUPPORTS_UNICAST_NEGOTIATE" },
																					});

/* StreamInfoFlag conversion */
NLOHMANN_JSON_SERIALIZE_ENUM(StreamInfoFlag, {
																							 { StreamInfoFlag::None, "UNKNOWN" },
																							 { StreamInfoFlag::ClassB, "CLASS_B" },
																							 { StreamInfoFlag::FastConnect, "FAST_CONNECT" },
																							 { StreamInfoFlag::SavedState, "SAVED_STATE" },
																							 { StreamInfoFlag::StreamingWait, "STREAMING_WAIT" },
																							 { StreamInfoFlag::SupportsEncrypted, "SUPPORTS_ENCRYPTED" },
																							 { StreamInfoFlag::EncryptedPdu, "ENCRYPTED_PDU" },
																							 { StreamInfoFlag::TalkerFailed, "TALKER_FAILED" },
																							 { StreamInfoFlag::StreamVlanIDValid, "STREAM_VLAN_ID_VALID" },
																							 { StreamInfoFlag::Connected, "CONNECTED" },
																							 { StreamInfoFlag::MsrpFailureValid, "MSRP_FAILURE_VALID" },
																							 { StreamInfoFlag::StreamDestMacValid, "STREAM_DEST_MAC_VALID" },
																							 { StreamInfoFlag::MsrpAccLatValid, "MSRP_ACC_LAT_VALID" },
																							 { StreamInfoFlag::StreamIDValid, "STREAM_ID_VALID" },
																							 { StreamInfoFlag::StreamFormatValid, "STREAM_FORMAT_VALID" },
																						 });

/* StreamInfoFlagEx conversion */
NLOHMANN_JSON_SERIALIZE_ENUM(StreamInfoFlagEx, {
																								 { StreamInfoFlagEx::None, "UNKNOWN" },
																								 { StreamInfoFlagEx::Registering, "REGISTERING" },
																							 });

/* AvbInfoFlag conversion */
NLOHMANN_JSON_SERIALIZE_ENUM(AvbInfoFlag, {
																						{ AvbInfoFlag::None, "UNKNOWN" },
																						{ AvbInfoFlag::AsCapable, "AS_CAPABLE" },
																						{ AvbInfoFlag::GptpEnabled, "GPTP_ENABLED" },
																						{ AvbInfoFlag::SrpEnabled, "SRP_ENABLED" },
																					});

/* EntityCounterValidFlag conversion */
NLOHMANN_JSON_SERIALIZE_ENUM(EntityCounterValidFlag, {
																											 { EntityCounterValidFlag::None, "UNKNOWN" },
																											 { EntityCounterValidFlag::EntitySpecific8, "ENTITY_SPECIFIC_8" },
																											 { EntityCounterValidFlag::EntitySpecific7, "ENTITY_SPECIFIC_7" },
																											 { EntityCounterValidFlag::EntitySpecific6, "ENTITY_SPECIFIC_6" },
																											 { EntityCounterValidFlag::EntitySpecific5, "ENTITY_SPECIFIC_5" },
																											 { EntityCounterValidFlag::EntitySpecific4, "ENTITY_SPECIFIC_4" },
																											 { EntityCounterValidFlag::EntitySpecific3, "ENTITY_SPECIFIC_3" },
																											 { EntityCounterValidFlag::EntitySpecific2, "ENTITY_SPECIFIC_2" },
																											 { EntityCounterValidFlag::EntitySpecific1, "ENTITY_SPECIFIC_1" },
																										 });

/* AvbInterfaceCounterValidFlag conversion */
NLOHMANN_JSON_SERIALIZE_ENUM(AvbInterfaceCounterValidFlag, {
																														 { AvbInterfaceCounterValidFlag::None, "UNKNOWN" },
																														 { AvbInterfaceCounterValidFlag::LinkUp, "LINK_UP" },
																														 { AvbInterfaceCounterValidFlag::LinkDown, "LINK_DOWN" },
																														 { AvbInterfaceCounterValidFlag::FramesTx, "FRAMES_TX" },
																														 { AvbInterfaceCounterValidFlag::FramesRx, "FRAMES_RX" },
																														 { AvbInterfaceCounterValidFlag::RxCrcError, "RX_CRC_ERROR" },
																														 { AvbInterfaceCounterValidFlag::GptpGmChanged, "GPTP_GM_CHANGED" },
																														 { AvbInterfaceCounterValidFlag::EntitySpecific8, "ENTITY_SPECIFIC_8" },
																														 { AvbInterfaceCounterValidFlag::EntitySpecific7, "ENTITY_SPECIFIC_7" },
																														 { AvbInterfaceCounterValidFlag::EntitySpecific6, "ENTITY_SPECIFIC_6" },
																														 { AvbInterfaceCounterValidFlag::EntitySpecific5, "ENTITY_SPECIFIC_5" },
																														 { AvbInterfaceCounterValidFlag::EntitySpecific4, "ENTITY_SPECIFIC_4" },
																														 { AvbInterfaceCounterValidFlag::EntitySpecific3, "ENTITY_SPECIFIC_3" },
																														 { AvbInterfaceCounterValidFlag::EntitySpecific2, "ENTITY_SPECIFIC_2" },
																														 { AvbInterfaceCounterValidFlag::EntitySpecific1, "ENTITY_SPECIFIC_1" },
																													 });

/* ClockDomainCounterValidFlag conversion */
NLOHMANN_JSON_SERIALIZE_ENUM(ClockDomainCounterValidFlag, {
																														{ ClockDomainCounterValidFlag::None, "UNKNOWN" },
																														{ ClockDomainCounterValidFlag::Locked, "LOCKED" },
																														{ ClockDomainCounterValidFlag::Unlocked, "UNLOCKED" },
																														{ ClockDomainCounterValidFlag::EntitySpecific8, "ENTITY_SPECIFIC_8" },
																														{ ClockDomainCounterValidFlag::EntitySpecific7, "ENTITY_SPECIFIC_7" },
																														{ ClockDomainCounterValidFlag::EntitySpecific6, "ENTITY_SPECIFIC_6" },
																														{ ClockDomainCounterValidFlag::EntitySpecific5, "ENTITY_SPECIFIC_5" },
																														{ ClockDomainCounterValidFlag::EntitySpecific4, "ENTITY_SPECIFIC_4" },
																														{ ClockDomainCounterValidFlag::EntitySpecific3, "ENTITY_SPECIFIC_3" },
																														{ ClockDomainCounterValidFlag::EntitySpecific2, "ENTITY_SPECIFIC_2" },
																														{ ClockDomainCounterValidFlag::EntitySpecific1, "ENTITY_SPECIFIC_1" },
																													});

/* StreamInputCounterValidFlag conversion */
NLOHMANN_JSON_SERIALIZE_ENUM(StreamInputCounterValidFlag, {
																														{ StreamInputCounterValidFlag::None, "UNKNOWN" },
																														{ StreamInputCounterValidFlag::MediaLocked, "MEDIA_LOCKED" },
																														{ StreamInputCounterValidFlag::MediaUnlocked, "MEDIA_UNLOCKED" },
																														{ StreamInputCounterValidFlag::StreamInterrupted, "STREAM_INTERRUPTED" },
																														{ StreamInputCounterValidFlag::SeqNumMismatch, "SEQ_NUM_MISMATCH" },
																														{ StreamInputCounterValidFlag::MediaReset, "MEDIA_RESET" },
																														{ StreamInputCounterValidFlag::TimestampUncertain, "TIMESTAMP_UNCERTAIN" },
																														{ StreamInputCounterValidFlag::TimestampValid, "TIMESTAMP_VALID" },
																														{ StreamInputCounterValidFlag::TimestampNotValid, "TIMESTAMP_NOT_VALID" },
																														{ StreamInputCounterValidFlag::UnsupportedFormat, "UNSUPPORTED_FORMAT" },
																														{ StreamInputCounterValidFlag::LateTimestamp, "LATE_TIMESTAMP" },
																														{ StreamInputCounterValidFlag::EarlyTimestamp, "EARLY_TIMESTAMP" },
																														{ StreamInputCounterValidFlag::FramesRx, "FRAMES_RX" },
																														{ StreamInputCounterValidFlag::FramesTx, "FRAMES_TX" },
																														{ StreamInputCounterValidFlag::EntitySpecific8, "ENTITY_SPECIFIC_8" },
																														{ StreamInputCounterValidFlag::EntitySpecific7, "ENTITY_SPECIFIC_7" },
																														{ StreamInputCounterValidFlag::EntitySpecific6, "ENTITY_SPECIFIC_6" },
																														{ StreamInputCounterValidFlag::EntitySpecific5, "ENTITY_SPECIFIC_5" },
																														{ StreamInputCounterValidFlag::EntitySpecific4, "ENTITY_SPECIFIC_4" },
																														{ StreamInputCounterValidFlag::EntitySpecific3, "ENTITY_SPECIFIC_3" },
																														{ StreamInputCounterValidFlag::EntitySpecific2, "ENTITY_SPECIFIC_2" },
																														{ StreamInputCounterValidFlag::EntitySpecific1, "ENTITY_SPECIFIC_1" },
																													});

/* StreamOutputCounterValidFlag conversion */
NLOHMANN_JSON_SERIALIZE_ENUM(StreamOutputCounterValidFlag, {
																														 { StreamOutputCounterValidFlag::None, "UNKNOWN" },
																														 { StreamOutputCounterValidFlag::StreamStart, "STREAM_START" },
																														 { StreamOutputCounterValidFlag::StreamStop, "STREAM_STOP" },
																														 { StreamOutputCounterValidFlag::MediaReset, "MEDIA_RESET" },
																														 { StreamOutputCounterValidFlag::TimestampUncertain, "TIMESTAMP_UNCERTAIN" },
																														 { StreamOutputCounterValidFlag::FramesTx, "FRAMES_TX" },
																													 });

/* MilanInfoFeaturesFlag conversion */
NLOHMANN_JSON_SERIALIZE_ENUM(MilanInfoFeaturesFlag, {
																											{ MilanInfoFeaturesFlag::None, "UNKNOWN" },
																											{ MilanInfoFeaturesFlag::Redundancy, "REDUNDANCY" },
																										});

/* Entity::CommonInformation conversion */
inline void to_json(json& j, Entity::CommonInformation const& commonInfo)
{
	j[keyName::Entity_CommonInformation_EntityID] = commonInfo.entityID;
	j[keyName::Entity_CommonInformation_EntityModelID] = commonInfo.entityModelID;
	j[keyName::Entity_CommonInformation_EntityCapabilities] = commonInfo.entityCapabilities;
	j[keyName::Entity_CommonInformation_TalkerStreamSources] = commonInfo.talkerStreamSources;
	j[keyName::Entity_CommonInformation_TalkerCapabilities] = commonInfo.talkerCapabilities;
	j[keyName::Entity_CommonInformation_ListenerStreamSinks] = commonInfo.listenerStreamSinks;
	j[keyName::Entity_CommonInformation_ListenerCapabilities] = commonInfo.listenerCapabilities;
	j[keyName::Entity_CommonInformation_ControllerCapabilities] = commonInfo.controllerCapabilities;
	j[keyName::Entity_CommonInformation_IdentifyControlIndex] = commonInfo.identifyControlIndex;
	j[keyName::Entity_CommonInformation_AssociationID] = commonInfo.associationID;
}
inline void from_json(json const& j, Entity::CommonInformation& commonInfo)
{
	j.at(keyName::Entity_CommonInformation_EntityID).get_to(commonInfo.entityID);
	j.at(keyName::Entity_CommonInformation_EntityModelID).get_to(commonInfo.entityModelID);
	j.at(keyName::Entity_CommonInformation_EntityCapabilities).get_to(commonInfo.entityCapabilities);
	get_optional_value(j, keyName::Entity_CommonInformation_TalkerStreamSources, commonInfo.talkerStreamSources);
	get_optional_value(j, keyName::Entity_CommonInformation_TalkerCapabilities, commonInfo.talkerCapabilities);
	get_optional_value(j, keyName::Entity_CommonInformation_ListenerStreamSinks, commonInfo.listenerStreamSinks);
	get_optional_value(j, keyName::Entity_CommonInformation_ListenerCapabilities, commonInfo.listenerCapabilities);
	get_optional_value(j, keyName::Entity_CommonInformation_ControllerCapabilities, commonInfo.controllerCapabilities);
	get_optional_value(j, keyName::Entity_CommonInformation_IdentifyControlIndex, commonInfo.identifyControlIndex);
	get_optional_value(j, keyName::Entity_CommonInformation_AssociationID, commonInfo.associationID);
}

/* Entity::InterfaceInformation conversion */
inline void to_json(json& j, Entity::InterfaceInformation const& intfcInfo)
{
	j[keyName::Entity_InterfaceInformation_MacAddress] = networkInterface::NetworkInterfaceHelper::macAddressToString(intfcInfo.macAddress, true);
	j[keyName::Entity_InterfaceInformation_ValidTime] = intfcInfo.validTime;
	j[keyName::Entity_InterfaceInformation_AvailableIndex] = intfcInfo.availableIndex;
	if (intfcInfo.gptpGrandmasterID)
	{
		j[keyName::Entity_InterfaceInformation_GptpGrandmasterID] = *intfcInfo.gptpGrandmasterID;
	}
	if (intfcInfo.gptpDomainNumber)
	{
		j[keyName::Entity_InterfaceInformation_GptpDomainNumber] = *intfcInfo.gptpDomainNumber;
	}
}
inline void from_json(json const& j, Entity::InterfaceInformation& intfcInfo)
{
	intfcInfo.macAddress = networkInterface::NetworkInterfaceHelper::stringToMacAddress(j.at(keyName::Entity_InterfaceInformation_MacAddress).get<std::string>());
	j.at(keyName::Entity_InterfaceInformation_ValidTime).get_to(intfcInfo.validTime);
	get_optional_value(j, keyName::Entity_InterfaceInformation_AvailableIndex, intfcInfo.availableIndex);
	get_optional_value(j, keyName::Entity_InterfaceInformation_GptpGrandmasterID, intfcInfo.gptpGrandmasterID);
	get_optional_value(j, keyName::Entity_InterfaceInformation_GptpDomainNumber, intfcInfo.gptpDomainNumber);
}

namespace model
{
namespace keyName
{
/* Tree nodes */
constexpr auto NodeName_EntityDescriptor = "entity_descriptor";
constexpr auto NodeName_ConfigurationDescriptors = "configuration_descriptors";
constexpr auto NodeName_AudioUnitDescriptors = "audio_unit_descriptors";
constexpr auto NodeName_StreamInputDescriptors = "stream_input_descriptors";
constexpr auto NodeName_StreamOutputDescriptors = "stream_output_descriptors";
constexpr auto NodeName_JackInputDescriptors = "jack_input_descriptors";
constexpr auto NodeName_JackOutputDescriptors = "jack_output_descriptors";
constexpr auto NodeName_AvbInterfaceDescriptors = "avb_interface_descriptors";
constexpr auto NodeName_ClockSourceDescriptors = "clock_source_descriptors";
constexpr auto NodeName_MemoryObjectDescriptors = "memory_object_descriptors";
constexpr auto NodeName_LocaleDescriptors = "locale_descriptors";
constexpr auto NodeName_StringsDescriptors = "strings_descriptors";
constexpr auto NodeName_StreamPortInputDescriptors = "stream_port_input_descriptors";
constexpr auto NodeName_StreamPortOutputDescriptors = "stream_port_output_descriptors";
constexpr auto NodeName_AudioClusterDescriptors = "audio_cluster_descriptors";
constexpr auto NodeName_AudioMapDescriptors = "audio_map_descriptors";
constexpr auto NodeName_ControlDescriptors = "control_descriptors";
constexpr auto NodeName_ClockDomainDescriptors = "clock_domain_descriptors";
constexpr auto NodeName_TimingDescriptors = "timing_descriptors";
constexpr auto NodeName_PtpInstanceDescriptors = "ptp_instance_descriptors";
constexpr auto NodeName_PtpPortDescriptors = "ptp_port_descriptors";

/* Globals */
constexpr auto Node_Informative_Index = "_index (informative)";
constexpr auto Node_StaticInformation = "static";
constexpr auto Node_DynamicInformation = "dynamic";
constexpr auto Node_NotCompliant = "not_compliant";

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
constexpr auto ConfigurationNode_Static_DescriptorCounts = "descriptor_counts";
constexpr auto ConfigurationNode_Dynamic_ObjectName = "object_name";

/* AudioUnitNode */
constexpr auto AudioUnitNode_Static_LocalizedDescription = "localized_description";
constexpr auto AudioUnitNode_Static_ClockDomainIndex = "clock_domain_index";
constexpr auto AudioUnitNode_Static_SamplingRates = "sampling_rates";
constexpr auto AudioUnitNode_Static_ControlCounts = "control_counts";
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
constexpr auto StreamInputNode_Dynamic_StreamFormat = "stream_format";
constexpr auto StreamInputNode_Dynamic_StreamRunning = "stream_running";
constexpr auto StreamInputNode_Dynamic_StreamDynamicInfo = "stream_dynamic_info";
constexpr auto StreamInputNode_Dynamic_ConnectedTalker = "connected_talker";
constexpr auto StreamInputNode_Dynamic_ConnectionState = "connection_state";
constexpr auto StreamInputNode_Dynamic_Counters = "counters";

/* StreamOutputNode */
constexpr auto StreamOutputNode_Dynamic_ObjectName = "object_name";
constexpr auto StreamOutputNode_Dynamic_StreamFormat = "stream_format";
constexpr auto StreamOutputNode_Dynamic_StreamRunning = "stream_running";
constexpr auto StreamOutputNode_Dynamic_StreamDynamicInfo = "stream_dynamic_info";
constexpr auto StreamOutputNode_Dynamic_Counters = "counters";

/* JackNode */
constexpr auto JackNode_Static_LocalizedDescription = "localized_description";
constexpr auto JackNode_Static_Flags = "flags";
constexpr auto JackNode_Static_Type = "type";
constexpr auto JackNode_Static_ControlCounts = "control_counts";
constexpr auto JackNode_Dynamic_ObjectName = "object_name";

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
constexpr auto AvbInterfaceNode_Dynamic_GptpGrandmasterID = "gptp_grandmaster_id";
constexpr auto AvbInterfaceNode_Dynamic_GptpDomainNumber = "gptp_domain_number";
constexpr auto AvbInterfaceNode_Dynamic_AvbInterfaceInfo = "avb_interface_info";
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
constexpr auto LocaleNode_Static_Informative_BaseStringDescriptor = "_base_string_descriptor (informative)";

/* StringsNode */
constexpr auto StringsNode_Static_Strings = "strings";

/* StreamPortNode */
constexpr auto StreamPortNode_Static_ClockDomainIndex = "clock_domain_index";
constexpr auto StreamPortNode_Static_Flags = "flags";
constexpr auto StreamPortNode_Static_ControlCounts = "control_counts";
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

/* ControlNode */
constexpr auto ControlNode_Static_LocalizedDescription = "localized_description";
constexpr auto ControlNode_Static_BlockLatency = "block_latency";
constexpr auto ControlNode_Static_ControlLatency = "control_latency";
constexpr auto ControlNode_Static_ControlDomain = "control_domain";
constexpr auto ControlNode_Static_ControlType = "control_type";
constexpr auto ControlNode_Static_ResetTime = "reset_time";
constexpr auto ControlNode_Static_SignalType = "signal_type";
constexpr auto ControlNode_Static_SignalIndex = "signal_index";
constexpr auto ControlNode_Static_SignalOutput = "signal_output";
constexpr auto ControlNode_Static_ControlValueType = "control_value_type";
constexpr auto ControlNode_Static_NumberOfValues = "number_of_values";
constexpr auto ControlNode_Static_Values = "values";
constexpr auto ControlNode_Dynamic_ObjectName = "object_name";
constexpr auto ControlNode_Dynamic_Values = "values";

/* ControlValueType */
constexpr auto ControlValueType_ReadOnly = "read_only";
constexpr auto ControlValueType_Unknown = "unknown";
constexpr auto ControlValueType_Type = "value_type";

/* ControlValues */
constexpr auto ControlValues_Type = "type";
constexpr auto ControlValues_Values = "values";

/* LinearValue - IEEE1722.1-2013 Clause 7.3.5.2.1 */
constexpr auto LinearValue_Minimum = "minimum";
constexpr auto LinearValue_Maximum = "maximum";
constexpr auto LinearValue_Step = "step";
constexpr auto LinearValue_Default = "default";
constexpr auto LinearValue_Unit = "unit";
constexpr auto LinearValue_String = "string";

/* SelectorValue - IEEE1722.1-2013 Clause 7.3.5.2.2 */
constexpr auto SelectorValue_Default = "default";
constexpr auto SelectorValue_Unit = "unit";
constexpr auto SelectorValue_Options = "options";

/* ArrayValue - IEEE1722.1-2013 Clause 7.3.5.2.3 */
constexpr auto ArrayValue_Minimum = "minimum";
constexpr auto ArrayValue_Maximum = "maximum";
constexpr auto ArrayValue_Step = "step";
constexpr auto ArrayValue_Default = "default";
constexpr auto ArrayValue_Unit = "unit";
constexpr auto ArrayValue_String = "string";

/* ClockDomainNode */
constexpr auto ClockDomainNode_Static_LocalizedDescription = "localized_description";
constexpr auto ClockDomainNode_Static_ClockSources = "clock_sources";
constexpr auto ClockDomainNode_Dynamic_ObjectName = "object_name";
constexpr auto ClockDomainNode_Dynamic_ClockSourceIndex = "clock_source_index";
constexpr auto ClockDomainNode_Dynamic_Counters = "counters";

/* TimingNode */
constexpr auto TimingNode_Static_LocalizedDescription = "localized_description";
constexpr auto TimingNode_Static_Algorithm = "algorithm";
constexpr auto TimingNode_Static_PtpInstances = "ptp_instances";
constexpr auto TimingNode_Dynamic_ObjectName = "object_name";

/* PtpInstanceNode */
constexpr auto PtpInstanceNode_Static_LocalizedDescription = "localized_description";
constexpr auto PtpInstanceNode_Static_ClockIdentity = "clock_identity";
constexpr auto PtpInstanceNode_Static_Flags = "flags";
constexpr auto PtpInstanceNode_Static_ControlCounts = "control_counts";
constexpr auto PtpInstanceNode_Static_PtpPortCounts = "ptp_port_counts";
constexpr auto PtpInstanceNode_Dynamic_ObjectName = "object_name";

/* PtpPortNode */
constexpr auto PtpPortNode_Static_LocalizedDescription = "localized_description";
constexpr auto PtpPortNode_Static_PortNumber = "port_number";
constexpr auto PtpPortNode_Static_PortType = "port_type";
constexpr auto PtpPortNode_Static_Flags = "flags";
constexpr auto PtpPortNode_Static_AvbInterfaceIndex = "avb_interface_index";
constexpr auto PtpPortNode_Static_ProfileIdentifier = "profile_identifier";
constexpr auto PtpPortNode_Dynamic_ObjectName = "object_name";

/* ControlValueUnit */
constexpr auto ControlValueUnit_Multiplier = "multiplier";
constexpr auto ControlValueUnit_Code = "code";

/* LocalizedStringReference */
constexpr auto LocalizedStringReference_Index = "index";
constexpr auto LocalizedStringReference_Offset = "offset";

/* MsrpMapping */
constexpr auto MsrpMapping_TrafficClass = "traffic_class";
constexpr auto MsrpMapping_Priority = "priority";
constexpr auto MsrpMapping_VlanID = "vlan_id";

/* AudioMapping */
constexpr auto AudioMapping_StreamIndex = "stream_index";
constexpr auto AudioMapping_StreamChannel = "stream_channel";
constexpr auto AudioMapping_ClusterOffset = "cluster_offset";
constexpr auto AudioMapping_ClusterChannel = "cluster_channel";

/* StreamIdentification */
constexpr auto StreamIdentification_EntityID = "entity_id";
constexpr auto StreamIdentification_StreamIndex = "stream_index";

/* StreamDynamicInfo */
constexpr auto StreamDynamicInfo_IsClassB = "is_class_b";
constexpr auto StreamDynamicInfo_HasSavedState = "has_saved_state";
constexpr auto StreamDynamicInfo_DoesSupportEncrypted = "does_support_encrypted";
constexpr auto StreamDynamicInfo_ArePdusEncrypted = "are_pdus_encrypted";
constexpr auto StreamDynamicInfo_HasTalkerFailed = "has_talker_failed";
constexpr auto StreamDynamicInfo_Flags = "last_received_flags";
constexpr auto StreamDynamicInfo_StreamID = "stream_id";
constexpr auto StreamDynamicInfo_MsrpAccumulatedLatency = "msrp_accumulated_latency";
constexpr auto StreamDynamicInfo_StreamDestMac = "stream_dest_mac";
constexpr auto StreamDynamicInfo_MsrpFailureCode = "msrp_failure_code";
constexpr auto StreamDynamicInfo_MsrpFailureBridgeID = "msrp_failure_bridge_id";
constexpr auto StreamDynamicInfo_StreamVlanID = "stream_vlan_id";
constexpr auto StreamDynamicInfo_FlagsEx = "flags_ex";
constexpr auto StreamDynamicInfo_ProbingStatus = "probing_status";
constexpr auto StreamDynamicInfo_AcmpStatus = "acmp_status";

/* AvbInterfaceInfo */
constexpr auto AvbInterfaceInfo_PropagationDelay = "propagation_delay";
constexpr auto AvbInterfaceInfo_Flags = "flags";
constexpr auto AvbInterfaceInfo_MsrpMappings = "msrp_mappings";

/* MilanInfo */
constexpr auto MilanInfo_ProtocolVersion = "protocol_version";
constexpr auto MilanInfo_Flags = "flags";
constexpr auto MilanInfo_CertificationVersion = "certification_version";

} // namespace keyName

/* DescriptorType conversion */
NLOHMANN_JSON_SERIALIZE_ENUM(DescriptorType, {
																							 { DescriptorType::Invalid, "INVALID" },
																							 { DescriptorType::Entity, "ENTITY" },
																							 { DescriptorType::Configuration, "CONFIGURATION" },
																							 { DescriptorType::AudioUnit, "AUDIO_UNIT" },
																							 { DescriptorType::VideoUnit, "VIDEO_UNIT" },
																							 { DescriptorType::SensorUnit, "SENSOR_UNIT" },
																							 { DescriptorType::StreamInput, "STREAM_INPUT" },
																							 { DescriptorType::StreamOutput, "STREAM_OUTPUT" },
																							 { DescriptorType::JackInput, "JACK_INPUT" },
																							 { DescriptorType::JackOutput, "JACK_OUTPUT" },
																							 { DescriptorType::AvbInterface, "AVB_INTERFACE" },
																							 { DescriptorType::ClockSource, "CLOCK_SOURCE" },
																							 { DescriptorType::MemoryObject, "MEMORY_OBJECT" },
																							 { DescriptorType::Locale, "LOCALE" },
																							 { DescriptorType::Strings, "STRINGS" },
																							 { DescriptorType::StreamPortInput, "STREAM_PORT_INPUT" },
																							 { DescriptorType::StreamPortOutput, "STREAM_PORT_OUTPUT" },
																							 { DescriptorType::ExternalPortInput, "EXTERNAL_PORT_INPUT" },
																							 { DescriptorType::ExternalPortOutput, "EXTERNAL_PORT_OUTPUT" },
																							 { DescriptorType::InternalPortInput, "INTERNAL_PORT_INPUT" },
																							 { DescriptorType::InternalPortOutput, "INTERNAL_PORT_OUTPUT" },
																							 { DescriptorType::AudioCluster, "AUDIO_CLUSTER" },
																							 { DescriptorType::VideoCluster, "VIDEO_CLUSTER" },
																							 { DescriptorType::SensorCluster, "SENSOR_CLUSTER" },
																							 { DescriptorType::AudioMap, "AUDIO_MAP" },
																							 { DescriptorType::VideoMap, "VIDEO_MAP" },
																							 { DescriptorType::SensorMap, "SENSOR_MAP" },
																							 { DescriptorType::Control, "CONTROL" },
																							 { DescriptorType::SignalSelector, "SIGNAL_SELECTOR" },
																							 { DescriptorType::Mixer, "MIXER" },
																							 { DescriptorType::Matrix, "MATRIX" },
																							 { DescriptorType::MatrixSignal, "MATRIX_SIGNAL" },
																							 { DescriptorType::SignalSplitter, "SIGNAL_SPLITTER" },
																							 { DescriptorType::SignalCombiner, "SIGNAL_COMBINER" },
																							 { DescriptorType::SignalDemultiplexer, "SIGNAL_DEMULTIPLEXER" },
																							 { DescriptorType::SignalMultiplexer, "SIGNAL_MULTIPLEXER" },
																							 { DescriptorType::SignalTranscoder, "SIGNAL_TRANSCODER" },
																							 { DescriptorType::ClockDomain, "CLOCK_DOMAIN" },
																							 { DescriptorType::ControlBlock, "CONTROL_BLOCK" },
																							 { DescriptorType::Timing, "TIMING" },
																							 { DescriptorType::PtpInstance, "PTP_INSTANCE" },
																							 { DescriptorType::PtpPort, "PTP_PORT" },
																						 });

/* JackType conversion */
NLOHMANN_JSON_SERIALIZE_ENUM(JackType, {
																				 { JackType::Expansion, "EXPANSION" },
																				 { JackType::Speaker, "SPEAKER" },
																				 { JackType::Headphone, "HEADPHONE" },
																				 { JackType::AnalogMicrophone, "ANALOG_MICROPHONE" },
																				 { JackType::Spdif, "SPDIF" },
																				 { JackType::Adat, "ADAT" },
																				 { JackType::Tdif, "TDIF" },
																				 { JackType::Madi, "MADI" },
																				 { JackType::UnbalancedAnalog, "UNBALANCED_ANALOG" },
																				 { JackType::BalancedAnalog, "BALANCED_ANALOG" },
																				 { JackType::Digital, "DIGITAL" },
																				 { JackType::Midi, "MIDI" },
																				 { JackType::AesEbu, "AES_EBU" },
																				 { JackType::CompositeVideo, "COMPOSITE_VIDEO" },
																				 { JackType::SVhsVideo, "S_VHS_VIDEO" },
																				 { JackType::ComponentVideo, "COMPONENT_VIDEO" },
																				 { JackType::Dvi, "DVI" },
																				 { JackType::Hdmi, "HDMI" },
																				 { JackType::Udi, "UDI" },
																				 { JackType::DisplayPort, "DISPLAYPORT" },
																				 { JackType::Antenna, "ANTENNA" },
																				 { JackType::AnalogTuner, "ANALOG_TUNER" },
																				 { JackType::Ethernet, "ETHERNET" },
																				 { JackType::Wifi, "WIFI" },
																				 { JackType::Usb, "USB" },
																				 { JackType::Pci, "PCI" },
																				 { JackType::PciE, "PCI_E" },
																				 { JackType::Scsi, "SCSI" },
																				 { JackType::Ata, "ATA" },
																				 { JackType::Imager, "IMAGER" },
																				 { JackType::Ir, "IR" },
																				 { JackType::Thunderbolt, "THUNDERBOLT" },
																				 { JackType::Sata, "SATA" },
																				 { JackType::SmpteLtc, "SMPTE_LTC" },
																				 { JackType::DigitalMicrophone, "DIGITAL_MICROPHONE" },
																				 { JackType::AudioMediaClock, "AUDIO_MEDIA_CLOCK" },
																				 { JackType::VideoMediaClock, "VIDEO_MEDIA_CLOCK" },
																				 { JackType::GnssClock, "GNSS_CLOCK" },
																				 { JackType::Pps, "PPS" },
																			 });

/* ClockSourceType conversion */
NLOHMANN_JSON_SERIALIZE_ENUM(ClockSourceType, {
																								{ ClockSourceType::Expansion, "EXPANSION" },
																								{ ClockSourceType::Internal, "INTERNAL" },
																								{ ClockSourceType::External, "EXTERNAL" },
																								{ ClockSourceType::InputStream, "INPUT_STREAM" },
																							});

/* MemoryObjectType conversion */
NLOHMANN_JSON_SERIALIZE_ENUM(MemoryObjectType, {
																								 { MemoryObjectType::FirmwareImage, "FIRMWARE_IMAGE" },
																								 { MemoryObjectType::VendorSpecific, "VENDOR_SPECIFIC" },
																								 { MemoryObjectType::CrashDump, "CRASH_DUMP" },
																								 { MemoryObjectType::LogObject, "LOG_OBJECT" },
																								 { MemoryObjectType::AutostartSettings, "AUTOSTART_SETTINGS" },
																								 { MemoryObjectType::SnapshotSettings, "SNAPSHOT_SETTINGS" },
																								 { MemoryObjectType::SvgManufacturer, "SVG_MANUFACTURER" },
																								 { MemoryObjectType::SvgEntity, "SVG_ENTITY" },
																								 { MemoryObjectType::SvgGeneric, "SVG_GENERIC" },
																								 { MemoryObjectType::PngManufacturer, "PNG_MANUFACTURER" },
																								 { MemoryObjectType::PngEntity, "PNG_ENTITY" },
																								 { MemoryObjectType::PngGeneric, "PNG_GENERIC" },
																								 { MemoryObjectType::DaeManufacturer, "DAE_MANUFACTURER" },
																								 { MemoryObjectType::DaeEntity, "DAE_ENTITY" },
																								 { MemoryObjectType::DaeGeneric, "DAE_GENERIC" },
																							 });

/* AudioClusterFormat conversion */
NLOHMANN_JSON_SERIALIZE_ENUM(AudioClusterFormat, {
																									 { AudioClusterFormat::Iec60958, "IEC_60958" },
																									 { AudioClusterFormat::Mbla, "MBLA" },
																									 { AudioClusterFormat::Midi, "MIDI" },
																									 { AudioClusterFormat::Smpte, "SMPTE" },
																								 });

/* TimingAlgorithm conversion */
NLOHMANN_JSON_SERIALIZE_ENUM(TimingAlgorithm, {
																								{ TimingAlgorithm::Single, "SINGLE" },
																								{ TimingAlgorithm::Fallback, "FALLBACK" },
																								{ TimingAlgorithm::Combined, "COMBINED" },
																							});

/* PtpPortType conversion */
NLOHMANN_JSON_SERIALIZE_ENUM(PtpPortType, {
																						{ PtpPortType::P2PLinkLayer, "P2P_LINK_LAYER" },
																						{ PtpPortType::P2PMulticastUdpV4, "P2P_MULTICAST_UDP_V4" },
																						{ PtpPortType::P2PMulticastUdpV6, "P2P_MULTICAST_UDP_V6" },
																						{ PtpPortType::TimingMeasurement, "TIMING_MEASUREMENT" },
																						{ PtpPortType::FineTimingMeasurement, "FINE_TIMING_MEASUREMENT" },
																						{ PtpPortType::E2ELinkLayer, "E2E_LINK_LAYER" },
																						{ PtpPortType::E2EMulticastUdpV4, "E2E_MULTICAST_UDP_V4" },
																						{ PtpPortType::E2EMulticastUdpV6, "E2E_MULTICAST_UDP_V6" },
																						{ PtpPortType::P2PUnicastUdpV4, "P2P_UNICAST_UDP_V4" },
																						{ PtpPortType::P2PUnicastUdpV6, "P2P_UNICAST_UDP_V6" },
																						{ PtpPortType::E2EUnicastUdpV4, "E2E_UNICAST_UDP_V4" },
																						{ PtpPortType::E2EUnicastUdpV6, "E2E_UNICAST_UDP_V6" },
																					});

/* ControlValueUnit::Unit conversion */
NLOHMANN_JSON_SERIALIZE_ENUM(ControlValueUnit::Unit, {
																											 { ControlValueUnit::Unit::Unitless, "UNITLESS" },
																											 { ControlValueUnit::Unit::Count, "COUNT" },
																											 { ControlValueUnit::Unit::Percent, "PERCENT" },
																											 { ControlValueUnit::Unit::FStop, "FSTOP" },
																											 { ControlValueUnit::Unit::Seconds, "SECONDS" },
																											 { ControlValueUnit::Unit::Minutes, "MINUTES" },
																											 { ControlValueUnit::Unit::Hours, "HOURS" },
																											 { ControlValueUnit::Unit::Days, "DAYS" },
																											 { ControlValueUnit::Unit::Months, "MONTHS" },
																											 { ControlValueUnit::Unit::Years, "YEARS" },
																											 { ControlValueUnit::Unit::Samples, "SAMPLES" },
																											 { ControlValueUnit::Unit::Frames, "FRAMES" },
																											 { ControlValueUnit::Unit::Hertz, "HERTZ" },
																											 { ControlValueUnit::Unit::Semitones, "SEMITONES" },
																											 { ControlValueUnit::Unit::Cents, "CENTS" },
																											 { ControlValueUnit::Unit::Octaves, "OCTAVES" },
																											 { ControlValueUnit::Unit::Fps, "FPS" },
																											 { ControlValueUnit::Unit::Metres, "METRES" },
																											 { ControlValueUnit::Unit::Kelvin, "KELVIN" },
																											 { ControlValueUnit::Unit::Grams, "GRAMS" },
																											 { ControlValueUnit::Unit::Volts, "VOLTS" },
																											 { ControlValueUnit::Unit::Dbv, "DBV" },
																											 { ControlValueUnit::Unit::Dbu, "DBU" },
																											 { ControlValueUnit::Unit::Amps, "AMPS" },
																											 { ControlValueUnit::Unit::Watts, "WATTS" },
																											 { ControlValueUnit::Unit::Dbm, "DBM" },
																											 { ControlValueUnit::Unit::Dbw, "DBW" },
																											 { ControlValueUnit::Unit::Pascals, "PASCALS" },
																											 { ControlValueUnit::Unit::Bits, "BITS" },
																											 { ControlValueUnit::Unit::Bytes, "BYTES" },
																											 { ControlValueUnit::Unit::KibiBytes, "KIBIBYTES" },
																											 { ControlValueUnit::Unit::MebiBytes, "MEBIBYTES" },
																											 { ControlValueUnit::Unit::GibiBytes, "GIBIBYTES" },
																											 { ControlValueUnit::Unit::TebiBytes, "TEBIBYTES" },
																											 { ControlValueUnit::Unit::BitsPerSec, "BITS_PER_SEC" },
																											 { ControlValueUnit::Unit::BytesPerSec, "BYTES_PER_SEC" },
																											 { ControlValueUnit::Unit::KibiBytesPerSec, "KIBIBYTES_PER_SEC" },
																											 { ControlValueUnit::Unit::MebiBytesPerSec, "MEBIBYTES_PER_SEC" },
																											 { ControlValueUnit::Unit::GibiBytesPerSec, "GIBIBYTES_PER_SEC" },
																											 { ControlValueUnit::Unit::TebiBytesPerSec, "TEBIBYTES_PER_SEC" },
																											 { ControlValueUnit::Unit::Candelas, "CANDELAS" },
																											 { ControlValueUnit::Unit::Joules, "JOULES" },
																											 { ControlValueUnit::Unit::Radians, "RADIANS" },
																											 { ControlValueUnit::Unit::Newtons, "NEWTONS" },
																											 { ControlValueUnit::Unit::Ohms, "OHMS" },
																											 { ControlValueUnit::Unit::MetresPerSec, "METRES_PER_SEC" },
																											 { ControlValueUnit::Unit::RadiansPerSec, "RADIANS_PER_SEC" },
																											 { ControlValueUnit::Unit::MetresPerSecSquared, "METRES_PER_SEC_SQUARED" },
																											 { ControlValueUnit::Unit::RadiansPerSecSquared, "RADIANS_PER_SEC_SQUARED" },
																											 { ControlValueUnit::Unit::Teslas, "TESLAS" },
																											 { ControlValueUnit::Unit::Webers, "WEBERS" },
																											 { ControlValueUnit::Unit::AmpsPerMetre, "AMPS_PER_METRE" },
																											 { ControlValueUnit::Unit::MetresSquared, "METRES_SQUARED" },
																											 { ControlValueUnit::Unit::MetresCubed, "METRES_CUBED" },
																											 { ControlValueUnit::Unit::Litres, "LITRES" },
																											 { ControlValueUnit::Unit::Db, "DB" },
																											 { ControlValueUnit::Unit::DbPeak, "DB_PEAK" },
																											 { ControlValueUnit::Unit::DbRms, "DB_RMS" },
																											 { ControlValueUnit::Unit::Dbfs, "DBFS" },
																											 { ControlValueUnit::Unit::DbfsPeak, "DBFS_PEAK" },
																											 { ControlValueUnit::Unit::DbfsRms, "DBFS_RMS" },
																											 { ControlValueUnit::Unit::Dbtp, "DBTP" },
																											 { ControlValueUnit::Unit::DbSplA, "DB_SPL_A" },
																											 { ControlValueUnit::Unit::DbZ, "DB_Z" },
																											 { ControlValueUnit::Unit::DbSplC, "DB_SPL_C" },
																											 { ControlValueUnit::Unit::DbSpl, "DB_SPL" },
																											 { ControlValueUnit::Unit::Lu, "LU" },
																											 { ControlValueUnit::Unit::Lufs, "LUFS" },
																											 { ControlValueUnit::Unit::DbA, "DB_A" },
																										 });

/* ControlValueType::Type conversion */
NLOHMANN_JSON_SERIALIZE_ENUM(ControlValueType::Type, {
																											 { ControlValueType::Type::ControlLinearInt8, "CONTROL_LINEAR_INT8" },
																											 { ControlValueType::Type::ControlLinearUInt8, "CONTROL_LINEAR_UINT8" },
																											 { ControlValueType::Type::ControlLinearInt16, "CONTROL_LINEAR_INT16" },
																											 { ControlValueType::Type::ControlLinearUInt16, "CONTROL_LINEAR_UINT16" },
																											 { ControlValueType::Type::ControlLinearInt32, "CONTROL_LINEAR_INT32" },
																											 { ControlValueType::Type::ControlLinearUInt32, "CONTROL_LINEAR_UINT32" },
																											 { ControlValueType::Type::ControlLinearInt64, "CONTROL_LINEAR_INT64" },
																											 { ControlValueType::Type::ControlLinearUInt64, "CONTROL_LINEAR_UINT64" },
																											 { ControlValueType::Type::ControlLinearFloat, "CONTROL_LINEAR_FLOAT" },
																											 { ControlValueType::Type::ControlLinearDouble, "CONTROL_LINEAR_DOUBLE" },
																											 { ControlValueType::Type::ControlSelectorInt8, "CONTROL_SELECTOR_INT8" },
																											 { ControlValueType::Type::ControlSelectorUInt8, "CONTROL_SELECTOR_UINT8" },
																											 { ControlValueType::Type::ControlSelectorInt16, "CONTROL_SELECTOR_INT16" },
																											 { ControlValueType::Type::ControlSelectorUInt16, "CONTROL_SELECTOR_UINT16" },
																											 { ControlValueType::Type::ControlSelectorInt32, "CONTROL_SELECTOR_INT32" },
																											 { ControlValueType::Type::ControlSelectorUInt32, "CONTROL_SELECTOR_UINT32" },
																											 { ControlValueType::Type::ControlSelectorInt64, "CONTROL_SELECTOR_INT64" },
																											 { ControlValueType::Type::ControlSelectorUInt64, "CONTROL_SELECTOR_UINT64" },
																											 { ControlValueType::Type::ControlSelectorFloat, "CONTROL_SELECTOR_FLOAT" },
																											 { ControlValueType::Type::ControlSelectorDouble, "CONTROL_SELECTOR_DOUBLE" },
																											 { ControlValueType::Type::ControlSelectorString, "CONTROL_SELECTOR_STRING" },
																											 { ControlValueType::Type::ControlArrayInt8, "CONTROL_ARRAY_INT8" },
																											 { ControlValueType::Type::ControlArrayUInt8, "CONTROL_ARRAY_UINT8" },
																											 { ControlValueType::Type::ControlArrayInt16, "CONTROL_ARRAY_INT16" },
																											 { ControlValueType::Type::ControlArrayUInt16, "CONTROL_ARRAY_UINT16" },
																											 { ControlValueType::Type::ControlArrayInt32, "CONTROL_ARRAY_INT32" },
																											 { ControlValueType::Type::ControlArrayUInt32, "CONTROL_ARRAY_UINT32" },
																											 { ControlValueType::Type::ControlArrayInt64, "CONTROL_ARRAY_INT64" },
																											 { ControlValueType::Type::ControlArrayUInt64, "CONTROL_ARRAY_UINT64" },
																											 { ControlValueType::Type::ControlArrayFloat, "CONTROL_ARRAY_FLOAT" },
																											 { ControlValueType::Type::ControlArrayDouble, "CONTROL_ARRAY_DOUBLE" },
																											 { ControlValueType::Type::ControlUtf8, "CONTROL_UTF8" },
																											 { ControlValueType::Type::ControlBodePlot, "CONTROL_BODE_PLOT" },
																											 { ControlValueType::Type::ControlSmpteTime, "CONTROL_SMPTE_TIME" },
																											 { ControlValueType::Type::ControlSampleRate, "CONTROL_SAMPLE_RATE" },
																											 { ControlValueType::Type::ControlGptpTime, "CONTROL_GPTP_TIME" },
																											 { ControlValueType::Type::ControlVendor, "CONTROL_VENDOR" },
																											 { ControlValueType::Type::Expansion, "EXPANSION" },
																										 });

/* StreamInputConnectionInfo::State conversion */
NLOHMANN_JSON_SERIALIZE_ENUM(StreamInputConnectionInfo::State, {
																																 { StreamInputConnectionInfo::State::NotConnected, "NOT_CONNECTED" },
																																 { StreamInputConnectionInfo::State::FastConnecting, "FAST_CONNECTING" },
																																 { StreamInputConnectionInfo::State::Connected, "CONNECTED" },
																															 });

/* MsrpFailureCode conversion */
NLOHMANN_JSON_SERIALIZE_ENUM(MsrpFailureCode, {
																								{ MsrpFailureCode::NoFailure, "NO_FAILURE" },
																								{ MsrpFailureCode::InsufficientBandwidth, "INSUFFICIENT_BANDWIDTH" },
																								{ MsrpFailureCode::InsufficientResources, "INSUFFICIENT_RESOURCES" },
																								{ MsrpFailureCode::InsufficientTrafficClassBandwidth, "INSUFFICIENT_TRAFFIC_CLASS_BANDWIDTH" },
																								{ MsrpFailureCode::StreamIDInUse, "STREAM_ID_IN_USE" },
																								{ MsrpFailureCode::StreamDestinationAddressInUse, "STREAM_DESTINATION_ADDRESS_IN_USE" },
																								{ MsrpFailureCode::StreamPreemptedByHigherRank, "STREAM_PREEMPTED_BY_HIGHER_RANK" },
																								{ MsrpFailureCode::LatencyHasChanged, "LATENCY_HAS_CHANGED" },
																								{ MsrpFailureCode::EgressPortNotAVBCapable, "EGRESS_PORT_NOT_AVB_CAPABLE" },
																								{ MsrpFailureCode::UseDifferentDestinationAddress, "USE_DIFFERENT_DESTINATION_ADDRESS" },
																								{ MsrpFailureCode::OutOfMSRPResources, "OUT_OF_MSRP_RESOURCES" },
																								{ MsrpFailureCode::OutOfMMRPResources, "OUT_OF_MMRP_RESOURCES" },
																								{ MsrpFailureCode::CannotStoreDestinationAddress, "CANNOT_STORE_DESTINATION_ADDRESS" },
																								{ MsrpFailureCode::PriorityIsNotAnSRClass, "PRIORITY_IS_NOT_AN_SR_CLASS" },
																								{ MsrpFailureCode::MaxFrameSizeTooLarge, "MAX_FRAME_SIZE_TOO_LARGE" },
																								{ MsrpFailureCode::MaxFanInPortsLimitReached, "MAX_FAN_IN_PORTS_LIMIT_REACHED" },
																								{ MsrpFailureCode::FirstValueChangedForStreamID, "FIRST_VALUE_CHANGED_FOR_STREAM_ID" },
																								{ MsrpFailureCode::VlanBlockedOnEgress, "VLAN_BLOCKED_ON_EGRESS" },
																								{ MsrpFailureCode::VlanTaggingDisabledOnEgress, "VLAN_TAGGING_DISABLED_ON_EGRESS" },
																								{ MsrpFailureCode::SrClassPriorityMismatch, "SR_CLASS_PRIORITY_MISMATCH" },
																							});

/* SamplingRate conversion */
inline void to_json(json& j, SamplingRate const& sr)
{
	j = utils::toHexString(sr.getValue(), true, true);
}
inline void from_json(json const& j, SamplingRate& sr)
{
	sr.setValue(utils::convertFromString<SamplingRate::value_type>(j.get<std::string>().c_str()));
}

/* StreamFormat conversion */
inline void to_json(json& j, StreamFormat const& sf)
{
	j = utils::toHexString(sf.getValue(), true, true);
}
inline void from_json(json const& j, StreamFormat& sf)
{
	sf.setValue(utils::convertFromString<StreamFormat::value_type>(j.get<std::string>().c_str()));
}

/* ControlValueUnit conversion */
inline void to_json(json& j, ControlValueUnit const& cvu)
{
	auto const [multiplier, unit] = cvu.getMultiplierUnit();
	j[keyName::ControlValueUnit_Multiplier] = multiplier;
	j[keyName::ControlValueUnit_Code] = unit;
}
inline void from_json(json const& j, ControlValueUnit& cvu)
{
	cvu.setMultiplierUnit(j.at(keyName::ControlValueUnit_Multiplier), j.at(keyName::ControlValueUnit_Code));
}

/* ControlValueType conversion */
inline void to_json(json& j, ControlValueType const& cvt)
{
	j[keyName::ControlValueType_ReadOnly] = cvt.isReadOnly();
	j[keyName::ControlValueType_Unknown] = cvt.isUnknown();
	j[keyName::ControlValueType_Type] = cvt.getType();
}
inline void from_json(json const& j, ControlValueType& cvt)
{
	cvt.setReadOnlyUnknownType(j.at(keyName::ControlValueType_ReadOnly), j.at(keyName::ControlValueType_Unknown), j.at(keyName::ControlValueType_Type));
}

/* LocalizedStringReference conversion */
inline void to_json(json& j, LocalizedStringReference const& ref)
{
	if (ref)
	{
		auto const [offset, index] = ref.getOffsetIndex();
		j[keyName::LocalizedStringReference_Offset] = offset;
		j[keyName::LocalizedStringReference_Index] = index;
	}
}
inline void from_json(json const& j, LocalizedStringReference& ref)
{
	if (!j.is_null())
	{
		ref.setOffsetIndex(j.at(keyName::LocalizedStringReference_Offset), j.at(keyName::LocalizedStringReference_Index));
	}
}

/* AvdeccFixedString conversion */
inline void to_json(json& j, AvdeccFixedString const& str)
{
	j = str.str();
}

/* MsrpMapping conversion */
inline void to_json(json& j, MsrpMapping const& mapping)
{
	j[keyName::MsrpMapping_TrafficClass] = mapping.trafficClass;
	j[keyName::MsrpMapping_Priority] = mapping.priority;
	j[keyName::MsrpMapping_VlanID] = mapping.vlanID;
}
inline void from_json(json const& j, MsrpMapping& mapping)
{
	j.at(keyName::MsrpMapping_TrafficClass).get_to(mapping.trafficClass);
	j.at(keyName::MsrpMapping_Priority).get_to(mapping.priority);
	j.at(keyName::MsrpMapping_VlanID).get_to(mapping.vlanID);
}

/* AudioMapping conversion */
inline void to_json(json& j, AudioMapping const& mapping)
{
	j[keyName::AudioMapping_StreamIndex] = mapping.streamIndex;
	j[keyName::AudioMapping_StreamChannel] = mapping.streamChannel;
	j[keyName::AudioMapping_ClusterOffset] = mapping.clusterOffset;
	j[keyName::AudioMapping_ClusterChannel] = mapping.clusterChannel;
}
inline void from_json(json const& j, AudioMapping& mapping)
{
	j.at(keyName::AudioMapping_StreamIndex).get_to(mapping.streamIndex);
	j.at(keyName::AudioMapping_StreamChannel).get_to(mapping.streamChannel);
	j.at(keyName::AudioMapping_ClusterOffset).get_to(mapping.clusterOffset);
	j.at(keyName::AudioMapping_ClusterChannel).get_to(mapping.clusterChannel);
}

/* StreamIdentification conversion */
inline void to_json(json& j, StreamIdentification const& stream)
{
	j[keyName::StreamIdentification_EntityID] = stream.entityID;
	j[keyName::StreamIdentification_StreamIndex] = stream.streamIndex;
}
inline void from_json(json const& j, StreamIdentification& stream)
{
	j.at(keyName::StreamIdentification_EntityID).get_to(stream.entityID);
	j.at(keyName::StreamIdentification_StreamIndex).get_to(stream.streamIndex);
}

/* StreamDynamicInfo conversion */
inline void to_json(json& j, StreamDynamicInfo const& info)
{
	j[keyName::StreamDynamicInfo_IsClassB] = info.isClassB;
	j[keyName::StreamDynamicInfo_HasSavedState] = info.hasSavedState;
	j[keyName::StreamDynamicInfo_DoesSupportEncrypted] = info.doesSupportEncrypted;
	j[keyName::StreamDynamicInfo_ArePdusEncrypted] = info.arePdusEncrypted;
	j[keyName::StreamDynamicInfo_HasTalkerFailed] = info.hasTalkerFailed;
	j[keyName::StreamDynamicInfo_Flags] = info._streamInfoFlags;
	j[keyName::StreamDynamicInfo_StreamID] = info.streamID;
	j[keyName::StreamDynamicInfo_MsrpAccumulatedLatency] = info.msrpAccumulatedLatency;
	if (info.streamDestMac)
	{
		j[keyName::StreamDynamicInfo_StreamDestMac] = networkInterface::NetworkInterfaceHelper::macAddressToString(*info.streamDestMac, true);
	}
	j[keyName::StreamDynamicInfo_MsrpFailureCode] = info.msrpFailureCode;
	if (info.msrpFailureBridgeID)
	{
		j[keyName::StreamDynamicInfo_MsrpFailureBridgeID] = utils::toHexString(*info.msrpFailureBridgeID, true, true);
	}
	j[keyName::StreamDynamicInfo_StreamVlanID] = info.streamVlanID;
	// Milan additions
	j[keyName::StreamDynamicInfo_FlagsEx] = info.streamInfoFlagsEx;
	j[keyName::StreamDynamicInfo_ProbingStatus] = info.probingStatus;
	j[keyName::StreamDynamicInfo_AcmpStatus] = info.acmpStatus;
}
inline void from_json(json const& j, StreamDynamicInfo& info)
{
	j.at(keyName::StreamDynamicInfo_IsClassB).get_to(info.isClassB);
	j.at(keyName::StreamDynamicInfo_HasSavedState).get_to(info.hasSavedState);
	j.at(keyName::StreamDynamicInfo_DoesSupportEncrypted).get_to(info.doesSupportEncrypted);
	j.at(keyName::StreamDynamicInfo_ArePdusEncrypted).get_to(info.arePdusEncrypted);
	j.at(keyName::StreamDynamicInfo_HasTalkerFailed).get_to(info.hasTalkerFailed);
	get_optional_value(j, keyName::StreamDynamicInfo_Flags, info._streamInfoFlags);
	get_optional_value(j, keyName::StreamDynamicInfo_StreamID, info.streamID);
	get_optional_value(j, keyName::StreamDynamicInfo_MsrpAccumulatedLatency, info.msrpAccumulatedLatency);
	{
		auto const it = j.find(keyName::StreamDynamicInfo_StreamDestMac);
		if (it != j.end())
		{
			info.streamDestMac = networkInterface::NetworkInterfaceHelper::stringToMacAddress(it->get<std::string>());
		}
	}
	{
		auto const it = j.find(keyName::StreamDynamicInfo_MsrpFailureCode);
		if (it != j.end() && !it->is_null())
		{
			// Check for new format (string)
			if (it->is_string())
			{
				it->get_to(info.msrpFailureCode);
			}
			// Convert from old format (number)
			else
			{
				auto failureCode = std::uint8_t{ 0u };
				it->get_to(failureCode);
				info.msrpFailureCode = static_cast<decltype(info.msrpFailureCode)::value_type>(failureCode);
			}
		}
	}
	{
		auto const it = j.find(keyName::StreamDynamicInfo_MsrpFailureBridgeID);
		if (it != j.end())
		{
			info.msrpFailureBridgeID = utils::convertFromString<decltype(info.msrpFailureBridgeID)::value_type>(it->get<std::string>().c_str());
		}
	}
	get_optional_value(j, keyName::StreamDynamicInfo_StreamVlanID, info.streamVlanID);
	// Milan additions
	get_optional_value(j, keyName::StreamDynamicInfo_FlagsEx, info.streamInfoFlagsEx);
	get_optional_value(j, keyName::StreamDynamicInfo_ProbingStatus, info.probingStatus);
	get_optional_value(j, keyName::StreamDynamicInfo_AcmpStatus, info.acmpStatus);
}

/* AvbInterfaceInfo conversion */
inline void to_json(json& j, AvbInterfaceInfo const& info)
{
	j[keyName::AvbInterfaceInfo_PropagationDelay] = info.propagationDelay;
	j[keyName::AvbInterfaceInfo_Flags] = info.flags;
	j[keyName::AvbInterfaceInfo_MsrpMappings] = info.mappings;
}
inline void from_json(json const& j, AvbInterfaceInfo& info)
{
	j.at(keyName::AvbInterfaceInfo_PropagationDelay).get_to(info.propagationDelay);
	j.at(keyName::AvbInterfaceInfo_Flags).get_to(info.flags);
	j.at(keyName::AvbInterfaceInfo_MsrpMappings).get_to(info.mappings);
}

/* AsPath conversion */
inline void to_json(json& j, AsPath const& path)
{
	j = path.sequence;
}
inline void from_json(json const& j, AsPath& path)
{
	j.get_to(path.sequence);
}

/* EntityNodeStaticModel conversion */
inline void to_json(json& j, EntityNodeStaticModel const& s)
{
	j[keyName::EntityNode_Static_VendorNameString] = s.vendorNameString;
	j[keyName::EntityNode_Static_ModelNameString] = s.modelNameString;
}
inline void from_json(json const& j, EntityNodeStaticModel& s)
{
	get_optional_value(j, keyName::EntityNode_Static_VendorNameString, s.vendorNameString);
	get_optional_value(j, keyName::EntityNode_Static_ModelNameString, s.modelNameString);
}

/* EntityNodeDynamicModel conversion */
inline void to_json(json& j, EntityNodeDynamicModel const& d)
{
	j[keyName::EntityNode_Dynamic_EntityName] = d.entityName;
	j[keyName::EntityNode_Dynamic_GroupName] = d.groupName;
	j[keyName::EntityNode_Dynamic_FirmwareVersion] = d.firmwareVersion;
	j[keyName::EntityNode_Dynamic_SerialNumber] = d.serialNumber;
	j[keyName::EntityNode_Dynamic_CurrentConfiguration] = d.currentConfiguration;
	j[keyName::EntityNode_Dynamic_Counters] = d.counters;
}
inline void from_json(json const& j, EntityNodeDynamicModel& d)
{
	j.at(keyName::EntityNode_Dynamic_EntityName).get_to(d.entityName);
	j.at(keyName::EntityNode_Dynamic_GroupName).get_to(d.groupName);
	j.at(keyName::EntityNode_Dynamic_FirmwareVersion).get_to(d.firmwareVersion);
	j.at(keyName::EntityNode_Dynamic_SerialNumber).get_to(d.serialNumber);
	j.at(keyName::EntityNode_Dynamic_CurrentConfiguration).get_to(d.currentConfiguration);
	get_optional_value(j, keyName::EntityNode_Dynamic_Counters, d.counters);
}

/* ConfigurationNodeStaticModel conversion */
inline void to_json(json& j, ConfigurationNodeStaticModel const& s)
{
	j[keyName::ConfigurationNode_Static_LocalizedDescription] = s.localizedDescription;
	j[keyName::ConfigurationNode_Static_DescriptorCounts] = s.descriptorCounts;
}
inline void from_json(json const& j, ConfigurationNodeStaticModel& s)
{
	get_optional_value(j, keyName::ConfigurationNode_Static_LocalizedDescription, s.localizedDescription);
	get_optional_value(j, keyName::ConfigurationNode_Static_DescriptorCounts, s.descriptorCounts);
}

/* ConfigurationNodeDynamicModel conversion */
inline void to_json(json& j, ConfigurationNodeDynamicModel const& d)
{
	j[keyName::ConfigurationNode_Dynamic_ObjectName] = d.objectName;
}
inline void from_json(json const& j, ConfigurationNodeDynamicModel& s)
{
	get_optional_value(j, keyName::ConfigurationNode_Dynamic_ObjectName, s.objectName);
}

/* AudioUnitNodeStaticModel conversion */
inline void to_json(json& j, AudioUnitNodeStaticModel const& s)
{
	j[keyName::AudioUnitNode_Static_LocalizedDescription] = s.localizedDescription;
	j[keyName::AudioUnitNode_Static_ClockDomainIndex] = s.clockDomainIndex;
	j[keyName::AudioUnitNode_Static_SamplingRates] = s.samplingRates;
	j[keyName::AudioUnitNode_Static_ControlCounts] = s.numberOfControls;
}
inline void from_json(json const& j, AudioUnitNodeStaticModel& s)
{
	get_optional_value(j, keyName::AudioUnitNode_Static_LocalizedDescription, s.localizedDescription);
	j.at(keyName::AudioUnitNode_Static_ClockDomainIndex).get_to(s.clockDomainIndex);
	j.at(keyName::AudioUnitNode_Static_SamplingRates).get_to(s.samplingRates);
	get_optional_value(j, keyName::AudioUnitNode_Static_ControlCounts, s.numberOfControls);
}

/* AudioUnitNodeDynamicModel conversion */
inline void to_json(json& j, AudioUnitNodeDynamicModel const& d)
{
	j[keyName::AudioUnitNode_Dynamic_ObjectName] = d.objectName;
	j[keyName::AudioUnitNode_Dynamic_CurrentSamplingRate] = d.currentSamplingRate;
}
inline void from_json(json const& j, AudioUnitNodeDynamicModel& d)
{
	get_optional_value(j, keyName::AudioUnitNode_Dynamic_ObjectName, d.objectName);
	j.at(keyName::AudioUnitNode_Dynamic_CurrentSamplingRate).get_to(d.currentSamplingRate);
}

/* StreamNodeStaticModel conversion */
inline void to_json(json& j, StreamNodeStaticModel const& s)
{
	j[keyName::StreamNode_Static_LocalizedDescription] = s.localizedDescription;
	j[keyName::StreamNode_Static_ClockDomainIndex] = s.clockDomainIndex;
	j[keyName::StreamNode_Static_StreamFlags] = s.streamFlags;
	j[keyName::StreamNode_Static_BackupTalkerEntityID0] = s.backupTalkerEntityID_0;
	j[keyName::StreamNode_Static_BackupTalkerUniqueID0] = s.backupTalkerUniqueID_0;
	j[keyName::StreamNode_Static_BackupTalkerEntityID1] = s.backupTalkerEntityID_1;
	j[keyName::StreamNode_Static_BackupTalkerUniqueID1] = s.backupTalkerUniqueID_1;
	j[keyName::StreamNode_Static_BackupTalkerEntityID2] = s.backupTalkerEntityID_2;
	j[keyName::StreamNode_Static_BackupTalkerUniqueID2] = s.backupTalkerUniqueID_2;
	j[keyName::StreamNode_Static_BackedupTalkerEntityID] = s.backedupTalkerEntityID;
	j[keyName::StreamNode_Static_BackedupTalkerUnique] = s.backedupTalkerUnique;
	j[keyName::StreamNode_Static_AvbInterfaceIndex] = s.avbInterfaceIndex;
	j[keyName::StreamNode_Static_BufferLength] = s.bufferLength;
	j[keyName::StreamNode_Static_Formats] = s.formats;
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
	if (!s.redundantStreams.empty())
	{
		j[keyName::StreamNode_Static_RedundantStreams] = s.redundantStreams;
	}
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
}
inline void from_json(json const& j, StreamNodeStaticModel& s)
{
	get_optional_value(j, keyName::StreamNode_Static_LocalizedDescription, s.localizedDescription);
	j.at(keyName::StreamNode_Static_ClockDomainIndex).get_to(s.clockDomainIndex);
	j.at(keyName::StreamNode_Static_StreamFlags).get_to(s.streamFlags);
	get_optional_value(j, keyName::StreamNode_Static_BackupTalkerEntityID0, s.backupTalkerEntityID_0);
	get_optional_value(j, keyName::StreamNode_Static_BackupTalkerUniqueID0, s.backupTalkerUniqueID_0);
	get_optional_value(j, keyName::StreamNode_Static_BackupTalkerEntityID1, s.backupTalkerEntityID_1);
	get_optional_value(j, keyName::StreamNode_Static_BackupTalkerUniqueID1, s.backupTalkerUniqueID_1);
	get_optional_value(j, keyName::StreamNode_Static_BackupTalkerEntityID2, s.backupTalkerEntityID_2);
	get_optional_value(j, keyName::StreamNode_Static_BackupTalkerUniqueID2, s.backupTalkerUniqueID_2);
	get_optional_value(j, keyName::StreamNode_Static_BackedupTalkerEntityID, s.backedupTalkerEntityID);
	get_optional_value(j, keyName::StreamNode_Static_BackedupTalkerUnique, s.backedupTalkerUnique);
	j.at(keyName::StreamNode_Static_AvbInterfaceIndex).get_to(s.avbInterfaceIndex);
	j.at(keyName::StreamNode_Static_BufferLength).get_to(s.bufferLength);
	j.at(keyName::StreamNode_Static_Formats).get_to(s.formats);
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
	get_optional_value(j, keyName::StreamNode_Static_RedundantStreams, s.redundantStreams);
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
}

/* StreamInputNodeDynamicModel conversion */
inline void to_json(json& j, StreamInputNodeDynamicModel const& d)
{
	j[keyName::StreamInputNode_Dynamic_ObjectName] = d.objectName;
	j[keyName::StreamInputNode_Dynamic_StreamFormat] = d.streamFormat;
	j[keyName::StreamInputNode_Dynamic_StreamRunning] = d.isStreamRunning;
	j[keyName::StreamInputNode_Dynamic_StreamDynamicInfo] = d.streamDynamicInfo;
	j[keyName::StreamInputNode_Dynamic_ConnectedTalker] = d.connectionInfo.talkerStream;
	j[keyName::StreamInputNode_Dynamic_ConnectionState] = d.connectionInfo.state;
	j[keyName::StreamInputNode_Dynamic_Counters] = d.counters;
}
inline void from_json(json const& j, StreamInputNodeDynamicModel& d)
{
	get_optional_value(j, keyName::StreamInputNode_Dynamic_ObjectName, d.objectName);
	j.at(keyName::StreamInputNode_Dynamic_StreamFormat).get_to(d.streamFormat);
	get_optional_value(j, keyName::StreamInputNode_Dynamic_StreamRunning, d.isStreamRunning);
	get_optional_value(j, keyName::StreamInputNode_Dynamic_StreamDynamicInfo, d.streamDynamicInfo);
	get_optional_value(j, keyName::StreamInputNode_Dynamic_ConnectedTalker, d.connectionInfo.talkerStream);
	get_optional_value(j, keyName::StreamInputNode_Dynamic_ConnectionState, d.connectionInfo.state);
	// Backward compatibility handling (StreamInputNode_Dynamic_ConnectionState was added after initial release)
	{
		// If we have a connection (talkerStream being defined to something) but State is set to NotConnected, it means it was not saved
		if (d.connectionInfo.talkerStream.entityID && d.connectionInfo.state == StreamInputConnectionInfo::State::NotConnected)
		{
			// Force it to Connected (impossible to get the ConnectionFlags here, so we cannot detect if it's a FastConnect State)
			d.connectionInfo.state = StreamInputConnectionInfo::State::Connected;
		}
	}
	get_optional_value(j, keyName::StreamInputNode_Dynamic_Counters, d.counters);
}

/* StreamOutputNodeDynamicModel conversion */
inline void to_json(json& j, StreamOutputNodeDynamicModel const& d)
{
	j[keyName::StreamOutputNode_Dynamic_ObjectName] = d.objectName;
	j[keyName::StreamOutputNode_Dynamic_StreamFormat] = d.streamFormat;
	j[keyName::StreamOutputNode_Dynamic_StreamRunning] = d.isStreamRunning;
	j[keyName::StreamOutputNode_Dynamic_StreamDynamicInfo] = d.streamDynamicInfo;
	j[keyName::StreamOutputNode_Dynamic_Counters] = d.counters;
}
inline void from_json(json const& j, StreamOutputNodeDynamicModel& d)
{
	get_optional_value(j, keyName::StreamOutputNode_Dynamic_ObjectName, d.objectName);
	j.at(keyName::StreamOutputNode_Dynamic_StreamFormat).get_to(d.streamFormat);
	get_optional_value(j, keyName::StreamOutputNode_Dynamic_StreamRunning, d.isStreamRunning);
	get_optional_value(j, keyName::StreamOutputNode_Dynamic_StreamDynamicInfo, d.streamDynamicInfo);
	get_optional_value(j, keyName::StreamOutputNode_Dynamic_Counters, d.counters);
}

/* JackNodeStaticModel conversion */
inline void to_json(json& j, JackNodeStaticModel const& s)
{
	j[keyName::JackNode_Static_LocalizedDescription] = s.localizedDescription;
	j[keyName::JackNode_Static_Flags] = s.jackFlags;
	j[keyName::JackNode_Static_Type] = s.jackType;
	j[keyName::JackNode_Static_ControlCounts] = s.numberOfControls;
}
inline void from_json(json const& j, JackNodeStaticModel& s)
{
	get_optional_value(j, keyName::JackNode_Static_LocalizedDescription, s.localizedDescription);
	j.at(keyName::JackNode_Static_Flags).get_to(s.jackFlags);
	j.at(keyName::JackNode_Static_Type).get_to(s.jackType);
	get_optional_value(j, keyName::JackNode_Static_ControlCounts, s.numberOfControls);
}

/* JackNodeDynamicModel conversion */
inline void to_json(json& j, JackNodeDynamicModel const& d)
{
	j[keyName::JackNode_Dynamic_ObjectName] = d.objectName;
}
inline void from_json(json const& j, JackNodeDynamicModel& d)
{
	get_optional_value(j, keyName::JackNode_Dynamic_ObjectName, d.objectName);
}

/* AvbInterfaceNodeStaticModel conversion */
inline void to_json(json& j, AvbInterfaceNodeStaticModel const& s)
{
	j[keyName::AvbInterfaceNode_Static_LocalizedDescription] = s.localizedDescription;
	j[keyName::AvbInterfaceNode_Static_MacAddress] = networkInterface::NetworkInterfaceHelper::macAddressToString(s.macAddress, true);
	j[keyName::AvbInterfaceNode_Static_Flags] = s.interfaceFlags;
	j[keyName::AvbInterfaceNode_Static_ClockIdentity] = s.clockIdentity;
	j[keyName::AvbInterfaceNode_Static_Priority1] = s.priority1;
	j[keyName::AvbInterfaceNode_Static_ClockClass] = s.clockClass;
	j[keyName::AvbInterfaceNode_Static_OffsetScaledLogVariance] = s.offsetScaledLogVariance;
	j[keyName::AvbInterfaceNode_Static_ClockAccuracy] = s.clockAccuracy;
	j[keyName::AvbInterfaceNode_Static_Priority2] = s.priority2;
	j[keyName::AvbInterfaceNode_Static_DomainNumber] = s.domainNumber;
	j[keyName::AvbInterfaceNode_Static_LogSyncInterval] = s.logSyncInterval;
	j[keyName::AvbInterfaceNode_Static_LogAnnounceInterval] = s.logAnnounceInterval;
	j[keyName::AvbInterfaceNode_Static_LogPdelayInterval] = s.logPDelayInterval;
	j[keyName::AvbInterfaceNode_Static_PortNumber] = s.portNumber;
}
inline void from_json(json const& j, AvbInterfaceNodeStaticModel& s)
{
	get_optional_value(j, keyName::AvbInterfaceNode_Static_LocalizedDescription, s.localizedDescription);
	s.macAddress = networkInterface::NetworkInterfaceHelper::stringToMacAddress(j.at(keyName::AvbInterfaceNode_Static_MacAddress).get<std::string>());
	j.at(keyName::AvbInterfaceNode_Static_Flags).get_to(s.interfaceFlags);
	j.at(keyName::AvbInterfaceNode_Static_ClockIdentity).get_to(s.clockIdentity);
	j.at(keyName::AvbInterfaceNode_Static_Priority1).get_to(s.priority1);
	j.at(keyName::AvbInterfaceNode_Static_ClockClass).get_to(s.clockClass);
	j.at(keyName::AvbInterfaceNode_Static_OffsetScaledLogVariance).get_to(s.offsetScaledLogVariance);
	j.at(keyName::AvbInterfaceNode_Static_ClockAccuracy).get_to(s.clockAccuracy);
	j.at(keyName::AvbInterfaceNode_Static_Priority2).get_to(s.priority2);
	j.at(keyName::AvbInterfaceNode_Static_DomainNumber).get_to(s.domainNumber);
	j.at(keyName::AvbInterfaceNode_Static_LogSyncInterval).get_to(s.logSyncInterval);
	j.at(keyName::AvbInterfaceNode_Static_LogAnnounceInterval).get_to(s.logAnnounceInterval);
	j.at(keyName::AvbInterfaceNode_Static_LogPdelayInterval).get_to(s.logPDelayInterval);
	j.at(keyName::AvbInterfaceNode_Static_PortNumber).get_to(s.portNumber);
}

/* AvbInterfaceNodeDynamicModel conversion */
inline void to_json(json& j, AvbInterfaceNodeDynamicModel const& d)
{
	j[keyName::AvbInterfaceNode_Dynamic_ObjectName] = d.objectName;
	j[keyName::AvbInterfaceNode_Dynamic_GptpGrandmasterID] = d.gptpGrandmasterID;
	j[keyName::AvbInterfaceNode_Dynamic_GptpDomainNumber] = d.gptpDomainNumber;
	j[keyName::AvbInterfaceNode_Dynamic_AvbInterfaceInfo] = d.avbInterfaceInfo;
	j[keyName::AvbInterfaceNode_Dynamic_AsPath] = d.asPath;
	j[keyName::AvbInterfaceNode_Dynamic_Counters] = d.counters;
}
inline void from_json(json const& j, AvbInterfaceNodeDynamicModel& d)
{
	get_optional_value(j, keyName::AvbInterfaceNode_Dynamic_ObjectName, d.objectName);
	j.at(keyName::AvbInterfaceNode_Dynamic_GptpGrandmasterID).get_to(d.gptpGrandmasterID);
	j.at(keyName::AvbInterfaceNode_Dynamic_GptpDomainNumber).get_to(d.gptpDomainNumber);
	get_optional_value(j, keyName::AvbInterfaceNode_Dynamic_AvbInterfaceInfo, d.avbInterfaceInfo);
	get_optional_value(j, keyName::AvbInterfaceNode_Dynamic_AsPath, d.asPath);
	get_optional_value(j, keyName::AvbInterfaceNode_Dynamic_Counters, d.counters);
}

/* ClockSourceNodeStaticModel conversion */
inline void to_json(json& j, ClockSourceNodeStaticModel const& s)
{
	j[keyName::ClockSourceNode_Static_LocalizedDescription] = s.localizedDescription;
	j[keyName::ClockSourceNode_Static_ClockSourceType] = s.clockSourceType;
	j[keyName::ClockSourceNode_Static_ClockSourceLocationType] = s.clockSourceLocationType;
	j[keyName::ClockSourceNode_Static_ClockSourceLocationIndex] = s.clockSourceLocationIndex;
}
inline void from_json(json const& j, ClockSourceNodeStaticModel& s)
{
	j.at(keyName::ClockSourceNode_Static_LocalizedDescription).get_to(s.localizedDescription);
	j.at(keyName::ClockSourceNode_Static_ClockSourceType).get_to(s.clockSourceType);
	j.at(keyName::ClockSourceNode_Static_ClockSourceLocationType).get_to(s.clockSourceLocationType);
	j.at(keyName::ClockSourceNode_Static_ClockSourceLocationIndex).get_to(s.clockSourceLocationIndex);
}

/* ClockSourceNodeDynamicModel conversion */
inline void to_json(json& j, ClockSourceNodeDynamicModel const& d)
{
	j[keyName::ClockSourceNode_Dynamic_ObjectName] = d.objectName;
	j[keyName::ClockSourceNode_Dynamic_ClockSourceFlags] = d.clockSourceFlags;
	j[keyName::ClockSourceNode_Dynamic_ClockSourceIdentifier] = d.clockSourceIdentifier;
}
inline void from_json(json const& j, ClockSourceNodeDynamicModel& d)
{
	get_optional_value(j, keyName::ClockSourceNode_Dynamic_ObjectName, d.objectName);
	get_optional_value(j, keyName::ClockSourceNode_Dynamic_ClockSourceFlags, d.clockSourceFlags);
	get_optional_value(j, keyName::ClockSourceNode_Dynamic_ClockSourceIdentifier, d.clockSourceIdentifier);
}

/* MemoryObjectNodeStaticModel conversion */
inline void to_json(json& j, MemoryObjectNodeStaticModel const& s)
{
	j[keyName::MemoryObjectNode_Static_LocalizedDescription] = s.localizedDescription;
	j[keyName::MemoryObjectNode_Static_MemoryObjectType] = s.memoryObjectType;
	j[keyName::MemoryObjectNode_Static_TargetDescriptorType] = s.targetDescriptorType;
	j[keyName::MemoryObjectNode_Static_TargetDescriptorIndex] = s.targetDescriptorIndex;
	j[keyName::MemoryObjectNode_Static_StartAddress] = utils::toHexString(s.startAddress, true, true);
	j[keyName::MemoryObjectNode_Static_MaximumLength] = s.maximumLength;
}
inline void from_json(json const& j, MemoryObjectNodeStaticModel& s)
{
	get_optional_value(j, keyName::MemoryObjectNode_Static_LocalizedDescription, s.localizedDescription);
	j.at(keyName::MemoryObjectNode_Static_MemoryObjectType).get_to(s.memoryObjectType);
	j.at(keyName::MemoryObjectNode_Static_TargetDescriptorType).get_to(s.targetDescriptorType);
	j.at(keyName::MemoryObjectNode_Static_TargetDescriptorIndex).get_to(s.targetDescriptorIndex);
	s.startAddress = utils::convertFromString<decltype(s.startAddress)>(j.at(keyName::MemoryObjectNode_Static_StartAddress).get<std::string>().c_str());
	j.at(keyName::MemoryObjectNode_Static_MaximumLength).get_to(s.maximumLength);
}

/* MemoryObjectNodeDynamicModel conversion */
inline void to_json(json& j, MemoryObjectNodeDynamicModel const& d)
{
	j[keyName::MemoryObjectNode_Dynamic_ObjectName] = d.objectName;
	j[keyName::MemoryObjectNode_Dynamic_Length] = d.length;
}
inline void from_json(json const& j, MemoryObjectNodeDynamicModel& d)
{
	get_optional_value(j, keyName::MemoryObjectNode_Dynamic_ObjectName, d.objectName);
	j.at(keyName::MemoryObjectNode_Dynamic_Length).get_to(d.length);
}

/* LocaleNodeStaticModel conversion */
inline void to_json(json& j, LocaleNodeStaticModel const& s)
{
	j[keyName::LocaleNode_Static_LocaleID] = s.localeID;
	j[keyName::LocaleNode_Static_Informative_BaseStringDescriptor] = s.baseStringDescriptorIndex;
}
inline void from_json(json const& j, LocaleNodeStaticModel& s)
{
	j.at(keyName::LocaleNode_Static_LocaleID).get_to(s.localeID);
}

/* StringsNodeStaticModel conversion */
inline void to_json(json& j, StringsNodeStaticModel const& s)
{
	j[keyName::StringsNode_Static_Strings] = s.strings;
}
inline void from_json(json const& j, StringsNodeStaticModel& s)
{
	j.at(keyName::StringsNode_Static_Strings).get_to(s.strings);
}

/* StreamPortNodeStaticModel conversion */
inline void to_json(json& j, StreamPortNodeStaticModel const& s)
{
	j[keyName::StreamPortNode_Static_ClockDomainIndex] = s.clockDomainIndex;
	j[keyName::StreamPortNode_Static_Flags] = s.portFlags;
	j[keyName::StreamPortNode_Static_ControlCounts] = s.numberOfControls;
}
inline void from_json(json const& j, StreamPortNodeStaticModel& s)
{
	j.at(keyName::StreamPortNode_Static_ClockDomainIndex).get_to(s.clockDomainIndex);
	j.at(keyName::StreamPortNode_Static_Flags).get_to(s.portFlags);
	get_optional_value(j, keyName::StreamPortNode_Static_ControlCounts, s.numberOfControls);
}

/* StreamPortNodeDynamicModel conversion */
inline void to_json(json& j, StreamPortNodeDynamicModel const& d)
{
	j[keyName::StreamPortNode_Dynamic_DynamicMappings] = d.dynamicAudioMap;
}
inline void from_json(json const& j, StreamPortNodeDynamicModel& d)
{
	get_optional_value(j, keyName::StreamPortNode_Dynamic_DynamicMappings, d.dynamicAudioMap);
}

/* AudioClusterNodeStaticModel conversion */
inline void to_json(json& j, AudioClusterNodeStaticModel const& s)
{
	j[keyName::AudioClusterNode_Static_LocalizedDescription] = s.localizedDescription;
	j[keyName::AudioClusterNode_Static_SignalType] = s.signalType;
	j[keyName::AudioClusterNode_Static_SignalIndex] = s.signalIndex;
	j[keyName::AudioClusterNode_Static_SignalOutput] = s.signalOutput;
	j[keyName::AudioClusterNode_Static_PathLatency] = s.pathLatency;
	j[keyName::AudioClusterNode_Static_BlockLatency] = s.blockLatency;
	j[keyName::AudioClusterNode_Static_ChannelCount] = s.channelCount;
	j[keyName::AudioClusterNode_Static_Format] = s.format;
}
inline void from_json(json const& j, AudioClusterNodeStaticModel& s)
{
	get_optional_value(j, keyName::AudioClusterNode_Static_LocalizedDescription, s.localizedDescription);
	j.at(keyName::AudioClusterNode_Static_SignalType).get_to(s.signalType);
	j.at(keyName::AudioClusterNode_Static_SignalIndex).get_to(s.signalIndex);
	j.at(keyName::AudioClusterNode_Static_SignalOutput).get_to(s.signalOutput);
	j.at(keyName::AudioClusterNode_Static_PathLatency).get_to(s.pathLatency);
	j.at(keyName::AudioClusterNode_Static_BlockLatency).get_to(s.blockLatency);
	j.at(keyName::AudioClusterNode_Static_ChannelCount).get_to(s.channelCount);
	j.at(keyName::AudioClusterNode_Static_Format).get_to(s.format);
}

/* AudioClusterNodeDynamicModel conversion */
inline void to_json(json& j, AudioClusterNodeDynamicModel const& d)
{
	j[keyName::AudioClusterNode_Dynamic_ObjectName] = d.objectName;
}
inline void from_json(json const& j, AudioClusterNodeDynamicModel& d)
{
	get_optional_value(j, keyName::AudioClusterNode_Dynamic_ObjectName, d.objectName);
}

/* AudioMapNodeStaticModel conversion */
inline void to_json(json& j, AudioMapNodeStaticModel const& s)
{
	j[keyName::AudioMapNode_Static_Mappings] = s.mappings;
}
inline void from_json(json const& j, AudioMapNodeStaticModel& s)
{
	j.at(keyName::AudioMapNode_Static_Mappings).get_to(s.mappings);
}

/* LinearValueStatic conversion - IEEE1722.1-2013 Clause 7.3.5.2.1 */
template<typename SizeType, typename = std::enable_if_t<std::is_arithmetic_v<SizeType>>>
inline void to_json(json& j, LinearValueStatic<SizeType> const& value)
{
	j[keyName::LinearValue_Minimum] = value.minimum;
	j[keyName::LinearValue_Maximum] = value.maximum;
	j[keyName::LinearValue_Step] = value.step;
	j[keyName::LinearValue_Default] = value.defaultValue;
	j[keyName::LinearValue_Unit] = value.unit;
	j[keyName::LinearValue_String] = value.localizedName;
}
template<typename SizeType, typename = std::enable_if_t<std::is_arithmetic_v<SizeType>>>
inline void from_json(json const& j, LinearValueStatic<SizeType>& value)
{
	j.at(keyName::LinearValue_Minimum).get_to(value.minimum);
	j.at(keyName::LinearValue_Maximum).get_to(value.maximum);
	j.at(keyName::LinearValue_Step).get_to(value.step);
	j.at(keyName::LinearValue_Default).get_to(value.defaultValue);
	j.at(keyName::LinearValue_Unit).get_to(value.unit);
	get_optional_value(j, keyName::LinearValue_String, value.localizedName);
}

/* LinearValueDynamic conversion - IEEE1722.1-2013 Clause 7.3.5.2.1 */
template<typename SizeType, typename = std::enable_if_t<std::is_arithmetic_v<SizeType>>>
inline void to_json(json& j, LinearValueDynamic<SizeType> const& value)
{
	j = value.currentValue;
}
template<typename SizeType, typename = std::enable_if_t<std::is_arithmetic_v<SizeType>>>
inline void from_json(json const& j, LinearValueDynamic<SizeType>& value)
{
	value.currentValue = j;
}

/* LinearValues conversion - IEEE1722.1-2013 Clause 7.3.5.2.1 */
template<typename ValueType, typename Traits = ControlValues::control_value_details_traits<LinearValues<ValueType>>>
inline void to_json(json& j, LinearValues<ValueType> const& values)
{
	static_assert(Traits::is_value_details, "to_json, ControlValues::control_value_details_traits::is_value_details trait not defined for requested ValueType. Did you include entityModelControlValuesTraits.hpp?");

	j = values.getValues();
}
template<typename ValueType, typename Traits = ControlValues::control_value_details_traits<LinearValues<ValueType>>>
inline void from_json(json const& j, LinearValues<ValueType>& values)
{
	static_assert(Traits::is_value_details, "from_json, ControlValues::control_value_details_traits::is_value_details trait not defined for requested ValueType. Did you include entityModelControlValuesTraits.hpp?");

	j.get_to(values.getValues());
}

/* SelectorValueStatic conversion - IEEE1722.1-2013 Clause 7.3.5.2.2 */
template<typename SizeType, typename = std::enable_if_t<std::is_arithmetic_v<SizeType> || std::is_same_v<SizeType, LocalizedStringReference>>, typename Traits = ControlValues::control_value_details_traits<SelectorValueStatic<SizeType>>>
inline void to_json(json& j, SelectorValueStatic<SizeType> const& value)
{
	static_assert(Traits::is_value_details, "from_json, ControlValues::control_value_details_traits::is_value_details trait not defined for requested ValueType. Did you include entityModelControlValuesTraits.hpp?");

	j[keyName::SelectorValue_Default] = value.defaultValue;
	j[keyName::SelectorValue_Unit] = value.unit;
	j[keyName::SelectorValue_Options] = value.options;
}
template<typename SizeType, typename = std::enable_if_t<std::is_arithmetic_v<SizeType> || std::is_same_v<SizeType, LocalizedStringReference>>, typename Traits = ControlValues::control_value_details_traits<SelectorValueStatic<SizeType>>>
inline void from_json(json const& j, SelectorValueStatic<SizeType>& value)
{
	static_assert(Traits::is_value_details, "from_json, ControlValues::control_value_details_traits::is_value_details trait not defined for requested ValueType. Did you include entityModelControlValuesTraits.hpp?");

	j.at(keyName::SelectorValue_Default).get_to(value.defaultValue);
	j.at(keyName::SelectorValue_Unit).get_to(value.unit);
	j.at(keyName::SelectorValue_Options).get_to(value.options);
}

/* SelectorValueDynamic conversion - IEEE1722.1-2013 Clause 7.3.5.2.2 */
template<typename SizeType, typename = std::enable_if_t<std::is_arithmetic_v<SizeType> || std::is_same_v<SizeType, LocalizedStringReference>>, typename Traits = ControlValues::control_value_details_traits<SelectorValueDynamic<SizeType>>>
inline void to_json(json& j, SelectorValueDynamic<SizeType> const& value)
{
	static_assert(Traits::is_value_details, "to_json, ControlValues::control_value_details_traits::is_value_details trait not defined for requested ValueType. Did you include entityModelControlValuesTraits.hpp?");

	j = value.currentValue;
}
template<typename SizeType, typename = std::enable_if_t<std::is_arithmetic_v<SizeType> || std::is_same_v<SizeType, LocalizedStringReference>>, typename Traits = ControlValues::control_value_details_traits<SelectorValueDynamic<SizeType>>>
inline void from_json(json const& j, SelectorValueDynamic<SizeType>& value)
{
	static_assert(Traits::is_value_details, "from_json, ControlValues::control_value_details_traits::is_value_details trait not defined for requested ValueType. Did you include entityModelControlValuesTraits.hpp?");

	j.get_to(value.currentValue);
}

/* ArrayValueStatic conversion - IEEE1722.1-2013 Clause 7.3.5.2.3 */
template<typename SizeType, typename = std::enable_if_t<std::is_arithmetic_v<SizeType>>, typename Traits = ControlValues::control_value_details_traits<ArrayValueStatic<SizeType>>>
inline void to_json(json& j, ArrayValueStatic<SizeType> const& value)
{
	static_assert(Traits::is_value_details, "from_json, ControlValues::control_value_details_traits::is_value_details trait not defined for requested ValueType. Did you include entityModelControlValuesTraits.hpp?");

	j[keyName::ArrayValue_Minimum] = value.minimum;
	j[keyName::ArrayValue_Maximum] = value.maximum;
	j[keyName::ArrayValue_Step] = value.step;
	j[keyName::ArrayValue_Default] = value.defaultValue;
	j[keyName::ArrayValue_Unit] = value.unit;
	j[keyName::ArrayValue_String] = value.localizedName;
}
template<typename SizeType, typename = std::enable_if_t<std::is_arithmetic_v<SizeType>>, typename Traits = ControlValues::control_value_details_traits<ArrayValueStatic<SizeType>>>
inline void from_json(json const& j, ArrayValueStatic<SizeType>& value)
{
	static_assert(Traits::is_value_details, "from_json, ControlValues::control_value_details_traits::is_value_details trait not defined for requested ValueType. Did you include entityModelControlValuesTraits.hpp?");

	j.at(keyName::ArrayValue_Minimum).get_to(value.minimum);
	j.at(keyName::ArrayValue_Maximum).get_to(value.maximum);
	j.at(keyName::ArrayValue_Step).get_to(value.step);
	j.at(keyName::ArrayValue_Default).get_to(value.defaultValue);
	j.at(keyName::ArrayValue_Unit).get_to(value.unit);
	get_optional_value(j, keyName::ArrayValue_String, value.localizedName);
}

/* ArrayValueDynamic conversion - IEEE1722.1-2013 Clause 7.3.5.2.3 */
template<typename SizeType, typename = std::enable_if_t<std::is_arithmetic_v<SizeType>>, typename Traits = ControlValues::control_value_details_traits<ArrayValueDynamic<SizeType>>>
inline void to_json(json& j, ArrayValueDynamic<SizeType> const& value)
{
	static_assert(Traits::is_value_details, "to_json, ControlValues::control_value_details_traits::is_value_details trait not defined for requested ValueType. Did you include entityModelControlValuesTraits.hpp?");

	j = value.currentValues;
}
template<typename SizeType, typename = std::enable_if_t<std::is_arithmetic_v<SizeType>>, typename Traits = ControlValues::control_value_details_traits<ArrayValueDynamic<SizeType>>>
inline void from_json(json const& j, ArrayValueDynamic<SizeType>& value)
{
	static_assert(Traits::is_value_details, "from_json, ControlValues::control_value_details_traits::is_value_details trait not defined for requested ValueType. Did you include entityModelControlValuesTraits.hpp?");

	j.get_to(value.currentValues);
}

/* UTF8StringValueStatic conversion - IEEE1722.1-2013 Clause 7.3.5.2.4 */
inline void to_json(json& /*j*/, UTF8StringValueStatic const& /*value*/)
{
	// UTF8 Static has no data
}
inline void from_json(json const& /*j*/, UTF8StringValueStatic& /*value*/)
{
	// UTF8 Static has no data
}

/* UTF8StringValueDynamic conversion - IEEE1722.1-2013 Clause 7.3.5.2.4 */
inline void to_json(json& j, UTF8StringValueDynamic const& value)
{
	auto constexpr nullCharacter = decltype(value.currentValue)::value_type{ 0u };

	auto arr = json::array();

	// We only want to dump the actual UTF8 string (including the trailing NULL character), not the garbage inside the array after the trailing NULL character
	for (auto const c : value.currentValue)
	{
		arr.push_back(c);
		if (c == nullCharacter)
		{
			break;
		}
	}

	j = std::move(arr);
}
inline void from_json(json const& j, UTF8StringValueDynamic& value)
{
	// We must load the array manually as json will try to read the whole std::array size, which is not what we dumped (we only dump meaningful characters)
	auto const /*expr*/ maxLength = value.currentValue.size(); // Unsure why clang doesn't accept this as a constexpr
	auto constexpr nullCharacter = decltype(value.currentValue)::value_type{ 0u };

	// Count the number of bytes to copy (including the trailing NULL)
	auto length = size_t{ 0 };
	for (auto const& val : j)
	{
		value.currentValue[length++] = val;
		if (length >= maxLength)
		{
			break;
		}
	}

	// Enforce NULL terminated string
	if (length == maxLength || value.currentValue[maxLength - 1] != nullCharacter)
	{
		value.currentValue[maxLength - 1] = nullCharacter;
	}
}

/* ControlValues Dispatcher Helper Templates */
template<class ValueDetailsType, typename Traits = ControlValues::control_value_details_traits<std::decay_t<ValueDetailsType>>>
inline std::function<json(ControlValues const&)> createToJsonDispatchFunctor() noexcept
{
	static_assert(Traits::is_value_details, "createToJsonDispatchFunctor, control_value_details_traits::is_value_details trait not defined for requested ValueDetailsType. Did you include entityModelControlValuesTraits.hpp?");
	return [](ControlValues const& controlValues)
	{
		auto j = json{};
		try
		{
			auto const& values = controlValues.getValues<ValueDetailsType>();
			j[keyName::ControlValues_Type] = controlValues.getType();
			j[keyName::ControlValues_Values] = values;
		}
		catch (...)
		{
		}
		return j;
	};
}
template<class ControlValuesClass>
inline std::function<ControlValues(json const&)> createFromJsonDispatchFunctor() noexcept
{
	return [](json const& j)
	{
		auto controlValues = ControlValuesClass{};
		j.get_to(controlValues);
		return ControlValues{ controlValues };
	};
}

template<bool IsDynamic>
inline void createToJsonDispatchTable(std::unordered_map<ControlValueType::Type, std::function<json(ControlValues const&)>>& dispatchTable)
{
	if constexpr (IsDynamic)
	{
		/** Linear Values - IEEE1722.1-2013 Clause 7.3.5.2.1 */
		dispatchTable[ControlValueType::Type::ControlLinearInt8] = createToJsonDispatchFunctor<LinearValues<LinearValueDynamic<std::int8_t>>>();
		dispatchTable[ControlValueType::Type::ControlLinearUInt8] = createToJsonDispatchFunctor<LinearValues<LinearValueDynamic<std::uint8_t>>>();
		dispatchTable[ControlValueType::Type::ControlLinearInt16] = createToJsonDispatchFunctor<LinearValues<LinearValueDynamic<std::int16_t>>>();
		dispatchTable[ControlValueType::Type::ControlLinearUInt16] = createToJsonDispatchFunctor<LinearValues<LinearValueDynamic<std::uint16_t>>>();
		dispatchTable[ControlValueType::Type::ControlLinearInt32] = createToJsonDispatchFunctor<LinearValues<LinearValueDynamic<std::int32_t>>>();
		dispatchTable[ControlValueType::Type::ControlLinearUInt32] = createToJsonDispatchFunctor<LinearValues<LinearValueDynamic<std::uint32_t>>>();
		dispatchTable[ControlValueType::Type::ControlLinearInt64] = createToJsonDispatchFunctor<LinearValues<LinearValueDynamic<std::int64_t>>>();
		dispatchTable[ControlValueType::Type::ControlLinearUInt64] = createToJsonDispatchFunctor<LinearValues<LinearValueDynamic<std::uint64_t>>>();
		dispatchTable[ControlValueType::Type::ControlLinearFloat] = createToJsonDispatchFunctor<LinearValues<LinearValueDynamic<float>>>();
		dispatchTable[ControlValueType::Type::ControlLinearDouble] = createToJsonDispatchFunctor<LinearValues<LinearValueDynamic<double>>>();

		/** Selector Value - IEEE1722.1-2013 Clause 7.3.5.2.2 */
		dispatchTable[ControlValueType::Type::ControlSelectorInt8] = createToJsonDispatchFunctor<SelectorValueDynamic<std::int8_t>>();
		dispatchTable[ControlValueType::Type::ControlSelectorUInt8] = createToJsonDispatchFunctor<SelectorValueDynamic<std::uint8_t>>();
		dispatchTable[ControlValueType::Type::ControlSelectorInt16] = createToJsonDispatchFunctor<SelectorValueDynamic<std::int16_t>>();
		dispatchTable[ControlValueType::Type::ControlSelectorUInt16] = createToJsonDispatchFunctor<SelectorValueDynamic<std::uint16_t>>();
		dispatchTable[ControlValueType::Type::ControlSelectorInt32] = createToJsonDispatchFunctor<SelectorValueDynamic<std::int32_t>>();
		dispatchTable[ControlValueType::Type::ControlSelectorUInt32] = createToJsonDispatchFunctor<SelectorValueDynamic<std::uint32_t>>();
		dispatchTable[ControlValueType::Type::ControlSelectorInt64] = createToJsonDispatchFunctor<SelectorValueDynamic<std::int64_t>>();
		dispatchTable[ControlValueType::Type::ControlSelectorUInt64] = createToJsonDispatchFunctor<SelectorValueDynamic<std::uint64_t>>();
		dispatchTable[ControlValueType::Type::ControlSelectorFloat] = createToJsonDispatchFunctor<SelectorValueDynamic<float>>();
		dispatchTable[ControlValueType::Type::ControlSelectorDouble] = createToJsonDispatchFunctor<SelectorValueDynamic<double>>();
		dispatchTable[ControlValueType::Type::ControlSelectorString] = createToJsonDispatchFunctor<SelectorValueDynamic<LocalizedStringReference>>();

		/** Array Values - IEEE1722.1-2013 Clause 7.3.5.2.3 */
		dispatchTable[ControlValueType::Type::ControlArrayInt8] = createToJsonDispatchFunctor<ArrayValueDynamic<std::int8_t>>();
		dispatchTable[ControlValueType::Type::ControlArrayUInt8] = createToJsonDispatchFunctor<ArrayValueDynamic<std::uint8_t>>();
		dispatchTable[ControlValueType::Type::ControlArrayInt16] = createToJsonDispatchFunctor<ArrayValueDynamic<std::int16_t>>();
		dispatchTable[ControlValueType::Type::ControlArrayUInt16] = createToJsonDispatchFunctor<ArrayValueDynamic<std::uint16_t>>();
		dispatchTable[ControlValueType::Type::ControlArrayInt32] = createToJsonDispatchFunctor<ArrayValueDynamic<std::int32_t>>();
		dispatchTable[ControlValueType::Type::ControlArrayUInt32] = createToJsonDispatchFunctor<ArrayValueDynamic<std::uint32_t>>();
		dispatchTable[ControlValueType::Type::ControlArrayInt64] = createToJsonDispatchFunctor<ArrayValueDynamic<std::int64_t>>();
		dispatchTable[ControlValueType::Type::ControlArrayUInt64] = createToJsonDispatchFunctor<ArrayValueDynamic<std::uint64_t>>();
		dispatchTable[ControlValueType::Type::ControlArrayFloat] = createToJsonDispatchFunctor<ArrayValueDynamic<float>>();
		dispatchTable[ControlValueType::Type::ControlArrayDouble] = createToJsonDispatchFunctor<ArrayValueDynamic<double>>();

		/** UTF-8 String Value - IEEE1722.1-2013 Clause 7.3.5.2.4 */
		dispatchTable[ControlValueType::Type::ControlUtf8] = createToJsonDispatchFunctor<UTF8StringValueDynamic>();
	}
	else
	{
		/** Linear Values - IEEE1722.1-2013 Clause 7.3.5.2.1 */
		dispatchTable[ControlValueType::Type::ControlLinearInt8] = createToJsonDispatchFunctor<LinearValues<LinearValueStatic<std::int8_t>>>();
		dispatchTable[ControlValueType::Type::ControlLinearUInt8] = createToJsonDispatchFunctor<LinearValues<LinearValueStatic<std::uint8_t>>>();
		dispatchTable[ControlValueType::Type::ControlLinearInt16] = createToJsonDispatchFunctor<LinearValues<LinearValueStatic<std::int16_t>>>();
		dispatchTable[ControlValueType::Type::ControlLinearUInt16] = createToJsonDispatchFunctor<LinearValues<LinearValueStatic<std::uint16_t>>>();
		dispatchTable[ControlValueType::Type::ControlLinearInt32] = createToJsonDispatchFunctor<LinearValues<LinearValueStatic<std::int32_t>>>();
		dispatchTable[ControlValueType::Type::ControlLinearUInt32] = createToJsonDispatchFunctor<LinearValues<LinearValueStatic<std::uint32_t>>>();
		dispatchTable[ControlValueType::Type::ControlLinearInt64] = createToJsonDispatchFunctor<LinearValues<LinearValueStatic<std::int64_t>>>();
		dispatchTable[ControlValueType::Type::ControlLinearUInt64] = createToJsonDispatchFunctor<LinearValues<LinearValueStatic<std::uint64_t>>>();
		dispatchTable[ControlValueType::Type::ControlLinearFloat] = createToJsonDispatchFunctor<LinearValues<LinearValueStatic<float>>>();
		dispatchTable[ControlValueType::Type::ControlLinearDouble] = createToJsonDispatchFunctor<LinearValues<LinearValueStatic<double>>>();

		/** Selector Value - IEEE1722.1-2013 Clause 7.3.5.2.2 */
		dispatchTable[ControlValueType::Type::ControlSelectorInt8] = createToJsonDispatchFunctor<SelectorValueStatic<std::int8_t>>();
		dispatchTable[ControlValueType::Type::ControlSelectorUInt8] = createToJsonDispatchFunctor<SelectorValueStatic<std::uint8_t>>();
		dispatchTable[ControlValueType::Type::ControlSelectorInt16] = createToJsonDispatchFunctor<SelectorValueStatic<std::int16_t>>();
		dispatchTable[ControlValueType::Type::ControlSelectorUInt16] = createToJsonDispatchFunctor<SelectorValueStatic<std::uint16_t>>();
		dispatchTable[ControlValueType::Type::ControlSelectorInt32] = createToJsonDispatchFunctor<SelectorValueStatic<std::int32_t>>();
		dispatchTable[ControlValueType::Type::ControlSelectorUInt32] = createToJsonDispatchFunctor<SelectorValueStatic<std::uint32_t>>();
		dispatchTable[ControlValueType::Type::ControlSelectorInt64] = createToJsonDispatchFunctor<SelectorValueStatic<std::int64_t>>();
		dispatchTable[ControlValueType::Type::ControlSelectorUInt64] = createToJsonDispatchFunctor<SelectorValueStatic<std::uint64_t>>();
		dispatchTable[ControlValueType::Type::ControlSelectorFloat] = createToJsonDispatchFunctor<SelectorValueStatic<float>>();
		dispatchTable[ControlValueType::Type::ControlSelectorDouble] = createToJsonDispatchFunctor<SelectorValueStatic<double>>();
		dispatchTable[ControlValueType::Type::ControlSelectorString] = createToJsonDispatchFunctor<SelectorValueStatic<LocalizedStringReference>>();

		/** Array Values - IEEE1722.1-2013 Clause 7.3.5.2.3 */
		dispatchTable[ControlValueType::Type::ControlArrayInt8] = createToJsonDispatchFunctor<ArrayValueStatic<std::int8_t>>();
		dispatchTable[ControlValueType::Type::ControlArrayUInt8] = createToJsonDispatchFunctor<ArrayValueStatic<std::uint8_t>>();
		dispatchTable[ControlValueType::Type::ControlArrayInt16] = createToJsonDispatchFunctor<ArrayValueStatic<std::int16_t>>();
		dispatchTable[ControlValueType::Type::ControlArrayUInt16] = createToJsonDispatchFunctor<ArrayValueStatic<std::uint16_t>>();
		dispatchTable[ControlValueType::Type::ControlArrayInt32] = createToJsonDispatchFunctor<ArrayValueStatic<std::int32_t>>();
		dispatchTable[ControlValueType::Type::ControlArrayUInt32] = createToJsonDispatchFunctor<ArrayValueStatic<std::uint32_t>>();
		dispatchTable[ControlValueType::Type::ControlArrayInt64] = createToJsonDispatchFunctor<ArrayValueStatic<std::int64_t>>();
		dispatchTable[ControlValueType::Type::ControlArrayUInt64] = createToJsonDispatchFunctor<ArrayValueStatic<std::uint64_t>>();
		dispatchTable[ControlValueType::Type::ControlArrayFloat] = createToJsonDispatchFunctor<ArrayValueStatic<float>>();
		dispatchTable[ControlValueType::Type::ControlArrayDouble] = createToJsonDispatchFunctor<ArrayValueStatic<double>>();

		/** UTF-8 String Value - IEEE1722.1-2013 Clause 7.3.5.2.4 */
		dispatchTable[ControlValueType::Type::ControlUtf8] = createToJsonDispatchFunctor<UTF8StringValueStatic>();
	}
}
template<bool IsDynamic>
inline void createFromJsonDispatchTable(std::unordered_map<ControlValueType::Type, std::function<ControlValues(json const&)>>& dispatchTable)
{
	if constexpr (IsDynamic)
	{
		/** Linear Values - IEEE1722.1-2013 Clause 7.3.5.2.1 */
		dispatchTable[ControlValueType::Type::ControlLinearInt8] = createFromJsonDispatchFunctor<LinearValues<LinearValueDynamic<std::int8_t>>>();
		dispatchTable[ControlValueType::Type::ControlLinearUInt8] = createFromJsonDispatchFunctor<LinearValues<LinearValueDynamic<std::uint8_t>>>();
		dispatchTable[ControlValueType::Type::ControlLinearInt16] = createFromJsonDispatchFunctor<LinearValues<LinearValueDynamic<std::int16_t>>>();
		dispatchTable[ControlValueType::Type::ControlLinearUInt16] = createFromJsonDispatchFunctor<LinearValues<LinearValueDynamic<std::uint16_t>>>();
		dispatchTable[ControlValueType::Type::ControlLinearInt32] = createFromJsonDispatchFunctor<LinearValues<LinearValueDynamic<std::int32_t>>>();
		dispatchTable[ControlValueType::Type::ControlLinearUInt32] = createFromJsonDispatchFunctor<LinearValues<LinearValueDynamic<std::uint32_t>>>();
		dispatchTable[ControlValueType::Type::ControlLinearInt64] = createFromJsonDispatchFunctor<LinearValues<LinearValueDynamic<std::int64_t>>>();
		dispatchTable[ControlValueType::Type::ControlLinearUInt64] = createFromJsonDispatchFunctor<LinearValues<LinearValueDynamic<std::uint64_t>>>();
		dispatchTable[ControlValueType::Type::ControlLinearFloat] = createFromJsonDispatchFunctor<LinearValues<LinearValueDynamic<float>>>();
		dispatchTable[ControlValueType::Type::ControlLinearDouble] = createFromJsonDispatchFunctor<LinearValues<LinearValueDynamic<double>>>();

		/** Selector Value - IEEE1722.1-2013 Clause 7.3.5.2.2 */
		dispatchTable[ControlValueType::Type::ControlSelectorInt8] = createFromJsonDispatchFunctor<SelectorValueDynamic<std::int8_t>>();
		dispatchTable[ControlValueType::Type::ControlSelectorUInt8] = createFromJsonDispatchFunctor<SelectorValueDynamic<std::uint8_t>>();
		dispatchTable[ControlValueType::Type::ControlSelectorInt16] = createFromJsonDispatchFunctor<SelectorValueDynamic<std::int16_t>>();
		dispatchTable[ControlValueType::Type::ControlSelectorUInt16] = createFromJsonDispatchFunctor<SelectorValueDynamic<std::uint16_t>>();
		dispatchTable[ControlValueType::Type::ControlSelectorInt32] = createFromJsonDispatchFunctor<SelectorValueDynamic<std::int32_t>>();
		dispatchTable[ControlValueType::Type::ControlSelectorUInt32] = createFromJsonDispatchFunctor<SelectorValueDynamic<std::uint32_t>>();
		dispatchTable[ControlValueType::Type::ControlSelectorInt64] = createFromJsonDispatchFunctor<SelectorValueDynamic<std::int64_t>>();
		dispatchTable[ControlValueType::Type::ControlSelectorUInt64] = createFromJsonDispatchFunctor<SelectorValueDynamic<std::uint64_t>>();
		dispatchTable[ControlValueType::Type::ControlSelectorFloat] = createFromJsonDispatchFunctor<SelectorValueDynamic<float>>();
		dispatchTable[ControlValueType::Type::ControlSelectorDouble] = createFromJsonDispatchFunctor<SelectorValueDynamic<double>>();
		dispatchTable[ControlValueType::Type::ControlSelectorString] = createFromJsonDispatchFunctor<SelectorValueDynamic<LocalizedStringReference>>();

		/** Array Values - IEEE1722.1-2013 Clause 7.3.5.2.3 */
		dispatchTable[ControlValueType::Type::ControlArrayInt8] = createFromJsonDispatchFunctor<ArrayValueDynamic<std::int8_t>>();
		dispatchTable[ControlValueType::Type::ControlArrayUInt8] = createFromJsonDispatchFunctor<ArrayValueDynamic<std::uint8_t>>();
		dispatchTable[ControlValueType::Type::ControlArrayInt16] = createFromJsonDispatchFunctor<ArrayValueDynamic<std::int16_t>>();
		dispatchTable[ControlValueType::Type::ControlArrayUInt16] = createFromJsonDispatchFunctor<ArrayValueDynamic<std::uint16_t>>();
		dispatchTable[ControlValueType::Type::ControlArrayInt32] = createFromJsonDispatchFunctor<ArrayValueDynamic<std::int32_t>>();
		dispatchTable[ControlValueType::Type::ControlArrayUInt32] = createFromJsonDispatchFunctor<ArrayValueDynamic<std::uint32_t>>();
		dispatchTable[ControlValueType::Type::ControlArrayInt64] = createFromJsonDispatchFunctor<ArrayValueDynamic<std::int64_t>>();
		dispatchTable[ControlValueType::Type::ControlArrayUInt64] = createFromJsonDispatchFunctor<ArrayValueDynamic<std::uint64_t>>();
		dispatchTable[ControlValueType::Type::ControlArrayFloat] = createFromJsonDispatchFunctor<ArrayValueDynamic<float>>();
		dispatchTable[ControlValueType::Type::ControlArrayDouble] = createFromJsonDispatchFunctor<ArrayValueDynamic<double>>();

		/** UTF-8 String Value - IEEE1722.1-2013 Clause 7.3.5.2.4 */
		dispatchTable[ControlValueType::Type::ControlUtf8] = createFromJsonDispatchFunctor<UTF8StringValueDynamic>();
	}
	else
	{
		/** Linear Values - IEEE1722.1-2013 Clause 7.3.5.2.1 */
		dispatchTable[ControlValueType::Type::ControlLinearInt8] = createFromJsonDispatchFunctor<LinearValues<LinearValueStatic<std::int8_t>>>();
		dispatchTable[ControlValueType::Type::ControlLinearUInt8] = createFromJsonDispatchFunctor<LinearValues<LinearValueStatic<std::uint8_t>>>();
		dispatchTable[ControlValueType::Type::ControlLinearInt16] = createFromJsonDispatchFunctor<LinearValues<LinearValueStatic<std::int16_t>>>();
		dispatchTable[ControlValueType::Type::ControlLinearUInt16] = createFromJsonDispatchFunctor<LinearValues<LinearValueStatic<std::uint16_t>>>();
		dispatchTable[ControlValueType::Type::ControlLinearInt32] = createFromJsonDispatchFunctor<LinearValues<LinearValueStatic<std::int32_t>>>();
		dispatchTable[ControlValueType::Type::ControlLinearUInt32] = createFromJsonDispatchFunctor<LinearValues<LinearValueStatic<std::uint32_t>>>();
		dispatchTable[ControlValueType::Type::ControlLinearInt64] = createFromJsonDispatchFunctor<LinearValues<LinearValueStatic<std::int64_t>>>();
		dispatchTable[ControlValueType::Type::ControlLinearUInt64] = createFromJsonDispatchFunctor<LinearValues<LinearValueStatic<std::uint64_t>>>();
		dispatchTable[ControlValueType::Type::ControlLinearFloat] = createFromJsonDispatchFunctor<LinearValues<LinearValueStatic<float>>>();
		dispatchTable[ControlValueType::Type::ControlLinearDouble] = createFromJsonDispatchFunctor<LinearValues<LinearValueStatic<double>>>();

		/** Selector Value - IEEE1722.1-2013 Clause 7.3.5.2.2 */
		dispatchTable[ControlValueType::Type::ControlSelectorInt8] = createFromJsonDispatchFunctor<SelectorValueStatic<std::int8_t>>();
		dispatchTable[ControlValueType::Type::ControlSelectorUInt8] = createFromJsonDispatchFunctor<SelectorValueStatic<std::uint8_t>>();
		dispatchTable[ControlValueType::Type::ControlSelectorInt16] = createFromJsonDispatchFunctor<SelectorValueStatic<std::int16_t>>();
		dispatchTable[ControlValueType::Type::ControlSelectorUInt16] = createFromJsonDispatchFunctor<SelectorValueStatic<std::uint16_t>>();
		dispatchTable[ControlValueType::Type::ControlSelectorInt32] = createFromJsonDispatchFunctor<SelectorValueStatic<std::int32_t>>();
		dispatchTable[ControlValueType::Type::ControlSelectorUInt32] = createFromJsonDispatchFunctor<SelectorValueStatic<std::uint32_t>>();
		dispatchTable[ControlValueType::Type::ControlSelectorInt64] = createFromJsonDispatchFunctor<SelectorValueStatic<std::int64_t>>();
		dispatchTable[ControlValueType::Type::ControlSelectorUInt64] = createFromJsonDispatchFunctor<SelectorValueStatic<std::uint64_t>>();
		dispatchTable[ControlValueType::Type::ControlSelectorFloat] = createFromJsonDispatchFunctor<SelectorValueStatic<float>>();
		dispatchTable[ControlValueType::Type::ControlSelectorDouble] = createFromJsonDispatchFunctor<SelectorValueStatic<double>>();
		dispatchTable[ControlValueType::Type::ControlSelectorString] = createFromJsonDispatchFunctor<SelectorValueStatic<LocalizedStringReference>>();

		/** Array Values - IEEE1722.1-2013 Clause 7.3.5.2.3 */
		dispatchTable[ControlValueType::Type::ControlArrayInt8] = createFromJsonDispatchFunctor<ArrayValueStatic<std::int8_t>>();
		dispatchTable[ControlValueType::Type::ControlArrayUInt8] = createFromJsonDispatchFunctor<ArrayValueStatic<std::uint8_t>>();
		dispatchTable[ControlValueType::Type::ControlArrayInt16] = createFromJsonDispatchFunctor<ArrayValueStatic<std::int16_t>>();
		dispatchTable[ControlValueType::Type::ControlArrayUInt16] = createFromJsonDispatchFunctor<ArrayValueStatic<std::uint16_t>>();
		dispatchTable[ControlValueType::Type::ControlArrayInt32] = createFromJsonDispatchFunctor<ArrayValueStatic<std::int32_t>>();
		dispatchTable[ControlValueType::Type::ControlArrayUInt32] = createFromJsonDispatchFunctor<ArrayValueStatic<std::uint32_t>>();
		dispatchTable[ControlValueType::Type::ControlArrayInt64] = createFromJsonDispatchFunctor<ArrayValueStatic<std::int64_t>>();
		dispatchTable[ControlValueType::Type::ControlArrayUInt64] = createFromJsonDispatchFunctor<ArrayValueStatic<std::uint64_t>>();
		dispatchTable[ControlValueType::Type::ControlArrayFloat] = createFromJsonDispatchFunctor<ArrayValueStatic<float>>();
		dispatchTable[ControlValueType::Type::ControlArrayDouble] = createFromJsonDispatchFunctor<ArrayValueStatic<double>>();

		/** UTF-8 String Value - IEEE1722.1-2013 Clause 7.3.5.2.4 */
		dispatchTable[ControlValueType::Type::ControlUtf8] = createFromJsonDispatchFunctor<UTF8StringValueStatic>();
	}
}

/* ControlNodeStaticModel conversion */
inline void to_json(json& j, ControlNodeStaticModel const& s)
{
	static auto s_toJsonDispatch = std::unordered_map<ControlValueType::Type, std::function<json(ControlValues const&)>>{};

	if (s_toJsonDispatch.empty())
	{
		// Create the dispatch table
		createToJsonDispatchTable<false>(s_toJsonDispatch);
	}

	// Pack non-type dependant values
	j[keyName::ControlNode_Static_LocalizedDescription] = s.localizedDescription;
	j[keyName::ControlNode_Static_BlockLatency] = s.blockLatency;
	j[keyName::ControlNode_Static_ControlLatency] = s.controlLatency;
	j[keyName::ControlNode_Static_ControlDomain] = s.controlDomain;
	j[keyName::ControlNode_Static_ControlType] = s.controlType;
	j[keyName::ControlNode_Static_ResetTime] = s.resetTime;
	j[keyName::ControlNode_Static_SignalType] = s.signalType;
	j[keyName::ControlNode_Static_SignalIndex] = s.signalIndex;
	j[keyName::ControlNode_Static_SignalOutput] = s.signalOutput;
	j[keyName::ControlNode_Static_ControlValueType] = s.controlValueType;
	j[keyName::ControlNode_Static_NumberOfValues] = s.numberOfValues;

	// Pack type dependant values
	if (s.values)
	{
		auto const valueType = s.values.getType();
		if (auto const& it = s_toJsonDispatch.find(valueType); it != s_toJsonDispatch.end())
		{
			j[keyName::ControlNode_Static_Values] = it->second(s.values);
		}
		else
		{
			throw avdecc::jsonSerializer::SerializationException{ avdecc::jsonSerializer::SerializationError::InternalError, "Unsupported ControlValues Type" };
		}
	}
	else
	{
		j[keyName::ControlNode_Static_Values] = nullptr;
	}
}
inline void from_json(json const& j, ControlNodeStaticModel& s)
{
	static auto s_fromJsonDispatch = std::unordered_map<ControlValueType::Type, std::function<ControlValues(json const&)>>{};

	if (s_fromJsonDispatch.empty())
	{
		// Create the dispatch table
		createFromJsonDispatchTable<false>(s_fromJsonDispatch);
	}

	// Unpack non-type dependant values
	get_optional_value(j, keyName::ControlNode_Static_LocalizedDescription, s.localizedDescription);
	j.at(keyName::ControlNode_Static_BlockLatency).get_to(s.blockLatency);
	j.at(keyName::ControlNode_Static_ControlLatency).get_to(s.controlLatency);
	j.at(keyName::ControlNode_Static_ControlDomain).get_to(s.controlDomain);
	j.at(keyName::ControlNode_Static_ControlType).get_to(s.controlType);
	j.at(keyName::ControlNode_Static_ResetTime).get_to(s.resetTime);
	j.at(keyName::ControlNode_Static_SignalType).get_to(s.signalType);
	j.at(keyName::ControlNode_Static_SignalIndex).get_to(s.signalIndex);
	j.at(keyName::ControlNode_Static_SignalOutput).get_to(s.signalOutput);
	j.at(keyName::ControlNode_Static_ControlValueType).get_to(s.controlValueType);
	// Optionnaly get this field for backward compatibility (not present in older versions, but we absolutely need it now so we'll build it in parent call if it's 0)
	get_optional_value(j, keyName::ControlNode_Static_NumberOfValues, s.numberOfValues);

	// Unpack type dependant values
	auto const& jvalues = j.at(keyName::ControlNode_Static_Values);
	if (!jvalues.is_null())
	{
		auto const type = jvalues.at(keyName::ControlValues_Type).get<ControlValueType::Type>();
		if (auto const& it = s_fromJsonDispatch.find(type); it != s_fromJsonDispatch.end())
		{
			s.values = it->second(jvalues.at(keyName::ControlValues_Values));
		}
		else
		{
			throw avdecc::jsonSerializer::DeserializationException{ avdecc::jsonSerializer::DeserializationError::InternalError, "Unsupported ControlValues Type" };
		}
	}
}

/* ControlNodeDynamicModel conversion */
inline void to_json(json& j, ControlNodeDynamicModel const& d)
{
	static auto s_toJsonDispatch = std::unordered_map<ControlValueType::Type, std::function<json(ControlValues const&)>>{};

	if (s_toJsonDispatch.empty())
	{
		// Create the dispatch table
		createToJsonDispatchTable<true>(s_toJsonDispatch);
	}

	// Pack non-type dependant values
	j[keyName::ControlNode_Dynamic_ObjectName] = d.objectName;

	// Pack type dependant values
	if (d.values)
	{
		auto const valueType = d.values.getType();
		if (auto const& it = s_toJsonDispatch.find(valueType); it != s_toJsonDispatch.end())
		{
			j[keyName::ControlNode_Dynamic_Values] = it->second(d.values);
		}
		else
		{
			throw avdecc::jsonSerializer::SerializationException{ avdecc::jsonSerializer::SerializationError::InternalError, "Unsupported ControlValues Type" };
		}
	}
	else
	{
		j[keyName::ControlNode_Dynamic_Values] = nullptr;
	}
}
inline void from_json(json const& j, ControlNodeDynamicModel& d)
{
	static auto s_fromJsonDispatch = std::unordered_map<ControlValueType::Type, std::function<ControlValues(json const&)>>{};

	if (s_fromJsonDispatch.empty())
	{
		// Create the dispatch table
		createFromJsonDispatchTable<true>(s_fromJsonDispatch);
	}

	// Unpack non-type dependant values
	get_optional_value(j, keyName::ControlNode_Dynamic_ObjectName, d.objectName);

	// Unpack type dependant values
	auto const& jvalues = j.at(keyName::ControlNode_Dynamic_Values);
	if (!jvalues.is_null())
	{
		auto const type = jvalues.at(keyName::ControlValues_Type).get<ControlValueType::Type>();
		if (auto const& it = s_fromJsonDispatch.find(type); it != s_fromJsonDispatch.end())
		{
			d.values = it->second(jvalues.at(keyName::ControlValues_Values));
		}
		else
		{
			throw avdecc::jsonSerializer::DeserializationException{ avdecc::jsonSerializer::DeserializationError::InternalError, "Unsupported ControlValues Type" };
		}
	}
}

/* ClockDomainNodeStaticModel conversion */
inline void to_json(json& j, ClockDomainNodeStaticModel const& s)
{
	j[keyName::ClockDomainNode_Static_LocalizedDescription] = s.localizedDescription;
	j[keyName::ClockDomainNode_Static_ClockSources] = s.clockSources;
}
inline void from_json(json const& j, ClockDomainNodeStaticModel& s)
{
	get_optional_value(j, keyName::ClockDomainNode_Static_LocalizedDescription, s.localizedDescription);
	j.at(keyName::ClockDomainNode_Static_ClockSources).get_to(s.clockSources);
}

/* ClockDomainNodeDynamicModel conversion */
inline void to_json(json& j, ClockDomainNodeDynamicModel const& d)
{
	j[keyName::ClockDomainNode_Dynamic_ObjectName] = d.objectName;
	j[keyName::ClockDomainNode_Dynamic_ClockSourceIndex] = d.clockSourceIndex;
	j[keyName::ClockDomainNode_Dynamic_Counters] = d.counters;
}
inline void from_json(json const& j, ClockDomainNodeDynamicModel& d)
{
	get_optional_value(j, keyName::ClockDomainNode_Dynamic_ObjectName, d.objectName);
	j.at(keyName::ClockDomainNode_Dynamic_ClockSourceIndex).get_to(d.clockSourceIndex);
	get_optional_value(j, keyName::ClockDomainNode_Dynamic_Counters, d.counters);
}

/* TimingNodeStaticModel conversion */
inline void to_json(json& j, TimingNodeStaticModel const& s)
{
	j[keyName::TimingNode_Static_LocalizedDescription] = s.localizedDescription;
	j[keyName::TimingNode_Static_Algorithm] = s.algorithm;
	j[keyName::TimingNode_Static_PtpInstances] = s.ptpInstances;
}
inline void from_json(json const& j, TimingNodeStaticModel& s)
{
	j.at(keyName::TimingNode_Static_LocalizedDescription).get_to(s.localizedDescription);
	j.at(keyName::TimingNode_Static_Algorithm).get_to(s.algorithm);
	j.at(keyName::TimingNode_Static_PtpInstances).get_to(s.ptpInstances);
}

/* TimingNodeDynamicModel conversion */
inline void to_json(json& j, TimingNodeDynamicModel const& d)
{
	j[keyName::TimingNode_Dynamic_ObjectName] = d.objectName;
}
inline void from_json(json const& j, TimingNodeDynamicModel& d)
{
	get_optional_value(j, keyName::TimingNode_Dynamic_ObjectName, d.objectName);
}

/* PtpInstanceNodeStaticModel conversion */
inline void to_json(json& j, PtpInstanceNodeStaticModel const& s)
{
	j[keyName::PtpInstanceNode_Static_LocalizedDescription] = s.localizedDescription;
	j[keyName::PtpInstanceNode_Static_ClockIdentity] = s.clockIdentity;
	j[keyName::PtpInstanceNode_Static_Flags] = s.flags;
	j[keyName::PtpInstanceNode_Static_ControlCounts] = s.numberOfControls;
	j[keyName::PtpInstanceNode_Static_PtpPortCounts] = s.numberOfPtpPorts;
}
inline void from_json(json const& j, PtpInstanceNodeStaticModel& s)
{
	j.at(keyName::PtpInstanceNode_Static_LocalizedDescription).get_to(s.localizedDescription);
	j.at(keyName::PtpInstanceNode_Static_ClockIdentity).get_to(s.clockIdentity);
	j.at(keyName::PtpInstanceNode_Static_Flags).get_to(s.flags);
	j.at(keyName::PtpInstanceNode_Static_ControlCounts).get_to(s.numberOfControls); // Not optional, field was added before that descriptor
	j.at(keyName::PtpInstanceNode_Static_PtpPortCounts).get_to(s.numberOfPtpPorts); // Not optional, field was added before that descriptor
}

/* PtpInstanceNodeDynamicModel conversion */
inline void to_json(json& j, PtpInstanceNodeDynamicModel const& d)
{
	j[keyName::PtpInstanceNode_Dynamic_ObjectName] = d.objectName;
}
inline void from_json(json const& j, PtpInstanceNodeDynamicModel& d)
{
	get_optional_value(j, keyName::PtpInstanceNode_Dynamic_ObjectName, d.objectName);
}

/* PtpPortNodeStaticModel conversion */
inline void to_json(json& j, PtpPortNodeStaticModel const& s)
{
	j[keyName::PtpPortNode_Static_LocalizedDescription] = s.localizedDescription;
	j[keyName::PtpPortNode_Static_PortNumber] = s.portNumber;
	j[keyName::PtpPortNode_Static_PortType] = s.portType;
	j[keyName::PtpPortNode_Static_Flags] = s.flags;
	j[keyName::PtpPortNode_Static_AvbInterfaceIndex] = s.avbInterfaceIndex;
	j[keyName::PtpPortNode_Static_ProfileIdentifier] = networkInterface::NetworkInterfaceHelper::macAddressToString(s.profileIdentifier, true);
}
inline void from_json(json const& j, PtpPortNodeStaticModel& s)
{
	j.at(keyName::PtpPortNode_Static_LocalizedDescription).get_to(s.localizedDescription);
	j.at(keyName::PtpPortNode_Static_PortNumber).get_to(s.portNumber);
	j.at(keyName::PtpPortNode_Static_PortType).get_to(s.portType);
	j.at(keyName::PtpPortNode_Static_Flags).get_to(s.flags);
	j.at(keyName::PtpPortNode_Static_AvbInterfaceIndex).get_to(s.avbInterfaceIndex);
	s.profileIdentifier = networkInterface::NetworkInterfaceHelper::stringToMacAddress(j.at(keyName::PtpPortNode_Static_ProfileIdentifier).get<std::string>());
}

/* PtpPortNodeDynamicModel conversion */
inline void to_json(json& j, PtpPortNodeDynamicModel const& d)
{
	j[keyName::PtpPortNode_Dynamic_ObjectName] = d.objectName;
}
inline void from_json(json const& j, PtpPortNodeDynamicModel& d)
{
	get_optional_value(j, keyName::PtpPortNode_Dynamic_ObjectName, d.objectName);
}

/* MilanInfo conversion */
inline void to_json(json& j, MilanInfo const& info)
{
	j[keyName::MilanInfo_ProtocolVersion] = info.protocolVersion;
	j[keyName::MilanInfo_Flags] = info.featuresFlags;
	{
		j[keyName::MilanInfo_CertificationVersion] = std::to_string(info.certificationVersion >> 24 & 0xFF) + "." + std::to_string(info.certificationVersion >> 16 & 0xFF) + "." + std::to_string(info.certificationVersion >> 8 & 0xFF) + "." + std::to_string(info.certificationVersion & 0xFF);
	}
}
inline void from_json(json const& j, MilanInfo& info)
{
	j.at(keyName::MilanInfo_ProtocolVersion).get_to(info.protocolVersion);
	j.at(keyName::MilanInfo_Flags).get_to(info.featuresFlags);
	{
		auto const str = j.at(keyName::MilanInfo_CertificationVersion).get<std::string>();
		auto certificationVersion = decltype(info.certificationVersion){ 0u };
		auto const tokens = utils::tokenizeString(str, '.', true);
		if (tokens.size() != 4)
		{
			throw std::invalid_argument("Invalid Milan CertificationVersion string representation: " + str);
		}
		for (auto i = 0u; i < 4u; ++i)
		{
			auto const tokValue = utils::convertFromString<decltype(certificationVersion)>(tokens[i].c_str());
			if (tokValue > 255)
			{
				throw std::invalid_argument("Invalid Milan CertificationVersion digit value (greater than 255): " + str);
			}
			certificationVersion += (tokValue & 0xFF) << (24 - i * 8);
		}
		info.certificationVersion = certificationVersion;
	}
}

} // namespace model
} // namespace entity
} // namespace avdecc
} // namespace la
