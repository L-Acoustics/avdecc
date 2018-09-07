/*
* Copyright (C) 2016-2018, L-Acoustics and its contributors

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
* @file entityEnums.hpp
* @author Christophe Calmejane
* @brief Avdecc entity enums.
*/

#pragma once

#include "la/avdecc/utils.hpp"
#include <cstdint>
#include <string>

namespace la
{
namespace avdecc
{
namespace entity
{

/** ADP Entity Capabilities - Clause 6.2.1.10 */
enum class EntityCapabilities : std::uint32_t
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

/** ADP Talker Capabilities - Clause 6.2.1.12 */
enum class TalkerCapabilities : std::uint16_t
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

/** ADP Listener Capabilities - Clause 6.2.1.14 */
enum class ListenerCapabilities : std::uint16_t
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

/** ADP Controller Capabilities - Clause 6.2.1.15 */
enum class ControllerCapabilities : std::uint32_t
{
	None = 0u,
	Implemented = 1u << 0, /**< Implements an AVDECC Controller. */
	/* Bits 0 to 20 reserved for future use */
};

/** ConnectionFlags - Clause 8.2.1.17 */
enum class ConnectionFlags : std::uint16_t
{
	None = 0u,
	ClassB = 1u << 0, /**< Indicates that the Stream is Class B instead of Class A (default 0 is class A). */
	FastConnect = 1u << 1, /**< Fast Connect Mode, the connection is being attempted in fast connect mode. */
	SavedState = 1u << 2, /**< Connection has saved state (used in Get State only). */
	StreamingWait = 1u << 3, /**< The AVDECC Talker does not start streaming until explicitly being told to by the control protocol. */
	SupportsEncrypted = 1u << 4, /**< Indicates that the Stream supports streaming with encrypted PDUs. */
	EncryptedPdu = 1u << 5, /**< Indicates that the Stream is using encrypted PDUs. */
	TalkerFailed = 1u << 6, /**< Indicates that the listener has registered an SRP Talker Failed attribute for the Stream. */
	/* Bits 0 to 8 reserved for future use */
};

/** StreamFlags - Clause 7.2.6.1 */
enum class StreamFlags : std::uint16_t
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
	/* Bits 0 to 5 reserved for future use */
};

/** JackFlags - Clause 7.2.7.1 */
enum class JackFlags : std::uint16_t
{
	None = 0u,
	ClockSyncSource = 1u << 0, /**< Indicates that the Jack can be used as a clock synchronization source. */
	Captive = 1u << 1, /**< Indicates that the Jack connection is hardwired, cannot be disconnected and may be physically within the device's structure. */
	/* Bits 0 to 13 reserved for future use */
};

/** AvbInterfaceFlags - Clause 7.2.8.1 */
enum class AvbInterfaceFlags : std::uint16_t
{
	None = 0u,
	GptpGrandmasterSupported = 1u << 0, /**< Indicates that the interface supports the IEEE Std 802.1AS-2011 grandmaster functionality. */
	GptpSupported = 1u << 1, /**< Indicates that the interface supports the IEEE Std 802.1AS-2011 functionality. */
	SrpSupported = 1u << 2, /**< Indicates that the interface supports Clause 35 of IEEE Std 802.1Q-2011, "Stream Reservation Protocol (SRP)" functionality. */
	/* Bits 0 to 12 reserved for future use */
};

/** ClockSourceFlags - Clause 7.2.9.1 */
enum class ClockSourceFlags : std::uint16_t
{
	None = 0u,
	StreamID = 1u << 0, /**< The INPUT_STREAM Clock Source is identified by the stream_id. */
	LocalID = 1u << 1, /**< The INPUT_STREAM Clock Source is identified by its local ID. */
	/* Bits 0 to 13 reserved for future use */
};

/** PortFlags - Clause 7.2.13.1 */
enum class PortFlags : std::uint16_t
{
	None = 0u,
	ClockSyncSource = 1u << 0, /**< Indicates that the Port can be used as a clock synchronization source. */
	AsyncSampleRateConv = 1u << 1, /**< Indicates that the Port has an asynchronous sample rate convertor to convert sample rates between another Clock Domain and the Unit's. */
	SyncSampleRateConv = 1u << 2, /**< Indicates that the Port has a synchronous sample rate convertor to convert between sample rates in the same Clock Domain. */
	/* Bits 0 to 12 reserved for future use */
};

/** StreamInfo Flags - Clause 7.4.15.1 */
enum class StreamInfoFlags : std::uint32_t
{
	None = 0u,
	ClassB = 1u << 0, /**< Indicates that the Stream is Class B instead of Class A (default 0 is class A). */
	FastConnect = 1u << 1, /**< Fast Connect Mode, the Stream was connected in Fast Connect Mode or is presently trying to connect in Fast Connect Mode. */
	SavedState = 1u << 2, /**< Connection has saved ACMP state. */
	StreamingWait = 1u << 3, /**< The Stream is presently in STREAMING_WAIT, either it was connected with STREAMING_WAIT flag set or it was stopped with STOP_STREAMING command. */
	SupportsEncrypted = 1u << 4, /**< Indicates that the Stream supports streaming with encrypted PDUs. */
	EncryptedPdu = 1u << 5, /**< Indicates that the Stream is using encrypted PDUs. */
	TalkerFailed = 1u << 6, /**< Indicates that the Listener has registered an SRP Talker Failed attribute for the Stream. */
	/* Bits 5 to 24 reserved for future use */
	StreamVlanIDValid = 1u << 25, /**< Indicates that the stream_vlan_id field is valid. */
	Connected = 1u << 26, /**< The Stream has been connected with ACMP. This may only be set in a response. */
	MsrpFailureValid = 1u << 27, /**< The values in the msrp_failure_code and msrp_failure_bridge_id fields are valid. */
	StreamDestMacValid = 1u << 28, /**< The value in the stream_dest_mac field is valid. */
	MsrpAccLatValid = 1u << 29, /**< The value in the msrp_accumulated_latency field is valid. */
	StreamIDValid = 1u << 30, /**< The value in the stream_id field is valid. */
	StreamFormatValid = 1u << 31, /**< The value in stream_format field is valid and is to be used to change the Stream format if it is a SET_STREAM_INFO command. */
};

/** AvbInfo Flags - Clause 7.4.40.2 */
enum class AvbInfoFlags : std::uint8_t
{
	None = 0u,
	AsCapable = 1u << 0, /**< The IEEE Std 802.1AS-2011 variable asCapable is set on this interface. */
	GptpEnabled = 1u << 1, /**< Indicates that the interface has the IEEE Std 802.1AS-2011 functionality enabled. */
	SrpEnabled = 1u << 2, /**< Indicates that the interface has the IEEE Std 802.1Q-2011 Clause 35, "Stream Reservation Protocol (SRP)" functionality enabled. */
	/* Bits 0 to 4 reserved for future use */
};

} // namespace entity

// Define bitfield enum traits for EntityCapabilities
template<>
struct enum_traits<entity::EntityCapabilities>
{
	static constexpr bool is_bitfield = true;
};

// Define bitfield enum traits for TalkerCapabilities
template<>
struct enum_traits<entity::TalkerCapabilities>
{
	static constexpr bool is_bitfield = true;
};

// Define bitfield enum traits for ListenerCapabilities
template<>
struct enum_traits<entity::ListenerCapabilities>
{
	static constexpr bool is_bitfield = true;
};

// Define bitfield enum traits for ControllerCapabilities
template<>
struct enum_traits<entity::ControllerCapabilities>
{
	static constexpr bool is_bitfield = true;
};

// Define bitfield enum traits for ConnectionFlags
template<>
struct enum_traits<entity::ConnectionFlags>
{
	static constexpr bool is_bitfield = true;
};

// Define bitfield enum traits for StreamFlags
template<>
struct enum_traits<entity::StreamFlags>
{
	static constexpr bool is_bitfield = true;
};

// Define bitfield enum traits for JackFlags
template<>
struct enum_traits<entity::JackFlags>
{
	static constexpr bool is_bitfield = true;
};

// Define bitfield enum traits for AvbInterfaceFlags
template<>
struct enum_traits<entity::AvbInterfaceFlags>
{
	static constexpr bool is_bitfield = true;
};

// Define bitfield enum traits for ClockSourceFlags
template<>
struct enum_traits<entity::ClockSourceFlags>
{
	static constexpr bool is_bitfield = true;
};

// Define bitfield enum traits for PortFlags
template<>
struct enum_traits<entity::PortFlags>
{
	static constexpr bool is_bitfield = true;
};

// Define bitfield enum traits for StreamInfoFlags
template<>
struct enum_traits<entity::StreamInfoFlags>
{
	static constexpr bool is_bitfield = true;
};

// Define bitfield enum traits for AvbInfoFlags
template<>
struct enum_traits<entity::AvbInfoFlags>
{
	static constexpr bool is_bitfield = true;
};

} // namespace avdecc
} // namespace la
