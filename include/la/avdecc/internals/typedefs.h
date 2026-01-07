/*
* Copyright (C) 2016-2026, L-Acoustics and its contributors

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
* @file avdecc/internals/typedefs.h
* @author Christophe Calmejane
* @brief Typedefs for the C bindings Library.
*/

#pragma once

#define LA_AVDECC_HANDLE void*
#define LA_AVDECC_INVALID_HANDLE (LA_AVDECC_HANDLE) NULL

#define LA_AVDECC_EXECUTOR_WRAPPER_HANDLE LA_AVDECC_HANDLE
#define LA_AVDECC_PROTOCOL_INTERFACE_HANDLE LA_AVDECC_HANDLE
#define LA_AVDECC_LOCAL_ENTITY_HANDLE LA_AVDECC_HANDLE

typedef unsigned int avdecc_interface_version_t;
typedef char const* avdecc_const_string_t;
typedef char* avdecc_string_t;
typedef char avdecc_fixed_string_t[65];
typedef avdecc_fixed_string_t* avdecc_fixed_string_p;
typedef avdecc_fixed_string_t const* avdecc_fixed_string_cp;
typedef unsigned char avdecc_mac_address_t[6];
typedef avdecc_mac_address_t* avdecc_mac_address_p;
typedef avdecc_mac_address_t const* avdecc_mac_address_cp;
typedef unsigned long long avdecc_unique_identifier_t;
typedef unsigned long long avdecc_bridge_identifier_t;
typedef unsigned char avdecc_bool_t;
typedef unsigned char avdecc_protocol_interface_type_t;
typedef avdecc_protocol_interface_type_t avdecc_protocol_interface_types_t;
typedef unsigned char avdecc_executor_error_t;
typedef unsigned char avdecc_protocol_interface_error_t;
typedef unsigned int avdecc_entity_entity_capabilities_t;
typedef unsigned short avdecc_entity_talker_capabilities_t;
typedef unsigned short avdecc_entity_listener_capabilities_t;
typedef unsigned short avdecc_entity_controller_capabilities_t;
typedef unsigned short avdecc_entity_connection_flags_t;
typedef unsigned short avdecc_entity_stream_flags_t;
typedef unsigned short avdecc_entity_jack_flags_t;
typedef unsigned short avdecc_entity_avb_interface_flags_t;
typedef unsigned short avdecc_entity_clock_source_flags_t;
typedef unsigned short avdecc_entity_port_flags_t;
typedef unsigned int avdecc_entity_stream_info_flags_t;
typedef unsigned int avdecc_entity_stream_info_flags_ex_t;
typedef unsigned char avdecc_entity_avb_info_flags_t;
typedef unsigned int avdecc_entity_entity_counter_valid_flags_t;
typedef unsigned int avdecc_entity_avb_interface_counter_valid_flags_t;
typedef unsigned int avdecc_entity_clock_domain_counter_valid_flags_t;
typedef unsigned int avdecc_entity_stream_input_counter_valid_flags_t;
typedef unsigned int avdecc_entity_stream_output_counter_valid_flags_t;
typedef unsigned int avdecc_entity_milan_info_features_flags_t;
typedef unsigned char avdecc_protocol_adp_message_type_t;
typedef unsigned char avdecc_protocol_aecp_message_type_t;
typedef unsigned char avdecc_protocol_aecp_status_t;
typedef unsigned short avdecc_protocol_aecp_sequence_id_t;
typedef avdecc_protocol_aecp_status_t avdecc_protocol_aem_status_t;
typedef unsigned short avdecc_protocol_aem_command_type_t;
typedef unsigned int avdecc_protocol_aem_acquire_entity_flags_t;
typedef unsigned int avdecc_protocol_aem_lock_entity_flags_t;
typedef unsigned char avdecc_protocol_aa_mode_t;
typedef avdecc_protocol_aecp_status_t avdecc_protocol_aa_status_t;
typedef unsigned char avdecc_protocol_vu_protocol_id[6];
typedef avdecc_protocol_aecp_status_t avdecc_protocol_mvu_status_t;
typedef unsigned short avdecc_protocol_mvu_command_type_t;
typedef unsigned char avdecc_protocol_acmp_message_type_t;
typedef unsigned char avdecc_protocol_acmp_status_t;
typedef unsigned short avdecc_protocol_acmp_unique_id_t;
typedef unsigned short avdecc_protocol_acmp_sequence_id_t;
typedef unsigned char avdecc_local_entity_advertise_flags_t;
typedef unsigned short avdecc_local_entity_aem_command_status_t;
typedef unsigned short avdecc_local_entity_aa_command_status_t;
typedef unsigned short avdecc_local_entity_mvu_command_status_t;
typedef unsigned short avdecc_local_entity_control_status_t;
typedef unsigned char avdecc_local_entity_error_t;
typedef unsigned short avdecc_entity_model_localized_string_reference_t;
typedef unsigned short avdecc_entity_model_descriptor_index_t;
typedef unsigned short avdecc_entity_model_descriptor_type_t;
typedef unsigned short avdecc_entity_model_jack_type_t;
typedef unsigned short avdecc_entity_model_clock_source_type_t;
typedef unsigned short avdecc_entity_model_memory_object_type_t;
typedef unsigned short avdecc_entity_model_memory_object_operation_type_t;
typedef unsigned char avdecc_entity_model_audio_cluster_format_t;
typedef unsigned int avdecc_entity_model_sampling_rate_t;
typedef unsigned long long avdecc_entity_model_stream_format_t;
typedef unsigned int avdecc_entity_model_descriptor_counter_t;
typedef unsigned short avdecc_entity_model_operation_id_t;
typedef unsigned char avdecc_entity_model_probing_status_t;
typedef unsigned char avdecc_entity_model_msrp_failure_code_t;
typedef unsigned int avdecc_entity_model_descriptor_counter_t;
typedef avdecc_entity_model_descriptor_counter_t avdecc_entity_model_descriptor_counters_t[32];
typedef avdecc_entity_model_descriptor_counters_t* avdecc_entity_model_descriptor_counters_p;
typedef avdecc_entity_model_descriptor_counters_t const* avdecc_entity_model_descriptor_counters_cp;

/** Valid values for avdecc_bool_t */
enum avdecc_bool_e
{
	avdecc_bool_false = 0,
	avdecc_bool_true = 1,
};

/** Valid values for avdecc_executor_error_t */
enum avdecc_executor_error_e
{
	avdecc_executor_error_no_error = 0,
	avdecc_executor_error_already_exists = 1, /**< An Executor with specified name already exists. */
	avdecc_executor_error_not_found = 2, /**< No such Executor with specified name exists. */
	avdecc_executor_error_invalid_protocol_interface_handle = 98, /**< Passed LA_AVDECC_EXECUTOR_WRAPPER_HANDLE is invalid. */
	avdecc_executor_error_internal_error = 99, /**< Internal error, please report the issue. */
};

/** Valid values for avdecc_protocol_interface_type_t */
enum avdecc_protocol_interface_type_e
{
	avdecc_protocol_interface_type_none = 0u, /**< No protocol interface (not a valid protocol interface type, should only be used to initialize variables). */
	avdecc_protocol_interface_type_pcap = 1u << 0, /**< Packet Capture protocol interface. */
	avdecc_protocol_interface_type_macos_native = 1u << 1, /**< macOS native API protocol interface - Only usable on macOS. */
	avdecc_protocol_interface_type_proxy = 1u << 2, /**< IEEE Std 1722.1 Proxy protocol interface. */
	avdecc_protocol_interface_type_virtual = 1u << 3, /**< Virtual protocol interface. */
	avdecc_protocol_interface_type_serial = 1u << 4, /**< Serial port protocol interface. */
	avdecc_protocol_interface_type_local = 1u << 5, /**< Local domain socket protocol interface. */
};

