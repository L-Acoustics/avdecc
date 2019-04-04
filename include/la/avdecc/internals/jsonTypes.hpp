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
#include "la/avdecc/avdecc.hpp"

#include <nlohmann/json.hpp>

#include <optional>

using json = nlohmann::json;

namespace nlohmann
{
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

template<typename KeyT, typename ValueType>
inline void get_optional_value(json const& j, KeyT&& key, ValueType& v)
{
	auto const it = j.find(std::forward<KeyT>(key));
	if (it != j.end())
	{
		it->get_to(v);
	}
}

template<>
struct adl_serializer<la::avdecc::entity::model::EntityCounters>
{
	static void to_json(json& j, la::avdecc::entity::model::EntityCounters const& counters)
	{
		auto object = json::object();

		for (auto const [name, value] : counters)
		{
			json const n = name;
			object[n.get<std::string>()] = value;
		}

		j = std::move(object);
	}
};
template<>
struct adl_serializer<la::avdecc::entity::model::AvbInterfaceCounters>
{
	static void to_json(json& j, la::avdecc::entity::model::AvbInterfaceCounters const& counters)
	{
		auto object = json::object();

		for (auto const [name, value] : counters)
		{
			json const n = name;
			object[n.get<std::string>()] = value;
		}

		j = std::move(object);
	}
};
template<>
struct adl_serializer<la::avdecc::entity::model::ClockDomainCounters>
{
	static void to_json(json& j, la::avdecc::entity::model::ClockDomainCounters const& counters)
	{
		auto object = json::object();

		for (auto const [name, value] : counters)
		{
			json const n = name;
			object[n.get<std::string>()] = value;
		}

		j = std::move(object);
	}
};
template<>
struct adl_serializer<la::avdecc::entity::model::StreamInputCounters>
{
	static void to_json(json& j, la::avdecc::entity::model::StreamInputCounters const& counters)
	{
		auto object = json::object();

		for (auto const [name, value] : counters)
		{
			json const n = name;
			object[n.get<std::string>()] = value;
		}

		j = std::move(object);
	}
};
template<>
struct adl_serializer<la::avdecc::entity::model::StreamOutputCounters>
{
	static void to_json(json& j, la::avdecc::entity::model::StreamOutputCounters const& counters)
	{
		auto object = json::object();

		for (auto const [name, value] : counters)
		{
			json const n = name;
			object[n.get<std::string>()] = value;
		}

		j = std::move(object);
	}
};
} // namespace nlohmann

