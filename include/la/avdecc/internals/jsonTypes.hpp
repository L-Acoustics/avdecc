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

using json = nlohmann::json;

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

} // namespace utils

/* UniqueIdentifier conversion */
void to_json(json& j, UniqueIdentifier const& eid)
{
	j = utils::toHexString(eid.getValue(), true, true);
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
void to_json(json& j, Entity::CommonInformation const& commonInfo)
{
	j[keyName::Entity_CommonInformation_EntityID] = commonInfo.entityID;
	j[keyName::Entity_CommonInformation_EntityModelID] = commonInfo.entityModelID;
	j[keyName::Entity_CommonInformation_EntityCapabilities] = commonInfo.entityCapabilities;
	j[keyName::Entity_CommonInformation_TalkerStreamSources] = commonInfo.talkerStreamSources;
	j[keyName::Entity_CommonInformation_TalkerCapabilities] = commonInfo.talkerCapabilities;
	j[keyName::Entity_CommonInformation_ListenerStreamSinks] = commonInfo.listenerStreamSinks;
	j[keyName::Entity_CommonInformation_ListenerCapabilities] = commonInfo.listenerCapabilities;
	j[keyName::Entity_CommonInformation_ControllerCapabilities] = commonInfo.controllerCapabilities;
	if (commonInfo.identifyControlIndex)
	{
		j[keyName::Entity_CommonInformation_IdentifyControlIndex] = *commonInfo.identifyControlIndex;
	}
	if (commonInfo.associationID)
	{
		j[keyName::Entity_CommonInformation_AssociationID] = *commonInfo.associationID;
	}
}

/* Entity::InterfaceInformation conversion */
void to_json(json& j, Entity::InterfaceInformation const& intfcInfo)
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

namespace model
{
namespace keyName
{
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
void to_json(json& j, SamplingRate const& sr)
{
	j = utils::toHexString(sr.getValue(), true, true);
}

/* StreamFormat conversion */
void to_json(json& j, StreamFormat const& sf)
{
	j = utils::toHexString(sf.getValue(), true, true);
}

/* LocalizedStringReference conversion */
void to_json(json& j, LocalizedStringReference const& ref)
{
	if (ref)
	{
		auto const [offset, index] = ref.getOffsetIndex();
		j[keyName::LocalizedStringReference_Index] = offset;
		j[keyName::LocalizedStringReference_Offset] = index;
	}
}

/* AvdeccFixedString conversion */
void to_json(json& j, AvdeccFixedString const& str)
{
	j = str.str();
}

/* MsrpMapping conversion */
void to_json(json& j, MsrpMapping const& mapping)
{
	j[keyName::MsrpMapping_TrafficClass] = mapping.trafficClass;
	j[keyName::MsrpMapping_Priority] = mapping.priority;
	j[keyName::MsrpMapping_VlanID] = mapping.vlanID;
}

/* AudioMapping conversion */
void to_json(json& j, AudioMapping const& mapping)
{
	j[keyName::AudioMapping_StreamIndex] = mapping.streamIndex;
	j[keyName::AudioMapping_StreamChannel] = mapping.streamChannel;
	j[keyName::AudioMapping_ClusterOffset] = mapping.clusterOffset;
	j[keyName::AudioMapping_ClusterChannel] = mapping.clusterChannel;
}

/* StreamIdentification conversion */
void to_json(json& j, StreamIdentification const& stream)
{
	j[keyName::StreamIdentification_EntityID] = stream.entityID;
	j[keyName::StreamIdentification_StreamIndex] = stream.streamIndex;
}

/* StreamInfo conversion */
void to_json(json& j, StreamInfo const& info)
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
void to_json(json& j, AvbInfo const& info)
{
	j[keyName::AvbInfo_GptpGrandmasterID] = info.gptpGrandmasterID;
	j[keyName::AvbInfo_GptpDomainNumber] = info.gptpDomainNumber;
	j[keyName::AvbInfo_PropagationDelay] = info.propagationDelay;
	j[keyName::AvbInfo_Flags] = info.flags;
	j[keyName::AvbInfo_MsrpMappings] = info.mappings;
}

/* AsPath conversion */
void to_json(json& j, AsPath const& path)
{
	j = path.sequence;
}

/* MilanInfo conversion */
void to_json(json& j, MilanInfo const& info)
{
	j[keyName::MilanInfo_ProtocolVersion] = info.protocolVersion;
	j[keyName::MilanInfo_Flags] = info.featuresFlags;
	j[keyName::MilanInfo_CertificationVersion] = info.certificationVersion;
}

} // namespace model
} // namespace entity
} // namespace avdecc
} // namespace la