/** Valid values for avdecc_protocol_interface_error_t */
enum avdecc_protocol_interface_error_e
{
	avdecc_protocol_interface_error_no_error = 0,
	avdecc_protocol_interface_error_transport_error = 1, /**< Transport interface error. This is critical and the interface is no longer usable. */
	avdecc_protocol_interface_error_timeout = 2, /**< A timeout occured during the operation. */
	avdecc_protocol_interface_error_unknown_remote_entity = 3, /**< Unknown remote entity. */
	avdecc_protocol_interface_error_unknown_local_entity = 4, /**< Unknown local entity. */
	avdecc_protocol_interface_error_invalid_entity_type = 5, /**< Invalid entity type for the operation. */
	avdecc_protocol_interface_error_duplicate_local_entity_id = 6, /**< The EntityID specified in a LocalEntity is already in use by another local entity. */
	avdecc_protocol_interface_error_interface_not_found = 7, /**< Specified InterfaceID not found. */
	avdecc_protocol_interface_error_invalid_parameters = 8, /**< Specified parameters are invalid (either InterfaceID and/or macAddress). */
	avdecc_protocol_interface_error_interface_not_supported = 9, /**< This protocol interface is not in the list of supported protocol interfaces. */
	avdecc_protocol_interface_error_message_not_supported = 10, /**< This type of message is not supported by this protocol interface. */
	avdecc_protocol_interface_error_executor_not_initialized = 11, /**< The executor is not initialized. */
	avdecc_protocol_interface_error_invalid_protocol_interface_handle = 98, /**< Passed LA_AVDECC_PROTOCOL_INTERFACE_HANDLE is invalid. */
	avdecc_protocol_interface_error_internal_error = 99, /**< Internal error, please report the issue. */
};

/** Valid values for avdecc_entity_entity_capabilities_t */
enum avdecc_entity_entity_capabilities_e
{
	avdecc_entity_entity_capabilities_none = 0u,
	avdecc_entity_entity_capabilities_efu_mode = 1u << 0, /**< Entity Firmware Upgrade mode is enabled on the AVDECC Entity. When this flag is set, the AVDECC Entity is in the mode to perform an AVDECC Entity firmware upgrade. */
	avdecc_entity_entity_capabilities_address_access_supported = 1u << 1, /**< Supports receiving the ADDRESS_ACCESS commands as defined in 9.2.1.3. */
	avdecc_entity_entity_capabilities_gateway_entity = 1u << 2, /**< AVDECC Entity serves as a gateway to a device on another type of media (typically a IEEE Std 1394 device) by proxying control services for it. */
	avdecc_entity_entity_capabilities_aem_supported = 1u << 3, /**< Supports receiving the AVDECC Entity Model (AEM) AECP commands as defined in 9.2.1.2. */
	avdecc_entity_entity_capabilities_legacy_avc = 1u << 4, /**< Supports using IEEE Std 1394 AV/C protocol (e.g., for a IEEE Std 1394 devices through a gateway). */
	avdecc_entity_entity_capabilities_association_id_supported = 1u << 5, /**< The AVDECC Entity supports the use of the association_id field for associating the AVDECC Entity with other AVDECC entities. */
	avdecc_entity_entity_capabilities_association_id_valid = 1u << 6, /**< The association_id field contains a valid value. This bit shall only be set in conjunction with ASSOCIATION_ID_SUPPORTED. */
	avdecc_entity_entity_capabilities_vendor_unique_supported = 1u << 7, /**< Supports receiving the AEM VENDOR_UNIQUE commands as defined in 9.2.1.5. */
	avdecc_entity_entity_capabilities_class_a_supported = 1u << 8, /**< Supports sending and/or receiving Class A Streams. */
	avdecc_entity_entity_capabilities_class_b_supported = 1u << 9, /**< Supports sending and/or receiving Class B Streams. */
	avdecc_entity_entity_capabilities_gptp_supported = 1u << 10, /**< The AVDECC Entity implements IEEE Std 802.1AS-2011. */
	avdecc_entity_entity_capabilities_aem_authentication_supported = 1u << 11, /**< Supports using AEM Authentication via the AUTHENTICATE command as defined in 7.4.66. This flag shall only be set if the AEM_SUPPORTED flag is set. */
	avdecc_entity_entity_capabilities_aem_authentication_required = 1u << 12, /**< Requires the use of AEM Authentication via the AUTHENTICATE command as defined in 7.4.66. This flag shall only be set if the AEM_SUPPORTED flag is set. */
	avdecc_entity_entity_capabilities_aem_persistent_acquire_supported = 1u << 13, /**< Supports the use of the PERSISTENT flag in the ACQUIRE command as defined in 7.4.1. This flag shall only be set if the AEM_SUPPORTED flag is set. */
	avdecc_entity_entity_capabilities_aem_identify_control_index_valid = 1u << 14, /**< The identify_control_index field contains a valid index of an AEM CONTROL descriptor for the primary IDENTIFY Control in the current Configuration. This flag shall only be set if the AEM_SUPPORTED flag is set. */
	avdecc_entity_entity_capabilities_aem_interface_index_valid = 1u << 15, /**< The interface_index field contains a valid index of an AEM AVB_INTERFACE descriptor for interface in the current Configuration which is transmitting the ADPDU. This flag shall only be set if the AEM_SUPPORTED flag is set. */
	avdecc_entity_entity_capabilities_general_controller_ignore = 1u << 16, /**< General purpose AVDECC Controllers ignore the presence of this AVDECC Entity when this flag is set. */
	avdecc_entity_entity_capabilities_entity_not_ready = 1u << 17, /**< The AVDECC Entity is not ready to be enumerated or connected by an AVDECC Controller. */
};

/** Valid values for avdecc_entity_talker_capabilities_t */
enum avdecc_entity_talker_capabilities_e
{
	avdecc_entity_talker_capabilities_none = 0u,
	avdecc_entity_talker_capabilities_implemented = 1u << 0, /**< Implements an AVDECC Talker. */
	avdecc_entity_talker_capabilities_other_source = 1u << 9, /**< The AVDECC Talker has other Stream sources not covered by the following. */
	avdecc_entity_talker_capabilities_control_source = 1u << 10, /**< The AVDECC Talker has Control Stream sources. */
	avdecc_entity_talker_capabilities_media_clock_source = 1u << 11, /**< The AVDECC Talker has Media Clock Stream sources. */
	avdecc_entity_talker_capabilities_smpte_source = 1u << 12, /**< The AVDECC Talker has Society of Motion Picture and Television Engineers (SMPTE) time code Stream sources. */
	avdecc_entity_talker_capabilities_midi_source = 1u << 13, /**< The AVDECC Talker has Musical Instrument Digital Interface (MIDI) Stream sources. */
	avdecc_entity_talker_capabilities_audio_source = 1u << 14, /**< The AVDECC Talker has Audio Stream sources. */
	avdecc_entity_talker_capabilities_video_source = 1u << 15, /**< The AVDECC Talker has Video Stream sources (which can include embedded audio). */
};

/** Valid values for avdecc_entity_listener_capabilities_t */
enum avdecc_entity_listener_capabilities_e
{
	avdecc_entity_listener_capabilities_none = 0u,
	avdecc_entity_listener_capabilities_implemented = 1u << 0, /**< Implements an AVDECC Listener. */
	avdecc_entity_listener_capabilities_other_sink = 1u << 9, /**< The AVDECC Listener has other Stream sinks not covered by the following. */
	avdecc_entity_listener_capabilities_control_sink = 1u << 10, /**< The AVDECC Listener has Control Stream sinks. */
	avdecc_entity_listener_capabilities_media_clock_sink = 1u << 11, /**< The AVDECC Listener has Media Clock Stream sinks. */
	avdecc_entity_listener_capabilities_smpte_sink = 1u << 12, /**< The AVDECC Listener has SMPTE time code Stream sinks. */
	avdecc_entity_listener_capabilities_midi_sink = 1u << 13, /**< The AVDECC Listener has MIDI Stream sinks. */
	avdecc_entity_listener_capabilities_audio_sink = 1u << 14, /**< The AVDECC Listener has Audio Stream sinks. */
	avdecc_entity_listener_capabilities_video_sink = 1u << 15, /**< The AVDECC Listener has Video Stream sinks (which can include embedded audio). */
};

/** Valid values for avdecc_entity_controller_capabilities_t */
enum avdecc_entity_controller_capabilities_e
{
	avdecc_entity_controller_capabilities_none = 0u,
	avdecc_entity_controller_capabilities_implemented = 1u << 0, /**< Implements an AVDECC Controller. */
};

/** Valid values for avdecc_entity_connection_flags_t */
enum avdecc_entity_connection_flags_e
{
	avdecc_entity_connection_flags_none = 0u,
	avdecc_entity_connection_flags_class_b = 1u << 0, /**< Indicates that the Stream is Class B instead of Class A (default 0 is class A). */
	avdecc_entity_connection_flags_fast_connect = 1u << 1, /**< Fast Connect Mode, the connection is being attempted in fast connect mode. */
	avdecc_entity_connection_flags_saved_state = 1u << 2, /**< Connection has saved state (used in Get State only). */
	avdecc_entity_connection_flags_streaming_wait = 1u << 3, /**< The AVDECC Talker does not start streaming until explicitly being told to by the control protocol. */
	avdecc_entity_connection_flags_supports_encrypted = 1u << 4, /**< Indicates that the Stream supports streaming with encrypted PDUs. */
	avdecc_entity_connection_flags_encrypted_pdu = 1u << 5, /**< Indicates that the Stream is using encrypted PDUs. */
	avdecc_entity_connection_flags_talker_failed = 1u << 6, /**< Indicates that the listener has registered an SRP Talker Failed attribute for the Stream. */
};