namespace la
{
namespace avdecc
{
namespace utils
{
/* EnumBitfield conversion */
template<typename Enum, typename EnumValue = typename Enum::value_type, EnumValue DefaultValue = EnumValue::None, typename = std::enable_if_t<std::is_same_v<Enum, EnumBitfield<EnumValue>>>>
void to_json(json& j, Enum const& flags)
{
	for (auto const flag : flags)
	{
		json const conv = flag; // Must use operator= instead of constructor to force usage of the to_json overload
		AVDECC_ASSERT(conv.is_string(), "Converted Enum should be a string (forgot to define the enum using NLOHMANN_JSON_SERIALIZE_ENUM?");
		if (AVDECC_ASSERT_WITH_RET(conv.get<EnumValue>() != DefaultValue, "Unknown Enum value"))
		{
			j.push_back(flag);
		}
		else
		{
			j.push_back(utils::toHexString(utils::to_integral(flag), true, true));
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
			throw std::invalid_argument("Invalid enum value: " + o.get<std::string>());
		}
		flags.set(v);
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
	j.at(keyName::Entity_CommonInformation_TalkerStreamSources).get_to(commonInfo.talkerStreamSources);
	j.at(keyName::Entity_CommonInformation_TalkerCapabilities).get_to(commonInfo.talkerCapabilities);
	j.at(keyName::Entity_CommonInformation_ListenerStreamSinks).get_to(commonInfo.listenerStreamSinks);
	j.at(keyName::Entity_CommonInformation_ListenerCapabilities).get_to(commonInfo.listenerCapabilities);
	j.at(keyName::Entity_CommonInformation_ControllerCapabilities).get_to(commonInfo.controllerCapabilities);
	get_optional_value(j, keyName::Entity_CommonInformation_IdentifyControlIndex, commonInfo.identifyControlIndex);
	get_optional_value(j, keyName::Entity_CommonInformation_AssociationID, commonInfo.associationID);
}


/* Entity::InterfaceInformation conversion */
inline void to_json(json& j, Entity::InterfaceInformation const& intfcInfo)
{
	j[keyName::Entity_InterfaceInformation_MacAddress] = networkInterface::macAddressToString(intfcInfo.macAddress, true);
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
	intfcInfo.macAddress = networkInterface::stringToMacAddress(j.at(keyName::Entity_InterfaceInformation_MacAddress).get<std::string>());
	j.at(keyName::Entity_InterfaceInformation_ValidTime).get_to(intfcInfo.validTime);
	j.at(keyName::Entity_InterfaceInformation_AvailableIndex).get_to(intfcInfo.availableIndex);
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
constexpr auto NodeName_AvbInterfaceDescriptors = "avb_interface_descriptors";
constexpr auto NodeName_ClockSourceDescriptors = "clock_source_descriptors";
constexpr auto NodeName_MemoryObjectDescriptors = "memory_object_descriptors";
constexpr auto NodeName_LocaleDescriptors = "locale_descriptors";
constexpr auto NodeName_StringsDescriptors = "strings_descriptors";
constexpr auto NodeName_StreamPortInputDescriptors = "stream_port_input_descriptors";
constexpr auto NodeName_StreamPortOutputDescriptors = "stream_port_output_descriptors";
constexpr auto NodeName_AudioClusterDescriptors = "audio_cluster_descriptors";
constexpr auto NodeName_AudioMapDescriptors = "audio_map_descriptors";
constexpr auto NodeName_ClockDomainDescriptors = "clock_domain_descriptors";

/* Globals */
constexpr auto Node_Informative_Index = "_index (informative)";
constexpr auto Node_StaticInformation = "static";
constexpr auto Node_DynamicInformation = "dynamic";

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

/* StreamInfo */
constexpr auto StreamInfo_Flags = "flags";
constexpr auto StreamInfo_StreamFormat = "stream_format";
constexpr auto StreamInfo_StreamID = "stream_id";
constexpr auto StreamInfo_MsrpAccumulatedLatency = "msrp_accumulated_latency";
constexpr auto StreamInfo_StreamDestMac = "stream_dest_mac";
constexpr auto StreamInfo_MsrpFailureCode = "msrp_failure_code";
constexpr auto StreamInfo_MsrpFailureBridgeID = "msrp_failure_bridge_id";
constexpr auto StreamInfo_StreamVlanID = "stream_vlan_id";
constexpr auto StreamInfo_FlagsEx = "flags_ex";
constexpr auto StreamInfo_ProbingStatus = "probing_status";
constexpr auto StreamInfo_AcmpStatus = "acmp_status";

/* AvbInfo */
constexpr auto AvbInfo_GptpGrandmasterID = "gptp_grandmaster_id";
constexpr auto AvbInfo_GptpDomainNumber = "gptp_domain_number";
constexpr auto AvbInfo_PropagationDelay = "propagation_delay";
constexpr auto AvbInfo_Flags = "flags";
constexpr auto AvbInfo_MsrpMappings = "msrp_mappings";

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

/* SamplingRate conversion */
inline void to_json(json& j, SamplingRate const& sr)
{
	j = utils::toHexString(sr.getValue(), true, true);
}

/* StreamFormat conversion */
inline void to_json(json& j, StreamFormat const& sf)
{
	j = utils::toHexString(sf.getValue(), true, true);
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

/* AudioMapping conversion */
inline void to_json(json& j, AudioMapping const& mapping)
{
	j[keyName::AudioMapping_StreamIndex] = mapping.streamIndex;
	j[keyName::AudioMapping_StreamChannel] = mapping.streamChannel;
	j[keyName::AudioMapping_ClusterOffset] = mapping.clusterOffset;
	j[keyName::AudioMapping_ClusterChannel] = mapping.clusterChannel;
}

/* StreamIdentification conversion */
inline void to_json(json& j, StreamIdentification const& stream)
{
	j[keyName::StreamIdentification_EntityID] = stream.entityID;
	j[keyName::StreamIdentification_StreamIndex] = stream.streamIndex;
}

/* StreamInfo conversion */
inline void to_json(json& j, StreamInfo const& info)
{
	j[keyName::StreamInfo_Flags] = info.streamInfoFlags;
	j[keyName::StreamInfo_StreamFormat] = info.streamFormat;
	j[keyName::StreamInfo_StreamID] = info.streamID;
	j[keyName::StreamInfo_MsrpAccumulatedLatency] = info.msrpAccumulatedLatency;
	j[keyName::StreamInfo_StreamDestMac] = networkInterface::macAddressToString(info.streamDestMac, true);
	j[keyName::StreamInfo_MsrpFailureCode] = info.msrpFailureCode;
	j[keyName::StreamInfo_MsrpFailureBridgeID] = info.msrpFailureBridgeID;
	j[keyName::StreamInfo_StreamVlanID] = info.streamVlanID;
	// Milan additions
	if (info.streamInfoFlagsEx)
	{
		j[keyName::StreamInfo_FlagsEx] = *info.streamInfoFlagsEx;
	}
	if (info.probingStatus)
	{
		j[keyName::StreamInfo_ProbingStatus] = *info.probingStatus;
	}
	if (info.acmpStatus)
	{
		j[keyName::StreamInfo_AcmpStatus] = *info.acmpStatus;
	}
}

/* AvbInfo conversion */
inline void to_json(json& j, AvbInfo const& info)
{
	j[keyName::AvbInfo_GptpGrandmasterID] = info.gptpGrandmasterID;
	j[keyName::AvbInfo_GptpDomainNumber] = info.gptpDomainNumber;
	j[keyName::AvbInfo_PropagationDelay] = info.propagationDelay;
	j[keyName::AvbInfo_Flags] = info.flags;
	j[keyName::AvbInfo_MsrpMappings] = info.mappings;
}

/* AsPath conversion */
inline void to_json(json& j, AsPath const& path)
{
	j = path.sequence;
}

/* EntityNodeStaticModel conversion */
inline void to_json(json& j, EntityNodeStaticModel const& s)
{
	j[keyName::EntityNode_Static_VendorNameString] = s.vendorNameString;
	j[keyName::EntityNode_Static_ModelNameString] = s.modelNameString;
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

/* ConfigurationNodeStaticModel conversion */
inline void to_json(json& j, ConfigurationNodeStaticModel const& s)
{
	j[keyName::ConfigurationNode_Static_LocalizedDescription] = s.localizedDescription;
}

/* ConfigurationNodeDynamicModel conversion */
inline void to_json(json& j, ConfigurationNodeDynamicModel const& d)
{
	j[keyName::ConfigurationNode_Dynamic_ObjectName] = d.objectName;
}

/* AudioUnitNodeStaticModel conversion */
inline void to_json(json& j, AudioUnitNodeStaticModel const& s)
{
	j[keyName::AudioUnitNode_Static_LocalizedDescription] = s.localizedDescription;
	j[keyName::AudioUnitNode_Static_ClockDomainIndex] = s.clockDomainIndex;
	j[keyName::AudioUnitNode_Static_SamplingRates] = s.samplingRates;
}

/* AudioUnitNodeDynamicModel conversion */
inline void to_json(json& j, AudioUnitNodeDynamicModel const& d)
{
	j[keyName::AudioUnitNode_Dynamic_ObjectName] = d.objectName;
	j[keyName::AudioUnitNode_Dynamic_CurrentSamplingRate] = d.currentSamplingRate;
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

/* StreamInputNodeDynamicModel conversion */
inline void to_json(json& j, StreamInputNodeDynamicModel const& d)
{
	j[keyName::StreamInputNode_Dynamic_ObjectName] = d.objectName;
	j[keyName::StreamInputNode_Dynamic_StreamInfo] = d.streamInfo;
	j[keyName::StreamInputNode_Dynamic_ConnectedTalker] = d.connectionState.talkerStream;
	j[keyName::StreamInputNode_Dynamic_Counters] = d.counters;
}

/* StreamOutputNodeDynamicModel conversion */
inline void to_json(json& j, StreamOutputNodeDynamicModel const& d)
{
	j[keyName::StreamOutputNode_Dynamic_ObjectName] = d.objectName;
	j[keyName::StreamOutputNode_Dynamic_StreamInfo] = d.streamInfo;
	j[keyName::StreamOutputNode_Dynamic_Counters] = d.counters;
}

/* AvbInterfaceNodeStaticModel conversion */
inline void to_json(json& j, AvbInterfaceNodeStaticModel const& s)
{
	j[keyName::AvbInterfaceNode_Static_LocalizedDescription] = s.localizedDescription;
	j[keyName::AvbInterfaceNode_Static_MacAddress] = networkInterface::macAddressToString(s.macAddress, true);
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

/* AvbInterfaceNodeDynamicModel conversion */
inline void to_json(json& j, AvbInterfaceNodeDynamicModel const& d)
{
	j[keyName::AvbInterfaceNode_Dynamic_ObjectName] = d.objectName;
	j[keyName::AvbInterfaceNode_Dynamic_AvbInfo] = d.avbInfo;
	j[keyName::AvbInterfaceNode_Dynamic_AsPath] = d.asPath;
	j[keyName::AvbInterfaceNode_Dynamic_Counters] = d.counters;
}

/* ClockSourceNodeStaticModel conversion */
inline void to_json(json& j, ClockSourceNodeStaticModel const& s)
{
	j[keyName::ClockSourceNode_Static_LocalizedDescription] = s.localizedDescription;
	j[keyName::ClockSourceNode_Static_ClockSourceType] = s.clockSourceType;
	j[keyName::ClockSourceNode_Static_ClockSourceLocationType] = s.clockSourceLocationType;
	j[keyName::ClockSourceNode_Static_ClockSourceLocationIndex] = s.clockSourceLocationIndex;
}

/* ClockSourceNodeDynamicModel conversion */
inline void to_json(json& j, ClockSourceNodeDynamicModel const& d)
{
	j[keyName::ClockSourceNode_Dynamic_ObjectName] = d.objectName;
	j[keyName::ClockSourceNode_Dynamic_ClockSourceFlags] = d.clockSourceFlags;
	j[keyName::ClockSourceNode_Dynamic_ClockSourceIdentifier] = d.clockSourceIdentifier;
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

/* MemoryObjectNodeDynamicModel conversion */
inline void to_json(json& j, MemoryObjectNodeDynamicModel const& d)
{
	j[keyName::MemoryObjectNode_Dynamic_ObjectName] = d.objectName;
	j[keyName::MemoryObjectNode_Dynamic_Length] = d.length;
}

/* LocaleNodeStaticModel conversion */
inline void to_json(json& j, LocaleNodeStaticModel const& s)
{
	j[keyName::LocaleNode_Static_LocaleID] = s.localeID;
	//j[keyName::LocaleNode_Static_BaseStringDescriptor] = s.baseStringDescriptorIndex; // INFORMATIVE
}

/* StringsNodeStaticModel conversion */
inline void to_json(json& j, StringsNodeStaticModel const& s)
{
	j[keyName::StringsNode_Static_Strings] = s.strings;
}

/* StreamPortNodeStaticModel conversion */
inline void to_json(json& j, StreamPortNodeStaticModel const& s)
{
	j[keyName::StreamPortNode_Static_ClockDomainIndex] = s.clockDomainIndex;
	j[keyName::StreamPortNode_Static_Flags] = s.portFlags;
}

/* StreamPortNodeDynamicModel conversion */
inline void to_json(json& j, StreamPortNodeDynamicModel const& d)
{
	j[keyName::StreamPortNode_Dynamic_DynamicMappings] = d.dynamicAudioMap;
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

/* AudioClusterNodeDynamicModel conversion */
inline void to_json(json& j, AudioClusterNodeDynamicModel const& d)
{
	j[keyName::AudioClusterNode_Dynamic_ObjectName] = d.objectName;
}

/* AudioMapNodeStaticModel conversion */
inline void to_json(json& j, AudioMapNodeStaticModel const& s)
{
	j[keyName::AudioMapNode_Static_Mappings] = s.mappings;
}

/* ClockDomainNodeStaticModel conversion */
inline void to_json(json& j, ClockDomainNodeStaticModel const& s)
{
	j[keyName::ClockDomainNode_Static_LocalizedDescription] = s.localizedDescription;
	j[keyName::ClockDomainNode_Static_ClockSources] = s.clockSources;
}

/* ClockDomainNodeDynamicModel conversion */
inline void to_json(json& j, ClockDomainNodeDynamicModel const& d)
{
	j[keyName::ClockDomainNode_Dynamic_ObjectName] = d.objectName;
	j[keyName::ClockDomainNode_Dynamic_ClockSourceIndex] = d.clockSourceIndex;
	j[keyName::ClockDomainNode_Dynamic_Counters] = d.counters;
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
	info.featuresFlags.fromString(j.at(keyName::MilanInfo_Flags).get<std::string>());
	{
		auto const str = j.at(keyName::MilanInfo_CertificationVersion).get<std::string>();
		auto certificationVersion = decltype(info.certificationVersion){ 0u };
		auto const tokens = utils::splitString(str, '.', true);
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
