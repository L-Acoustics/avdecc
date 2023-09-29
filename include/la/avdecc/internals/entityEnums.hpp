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
* @file entityEnums.hpp
* @author Christophe Calmejane
* @brief Avdecc entity enums.
*/

#pragma once

#include "la/avdecc/utils.hpp"

#include "entityModelTypes.hpp"

#include <cstdint>
#include <string>

namespace la
{
namespace avdecc
{
namespace entity
{
/** ADP Entity Capabilities - IEEE1722.1-2021 Clause 6.2.2.9 */
enum class EntityCapability : std::uint32_t
{
	None = 0u,
	EfuMode = 1u << 0, /**< Entity Firmware Upgrade mode is enabled on the AVDECC Entity. When this flag is set, the AVDECC Entity is in the mode to perform an AVDECC Entity firmware upgrade. */
	AddressAccessSupported = 1u << 1, /**< Supports receiving the ADDRESS_ACCESS commands as defined in 9.2.1.3. */
	GatewayEntity = 1u << 2, /**< AVDECC Entity serves as a gateway to a device on another type of media (typically a IEEE Std 1394 device) by proxying control services for it. */
	AemSupported = 1u << 3, /**< Supports receiving the AVDECC Entity Model (AEM) AECP commands as defined in 9.2.1.2. */
	LegacyAvc = 1u << 4, /**< Supports using IEEE Std 1394 AV/C protocol (e.g., for a IEEE Std 1394 devices through a gateway). */
	AssociationIDSupported = 1u << 5, /**< The AVDECC Entity supports the use of the association_id field for associating the AVDECC Entity with other AVDECC entities. */
	AssociationIDValid = 1u << 6, /**< The association_id field contains a valid value. This bit shall only be set in conjunction with ASSOCIATION_ID_SUPPORTED. */
	VendorUniqueSupported = 1u << 7, /**< Supports receiving the AEM VENDOR_UNIQUE commands as defined in 9.2.1.5. */
	ClassASupported = 1u << 8, /**< Supports sending and/or receiving Class A Streams. */
	ClassBSupported = 1u << 9, /**< Supports sending and/or receiving Class B Streams. */
	GptpSupported = 1u << 10, /**< The AVDECC Entity implements IEEE Std 802.1AS-2011. */
	AemAuthenticationSupported = 1u << 11, /**< Supports using AEM Authentication via the AUTHENTICATE command as defined in 7.4.66. This flag shall only be set if the AEM_SUPPORTED flag is set. */
	AemAuthenticationRequired = 1u << 12, /**< Requires the use of AEM Authentication via the AUTHENTICATE command as defined in 7.4.66. This flag shall only be set if the AEM_SUPPORTED flag is set. */
	AemPersistentAcquireSupported = 1u << 13, /**< Supports the use of the PERSISTENT flag in the ACQUIRE command as defined in 7.4.1. This flag shall only be set if the AEM_SUPPORTED flag is set. */
	AemIdentifyControlIndexValid = 1u << 14, /**< The identify_control_index field contains a valid index of an AEM CONTROL descriptor for the primary IDENTIFY Control in the current Configuration. This flag shall only be set if the AEM_SUPPORTED flag is set. */
	AemInterfaceIndexValid = 1u << 15, /**< The interface_index field contains a valid index of an AEM AVB_INTERFACE descriptor for interface in the current Configuration which is transmitting the ADPDU. This flag shall only be set if the AEM_SUPPORTED flag is set. */
	GeneralControllerIgnore = 1u << 16, /**< General purpose AVDECC Controllers ignore the presence of this AVDECC Entity when this flag is set. */
	EntityNotReady = 1u << 17, /**< The AVDECC Entity is not ready to be enumerated or connected by an AVDECC Controller. */
	/* Bits 0 to 13 reserved for future use */
};
using EntityCapabilities = utils::EnumBitfield<EntityCapability>;

/** ADP Talker Capabilities - IEEE1722.1-2021 Clause 6.2.2.11 */
enum class TalkerCapability : std::uint16_t
{
	None = 0u,
	Implemented = 1u << 0, /**< Implements an AVDECC Talker. */
	/* Bits 7 to 14 reserved for future use */
	OtherSource = 1u << 9, /**< The AVDECC Talker has other Stream sources not covered by the following. */
	ControlSource = 1u << 10, /**< The AVDECC Talker has Control Stream sources. */
	MediaClockSource = 1u << 11, /**< The AVDECC Talker has Media Clock Stream sources. */
	SmpteSource = 1u << 12, /**< The AVDECC Talker has Society of Motion Picture and Television Engineers (SMPTE) time code Stream sources. */
	MidiSource = 1u << 13, /**< The AVDECC Talker has Musical Instrument Digital Interface (MIDI) Stream sources. */
	AudioSource = 1u << 14, /**< The AVDECC Talker has Audio Stream sources. */
	VideoSource = 1u << 15, /**< The AVDECC Talker has Video Stream sources (which can include embedded audio). */
};
using TalkerCapabilities = utils::EnumBitfield<TalkerCapability>;

/** ADP Listener Capabilities - IEEE1722.1-2021 Clause 6.2.2.13 */
enum class ListenerCapability : std::uint16_t
{
	None = 0u,
	Implemented = 1u << 0, /**< Implements an AVDECC Listener. */
	/* Bits 7 to 14 reserved for future use */
	OtherSink = 1u << 9, /**< The AVDECC Listener has other Stream sinks not covered by the following. */
	ControlSink = 1u << 10, /**< The AVDECC Listener has Control Stream sinks. */
	MediaClockSink = 1u << 11, /**< The AVDECC Listener has Media Clock Stream sinks. */
	SmpteSink = 1u << 12, /**< The AVDECC Listener has SMPTE time code Stream sinks. */
	MidiSink = 1u << 13, /**< The AVDECC Listener has MIDI Stream sinks. */
	AudioSink = 1u << 14, /**< The AVDECC Listener has Audio Stream sinks. */
	VideoSink = 1u << 15, /**< The AVDECC Listener has Video Stream sinks (which can include embedded audio). */
};
using ListenerCapabilities = utils::EnumBitfield<ListenerCapability>;

/** ADP Controller Capabilities - IEEE1722.1-2021 Clause 6.2.2.14 */
enum class ControllerCapability : std::uint32_t
{
	None = 0u,
	Implemented = 1u << 0, /**< Implements an AVDECC Controller. */
	/* Bits 0 to 20 reserved for future use */
};
using ControllerCapabilities = utils::EnumBitfield<ControllerCapability>;

/** ConnectionFlags - IEEE1722.1-2021 Clause 8.2.1.16 */
enum class ConnectionFlag : std::uint16_t
{
	None = 0u,
	ClassB = 1u << 0, /**< Indicates that the Stream is Class B instead of Class A (default 0 is class A). */
	FastConnect = 1u << 1, /**< Fast Connect Mode, the connection is being attempted in fast connect mode. */
	SavedState = 1u << 2, /**< Connection has saved state (used in Get State only). */
	StreamingWait = 1u << 3, /**< The AVDECC Talker does not start streaming until explicitly being told to by the control protocol. */
	SupportsEncrypted = 1u << 4, /**< Indicates that the Stream supports streaming with encrypted PDUs. */
	EncryptedPdu = 1u << 5, /**< Indicates that the Stream is using encrypted PDUs. */
	TalkerFailed = 1u << 6, /**< IEEE1722.1-2013 - Indicates that the listener has registered an SRP Talker Failed attribute for the Stream. */
	SrpRegistrationFailed = 1u << 6, /**< IEEE1722.1-2021 - Indicates that the listener has registered an SRP Talker Failed attribute for the stream or that the talker has registered an SRP Listener Asking Failed attribute for the stream (used in Get State only). */
	ClEntriedValid = 1u << 7, /**< Indicates that the connected_listeners_entries field in the ACM-PDU is valid. */
	NoSrp = 1u << 8, /**< Indicates that SRP is not being used for the stream. The Talker will not register a Talker Advertise nor wait for a Listener registration before streaming. */
	Udp = 1u << 9, /**< Indicates that the stream is using UDP based transport and not Layer 2 AVTPDUs. */
	/* Bits 0 to 5 reserved for future use */
};
using ConnectionFlags = utils::EnumBitfield<ConnectionFlag>;

/** StreamFlags - IEEE1722.1-2021 Clause 7.2.6.1 */
enum class StreamFlag : std::uint16_t
{
	None = 0u,
	ClockSyncSource = 1u << 0, /**< Indicates that the Stream can be used as a clock synchronization source. */
	ClassA = 1u << 1, /**< Indicates that the Stream supports streaming at Class A. */
	ClassB = 1u << 2, /**< Indicates that the Stream supports streaming at Class B. */
	SupportsEncrypted = 1u << 3, /**< Indicates that the Stream supports streaming with encrypted PDUs. */
	PrimaryBackupSupported = 1u << 4, /**< Indicates that the backup_talker_entity_id_0 and the backup_talker_entity_id_0 fields are supported. */
	PrimaryBackupValid = 1u << 5, /**< Indicates that the backup_talker_entity_id_0 and the backup_talker_entity_id_0 fields are valid. */
	SecondaryBackupSupported = 1u << 6, /**< Indicates that the backup_talker_entity_id_1 and the backup_talker_entity_id_1 fields are supported. */
	SecondaryBackupValid = 1u << 7, /**< Indicates that the backup_talker_entity_id_1 and the backup_talker_entity_id_1 fields are valid. */
	TertiaryBackupSupported = 1u << 8, /**< Indicates that the backup_talker_entity_id_2 and the backup_talker_entity_id_2 fields are supported. */
	TertiaryBackupValid = 1u << 9, /**< Indicates that the backup_talker_entity_id_2 and the backup_talker_entity_id_2 fields are valid. */
	SupportsAvtpUdpV4 = 1u << 10, /**< Indicates that the Stream supports streaming using AVTP over UDP/IPv4 (1722-2016 Annex J). */
	SupportsAvtpUdpV6 = 1u << 11, /**< Indicates that the Stream supports streaming using AVTP over UDP/IPv6 (1722-2016 Annex J). */
	NoSupportAvtpNative = 1u << 12, /**< Indicates that the Stream does not support streaming with native (L2, Ethertype 0x22f0) AVTPDUs. */
	TimingFieldValid = 1u << 13, /**< The timing field contains a valid TIMING dscriptor index. */
	NoMediaClock = 1u << 14, /**< The stream does not use a media clock and so the clock_domain_index field does not contain a valid index. */
	SupportsNoSrp = 1u << 15, /**< The stream supports streaming without an SRP reservation. */
};
using StreamFlags = utils::EnumBitfield<StreamFlag>;

/** JackFlags - IEEE1722.1-2021 Clause 7.2.7.1 */
enum class JackFlag : std::uint16_t
{
	None = 0u,
	ClockSyncSource = 1u << 0, /**< Indicates that the Jack can be used as a clock synchronization source. */
	Captive = 1u << 1, /**< Indicates that the Jack connection is hardwired, cannot be disconnected and may be physically within the device's structure. */
	/* Bits 0 to 13 reserved for future use */
};
using JackFlags = utils::EnumBitfield<JackFlag>;

/** AvbInterfaceFlags - IEEE1722.1-2021 Clause 7.2.8.1 */
enum class AvbInterfaceFlag : std::uint16_t
{
	None = 0u,
	GptpGrandmasterSupported = 1u << 0, /**< Indicates that the interface supports the IEEE Std 802.1AS-2011 grandmaster functionality. */
	GptpSupported = 1u << 1, /**< Indicates that the interface supports the IEEE Std 802.1AS-2011 functionality. */
	SrpSupported = 1u << 2, /**< Indicates that the interface supports Clause 35 of IEEE Std 802.1Q-2011, "Stream Reservation Protocol (SRP)" functionality. */
	FqtssNotSupported = 1u << 3, /**< Indicates that the interface does not support the IEEE Std 802.1Q-2018 Clause 34, "Forwarding and Queuing Enhancements for time-sensitive streams (FQTSS)" functionality. */
	ScheduledTrafficSupported = 1u << 4, /**< Indicates that the interface supports the IEEE Std 802.1Q-2018 Clauses 8.6.8.4 "Enhancements for scheduled traffic", 8.6.9 "Scheduled traffic state machines" and Annex Q "Traffic Scheduling" functionality. */
	CanListenToSelf = 1u << 5, /**< Indicates that a Listener stream sink on the interface can listen to a Talker stream source on the same interface. */
	CanListenToOtherSelf = 1u << 6, /**< Indicates that a Listener stream sink on the interface can listen to a Talker stream source of another interface within the same ATDECC Entity. */
	/* Bits 0 to 8 reserved for future use */
};
using AvbInterfaceFlags = utils::EnumBitfield<AvbInterfaceFlag>;

/** ClockSourceFlags - IEEE1722.1-2021 Clause 7.2.9.1 */
enum class ClockSourceFlag : std::uint16_t
{
	None = 0u,
	StreamID = 1u << 0, /**< The INPUT_STREAM Clock Source is identified by the stream_id. */
	LocalID = 1u << 1, /**< The INPUT_STREAM Clock Source is identified by its local ID. */
	/* Bits 0 to 13 reserved for future use */
};
using ClockSourceFlags = utils::EnumBitfield<ClockSourceFlag>;

/** PortFlags - IEEE1722.1-2021 Clause 7.2.13.1 */
enum class PortFlag : std::uint16_t
{
	None = 0u,
	ClockSyncSource = 1u << 0, /**< Indicates that the Port can be used as a clock synchronization source. */
	AsyncSampleRateConv = 1u << 1, /**< Indicates that the Port has an asynchronous sample rate convertor to convert sample rates between another Clock Domain and the Unit's. */
	SyncSampleRateConv = 1u << 2, /**< Indicates that the Port has a synchronous sample rate convertor to convert between sample rates in the same Clock Domain. */
	/* Bits 0 to 12 reserved for future use */
};
using PortFlags = utils::EnumBitfield<PortFlag>;

/** PtpInstanceFlags - IEEE1722.1-2021 Clause 7.2.35.1 */
enum class PtpInstanceFlag : std::uint32_t
{
	None = 0u,
	CanSetInstanceEnable = 1u << 0,
	CanSetPriority1 = 1u << 1,
	CanSetPriority2 = 1u << 2,
	CanSetDomainNumber = 1u << 3,
	CanSetExternalPortConfiguration = 1u << 4,
	CanSetSlaveOnly = 1u << 5,
	CanEnablePerformance = 1u << 6,
	/* Bits 2 to 24 reserved for future use */
	PerformanceMonitoring = 1u << 30,
	GrandmasterCapable = 1u << 31,
};
using PtpInstanceFlags = utils::EnumBitfield<PtpInstanceFlag>;

/** PtpPortFlags - IEEE1722.1-2021 Clause 7.2.36.2 */
enum class PtpPortFlag : std::uint32_t
{
	None = 0u,
	CanSetEnable = 1u << 0,
	CanSetLinkDelayThreshold = 1u << 1,
	CanSetDelayMechanism = 1u << 2,
	CanSetDelayAsymmetry = 1u << 3,
	CanSetInitialMessageIntervals = 1u << 4,
	CanSetTimeouts = 1u << 5,
	CanOverrideAnnounceInterval = 1u << 6,
	CanOverrideSyncInterval = 1u << 7,
	CanOverridePDelayInterval = 1u << 8,
	CanOverrideGptpCapableInterval = 1u << 9,
	CanOverrideComputeNeighbor = 1u << 10,
	CanOverrideComputeLinkDelay = 1u << 11,
	CanOverrideOnestep = 1u << 12,
	/* Bits 4 to 18 reserved for future use */
	SupportsRemoteIntervalSignal = 1u << 28,
	SupportsOnestepTransmit = 1u << 29,
	SupportsOnestepReceive = 1u << 30,
	SupportsUnicastNegotiate = 1u << 31,
};
using PtpPortFlags = utils::EnumBitfield<PtpPortFlag>;

/** StreamInfo Flags - IEEE1722.1-2021 Clause 7.4.15.1 */
enum class StreamInfoFlag : std::uint32_t
{
	None = 0u,
	ClassB = 1u << 0, /**< Indicates that the Stream is Class B instead of Class A (default 0 is class A). */
	FastConnect = 1u << 1, /**< Fast Connect Mode, the Stream was connected in Fast Connect Mode or is presently trying to connect in Fast Connect Mode. */
	SavedState = 1u << 2, /**< Connection has saved ACMP state. */
	StreamingWait = 1u << 3, /**< The Stream is presently in STREAMING_WAIT, either it was connected with STREAMING_WAIT flag set or it was stopped with STOP_STREAMING command. */
	SupportsEncrypted = 1u << 4, /**< Indicates that the Stream supports streaming with encrypted PDUs. */
	EncryptedPdu = 1u << 5, /**< Indicates that the Stream is using encrypted PDUs. */
	TalkerFailed = 1u << 6, /**< Indicates that the Listener has registered an SRP Talker Failed attribute for the Stream. */
	/* Bit 24 reserved for future use */
	NoSrp = 1u << 8, /**< Indicates that SRP is not being used for the stream. The Talker will not register a Talker Advertise nor wait for a Listener registration before streaming. */
	/* Bits 13 to 22 reserved for future use */
	IpFlagsValid = 1u << 19, /**< The value in the ip_flags field is valid. */
	IpSrcPortValid = 1u << 20, /**< The value in the source_port field is valid. */
	IpDstPortValid = 1u << 21, /**< The value in the destination_port field is valid. */
	IpSrcAddrValid = 1u << 22, /**< The value in the source_ip_address field is valid. */
	IpDstAddrValid = 1u << 23, /**< The value in the destination_ip_address field is valid. */
	NotRegisteringSrp = 1u << 24, /**< For a STREAM_INPUT, indicates that the Listener is not registering an SRP Talker Advertise or Talker Failed attribute for the stream. For a STREAM_OUTPUT, indicates that the Talker is declaring an SRP Talker Advertise or Talker Failed attribute and not registering a matching Listener attribute for the stream. */
	StreamVlanIDValid = 1u << 25, /**< Indicates that the stream_vlan_id field is valid. */
	Connected = 1u << 26, /**< The Stream has been connected with ACMP. This may only be set in a response. */
	MsrpFailureValid = 1u << 27, /**< The values in the msrp_failure_code and msrp_failure_bridge_id fields are valid. */
	StreamDestMacValid = 1u << 28, /**< The value in the stream_dest_mac field is valid. */
	MsrpAccLatValid = 1u << 29, /**< The value in the msrp_accumulated_latency field is valid. */
	StreamIDValid = 1u << 30, /**< The value in the stream_id field is valid. */
	StreamFormatValid = 1u << 31, /**< The value in stream_format field is valid and is to be used to change the Stream format if it is a SET_STREAM_INFO command. */
};
using StreamInfoFlags = utils::EnumBitfield<StreamInfoFlag>;

/** StreamInfoEx Flags - Milan-2019 Clause 7.3.10 */
enum class StreamInfoFlagEx : std::uint32_t
{
	None = 0u,
	Registering = 1u << 0, /**< StreamInput: Registering either a matching Talker Advertise or a matching Talker Failed attribute. StreamOutput: Declaring a Talker Advertise or a Talker Failed attribute and registering a matching Listener attribute. */
	/* Bits 0 to 30 reserved for future use */
};
using StreamInfoFlagsEx = utils::EnumBitfield<StreamInfoFlagEx>;

/** AvbInfo Flags - IEEE1722.1-2021 Clause 7.4.40.2 */
enum class AvbInfoFlag : std::uint8_t
{
	None = 0u,
	AsCapable = 1u << 0, /**< The IEEE Std 802.1AS-2011 variable asCapable is set on this interface. */
	GptpEnabled = 1u << 1, /**< Indicates that the interface has the IEEE Std 802.1AS-2011 functionality enabled. */
	SrpEnabled = 1u << 2, /**< Indicates that the interface has the IEEE Std 802.1Q-2011 Clause 35, "Stream Reservation Protocol (SRP)" functionality enabled. */
	AvtpDown = 1u << 3, /**< Indicates that the interface is not capable of transmitting or receiving AVTPDUs. */
	AvtpDownValid = 1u << 4, /**< Indicates that the value of the AVTP_DOWN flag bit is valid. */
	/* Bits 0 to 2 reserved for future use */
};
using AvbInfoFlags = utils::EnumBitfield<AvbInfoFlag>;

/* ENTITY Counters - IEEE1722.1-2021 Clause 7.4.42.2.1 */
enum class EntityCounterValidFlag : model::DescriptorCounterValidFlag
{
	None = 0u,
	EntitySpecific8 = 1u << 24, /**< Entity Specific counter 8. */
	EntitySpecific7 = 1u << 25, /**< Entity Specific counter 7. */
	EntitySpecific6 = 1u << 26, /**< Entity Specific counter 6. */
	EntitySpecific5 = 1u << 27, /**< Entity Specific counter 5. */
	EntitySpecific4 = 1u << 28, /**< Entity Specific counter 4. */
	EntitySpecific3 = 1u << 29, /**< Entity Specific counter 3. */
	EntitySpecific2 = 1u << 30, /**< Entity Specific counter 2. */
	EntitySpecific1 = 1u << 31, /**< Entity Specific counter 1. */
};
using EntityCounterValidFlags = utils::EnumBitfield<EntityCounterValidFlag>;

/* AVB_INTERFACE Counters - IEEE1722.1-2021 Clause 7.4.42.2.2 */
enum class AvbInterfaceCounterValidFlag : model::DescriptorCounterValidFlag
{
	None = 0u,
	LinkUp = 1u << 0, /**< Total number of network link up events. */
	LinkDown = 1u << 1, /**< Total number of network link down events. */
	FramesTx = 1u << 2, /**< Total number of network frames sent. */
	FramesRx = 1u << 3, /**< Total number of network frames received. */
	RxCrcError = 1u << 4, /**< Total number of network frames received with an incorrect CRC. */
	GptpGmChanged = 1u << 5, /**< gPTP grandmaster change count. */
	EntitySpecific8 = 1u << 24, /**< Entity Specific counter 8. */
	EntitySpecific7 = 1u << 25, /**< Entity Specific counter 7. */
	EntitySpecific6 = 1u << 26, /**< Entity Specific counter 6. */
	EntitySpecific5 = 1u << 27, /**< Entity Specific counter 5. */
	EntitySpecific4 = 1u << 28, /**< Entity Specific counter 4. */
	EntitySpecific3 = 1u << 29, /**< Entity Specific counter 3. */
	EntitySpecific2 = 1u << 30, /**< Entity Specific counter 2. */
	EntitySpecific1 = 1u << 31, /**< Entity Specific counter 1. */
};
using AvbInterfaceCounterValidFlags = utils::EnumBitfield<AvbInterfaceCounterValidFlag>;

/* CLOCK_DOMAIN Counters - IEEE1722.1-2021 Clause 7.4.42.2.3 */
enum class ClockDomainCounterValidFlag : model::DescriptorCounterValidFlag
{
	None = 0u,
	Locked = 1u << 0, /**< Increments on a clock locking event. */
	Unlocked = 1u << 1, /**< Increments on a clock unlocking event. */
	EntitySpecific8 = 1u << 24, /**< Entity Specific counter 8. */
	EntitySpecific7 = 1u << 25, /**< Entity Specific counter 7. */
	EntitySpecific6 = 1u << 26, /**< Entity Specific counter 6. */
	EntitySpecific5 = 1u << 27, /**< Entity Specific counter 5. */
	EntitySpecific4 = 1u << 28, /**< Entity Specific counter 4. */
	EntitySpecific3 = 1u << 29, /**< Entity Specific counter 3. */
	EntitySpecific2 = 1u << 30, /**< Entity Specific counter 2. */
	EntitySpecific1 = 1u << 31, /**< Entity Specific counter 1. */
};
using ClockDomainCounterValidFlags = utils::EnumBitfield<ClockDomainCounterValidFlag>;

/* STREAM_INPUT Counters - IEEE1722.1-2021 Clause 7.4.42.2.4 / Milan-2019 Clause 6.8.10 */
enum class StreamInputCounterValidFlag : model::DescriptorCounterValidFlag
{
	None = 0u,
	MediaLocked = 1u << 0, /**< Increments on a Stream media clock locking. */
	MediaUnlocked = 1u << 1, /**< Increments on a Stream media clock unlocking. */
	StreamReset = 1u << 2, /**< IEEE1722.1-2013 - Increments whenever the Stream playback is reset. */
	StreamInterrupted = 1u << 2, /**< IEEE1722.1-2021 / Milan - Incremented each time the stream playback is interrupted for any reason other than a Controller Unbind operation. */
	SeqNumMismatch = 1u << 3, /**< Increments when a Stream data AVTPDU is received with a nonsequential sequence_num field. */
	MediaReset = 1u << 4, /**< Increments on a toggle of the mr bit in the Stream data AVTPDU. */
	TimestampUncertain = 1u << 5, /**< Increments on a toggle of the tu bit in the Stream data AVTPDU. */
	TimestampValid = 1u << 6, /**< Increments on receipt of a Stream data AVTPDU with the tv bit set. */
	TimestampNotValid = 1u << 7, /**< Increments on receipt of a Stream data AVTPDU with tv bit cleared. */
	UnsupportedFormat = 1u << 8, /**< Increments on receipt of a Stream data AVTPDU that contains an unsupported media type. */
	LateTimestamp = 1u << 9, /**< Increments on receipt of a Stream data AVTPDU with an avtp_timestamp field that is in the past. */
	EarlyTimestamp = 1u << 10, /**< Increments on receipt of a Stream data AVTPDU with an avtp_timestamp field that is too far in the future to process. */
	FramesRx = 1u << 11, /**< Increments on each Stream data AVTPDU received. */
	FramesTx = 1u << 12, /**< Increments on each Stream data AVTPDU transmitted. */
	EntitySpecific8 = 1u << 24, /**< Entity Specific counter 8. */
	EntitySpecific7 = 1u << 25, /**< Entity Specific counter 7. */
	EntitySpecific6 = 1u << 26, /**< Entity Specific counter 6. */
	EntitySpecific5 = 1u << 27, /**< Entity Specific counter 5. */
	EntitySpecific4 = 1u << 28, /**< Entity Specific counter 4. */
	EntitySpecific3 = 1u << 29, /**< Entity Specific counter 3. */
	EntitySpecific2 = 1u << 30, /**< Entity Specific counter 2. */
	EntitySpecific1 = 1u << 31, /**< Entity Specific counter 1. */
};
using StreamInputCounterValidFlags = utils::EnumBitfield<StreamInputCounterValidFlag>;

/* STREAM_OUTPUT Counters - Milan-2019 Clause 6.7.7/7.3.25 */
enum class StreamOutputCounterValidFlag : model::DescriptorCounterValidFlag
{
	None = 0u,
	StreamStart = 1u << 0, /**< Incremented each time the Talker starts streaming. */
	StreamStop = 1u << 1, /**< Incremented each time the Talker stops streaming. At any time, the PAAD-AE shall ensure that either STREAM_START=STREAM_STOP+1 (in this case, the Talker is currently streaming), or STREAM_START=STREAM_STOP (in this case, the Talker is not currently streaming). */
	MediaReset = 1u << 2, /**< Incremented at the end of every observation interval during which the "mr" bit has been toggled in any of the transmitted Stream Data AVTPDUs. The duration of the observation interval is implementation-specific and shall be less than or equal to 1 second. */
	TimestampUncertain = 1u << 3, /**< Incremented at the end of every observation interval during which the "tu" bit has been set in any of the transmitted Stream Data AVTPDUs. The duration of the observation interval is implementation-specific and shall be less than or equal to 1 second. */
	FramesTx = 1u << 4, /**< Incremented at the end of every observation interval during which at least one Stream Data AVTPDU has been transmitted on this STREAM_OUTPUT. The duration of the observation interval is implementation-specific and shall be less than or equal to 1 second. */
};
using StreamOutputCounterValidFlags = utils::EnumBitfield<StreamOutputCounterValidFlag>;

/* STREAM_OUTPUT Counters - IEEE1722.1-2021 Clause 7.4.42.2.5 */
enum class StreamOutputCounterValidFlag17221 : model::DescriptorCounterValidFlag
{
	None = 0u,
	StreamStart = 1u << 0, /**< Incremented when a stream is started. */
	StreamStop = 1u << 1, /**< Incremented when a stream is stopped. */
	StreamInterrupted = 1u << 2, /**< Incremented when Stream playback is interrupted. */
	MediaReset = 1u << 3, /**< Increments on a toggle of the mr bit in the Stream data AVTPDU. */
	TimestampUncertain = 1u << 4, /**< Increments on a toggle of the tu bit in the Stream data AVTPDU. */
	TimestampValid = 1u << 5, /**< Increments on receipt of a Stream data AVTPDU with the tv bit set. */
	TimestampNotValid = 1u << 6, /**< Increments on receipt of a Stream data AVTPDU with tv bit cleared. */
	FramesTx = 1u << 7, /**< Increments on each Stream data AVTPDU transmitted. */
};
using StreamOutputCounterValidFlags17221 = utils::EnumBitfield<StreamOutputCounterValidFlag17221>;

/** Milan Info Features Flags - Milan-2019 Clause 7.4.1 */
enum class MilanInfoFeaturesFlag : std::uint32_t
{
	None = 0u,
	Redundancy = 1u << 0, /**< The entity supports the milan redundancy scheme. */
};
using MilanInfoFeaturesFlags = utils::EnumBitfield<MilanInfoFeaturesFlag>;

} // namespace entity
} // namespace avdecc
} // namespace la