/** Valid values for avdecc_entity_stream_flags_t */
enum avdecc_entity_stream_flags_e
{
	avdecc_entity_stream_flags_none = 0u,
	avdecc_entity_stream_flags_clock_sync_source = 1u << 0, /**< Indicates that the Stream can be used as a clock synchronization source. */
	avdecc_entity_stream_flags_class_a = 1u << 1, /**< Indicates that the Stream supports streaming at Class A. */
	avdecc_entity_stream_flags_class_b = 1u << 2, /**< Indicates that the Stream supports streaming at Class B. */
	avdecc_entity_stream_flags_supports_encrypted = 1u << 3, /**< Indicates that the Stream supports streaming with encrypted PDUs. */
	avdecc_entity_stream_flags_primary_backup_supported = 1u << 4, /**< Indicates that the backup_talker_entity_id_0 and the backup_talker_entity_id_0 fields are supported. */
	avdecc_entity_stream_flags_primary_backup_valid = 1u << 5, /**< Indicates that the backup_talker_entity_id_0 and the backup_talker_entity_id_0 fields are valid. */
	avdecc_entity_stream_flags_secondary_backup_supported = 1u << 6, /**< Indicates that the backup_talker_entity_id_1 and the backup_talker_entity_id_1 fields are supported. */
	avdecc_entity_stream_flags_secondary_backup_valid = 1u << 7, /**< Indicates that the backup_talker_entity_id_1 and the backup_talker_entity_id_1 fields are valid. */
	avdecc_entity_stream_flags_tertiary_backup_supported = 1u << 8, /**< Indicates that the backup_talker_entity_id_2 and the backup_talker_entity_id_2 fields are supported. */
	avdecc_entity_stream_flags_tertiary_backup_valid = 1u << 9, /**< Indicates that the backup_talker_entity_id_2 and the backup_talker_entity_id_2 fields are valid. */
};

/** Valid values for avdecc_entity_jack_flags_t */
enum avdecc_entity_jack_flags_e
{
	avdecc_entity_jack_flags_none = 0u,
	avdecc_entity_jack_flags_clock_sync_source = 1u << 0, /**< Indicates that the Jack can be used as a clock synchronization source. */
	avdecc_entity_jack_flags_captive = 1u << 1, /**< Indicates that the Jack connection is hardwired, cannot be disconnected and may be physically within the device's structure. */
};

/** Valid values for avdecc_entity_avb_interface_flags_t */
enum avdecc_entity_avb_interface_flags_e
{
	avdecc_entity_avb_interface_flags_none = 0u,
	avdecc_entity_avb_interface_flags_gptp_grandmaster_supported = 1u << 0, /**< Indicates that the interface supports the IEEE Std 802.1AS-2011 grandmaster functionality. */
	avdecc_entity_avb_interface_flags_gptp_supported = 1u << 1, /**< Indicates that the interface supports the IEEE Std 802.1AS-2011 functionality. */
	avdecc_entity_avb_interface_flags_srp_supported = 1u << 2, /**< Indicates that the interface supports Clause 35 of IEEE Std 802.1Q-2011, "Stream Reservation Protocol (SRP)" functionality. */
};

/** Valid values for avdecc_entity_clock_source_flags_t */
enum avdecc_entity_clock_source_flags_e
{
	avdecc_entity_clock_source_flags_none = 0u,
	avdecc_entity_clock_source_flags_stream_id = 1u << 0, /**< The INPUT_STREAM Clock Source is identified by the stream_id. */
	avdecc_entity_clock_source_flags_local_id = 1u << 1, /**< The INPUT_STREAM Clock Source is identified by its local ID. */
};

/** Valid values for avdecc_entity_port_flags_t */
enum avdecc_entity_port_flags_e
{
	avdecc_entity_port_flags_none = 0u,
	avdecc_entity_port_flags_clock_sync_source = 1u << 0, /**< Indicates that the Port can be used as a clock synchronization source. */
	avdecc_entity_port_flags_async_sample_rate_conv = 1u << 1, /**< Indicates that the Port has an asynchronous sample rate convertor to convert sample rates between another Clock Domain and the Unit's. */
	avdecc_entity_port_flags_sync_sample_rate_conv = 1u << 2, /**< Indicates that the Port has a synchronous sample rate convertor to convert between sample rates in the same Clock Domain. */
};

/** Valid values for avdecc_entity_stream_info_flags_t */
enum avdecc_entity_stream_info_flags_e
{
	avdecc_entity_stream_info_flags_none = 0u,
	avdecc_entity_stream_info_flags_class_b = 1u << 0, /**< Indicates that the Stream is Class B instead of Class A (default 0 is class A). */
	avdecc_entity_stream_info_flags_fast_connect = 1u << 1, /**< Fast Connect Mode, the Stream was connected in Fast Connect Mode or is presently trying to connect in Fast Connect Mode. */
	avdecc_entity_stream_info_flags_saved_state = 1u << 2, /**< Connection has saved ACMP state. */
	avdecc_entity_stream_info_flags_streaming_wait = 1u << 3, /**< The Stream is presently in STREAMING_WAIT, either it was connected with STREAMING_WAIT flag set or it was stopped with STOP_STREAMING command. */
	avdecc_entity_stream_info_flags_supports_encrypted = 1u << 4, /**< Indicates that the Stream supports streaming with encrypted PDUs. */
	avdecc_entity_stream_info_flags_encrypted_pdu = 1u << 5, /**< Indicates that the Stream is using encrypted PDUs. */
	avdecc_entity_stream_info_flags_talker_failed = 1u << 6, /**< Indicates that the Listener has registered an SRP Talker Failed attribute for the Stream. */
	avdecc_entity_stream_info_flags_stream_vlan_id_valid = 1u << 25, /**< Indicates that the stream_vlan_id field is valid. */
	avdecc_entity_stream_info_flags_connected = 1u << 26, /**< The Stream has been connected with ACMP. This may only be set in a response. */
	avdecc_entity_stream_info_flags_msrp_failure_valid = 1u << 27, /**< The values in the msrp_failure_code and msrp_failure_bridge_id fields are valid. */
	avdecc_entity_stream_info_flags_stream_dest_mac_valid = 1u << 28, /**< The value in the stream_dest_mac field is valid. */
	avdecc_entity_stream_info_flags_msrp_acc_lat_valid = 1u << 29, /**< The value in the msrp_accumulated_latency field is valid. */
	avdecc_entity_stream_info_flags_stream_id_valid = 1u << 30, /**< The value in the stream_id field is valid. */
	avdecc_entity_stream_info_flags_stream_format_valid = 1u << 31, /**< The value in stream_format field is valid and is to be used to change the Stream format if it is a SET_STREAM_INFO command. */
};

/** Valid values for avdecc_entity_stream_info_flags_ex_t */
enum avdecc_entity_stream_info_flags_ex_e
{
	avdecc_entity_stream_info_flags_ex_none = 0u,
	avdecc_entity_stream_info_flags_ex_registering = 1u << 0, /**< StreamInput: Registering either a matching Talker Advertise or a matching Talker Failed attribute. StreamOutput: Declaring a Talker Advertise or a Talker Failed attribute and registering a matching Listener attribute. */
};

/** Valid values for avdecc_entity_avb_info_flags_t */
enum avdecc_entity_avb_info_flags_e
{
	avdecc_entity_avb_info_flags_none = 0u,
	avdecc_entity_avb_info_flags_as_capable = 1u << 0, /**< The IEEE Std 802.1AS-2011 variable asCapable is set on this interface. */
	avdecc_entity_avb_info_flags_gptp_enabled = 1u << 1, /**< Indicates that the interface has the IEEE Std 802.1AS-2011 functionality enabled. */
	avdecc_entity_avb_info_flags_srp_enabled = 1u << 2, /**< Indicates that the interface has the IEEE Std 802.1Q-2011 Clause 35, "Stream Reservation Protocol (SRP)" functionality enabled. */
};

/** Valid values for avdecc_entity_entity_counter_valid_flags_t */
enum avdecc_entity_entity_counter_valid_flag_e
{
	avdecc_entity_entity_counter_valid_flag_none = 0u,
	avdecc_entity_entity_counter_valid_flag_entity_specific_8 = 1u << 24, /**< Entity Specific counter 8. */
	avdecc_entity_entity_counter_valid_flag_entity_specific_7 = 1u << 25, /**< Entity Specific counter 7. */
	avdecc_entity_entity_counter_valid_flag_entity_specific_6 = 1u << 26, /**< Entity Specific counter 6. */
	avdecc_entity_entity_counter_valid_flag_entity_specific_5 = 1u << 27, /**< Entity Specific counter 5. */
	avdecc_entity_entity_counter_valid_flag_entity_specific_4 = 1u << 28, /**< Entity Specific counter 4. */
	avdecc_entity_entity_counter_valid_flag_entity_specific_3 = 1u << 29, /**< Entity Specific counter 3. */
	avdecc_entity_entity_counter_valid_flag_entity_specific_2 = 1u << 30, /**< Entity Specific counter 2. */
	avdecc_entity_entity_counter_valid_flag_entity_specific_1 = 1u << 31, /**< Entity Specific counter 1. */
};

/** Valid values for avdecc_entity_avb_interface_counter_valid_flags_t */
enum avdecc_entity_avb_interface_counter_valid_flag_e
{
	avdecc_entity_avb_interface_counter_valid_flag_none = 0u,
	avdecc_entity_avb_interface_counter_valid_flag_link_up = 1u << 0, /**< Total number of network link up events. */
	avdecc_entity_avb_interface_counter_valid_flag_link_down = 1u << 1, /**< Total number of network link down events. */
	avdecc_entity_avb_interface_counter_valid_flag_frames_tx = 1u << 2, /**< Total number of network frames sent. */
	avdecc_entity_avb_interface_counter_valid_flag_frames_rx = 1u << 3, /**< Total number of network frames received. */
	avdecc_entity_avb_interface_counter_valid_flag_rx_crc_error = 1u << 4, /**< Total number of network frames received with an incorrect CRC. */
	avdecc_entity_avb_interface_counter_valid_flag_gptp_gm_changed = 1u << 5, /**< gPTP grandmaster change count. */
	avdecc_entity_avb_interface_counter_valid_flag_entity_specific_8 = 1u << 24, /**< Entity Specific counter 8. */
	avdecc_entity_avb_interface_counter_valid_flag_entity_specific_7 = 1u << 25, /**< Entity Specific counter 7. */
	avdecc_entity_avb_interface_counter_valid_flag_entity_specific_6 = 1u << 26, /**< Entity Specific counter 6. */
	avdecc_entity_avb_interface_counter_valid_flag_entity_specific_5 = 1u << 27, /**< Entity Specific counter 5. */
	avdecc_entity_avb_interface_counter_valid_flag_entity_specific_4 = 1u << 28, /**< Entity Specific counter 4. */
	avdecc_entity_avb_interface_counter_valid_flag_entity_specific_3 = 1u << 29, /**< Entity Specific counter 3. */
	avdecc_entity_avb_interface_counter_valid_flag_entity_specific_2 = 1u << 30, /**< Entity Specific counter 2. */
	avdecc_entity_avb_interface_counter_valid_flag_entity_specific_1 = 1u << 31, /**< Entity Specific counter 1. */
};

/** Valid values for avdecc_entity_clock_domain_counter_valid_flags_t */
enum avdecc_entity_clock_domain_counter_valid_flag_e
{
	avdecc_entity_clock_domain_counter_valid_flag_none = 0u,
	avdecc_entity_clock_domain_counter_valid_flag_locked = 1u << 0, /**< Increments on a clock locking event. */
	avdecc_entity_clock_domain_counter_valid_flag_unlocked = 1u << 1, /**< Increments on a clock unlocking event. */
	avdecc_entity_clock_domain_counter_valid_flag_entity_specific_8 = 1u << 24, /**< Entity Specific counter 8. */
	avdecc_entity_clock_domain_counter_valid_flag_entity_specific_7 = 1u << 25, /**< Entity Specific counter 7. */
	avdecc_entity_clock_domain_counter_valid_flag_entity_specific_6 = 1u << 26, /**< Entity Specific counter 6. */
	avdecc_entity_clock_domain_counter_valid_flag_entity_specific_5 = 1u << 27, /**< Entity Specific counter 5. */
	avdecc_entity_clock_domain_counter_valid_flag_entity_specific_4 = 1u << 28, /**< Entity Specific counter 4. */
	avdecc_entity_clock_domain_counter_valid_flag_entity_specific_3 = 1u << 29, /**< Entity Specific counter 3. */
	avdecc_entity_clock_domain_counter_valid_flag_entity_specific_2 = 1u << 30, /**< Entity Specific counter 2. */
	avdecc_entity_clock_domain_counter_valid_flag_entity_specific_1 = 1u << 31, /**< Entity Specific counter 1. */
};

/** Valid values for avdecc_entity_stream_input_counter_valid_flags_t */
enum avdecc_entity_stream_input_counter_valid_flag_e
{
	avdecc_entity_stream_input_counter_valid_flag_none = 0u,
	avdecc_entity_stream_input_counter_valid_flag_media_locked = 1u << 0, /**< Increments on a Stream media clock locking. */
	avdecc_entity_stream_input_counter_valid_flag_media_unlocked = 1u << 1, /**< Increments on a Stream media clock unlocking. */
	avdecc_entity_stream_input_counter_valid_flag_stream_reset = 1u << 2, /**< IEEE1722.1-2013 - Increments whenever the Stream playback is reset. */
	avdecc_entity_stream_input_counter_valid_flag_stream_interrupted = 1u << 2, /**< IEEE1722-2016 / Milan - Incremented each time the stream playback is interrupted for any reason other than a Controller Unbind operation. */
	avdecc_entity_stream_input_counter_valid_flag_seq_num_mismatch = 1u << 3, /**< Increments when a Stream data AVTPDU is received with a nonsequential sequence_num field. */
	avdecc_entity_stream_input_counter_valid_flag_media_reset = 1u << 4, /**< Increments on a toggle of the mr bit in the Stream data AVTPDU. */
	avdecc_entity_stream_input_counter_valid_flag_timestamp_uncertain = 1u << 5, /**< Increments on a toggle of the tu bit in the Stream data AVTPDU. */
	avdecc_entity_stream_input_counter_valid_flag_timestamp_valid = 1u << 6, /**< Increments on receipt of a Stream data AVTPDU with the tv bit set. */
	avdecc_entity_stream_input_counter_valid_flag_timestamp_not_valid = 1u << 7, /**< Increments on receipt of a Stream data AVTPDU with tv bit cleared. */
	avdecc_entity_stream_input_counter_valid_flag_unsupported_format = 1u << 8, /**< Increments on receipt of a Stream data AVTPDU that contains an unsupported media type. */
	avdecc_entity_stream_input_counter_valid_flag_late_timestamp = 1u << 9, /**< Increments on receipt of a Stream data AVTPDU with an avtp_timestamp field that is in the past. */
	avdecc_entity_stream_input_counter_valid_flag_early_timestamp = 1u << 10, /**< Increments on receipt of a Stream data AVTPDU with an avtp_timestamp field that is too far in the future to process. */
	avdecc_entity_stream_input_counter_valid_flag_frames_rx = 1u << 11, /**< Increments on each Stream data AVTPDU received. */
	avdecc_entity_stream_input_counter_valid_flag_frames_tx = 1u << 12, /**< Increments on each Stream data AVTPDU transmitted. */
	avdecc_entity_stream_input_counter_valid_flag_entity_specific_8 = 1u << 24, /**< Entity Specific counter 8. */
	avdecc_entity_stream_input_counter_valid_flag_entity_specific_7 = 1u << 25, /**< Entity Specific counter 7. */
	avdecc_entity_stream_input_counter_valid_flag_entity_specific_6 = 1u << 26, /**< Entity Specific counter 6. */
	avdecc_entity_stream_input_counter_valid_flag_entity_specific_5 = 1u << 27, /**< Entity Specific counter 5. */
	avdecc_entity_stream_input_counter_valid_flag_entity_specific_4 = 1u << 28, /**< Entity Specific counter 4. */
	avdecc_entity_stream_input_counter_valid_flag_entity_specific_3 = 1u << 29, /**< Entity Specific counter 3. */
	avdecc_entity_stream_input_counter_valid_flag_entity_specific_2 = 1u << 30, /**< Entity Specific counter 2. */
	avdecc_entity_stream_input_counter_valid_flag_entity_specific_1 = 1u << 31, /**< Entity Specific counter 1. */
};

/** Valid values for avdecc_entity_stream_output_counter_valid_flags_t */
enum avdecc_entity_stream_output_counter_valid_flag_e
{
	avdecc_entity_stream_output_counter_valid_flag_none = 0u,
	avdecc_entity_stream_output_counter_valid_flag_stream_start = 1u << 0, /**< Incremented each time the Talker starts streaming. */
	avdecc_entity_stream_output_counter_valid_flag_stream_stop = 1u << 1, /**< Incremented each time the Talker stops streaming. At any time, the PAAD-AE shall ensure that either STREAM_START=STREAM_STOP+1 (in this case, the Talker is currently streaming), or STREAM_START=STREAM_STOP (in this case, the Talker is not currently streaming). */
	avdecc_entity_stream_output_counter_valid_flag_media_reset = 1u << 2, /**< Incremented at the end of every observation interval during which the "mr" bit has been toggled in any of the transmitted Stream Data AVTPDUs. The duration of the observation interval is implementation-specific and shall be less than or equal to 1 second. */
	avdecc_entity_stream_output_counter_valid_flag_timestamp_uncertain = 1u << 3, /**< Incremented at the end of every observation interval during which the "tu" bit has been set in any of the transmitted Stream Data AVTPDUs. The duration of the observation interval is implementation-specific and shall be less than or equal to 1 second. */
	avdecc_entity_stream_output_counter_valid_flag_frames_tx = 1u << 4, /**< Incremented at the end of every observation interval during which at least one Stream Data AVTPDU has been transmitted on this STREAM_OUTPUT. The duration of the observation interval is implementation-specific and shall be less than or equal to 1 second. */
};

/** Valid values for avdecc_entity_milan_info_features_flags_t */
enum avdecc_entity_milan_info_features_flags_e
{
	avdecc_entity_milan_info_features_flags_none = 0u,
	avdecc_entity_milan_info_features_flags_redundancy = 1u << 0, /**< The entity supports the milan redundancy scheme. */
};


/** Valid values for avdecc_protocol_adp_message_type_t */
enum avdecc_protocol_adp_message_type_e
{
	avdecc_protocol_adp_message_type_entity_available = 0,
	avdecc_protocol_adp_message_type_entity_departing = 1,
	avdecc_protocol_adp_message_type_entity_discover = 2,
};

/** Valid values for avdecc_protocol_aecp_message_type_t */
enum avdecc_protocol_aecp_message_type_e
{
	avdecc_protocol_aecp_message_type_aem_command = 0,
	avdecc_protocol_aecp_message_type_aem_response = 1,
	avdecc_protocol_aecp_message_type_address_access_command = 2,
	avdecc_protocol_aecp_message_type_address_access_response = 3,
	avdecc_protocol_aecp_message_type_avc_command = 4,
	avdecc_protocol_aecp_message_type_avc_response = 5,
	avdecc_protocol_aecp_message_type_vendor_unique_command = 6,
	avdecc_protocol_aecp_message_type_vendor_unique_response = 7,
	avdecc_protocol_aecp_message_type_hdcp_aem_command = 8,
	avdecc_protocol_aecp_message_type_hdcp_aem_response = 9,
	avdecc_protocol_aecp_message_type_extended_command = 14,
	avdecc_protocol_aecp_message_type_extended_response = 15,
};

/** Valid values for avdecc_protocol_aem_status_t */
enum avdecc_protocol_aem_status_e
{
	avdecc_protocol_aem_status_success = 0,
	avdecc_protocol_aem_status_not_implemented = 1,
	avdecc_protocol_aem_status_no_such_descriptor = 2,
	avdecc_protocol_aem_status_entity_locked = 3,
	avdecc_protocol_aem_status_entity_acquired = 4,
	avdecc_protocol_aem_status_not_authenticated = 5,
	avdecc_protocol_aem_status_authentication_disabled = 6,
	avdecc_protocol_aem_status_bad_arguments = 7,
	avdecc_protocol_aem_status_no_resources = 8,
	avdecc_protocol_aem_status_in_progress = 9,
	avdecc_protocol_aem_status_entity_misbehaving = 10,
	avdecc_protocol_aem_status_not_supported = 11,
	avdecc_protocol_aem_status_stream_is_running = 12,
};

/** Valid values for avdecc_protocol_aem_command_type_t */
enum avdecc_protocol_aem_command_type_e
{
	avdecc_protocol_aem_command_type_acquire_entity = 0x0000,
	avdecc_protocol_aem_command_type_lock_entity = 0x0001,
	avdecc_protocol_aem_command_type_entity_available = 0x0002,
	avdecc_protocol_aem_command_type_controller_available = 0x0003,
	avdecc_protocol_aem_command_type_read_descriptor = 0x0004,
	avdecc_protocol_aem_command_type_write_descriptor = 0x0005,
	avdecc_protocol_aem_command_type_set_configuration = 0x0006,
	avdecc_protocol_aem_command_type_get_configuration = 0x0007,
	avdecc_protocol_aem_command_type_set_stream_format = 0x0008,
	avdecc_protocol_aem_command_type_get_stream_format = 0x0009,
	avdecc_protocol_aem_command_type_set_video_format = 0x000a,
	avdecc_protocol_aem_command_type_get_video_format = 0x000b,
	avdecc_protocol_aem_command_type_set_sensor_format = 0x000c,
	avdecc_protocol_aem_command_type_get_sensor_format = 0x000d,
	avdecc_protocol_aem_command_type_set_stream_info = 0x000e,
	avdecc_protocol_aem_command_type_get_stream_info = 0x000f,
	avdecc_protocol_aem_command_type_set_name = 0x0010,
	avdecc_protocol_aem_command_type_get_name = 0x0011,
	avdecc_protocol_aem_command_type_set_association_id = 0x0012,
	avdecc_protocol_aem_command_type_get_association_id = 0x0013,
	avdecc_protocol_aem_command_type_set_sampling_rate = 0x0014,
	avdecc_protocol_aem_command_type_get_sampling_rate = 0x0015,
	avdecc_protocol_aem_command_type_set_clock_source = 0x0016,
	avdecc_protocol_aem_command_type_get_clock_source = 0x0017,
	avdecc_protocol_aem_command_type_set_control = 0x0018,
	avdecc_protocol_aem_command_type_get_control = 0x0019,
	avdecc_protocol_aem_command_type_increment_control = 0x001a,
	avdecc_protocol_aem_command_type_decrement_control = 0x001b,
	avdecc_protocol_aem_command_type_set_signal_selector = 0x001c,
	avdecc_protocol_aem_command_type_get_signal_selector = 0x001d,
	avdecc_protocol_aem_command_type_set_mixer = 0x001e,
	avdecc_protocol_aem_command_type_get_mixer = 0x001f,
	avdecc_protocol_aem_command_type_set_matrix = 0x0020,
	avdecc_protocol_aem_command_type_get_matrix = 0x0021,
	avdecc_protocol_aem_command_type_start_streaming = 0x0022,
	avdecc_protocol_aem_command_type_stop_streaming = 0x0023,
	avdecc_protocol_aem_command_type_register_unsolicited_notification = 0x0024,
	avdecc_protocol_aem_command_type_deregister_unsolicited_notification = 0x0025,
	avdecc_protocol_aem_command_type_identify_notification = 0x0026,
	avdecc_protocol_aem_command_type_get_avb_info = 0x0027,
	avdecc_protocol_aem_command_type_get_as_path = 0x0028,
	avdecc_protocol_aem_command_type_get_counters = 0x0029,
	avdecc_protocol_aem_command_type_reboot = 0x002a,
	avdecc_protocol_aem_command_type_get_audio_map = 0x002b,
	avdecc_protocol_aem_command_type_add_audio_mappings = 0x002c,
	avdecc_protocol_aem_command_type_remove_audio_mappings = 0x002d,
	avdecc_protocol_aem_command_type_get_video_map = 0x002e,
	avdecc_protocol_aem_command_type_add_video_mappings = 0x002f,
	avdecc_protocol_aem_command_type_remove_video_mappings = 0x0030,
	avdecc_protocol_aem_command_type_get_sensor_map = 0x0031,
	avdecc_protocol_aem_command_type_add_sensor_mappings = 0x0032,
	avdecc_protocol_aem_command_type_remove_sensor_mappings = 0x0033,
	avdecc_protocol_aem_command_type_start_operation = 0x0034,
	avdecc_protocol_aem_command_type_abort_operation = 0x0035,
	avdecc_protocol_aem_command_type_operation_status = 0x0036,
	avdecc_protocol_aem_command_type_auth_add_key = 0x0037,
	avdecc_protocol_aem_command_type_auth_delete_key = 0x0038,
	avdecc_protocol_aem_command_type_auth_get_key_list = 0x0039,
	avdecc_protocol_aem_command_type_auth_get_key = 0x003a,
	avdecc_protocol_aem_command_type_auth_add_key_to_chain = 0x003b,
	avdecc_protocol_aem_command_type_auth_delete_key_from_chain = 0x003c,
	avdecc_protocol_aem_command_type_auth_get_keychain_list = 0x003d,
	avdecc_protocol_aem_command_type_auth_get_identity = 0x003e,
	avdecc_protocol_aem_command_type_auth_add_token = 0x003f,
	avdecc_protocol_aem_command_type_auth_delete_token = 0x0040,
	avdecc_protocol_aem_command_type_authenticate = 0x0041,
	avdecc_protocol_aem_command_type_deauthenticate = 0x0042,
	avdecc_protocol_aem_command_type_enable_transport_security = 0x0043,
	avdecc_protocol_aem_command_type_disable_transport_security = 0x0044,
	avdecc_protocol_aem_command_type_enable_stream_encryption = 0x0045,
	avdecc_protocol_aem_command_type_disable_stream_encryption = 0x0046,
	avdecc_protocol_aem_command_type_set_memory_object_length = 0x0047,
	avdecc_protocol_aem_command_type_get_memory_object_length = 0x0048,
	avdecc_protocol_aem_command_type_set_stream_backup = 0x0049,
	avdecc_protocol_aem_command_type_get_stream_backup = 0x004a,
	avdecc_protocol_aem_command_type_expansion = 0x7fff,
	avdecc_protocol_aem_command_type_invalid_command_type = 0xffff,
};

/** Valid values for avdecc_protocol_aem_acquire_entity_flags_t */
enum avdecc_protocol_aem_acquire_entity_flags_e
{
	avdecc_protocol_aem_acquire_entity_flags_none = 0x00000000,
	avdecc_protocol_aem_acquire_entity_flags_persistent = 0x00000001,
	avdecc_protocol_aem_acquire_entity_flags_release = 0x80000000,
};

/** Valid values for avdecc_protocol_aem_lock_entity_flags_t */
enum avdecc_protocol_aem_lock_entity_flags_e
{
	avdecc_protocol_aem_lock_entity_flags_none = 0x00000000,
	avdecc_protocol_aem_lock_entity_flags_unlock = 0x80000000,
};

/** Valid values for avdecc_protocol_aa_mode_t */
enum avdecc_protocol_aa_mode_e
{
	avdecc_protocol_aa_mode_read = 0x0,
	avdecc_protocol_aa_mode_write = 0x1,
	avdecc_protocol_aa_mode_execute = 0x2,
};

/** Valid values for avdecc_protocol_aa_status_t */
enum avdecc_protocol_aa_status_e
{
	avdecc_protocol_aa_status_success = 0,
	avdecc_protocol_aa_status_not_implemented = 1,
	avdecc_protocol_aa_status_address_too_low = 2,
	avdecc_protocol_aa_status_address_too_high = 3,
	avdecc_protocol_aa_status_address_invalid = 4,
	avdecc_protocol_aa_status_tlv_invalid = 5,
	avdecc_protocol_aa_status_data_invalid = 6,
	avdecc_protocol_aa_status_unsupported = 7,
};

/** Valid values for avdecc_protocol_mvu_status_t */
enum avdecc_protocol_mvu_status_e
{
	avdecc_protocol_mvu_status_success = 0,
	avdecc_protocol_mvu_status_not_implemented = 1,
};

/** Valid values for avdecc_protocol_mvu_command_type_t */
enum avdecc_protocol_mvu_command_type_e
{
	avdecc_protocol_mvu_command_type_get_milan_info = 0x0000,
	avdecc_protocol_mvu_command_type_invalid_command_type = 0xffff,
};

/** Valid values for avdecc_protocol_acmp_message_type_t */
enum avdecc_protocol_acmp_message_type_e
{
	avdecc_protocol_acmp_message_type_connect_tx_command = 0,
	avdecc_protocol_acmp_message_type_connect_tx_response = 1,
	avdecc_protocol_acmp_message_type_disconnect_tx_command = 2,
	avdecc_protocol_acmp_message_type_disconnect_tx_response = 3,
	avdecc_protocol_acmp_message_type_get_tx_state_command = 4,
	avdecc_protocol_acmp_message_type_get_tx_state_response = 5,
	avdecc_protocol_acmp_message_type_connect_rx_command = 6,
	avdecc_protocol_acmp_message_type_connect_rx_response = 7,
	avdecc_protocol_acmp_message_type_disconnect_rx_command = 8,
	avdecc_protocol_acmp_message_type_disconnect_rx_response = 9,
	avdecc_protocol_acmp_message_type_get_rx_state_command = 10,
	avdecc_protocol_acmp_message_type_get_rx_state_response = 11,
	avdecc_protocol_acmp_message_type_get_tx_connection_command = 12,
	avdecc_protocol_acmp_message_type_get_tx_connection_response = 13,
};

/** Valid values for avdecc_protocol_acmp_status_t */
enum avdecc_protocol_acmp_status_e
{
	avdecc_protocol_acmp_status_success = 0,
	avdecc_protocol_acmp_status_listener_unknown_id = 1,
	avdecc_protocol_acmp_status_talker_unknown_id = 2,
	avdecc_protocol_acmp_status_talker_dest_mac_fail = 3,
	avdecc_protocol_acmp_status_talker_no_stream_index = 4,
	avdecc_protocol_acmp_status_talker_no_bandwidth = 5,
	avdecc_protocol_acmp_status_talker_exclusive = 6,
	avdecc_protocol_acmp_status_listener_talker_timeout = 7,
	avdecc_protocol_acmp_status_listener_exclusive = 8,
	avdecc_protocol_acmp_status_state_unavailable = 9,
	avdecc_protocol_acmp_status_not_connected = 10,
	avdecc_protocol_acmp_status_no_such_connection = 11,
	avdecc_protocol_acmp_status_could_not_send_message = 12,
	avdecc_protocol_acmp_status_talker_misbehaving = 13,
	avdecc_protocol_acmp_status_listener_misbehaving = 14,
	avdecc_protocol_acmp_status_controller_not_authorized = 16,
	avdecc_protocol_acmp_status_incompatible_request = 17,
	avdecc_protocol_acmp_status_not_supported = 31,
};

/** Valid values for avdecc_local_entity_advertise_flags_t */
enum avdecc_local_entity_advertise_flags_e
{
	avdecc_local_entity_advertise_flags_none = 0,
	avdecc_local_entity_advertise_flags_entity_capabilities = 1u << 0, /**< EntityCapabilities field changed */
	avdecc_local_entity_advertise_flags_association_id = 1u << 1, /**< AssociationID field changed */
	avdecc_local_entity_advertise_flags_valid_time = 1u << 2, /**< ValidTime field changed */
	avdecc_local_entity_advertise_flags_gptp_grandmaster_id = 1u << 3, /**< gPTP GrandmasterID field changed */
	avdecc_local_entity_advertise_flags_gptp_domain_number = 1u << 4, /**< gPTP DomainNumber field changed */
};

/** Valid values for avdecc_local_entity_aem_command_status_t */
enum avdecc_local_entity_aem_command_status_e
{
	// AVDECC Protocol Error Codes
	avdecc_local_entity_aem_command_status_success = 0,
	avdecc_local_entity_aem_command_status_not_implemented = 1,
	avdecc_local_entity_aem_command_status_no_such_descriptor = 2,
	avdecc_local_entity_aem_command_status_locked_by_other = 3,
	avdecc_local_entity_aem_command_status_acquired_by_other = 4,
	avdecc_local_entity_aem_command_status_not_authenticated = 5,
	avdecc_local_entity_aem_command_status_authentication_disabled = 6,
	avdecc_local_entity_aem_command_status_bad_arguments = 7,
	avdecc_local_entity_aem_command_status_no_resources = 8,
	avdecc_local_entity_aem_command_status_in_progress = 9,
	avdecc_local_entity_aem_command_status_entity_misbehaving = 10,
	avdecc_local_entity_aem_command_status_not_supported = 11,
	avdecc_local_entity_aem_command_status_stream_is_running = 12,
	// Library Error Codes
	avdecc_local_entity_aem_command_status_network_error = 995,
	avdecc_local_entity_aem_command_status_protocol_error = 996,
	avdecc_local_entity_aem_command_status_timed_out = 997,
	avdecc_local_entity_aem_command_status_unknown_entity = 998,
	avdecc_local_entity_aem_command_status_internal_error = 999,
};

/** Valid values for avdecc_local_entity_aa_command_status_t */
enum avdecc_local_entity_aa_command_status_e
{
	// AVDECC Protocol Error Codes
	avdecc_local_entity_aa_command_status_success = 0,
	avdecc_local_entity_aa_command_status_not_implemented = 1,
	avdecc_local_entity_aa_command_status_address_too_low = 2,
	avdecc_local_entity_aa_command_status_address_too_high = 3,
	avdecc_local_entity_aa_command_status_address_invalid = 4,
	avdecc_local_entity_aa_command_status_tlv_invalid = 5,
	avdecc_local_entity_aa_command_status_data_invalid = 6,
	avdecc_local_entity_aa_command_status_unsupported = 7,
	// Library Error Codes
	avdecc_local_entity_aa_command_status_network_error = 995,
	avdecc_local_entity_aa_command_status_protocol_error = 996,
	avdecc_local_entity_aa_command_status_timed_out = 997,
	avdecc_local_entity_aa_command_status_unknown_entity = 998,
	avdecc_local_entity_aa_command_status_internal_error = 999,
};

/** Valid values for avdecc_local_entity_mvu_command_status_t */
enum avdecc_local_entity_mvu_command_status_e
{
	// AVDECC Protocol Error Codes
	avdecc_local_entity_mvu_command_status_success = 0,
	avdecc_local_entity_mvu_command_status_not_implemented = 1,
	avdecc_local_entity_mvu_command_status_bad_arguments = 2,
	// Library Error Codes
	avdecc_local_entity_mvu_command_status_network_error = 995,
	avdecc_local_entity_mvu_command_status_protocol_error = 996,
	avdecc_local_entity_mvu_command_status_timed_out = 997,
	avdecc_local_entity_mvu_command_status_unknown_entity = 998,
	avdecc_local_entity_mvu_command_status_internal_error = 999,
};

/** Valid values for avdecc_local_entity_control_status_t */
enum avdecc_local_entity_control_status_e
{
	// AVDECC Protocol Error Codes
	avdecc_local_entity_control_status_success = 0,
	avdecc_local_entity_control_status_listenerunknown_id = 1, /**< Listener does not have the specified unique identifier. */
	avdecc_local_entity_control_status_talker_unknown_id = 2, /**< Talker does not have the specified unique identifier. */
	avdecc_local_entity_control_status_talker_dest_mac_fail = 3, /**< Talker could not allocate a destination MAC for the Stream. */
	avdecc_local_entity_control_status_talker_no_stream_index = 4, /**< Talker does not have an available Stream index for the Stream. */
	avdecc_local_entity_control_status_talker_no_bandwidth = 5, /**< Talker could not allocate bandwidth for the Stream. */
	avdecc_local_entity_control_status_talker_exclusive = 6, /**< Talker already has an established Stream and only supports one Listener. */
	avdecc_local_entity_control_status_listener_talker_timeout = 7, /**< Listener had timeout for all retries when trying to send command to Talker. */
	avdecc_local_entity_control_status_listener_exclusive = 8, /**< The AVDECC Listener already has an established connection to a Stream. */
	avdecc_local_entity_control_status_state_unavailable = 9, /**< Could not get the state from the AVDECC Entity. */
	avdecc_local_entity_control_status_not_connected = 10, /**< Trying to disconnect when not connected or not connected to the AVDECC Talker specified. */
	avdecc_local_entity_control_status_no_such_connection = 11, /**< Trying to obtain connection info for an AVDECC Talker connection which does not exist. */
	avdecc_local_entity_control_status_could_not_send_message = 12, /**< The AVDECC Listener failed to send the message to the AVDECC Talker. */
	avdecc_local_entity_control_status_talker_misbehaving = 13, /**< Talker was unable to complete the command because an internal error occurred. */
	avdecc_local_entity_control_status_listener_misbehaving = 14, /**< Listener was unable to complete the command because an internal error occurred. */
	avdecc_local_entity_control_status_controller_not_authorized = 16, /**< The AVDECC Controller with the specified Entity ID is not authorized to change Stream connections. */
	avdecc_local_entity_control_status_incompatible_request = 17, /**< The AVDECC Listener is trying to connect to an AVDECC Talker that is already streaming with a different traffic class, etc. or does not support the requested traffic class. */
	avdecc_local_entity_control_status_not_supported = 31, /**< The command is not supported. */
	// Library Error Codes
	avdecc_local_entity_control_status_network_error = 995, /**< A network error occured. */
	avdecc_local_entity_control_status_protocol_error = 996, /**< A protocol error occured. */
	avdecc_local_entity_control_status_timed_out = 997, /**< Command timed out. */
	avdecc_local_entity_control_status_unknown_entity = 998, /**< Entity is unknown. */
	avdecc_local_entity_control_status_internal_error = 999, /**< Internal library error. */
};

/** Valid values for avdecc_local_entity_error_t */
enum avdecc_local_entity_error_e
{
	avdecc_local_entity_error_no_error = 0,
	avdecc_local_entity_error_invalid_parameters = 1, /**< Specified parameters are invalid. */
	avdecc_local_entity_error_duplicate_local_entity_id = 2, /**< The specified EntityID is already in use on the local computer. */
	avdecc_local_entity_error_invalid_entity_handle = 98, /**< Passed LA_AVDECC_LOCAL_ENTITY_HANDLE is invalid. */
	avdecc_local_entity_error_internal_error = 99, /**< Internal error, please report the issue. */
};

/** Valid values for avdecc_entity_model_descriptor_type_t */
enum avdecc_entity_model_descriptor_type_e
{
	avdecc_entity_model_descriptor_type_entity = 0x0000,
	avdecc_entity_model_descriptor_type_configuration = 0x0001,
	avdecc_entity_model_descriptor_type_audio_unit = 0x0002,
	avdecc_entity_model_descriptor_type_video_unit = 0x0003,
	avdecc_entity_model_descriptor_type_sensor_unit = 0x0004,
	avdecc_entity_model_descriptor_type_stream_input = 0x0005,
	avdecc_entity_model_descriptor_type_stream_output = 0x0006,
	avdecc_entity_model_descriptor_type_jack_input = 0x0007,
	avdecc_entity_model_descriptor_type_jack_output = 0x0008,
	avdecc_entity_model_descriptor_type_avb_interface = 0x0009,
	avdecc_entity_model_descriptor_type_clock_source = 0x000a,
	avdecc_entity_model_descriptor_type_memory_object = 0x000b,
	avdecc_entity_model_descriptor_type_locale = 0x000c,
	avdecc_entity_model_descriptor_type_strings = 0x000d,
	avdecc_entity_model_descriptor_type_stream_port_input = 0x000e,
	avdecc_entity_model_descriptor_type_stream_port_output = 0x000f,
	avdecc_entity_model_descriptor_type_external_Port_input = 0x0010,
	avdecc_entity_model_descriptor_type_external_Port_output = 0x0011,
	avdecc_entity_model_descriptor_type_internal_Port_input = 0x0012,
	avdecc_entity_model_descriptor_type_internal_Port_output = 0x0013,
	avdecc_entity_model_descriptor_type_audio_cluster = 0x0014,
	avdecc_entity_model_descriptor_type_video_cluster = 0x0015,
	avdecc_entity_model_descriptor_type_sensor_cluster = 0x0016,
	avdecc_entity_model_descriptor_type_audio_map = 0x0017,
	avdecc_entity_model_descriptor_type_video_map = 0x0018,
	avdecc_entity_model_descriptor_type_sensor_map = 0x0019,
	avdecc_entity_model_descriptor_type_control = 0x001a,
	avdecc_entity_model_descriptor_type_signal_selector = 0x001b,
	avdecc_entity_model_descriptor_type_mixer = 0x001c,
	avdecc_entity_model_descriptor_type_matrix = 0x001d,
	avdecc_entity_model_descriptor_type_matrix_signal = 0x001e,
	avdecc_entity_model_descriptor_type_signal_splitter = 0x001f,
	avdecc_entity_model_descriptor_type_signal_combiner = 0x0020,
	avdecc_entity_model_descriptor_type_signal_demultiplexer = 0x0021,
	avdecc_entity_model_descriptor_type_signal_multiplexer = 0x0022,
	avdecc_entity_model_descriptor_type_signal_transcoder = 0x0023,
	avdecc_entity_model_descriptor_type_clock_domain = 0x0024,
	avdecc_entity_model_descriptor_type_control_block = 0x0025,
	/* 0026 to fffe reserved for future use */
	avdecc_entity_model_descriptor_type_invalid = 0xffff
};

/** Valid values for avdecc_entity_model_jack_type_t */
enum avdecc_entity_model_jack_type_e
{
	avdecc_entity_model_jack_type_speaker = 0x0000,
	avdecc_entity_model_jack_type_headphone = 0x0001,
	avdecc_entity_model_jack_type_analog_microphone = 0x0002,
	avdecc_entity_model_jack_type_spdif = 0x0003,
	avdecc_entity_model_jack_type_adat = 0x0004,
	avdecc_entity_model_jack_type_tdif = 0x0005,
	avdecc_entity_model_jack_type_madi = 0x0006,
	avdecc_entity_model_jack_type_unbalanced_analog = 0x0007,
	avdecc_entity_model_jack_type_balanced_analog = 0x0008,
	avdecc_entity_model_jack_type_digital = 0x0009,
	avdecc_entity_model_jack_type_midi = 0x000a,
	avdecc_entity_model_jack_type_aes_ebu = 0x000b,
	avdecc_entity_model_jack_type_composite_video = 0x000c,
	avdecc_entity_model_jack_type_svhs_video = 0x000d,
	avdecc_entity_model_jack_type_component_video = 0x000e,
	avdecc_entity_model_jack_type_dvi = 0x000f,
	avdecc_entity_model_jack_type_hdmi = 0x0010,
	avdecc_entity_model_jack_type_udi = 0x0011,
	avdecc_entity_model_jack_type_display_port = 0x0012,
	avdecc_entity_model_jack_type_antenna = 0x0013,
	avdecc_entity_model_jack_type_analog_tuner = 0x0014,
	avdecc_entity_model_jack_type_ethernet = 0x0015,
	avdecc_entity_model_jack_type_wifi = 0x0016,
	avdecc_entity_model_jack_type_usb = 0x0017,
	avdecc_entity_model_jack_type_pci = 0x0018,
	avdecc_entity_model_jack_type_pci_e = 0x0019,
	avdecc_entity_model_jack_type_scsi = 0x001a,
	avdecc_entity_model_jack_type_ata = 0x001b,
	avdecc_entity_model_jack_type_imager = 0x001c,
	avdecc_entity_model_jack_type_ir = 0x001d,
	avdecc_entity_model_jack_type_thunderbolt = 0x001e,
	avdecc_entity_model_jack_type_sata = 0x001f,
	avdecc_entity_model_jack_type_smpte_ltc = 0x0020,
	avdecc_entity_model_jack_type_digital_microphone = 0x0021,
	avdecc_entity_model_jack_type_audioMedia_clock = 0x0022,
	avdecc_entity_model_jack_type_videoMedia_clock = 0x0023,
	avdecc_entity_model_jack_type_gnss_clock = 0x0024,
	avdecc_entity_model_jack_type_pps = 0x0025,
	/* 0026 to fffe reserved for future use */
	avdecc_entity_model_jack_type_expansion = 0xffff
};

/** Valid values for avdecc_entity_model_clock_source_type_t */
enum avdecc_entity_model_clock_source_type_e
{
	avdecc_entity_model_clock_source_type_internal = 0x0000,
	avdecc_entity_model_clock_source_type_external = 0x0001,
	avdecc_entity_model_clock_source_type_input_stream = 0x0002,
	/* 0003 to fffe reserved for future use */
	avdecc_entity_model_clock_source_type_expansion = 0xffff
};

/** Valid values for avdecc_entity_model_memory_object_type_t */
enum avdecc_entity_model_memory_object_type_e
{
	avdecc_entity_model_memory_object_type_firmware_image = 0x0000,
	avdecc_entity_model_memory_object_type_vendor_specific = 0x0001,
	avdecc_entity_model_memory_object_type_crash_dump = 0x0002,
	avdecc_entity_model_memory_object_type_log_object = 0x0003,
	avdecc_entity_model_memory_object_type_autostart_settings = 0x0004,
	avdecc_entity_model_memory_object_type_snapshot_settings = 0x0005,
	avdecc_entity_model_memory_object_type_svg_manufacturer = 0x0006,
	avdecc_entity_model_memory_object_type_svg_entity = 0x0007,
	avdecc_entity_model_memory_object_type_svg_generic = 0x0008,
	avdecc_entity_model_memory_object_type_png_manufacturer = 0x0009,
	avdecc_entity_model_memory_object_type_png_entity = 0x000a,
	avdecc_entity_model_memory_object_type_png_generic = 0x000b,
	avdecc_entity_model_memory_object_type_dae_manufacturer = 0x000c,
	avdecc_entity_model_memory_object_type_dae_entity = 0x000d,
	avdecc_entity_model_memory_object_type_dae_generic = 0x000e,
	/* 000f to ffff reserved for future use */
};

/** Valid values for avdecc_entity_model_memory_object_operation_type_t */
enum avdecc_entity_model_memory_object_operation_type_e
{
	avdecc_entity_model_memory_object_operation_type_store = 0x0000,
	avdecc_entity_model_memory_object_operation_type_store_and_reboot = 0x0001,
	avdecc_entity_model_memory_object_operation_type_read = 0x0002,
	avdecc_entity_model_memory_object_operation_type_erase = 0x0003,
	avdecc_entity_model_memory_object_operation_type_upload = 0x0004,
	/* 0005 to ffff reserved for future use */
};

/** Valid values for avdecc_entity_model_audio_cluster_format_t */
enum avdecc_entity_model_audio_cluster_format_e
{
	avdecc_entity_model_audio_cluster_format_iec_60958 = 0x00,
	avdecc_entity_model_audio_cluster_format_mbla = 0x40,
	avdecc_entity_model_audio_cluster_format_midi = 0x80,
	avdecc_entity_model_audio_cluster_format_smpte = 0x88,
};

/** Valid values for avdecc_entity_model_probing_status_t */
enum avdecc_entity_model_probing_status_e
{
	avdecc_entity_model_probing_status_disabled = 0x00, /** The sink is not probing because it is not bound. */
	avdecc_entity_model_probing_status_passive = 0x01, /** The sink is probing passively. It waits until the bound talker has been discovered. */
	avdecc_entity_model_probing_status_active = 0x02, /** The sink is probing actively. It is querying the stream parameters to the talker. */
	avdecc_entity_model_probing_status_completed = 0x03, /** The sink is not probing because it is settled. */
	/* 04 to 07 reserved for future use */
};

/** Valid values for avdecc_entity_model_msrp_failure_code_t */
enum avdecc_entity_model_msrp_failure_code_e
{
	avdecc_entity_model_msrp_failure_code_no_Failure = 0,
	avdecc_entity_model_msrp_failure_code_insufficient_bandwidth = 1,
	avdecc_entity_model_msrp_failure_code_insufficient_resources = 2,
	avdecc_entity_model_msrp_failure_code_insufficient_traffic_class_bandwidth = 3,
	avdecc_entity_model_msrp_failure_code_stream_id_in_use = 4,
	avdecc_entity_model_msrp_failure_code_stream_destination_address_in_use = 5,
	avdecc_entity_model_msrp_failure_code_stream_preempted_by_higher_rank = 6,
	avdecc_entity_model_msrp_failure_code_latency_has_changed = 7,
	avdecc_entity_model_msrp_failure_code_egress_port_not_avb_capable = 8,
	avdecc_entity_model_msrp_failure_code_use_different_destination_address = 9,
	avdecc_entity_model_msrp_failure_code_out_of_msrp_resources = 10,
	avdecc_entity_model_msrp_failure_code_out_of_mmrp_resources = 11,
	avdecc_entity_model_msrp_failure_code_cannot_store_destination_address = 12,
	avdecc_entity_model_msrp_failure_code_priority_is_not_an_sr_class = 13,
	avdecc_entity_model_msrp_failure_code_max_frame_size_too_large = 14,
	avdecc_entity_model_msrp_failure_code_max_fan_in_ports_limit_reached = 15,
	avdecc_entity_model_msrp_failure_code_first_value_changed_for_stream_id = 16,
	avdecc_entity_model_msrp_failure_code_vlan_blocked_on_egress = 17,
	avdecc_entity_model_msrp_failure_code_vlan_tagging_disabled_on_egress = 18,
	avdecc_entity_model_msrp_failure_code_sr_class_priority_mismatch = 19,
};
