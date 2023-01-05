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
* @file avdecc.h
* @author Christophe Calmejane
* @brief Main Avdecc C Bindings Library header file.
*/

#pragma once

/** Typedefs */
#include "internals/typedefs.h"

/** Symbols export definition */
#include "internals/exports.h"

/* ************************************************************************** */
/* General library APIs                                                       */
/* ************************************************************************** */

/**
* Interface version of the library, used to check for compatibility between the version used to compile and the runtime version.<BR>
* Everytime the interface changes (what is visible from the user) you increase the InterfaceVersion value.<BR>
* A change in the visible interface is any modification in a public header file.
* Any other change (including inline methods, defines, typedefs, ...) are considered a modification of the interface.
*/
#define LA_AVDECC_InterfaceVersion 100

/**
* @brief Checks if the library is compatible with specified interface version.
* @param[in] interfaceVersion The interface version to check for compatibility.
* @return True if the library is compatible.
* @note If the library is not compatible, the application should no longer use the library.<BR>
*       When using the avdecc shared library, you must call this version to check the compatibility between the compiled and the loaded version.
*/
LA_AVDECC_BINDINGS_C_API avdecc_bool_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_isCompatibleWithInterfaceVersion(avdecc_interface_version_t const interfaceVersion);

/**
* @brief Gets the avdecc library version.
* @details Returns the avdecc library version.
* @return A string representing the library version.
*/
LA_AVDECC_BINDINGS_C_API avdecc_const_string_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_getVersion();

/**
* @brief Gets the avdecc shared library interface version.
* @details Returns the avdecc shared library interface version.
* @return The interface version.
*/
LA_AVDECC_BINDINGS_C_API avdecc_interface_version_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_getInterfaceVersion();

/** Initializes the library, must be called before any other calls. */
LA_AVDECC_BINDINGS_C_API void LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_initialize();

/** Uninitializes the library, must be called before exiting the program or unexpected behavior might occur. */
LA_AVDECC_BINDINGS_C_API void LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_uninitialize();


/* ************************************************************************** */
/* Global APIs                                                                */
/* ************************************************************************** */

LA_AVDECC_BINDINGS_C_API avdecc_unique_identifier_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_getNullUniqueIdentifier();
LA_AVDECC_BINDINGS_C_API avdecc_unique_identifier_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_getUninitializedUniqueIdentifier();
LA_AVDECC_BINDINGS_C_API avdecc_unique_identifier_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_generateEID(avdecc_mac_address_cp const address, unsigned short const progID); // DEPRECATED: Use LA_AVDECC_ProtocolInterface_getDynamicEID instead
LA_AVDECC_BINDINGS_C_API avdecc_entity_model_descriptor_index_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_getGlobalAvbInterfaceIndex();

/** Frees avdecc_string_t */
LA_AVDECC_BINDINGS_C_API void LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_freeString(avdecc_string_t str);


/* ************************************************************************** */
/* Entity                                                                     */
/* ************************************************************************** */

typedef struct avdecc_entity_common_information_s
{
	avdecc_unique_identifier_t entity_id;
	avdecc_unique_identifier_t entity_model_id;
	avdecc_entity_entity_capabilities_t entity_capabilities;
	unsigned short talker_stream_sources;
	avdecc_entity_talker_capabilities_t talker_capabilities;
	unsigned short listener_stream_sinks;
	avdecc_entity_listener_capabilities_t listener_capabilities;
	avdecc_entity_controller_capabilities_t controller_capabilities;
	avdecc_bool_t identify_control_index_valid;
	avdecc_entity_model_descriptor_index_t identify_control_index;
	avdecc_bool_t association_id_valid;
	avdecc_unique_identifier_t association_id;
} avdecc_entity_common_information_t, *avdecc_entity_common_information_p;
typedef avdecc_entity_common_information_t const* avdecc_entity_common_information_cp;

typedef struct avdecc_entity_interface_information_s
{
	avdecc_entity_model_descriptor_index_t interface_index;
	avdecc_mac_address_t mac_address;
	unsigned char valid_time;
	unsigned int available_index;
	avdecc_bool_t gptp_grandmaster_id_valid;
	avdecc_unique_identifier_t gptp_grandmaster_id;
	avdecc_bool_t gptp_domain_number_valid;
	unsigned char gptp_domain_number;
	struct avdecc_entity_interface_information_s* next;
} avdecc_entity_interface_information_t, *avdecc_entity_interface_information_p;
typedef avdecc_entity_interface_information_t const* avdecc_entity_interface_information_cp;

typedef struct avdecc_entity_s
{
	avdecc_entity_common_information_t common_information;
	avdecc_entity_interface_information_t interfaces_information;
} avdecc_entity_t, *avdecc_entity_p;
typedef avdecc_entity_t const* avdecc_entity_cp;


/* ************************************************************************** */
/* Entity Model                                                               */
/* ************************************************************************** */

/** Stream Identification (EntityID/StreamIndex couple) */
typedef struct avdecc_entity_model_stream_identification_s
{
	avdecc_unique_identifier_t entity_id;
	avdecc_entity_model_descriptor_index_t stream_index;
} avdecc_entity_model_stream_identification_t, *avdecc_entity_model_stream_identification_p;
typedef avdecc_entity_model_stream_identification_t const* avdecc_entity_model_stream_identification_cp;

typedef struct avdecc_entity_model_audio_mapping_s
{
	avdecc_entity_model_descriptor_index_t stream_index;
	unsigned short stream_channel;
	avdecc_entity_model_descriptor_index_t cluster_offset;
	unsigned short cluster_channel;
} avdecc_entity_model_audio_mapping_t, *avdecc_entity_model_audio_mapping_p;
typedef avdecc_entity_model_audio_mapping_t const* avdecc_entity_model_audio_mapping_cp;

typedef struct avdecc_entity_model_stream_info_s
{
	avdecc_entity_stream_info_flags_t stream_info_flags;
	avdecc_entity_model_stream_format_t stream_format;
	avdecc_unique_identifier_t stream_id;
	unsigned int msrp_accumulated_latency;
	avdecc_mac_address_t stream_dest_mac;
	avdecc_entity_model_msrp_failure_code_t msrp_failure_code;
	avdecc_bridge_identifier_t msrp_failure_bridge_id;
	unsigned short stream_vlan_id;
	// Milan additions
	avdecc_bool_t stream_info_flags_ex_valid;
	avdecc_entity_stream_info_flags_ex_t stream_info_flags_ex;
	avdecc_bool_t probing_status_valid;
	avdecc_entity_model_probing_status_t probing_status;
	avdecc_bool_t acmp_status_valid;
	avdecc_protocol_acmp_status_t acmp_status;
} avdecc_entity_model_stream_info_t, *avdecc_entity_model_stream_info_p;
typedef avdecc_entity_model_stream_info_t const* avdecc_entity_model_stream_info_cp;

typedef struct avdecc_entity_model_msrp_mapping_s
{
	unsigned char traffic_class;
	unsigned char priority;
	unsigned short vlan_id;
} avdecc_entity_model_msrp_mapping_t, *avdecc_entity_model_msrp_mapping_p;
typedef avdecc_entity_model_msrp_mapping_t const* avdecc_entity_model_msrp_mapping_cp;

typedef struct avdecc_entity_model_avb_info_s
{
	avdecc_unique_identifier_t gptp_grandmaster_id;
	unsigned int propagation_delay;
	unsigned char gptp_domain_number;
	avdecc_entity_avb_info_flags_t flags;
	avdecc_entity_model_msrp_mapping_p* mappings; // NULL terminated list of mappings
} avdecc_entity_model_avb_info_t, *avdecc_entity_model_avb_info_p;
typedef avdecc_entity_model_avb_info_t const* avdecc_entity_model_avb_info_cp;

typedef struct avdecc_entity_model_as_path_s
{
	avdecc_unique_identifier_t** sequence; // NULL terminated list of identifiers
} avdecc_entity_model_as_path_t, *avdecc_entity_model_as_path_p;
typedef avdecc_entity_model_as_path_t const* avdecc_entity_model_as_path_cp;

typedef struct avdecc_entity_model_milan_info_s
{
	unsigned int protocol_version;
	avdecc_entity_milan_info_features_flags_t features_flags;
	unsigned int certification_version;
} avdecc_entity_model_milan_info_t, *avdecc_entity_model_milan_info_p;
typedef avdecc_entity_model_milan_info_t const* avdecc_entity_model_milan_info_cp;

typedef struct avdecc_entity_model_entity_descriptor_s
{
	avdecc_unique_identifier_t entity_id;
	avdecc_unique_identifier_t entity_model_id;
	avdecc_entity_entity_capabilities_t entity_capabilities;
	unsigned short talker_stream_sources;
	avdecc_entity_talker_capabilities_t talker_capabilities;
	unsigned short listener_stream_sinks;
	avdecc_entity_listener_capabilities_t listener_capabilities;
	avdecc_entity_controller_capabilities_t controller_capabilities;
	unsigned int available_index;
	avdecc_unique_identifier_t association_id;
	avdecc_fixed_string_t entity_name;
	avdecc_entity_model_localized_string_reference_t vendor_name_string;
	avdecc_entity_model_localized_string_reference_t model_name_string;
	avdecc_fixed_string_t firmware_version;
	avdecc_fixed_string_t group_name;
	avdecc_fixed_string_t serial_number;
	unsigned short configurations_count;
	unsigned short current_configuration;
} avdecc_entity_model_entity_descriptor_t, *avdecc_entity_model_entity_descriptor_p;
typedef avdecc_entity_model_entity_descriptor_t const* avdecc_entity_model_entity_descriptor_cp;

typedef struct avdecc_entity_model_descriptors_count_s
{
	avdecc_entity_model_descriptor_type_t descriptor_type;
	unsigned short count;
} avdecc_entity_model_descriptors_count_t, *avdecc_entity_model_descriptors_count_p;
typedef avdecc_entity_model_descriptors_count_t const* avdecc_entity_model_descriptors_count_cp;

typedef struct avdecc_entity_model_configuration_descriptor_s
{
	avdecc_fixed_string_t object_name;
	avdecc_entity_model_localized_string_reference_t localized_description;
	avdecc_entity_model_descriptors_count_p* counts; // NULL terminated list of descriptor count
} avdecc_entity_model_configuration_descriptor_t, *avdecc_entity_model_configuration_descriptor_p;
typedef avdecc_entity_model_configuration_descriptor_t const* avdecc_entity_model_configuration_descriptor_cp;

typedef struct avdecc_entity_model_audio_unit_descriptor_s
{
	avdecc_fixed_string_t object_name;
	avdecc_entity_model_localized_string_reference_t localized_description;
	avdecc_entity_model_descriptor_index_t clock_domain_index;
	unsigned short number_of_stream_input_ports;
	avdecc_entity_model_descriptor_index_t base_stream_input_port;
	unsigned short number_of_stream_output_ports;
	avdecc_entity_model_descriptor_index_t base_stream_output_port;
	unsigned short number_of_external_input_ports;
	avdecc_entity_model_descriptor_index_t base_external_input_port;
	unsigned short number_of_external_output_ports;
	avdecc_entity_model_descriptor_index_t base_external_output_port;
	unsigned short number_of_internal_input_ports;
	avdecc_entity_model_descriptor_index_t base_internal_input_port;
	unsigned short number_of_internal_output_ports;
	avdecc_entity_model_descriptor_index_t base_internal_output_port;
	unsigned short number_of_controls;
	avdecc_entity_model_descriptor_index_t base_control;
	unsigned short number_of_signal_selectors;
	avdecc_entity_model_descriptor_index_t base_signal_selector;
	unsigned short number_of_mixers;
	avdecc_entity_model_descriptor_index_t base_mixer;
	unsigned short number_of_matrices;
	avdecc_entity_model_descriptor_index_t base_matrix;
	unsigned short number_of_splitters;
	avdecc_entity_model_descriptor_index_t base_splitter;
	unsigned short number_of_combiners;
	avdecc_entity_model_descriptor_index_t base_combiner;
	unsigned short number_of_demultiplexers;
	avdecc_entity_model_descriptor_index_t base_demultiplexer;
	unsigned short number_of_multiplexers;
	avdecc_entity_model_descriptor_index_t base_multiplexer;
	unsigned short number_of_transcoders;
	avdecc_entity_model_descriptor_index_t base_transcoder;
	unsigned short number_of_control_blocks;
	avdecc_entity_model_descriptor_index_t base_control_block;
	avdecc_entity_model_sampling_rate_t current_sampling_rate;
	avdecc_entity_model_sampling_rate_t** sampling_rates; // NULL terminated list of supported sampling rates
} avdecc_entity_model_audio_unit_descriptor_t, *avdecc_entity_model_audio_unit_descriptor_p;
typedef avdecc_entity_model_audio_unit_descriptor_t const* avdecc_entity_model_audio_unit_descriptor_cp;

typedef struct avdecc_entity_model_stream_descriptor_s
{
	avdecc_fixed_string_t object_name;
	avdecc_entity_model_localized_string_reference_t localized_description;
	avdecc_entity_model_descriptor_index_t clock_domain_index;
	avdecc_entity_stream_flags_t stream_flags;
	avdecc_entity_model_stream_format_t current_format;
	avdecc_unique_identifier_t backupTalker_entity_id_0;
	unsigned short backup_talker_unique_id_0;
	avdecc_unique_identifier_t backup_talker_entity_id_1;
	unsigned short backup_talker_unique_id_1;
	avdecc_unique_identifier_t backup_talker_entity_id_2;
	unsigned short backup_talker_unique_id_2;
	avdecc_unique_identifier_t backedup_talker_entity_id;
	unsigned short backedup_talker_unique;
	avdecc_entity_model_descriptor_index_t avb_interface_index;
	unsigned int buffer_length;
	avdecc_entity_model_stream_format_t** formats; // NULL terminated list of supported stream formats
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
	avdecc_entity_model_descriptor_index_t** redundant_streams; // NULL terminated list of redundant stream indexes
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
} avdecc_entity_model_stream_descriptor_t, *avdecc_entity_model_stream_descriptor_p;
typedef avdecc_entity_model_stream_descriptor_t const* avdecc_entity_model_stream_descriptor_cp;

typedef struct avdecc_entity_model_jack_descriptor_s
{
	avdecc_fixed_string_t object_name;
	avdecc_entity_model_localized_string_reference_t localized_description;
	avdecc_entity_jack_flags_t jack_flags;
	avdecc_entity_model_jack_type_t jack_type;
	unsigned short number_of_controls;
	avdecc_entity_model_descriptor_index_t base_control;
} avdecc_entity_model_jack_descriptor_t, *avdecc_entity_model_jack_descriptor_p;
typedef avdecc_entity_model_jack_descriptor_t const* avdecc_entity_model_jack_descriptor_cp;

typedef struct avdecc_entity_model_avb_interface_descriptor_s
{
	avdecc_fixed_string_t object_name;
	avdecc_entity_model_localized_string_reference_t localized_description;
	avdecc_mac_address_t mac_address;
	avdecc_entity_avb_interface_flags_t interface_flags;
	avdecc_unique_identifier_t clock_identity;
	unsigned char priority1;
	unsigned char clock_class;
	unsigned short offset_scaled_log_variance;
	unsigned char clock_accuracy;
	unsigned char priority2;
	unsigned char domain_number;
	unsigned char log_sync_interval;
	unsigned char log_announce_interval;
	unsigned char log_p_delay_interval;
	unsigned short port_number;
} avdecc_entity_model_avb_interface_descriptor_t, *avdecc_entity_model_avb_interface_descriptor_p;
typedef avdecc_entity_model_avb_interface_descriptor_t const* avdecc_entity_model_avb_interface_descriptor_cp;

typedef struct avdecc_entity_model_clock_source_descriptor_s
{
	avdecc_fixed_string_t object_name;
	avdecc_entity_model_localized_string_reference_t localized_description;
	avdecc_entity_clock_source_flags_t clock_source_flags;
	avdecc_entity_model_clock_source_type_t clock_source_type;
	avdecc_unique_identifier_t clock_source_identifier;
	avdecc_entity_model_descriptor_type_t clock_source_location_type;
	avdecc_entity_model_descriptor_index_t clock_source_location_index;
} avdecc_entity_model_clock_source_descriptor_t, *avdecc_entity_model_clock_source_descriptor_p;
typedef avdecc_entity_model_clock_source_descriptor_t const* avdecc_entity_model_clock_source_descriptor_cp;

typedef struct avdecc_entity_model_memory_object_descriptor_s
{
	avdecc_fixed_string_t object_name;
	avdecc_entity_model_localized_string_reference_t localized_description;
	avdecc_entity_model_memory_object_type_t memory_object_type;
	avdecc_entity_model_descriptor_type_t target_descriptor_type;
	avdecc_entity_model_descriptor_index_t target_descriptor_index;
	unsigned long long start_address;
	unsigned long long maximum_length;
	unsigned long long length;
} avdecc_entity_model_memory_object_descriptor_t, *avdecc_entity_model_memory_object_descriptor_p;
typedef avdecc_entity_model_memory_object_descriptor_t const* avdecc_entity_model_memory_object_descriptor_cp;

typedef struct avdecc_entity_model_locale_descriptor_s
{
	avdecc_fixed_string_t locale_id;
	unsigned short number_of_string_descriptors;
	avdecc_entity_model_descriptor_index_t base_string_descriptor_index;
} avdecc_entity_model_locale_descriptor_t, *avdecc_entity_model_locale_descriptor_p;
typedef avdecc_entity_model_locale_descriptor_t const* avdecc_entity_model_locale_descriptor_cp;

typedef struct avdecc_entity_model_strings_descriptor_s
{
	avdecc_fixed_string_t strings[7];
} avdecc_entity_model_strings_descriptor_t, *avdecc_entity_model_strings_descriptor_p;
typedef avdecc_entity_model_strings_descriptor_t const* avdecc_entity_model_strings_descriptor_cp;

typedef struct avdecc_entity_model_stream_port_descriptor_s
{
	avdecc_entity_model_descriptor_index_t clock_domain_index;
	avdecc_entity_port_flags_t port_flags;
	unsigned short number_of_controls;
	avdecc_entity_model_descriptor_index_t base_control;
	unsigned short number_of_clusters;
	avdecc_entity_model_descriptor_index_t base_cluster;
	unsigned short number_of_maps;
	avdecc_entity_model_descriptor_index_t base_map;
} avdecc_entity_model_stream_port_descriptor_t, *avdecc_entity_model_stream_port_descriptor_p;
typedef avdecc_entity_model_stream_port_descriptor_t const* avdecc_entity_model_stream_port_descriptor_cp;

typedef struct avdecc_entity_model_external_port_descriptor_s
{
	avdecc_entity_model_descriptor_index_t clock_domain_index;
	avdecc_entity_port_flags_t port_flags;
	unsigned short number_of_controls;
	avdecc_entity_model_descriptor_index_t base_control;
	avdecc_entity_model_descriptor_type_t signal_type;
	avdecc_entity_model_descriptor_index_t signal_index;
	unsigned short signal_output;
	unsigned int block_latency;
	avdecc_entity_model_descriptor_index_t jack_index;
} avdecc_entity_model_external_port_descriptor_t, *avdecc_entity_model_external_port_descriptor_p;
typedef avdecc_entity_model_external_port_descriptor_t const* avdecc_entity_model_external_port_descriptor_cp;

typedef struct avdecc_entity_model_internal_port_descriptor_s
{
	avdecc_entity_model_descriptor_index_t clock_domain_index;
	avdecc_entity_port_flags_t port_flags;
	unsigned short number_of_controls;
	avdecc_entity_model_descriptor_index_t base_control;
	avdecc_entity_model_descriptor_type_t signal_type;
	avdecc_entity_model_descriptor_index_t signal_index;
	unsigned short signal_output;
	unsigned int block_latency;
	avdecc_entity_model_descriptor_index_t internal_index;
} avdecc_entity_model_internal_port_descriptor_t, *avdecc_entity_model_internal_port_descriptor_p;
typedef avdecc_entity_model_internal_port_descriptor_t const* avdecc_entity_model_internal_port_descriptor_cp;

typedef struct avdecc_entity_model_audio_cluster_descriptor_s
{
	avdecc_fixed_string_t object_name;
	avdecc_entity_model_localized_string_reference_t localized_description;
	avdecc_entity_model_descriptor_type_t signal_type;
	avdecc_entity_model_descriptor_index_t signal_index;
	unsigned short signal_output;
	unsigned int path_latency;
	unsigned int block_latency;
	unsigned short channel_count;
	avdecc_entity_model_audio_cluster_format_t format;
} avdecc_entity_model_audio_cluster_descriptor_t, *avdecc_entity_model_audio_cluster_descriptor_p;
typedef avdecc_entity_model_audio_cluster_descriptor_t const* avdecc_entity_model_audio_cluster_descriptor_cp;

typedef struct avdecc_entity_model_audio_map_descriptor_s
{
	avdecc_entity_model_audio_mapping_p* mappings; // NULL terminated list of mappings
} avdecc_entity_model_audio_map_descriptor_t, *avdecc_entity_model_audio_map_descriptor_p;
typedef avdecc_entity_model_audio_map_descriptor_t const* avdecc_entity_model_audio_map_descriptor_cp;

typedef struct avdecc_entity_model_clock_domain_descriptor_s
{
	avdecc_fixed_string_t object_name;
	avdecc_entity_model_localized_string_reference_t localized_description;
	avdecc_entity_model_descriptor_index_t clock_source_index;
	avdecc_entity_model_descriptor_index_t** clock_sources; // NULL terminated list of clock sources
} avdecc_entity_model_clock_domain_descriptor_t, *avdecc_entity_model_clock_domain_descriptor_p;
typedef avdecc_entity_model_clock_domain_descriptor_t const* avdecc_entity_model_clock_domain_descriptor_cp;

/* ************************************************************************** */
/* LocalEntity                                                                */
/* ************************************************************************** */

// Delegate
typedef struct avdecc_local_entity_controller_delegate_s
{
	/* **** Global notifications **** */
	/** Called when a fatal error on the transport layer occured. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onTransportError)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/);

	/* Discovery Protocol (ADP) */
	/** Called when a new entity was discovered on the network (either local or remote). */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onEntityOnline)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_cp const /*entity*/);
	/** Called when an already discovered entity updated its discovery (ADP) information. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onEntityUpdate)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_cp const /*entity*/);
	/** Called when an already discovered entity went offline or timed out (either local or remote). */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onEntityOffline)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/);

	/* Connection Management Protocol sniffed messages (ACMP) (not triggered for our own commands even though ACMP messages are broadcasted, the command's 'result' method will be called in that case) */
	/** Called when a controller connect request has been sniffed on the network. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onControllerConnectResponseSniffed)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_entity_model_stream_identification_cp const /*talkerStream*/, avdecc_entity_model_stream_identification_cp const /*listenerStream*/, unsigned short const /*connectionCount*/, avdecc_entity_connection_flags_t const /*flags*/, avdecc_local_entity_control_status_t const /*status*/);
	/** Called when a controller disconnect request has been sniffed on the network. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onControllerDisconnectResponseSniffed)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_entity_model_stream_identification_cp const /*talkerStream*/, avdecc_entity_model_stream_identification_cp const /*listenerStream*/, unsigned short const /*connectionCount*/, avdecc_entity_connection_flags_t const /*flags*/, avdecc_local_entity_control_status_t const /*status*/);
	/** Called when a listener connect request has been sniffed on the network (either due to a another controller connect, or a fast connect). */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onListenerConnectResponseSniffed)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_entity_model_stream_identification_cp const /*talkerStream*/, avdecc_entity_model_stream_identification_cp const /*listenerStream*/, unsigned short const /*connectionCount*/, avdecc_entity_connection_flags_t const /*flags*/, avdecc_local_entity_control_status_t const /*status*/);
	/** Called when a listener disconnect request has been sniffed on the network (either due to a another controller disconnect, or a fast disconnect). */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onListenerDisconnectResponseSniffed)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_entity_model_stream_identification_cp const /*talkerStream*/, avdecc_entity_model_stream_identification_cp const /*listenerStream*/, unsigned short const /*connectionCount*/, avdecc_entity_connection_flags_t const /*flags*/, avdecc_local_entity_control_status_t const /*status*/);
	/** Called when a stream state query has been sniffed on the network. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onGetTalkerStreamStateResponseSniffed)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_entity_model_stream_identification_cp const /*talkerStream*/, avdecc_entity_model_stream_identification_cp const /*listenerStream*/, unsigned short const /*connectionCount*/, avdecc_entity_connection_flags_t const /*flags*/, avdecc_local_entity_control_status_t const /*status*/);
	/** Called when a stream state query has been sniffed on the network */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onGetListenerStreamStateResponseSniffed)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_entity_model_stream_identification_cp const /*talkerStream*/, avdecc_entity_model_stream_identification_cp const /*listenerStream*/, unsigned short const /*connectionCount*/, avdecc_entity_connection_flags_t const /*flags*/, avdecc_local_entity_control_status_t const /*status*/);

	/* Unsolicited notifications (not triggered for our own commands, the command's 'result' method will be called in that case). Only successfull commands can cause an unsolicited notification. */
	/** Called when an entity has been deregistered from unsolicited notifications. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onDeregisteredFromUnsolicitedNotifications)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/);
	/** Called when an entity has been acquired by another controller. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onEntityAcquired)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_unique_identifier_t const /*owningEntity*/, avdecc_entity_model_descriptor_type_t const /*descriptorType*/, avdecc_entity_model_descriptor_index_t const /*descriptorIndex*/);
	/** Called when an entity has been released by another controller. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onEntityReleased)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_unique_identifier_t const /*owningEntity*/, avdecc_entity_model_descriptor_type_t const /*descriptorType*/, avdecc_entity_model_descriptor_index_t const /*descriptorIndex*/);
	/** Called when an entity has been locked by another controller. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onEntityLocked)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_unique_identifier_t const /*lockingEntity*/, avdecc_entity_model_descriptor_type_t const /*descriptorType*/, avdecc_entity_model_descriptor_index_t const /*descriptorIndex*/);
	/** Called when an entity has been unlocked by another controller (or because of the lock timeout). */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onEntityUnlocked)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_unique_identifier_t const /*lockingEntity*/, avdecc_entity_model_descriptor_type_t const /*descriptorType*/, avdecc_entity_model_descriptor_index_t const /*descriptorIndex*/);
	/** Called when the current configuration was changed by another controller. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onConfigurationChanged)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/);
	/** Called when the format of an input stream was changed by another controller. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onStreamInputFormatChanged)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_model_descriptor_index_t const /*streamIndex*/, avdecc_entity_model_stream_format_t const /*streamFormat*/);
	/** Called when the format of an output stream was changed by another controller. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onStreamOutputFormatChanged)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_model_descriptor_index_t const /*streamIndex*/, avdecc_entity_model_stream_format_t const /*streamFormat*/);
	/** Called when the audio mappings of a stream port input was changed by another controller. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onStreamPortInputAudioMappingsChanged)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_model_descriptor_index_t const /*streamPortIndex*/, avdecc_entity_model_descriptor_index_t const /*numberOfMaps*/, avdecc_entity_model_descriptor_index_t const /*mapIndex*/, avdecc_entity_model_audio_mapping_cp const* const /*mappings*/);
	/** Called when the audio mappings of a stream port output was changed by another controller. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onStreamPortOutputAudioMappingsChanged)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_model_descriptor_index_t const /*streamPortIndex*/, avdecc_entity_model_descriptor_index_t const /*numberOfMaps*/, avdecc_entity_model_descriptor_index_t const /*mapIndex*/, avdecc_entity_model_audio_mapping_cp const* const /*mappings*/);
	/** Called when the information of an input stream was changed by another controller. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onStreamInputInfoChanged)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_model_descriptor_index_t const /*streamIndex*/, avdecc_entity_model_stream_info_cp const /*info*/, avdecc_bool_t const /*fromGetStreamInfoResponse*/);
	/** Called when the information of an output stream was changed by another controller. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onStreamOutputInfoChanged)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_model_descriptor_index_t const /*streamIndex*/, avdecc_entity_model_stream_info_cp const /*info*/, avdecc_bool_t const /*fromGetStreamInfoResponse*/);
	/** Called when the entity's name was changed by another controller. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onEntityNameChanged)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_fixed_string_t const /*entityName*/);
	/** Called when the entity's group name was changed by another controller. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onEntityGroupNameChanged)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_fixed_string_t const /*entityGroupName*/);
	/** Called when a configuration name was changed by another controller. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onConfigurationNameChanged)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_fixed_string_t const /*configurationName*/);
	/** Called when an audio unit name was changed by another controller. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onAudioUnitNameChanged)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*audioUnitIndex*/, avdecc_fixed_string_t const /*audioUnitName*/);
	/** Called when an input stream name was changed by another controller. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onStreamInputNameChanged)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*streamIndex*/, avdecc_fixed_string_t const /*streamName*/);
	/** Called when an output stream name was changed by another controller. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onStreamOutputNameChanged)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*streamIndex*/, avdecc_fixed_string_t const /*streamName*/);
	/** Called when an avb interface name was changed by another controller. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onAvbInterfaceNameChanged)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*avbInterfaceIndex*/, avdecc_fixed_string_t const /*avbInterfaceName*/);
	/** Called when a clock source name was changed by another controller. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onClockSourceNameChanged)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*clockSourceIndex*/, avdecc_fixed_string_t const /*clockSourceName*/);
	/** Called when a memory object name was changed by another controller. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onMemoryObjectNameChanged)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*memoryObjectIndex*/, avdecc_fixed_string_t const /*memoryObjectName*/);
	/** Called when an audio cluster name was changed by another controller. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onAudioClusterNameChanged)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*audioClusterIndex*/, avdecc_fixed_string_t const /*audioClusterName*/);
	/** Called when a clock domain name was changed by another controller. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onClockDomainNameChanged)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*clockDomainIndex*/, avdecc_fixed_string_t const /*clockDomainName*/);
	/** Called when an AudioUnit sampling rate was changed by another controller. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onAudioUnitSamplingRateChanged)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_model_descriptor_index_t const /*audioUnitIndex*/, avdecc_entity_model_sampling_rate_t const /*samplingRate*/);
	/** Called when a VideoCluster sampling rate was changed by another controller. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onVideoClusterSamplingRateChanged)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_model_descriptor_index_t const /*videoClusterIndex*/, avdecc_entity_model_sampling_rate_t const /*samplingRate*/);
	/** Called when a SensorCluster sampling rate was changed by another controller. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onSensorClusterSamplingRateChanged)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_model_descriptor_index_t const /*sensorClusterIndex*/, avdecc_entity_model_sampling_rate_t const /*samplingRate*/);
	/** Called when a clock source was changed by another controller. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onClockSourceChanged)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_model_descriptor_index_t const /*clockDomainIndex*/, avdecc_entity_model_descriptor_index_t const /*clockSourceIndex*/);
	/** Called when an input stream was started by another controller. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onStreamInputStarted)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_model_descriptor_index_t const /*streamIndex*/);
	/** Called when an output stream was started by another controller. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onStreamOutputStarted)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_model_descriptor_index_t const /*streamIndex*/);
	/** Called when an input stream was stopped by another controller. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onStreamInputStopped)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_model_descriptor_index_t const /*streamIndex*/);
	/** Called when an output stream was stopped by another controller. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onStreamOutputStopped)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_model_descriptor_index_t const /*streamIndex*/);
	/** Called when the Avb Info of an Avb Interface changed. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onAvbInfoChanged)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_model_descriptor_index_t const /*avbInterfaceIndex*/, avdecc_entity_model_avb_info_cp const /*info*/);
	/** Called when the AS Path of an Avb Interface changed. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onAsPathChanged)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_model_descriptor_index_t const /*avbInterfaceIndex*/, avdecc_entity_model_as_path_cp const /*asPath*/);
	/** Called when the counters of Entity changed. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onEntityCountersChanged)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_entity_counter_valid_flags_t const /*validCounters*/, avdecc_entity_model_descriptor_counters_t const /*counters*/);
	/** Called when the counters of an Avb Interface changed. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onAvbInterfaceCountersChanged)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_model_descriptor_index_t const /*avbInterfaceIndex*/, avdecc_entity_avb_interface_counter_valid_flags_t const /*validCounters*/, avdecc_entity_model_descriptor_counters_t const /*counters*/);
	/** Called when the counters of a Clock Domain changed. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onClockDomainCountersChanged)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_model_descriptor_index_t const /*clockDomainIndex*/, avdecc_entity_clock_domain_counter_valid_flags_t const /*validCounters*/, avdecc_entity_model_descriptor_counters_t const /*counters*/);
	/** Called when the counters of a Stream Input changed. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onStreamInputCountersChanged)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_model_descriptor_index_t const /*streamIndex*/, avdecc_entity_stream_input_counter_valid_flags_t const /*validCounters*/, avdecc_entity_model_descriptor_counters_t const /*counters*/);
	/** Called when the counters of a Stream Output changed. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onStreamOutputCountersChanged)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_model_descriptor_index_t const /*streamIndex*/, avdecc_entity_stream_output_counter_valid_flags_t const /*validCounters*/, avdecc_entity_model_descriptor_counters_t const /*counters*/);
	/** Called when (some or all) audio mappings of a stream port input were added by another controller. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onStreamPortInputAudioMappingsAdded)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_model_descriptor_index_t const /*streamPortIndex*/, avdecc_entity_model_audio_mapping_cp const* const /*mappings*/);
	/** Called when (some or all) audio mappings of a stream port output were added by another controller. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onStreamPortOutputAudioMappingsAdded)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_model_descriptor_index_t const /*streamPortIndex*/, avdecc_entity_model_audio_mapping_cp const* const /*mappings*/);
	/** Called when (some or all) audio mappings of a stream port input were removed by another controller. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onStreamPortInputAudioMappingsRemoved)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_model_descriptor_index_t const /*streamPortIndex*/, avdecc_entity_model_audio_mapping_cp const* const /*mappings*/);
	/** Called when (some or all) audio mappings of a stream port output were removed by another controller. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onStreamPortOutputAudioMappingsRemoved)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_model_descriptor_index_t const /*streamPortIndex*/, avdecc_entity_model_audio_mapping_cp const* const /*mappings*/);
	/** Called when the length of a MemoryObject changed. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onMemoryObjectLengthChanged)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*memoryObjectIndex*/, unsigned long long const /*length*/);
	/** Called when there is a status update on an ongoing Operation */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onOperationStatus)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_model_descriptor_type_t const /*descriptorType*/, avdecc_entity_model_descriptor_index_t const /*descriptorIndex*/, avdecc_entity_model_operation_id_t const /*operationID*/, unsigned short const /*percentComplete*/);

	/* Identification notifications */
	/** Called when an entity emits an identify notification. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onEntityIdentifyNotification)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/);

	/* **** Statistics **** */
	/** Notification for when an AECP Command was resent due to a timeout. If the retry time out again, then onAecpTimeout will be called. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onAecpRetry)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/);
	/** Notification for when an AECP Command timed out (not called when onAecpRetry is called). */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onAecpTimeout)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/);
	/** Notification for when an AECP Response is received but is not expected (might have already timed out). */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onAecpUnexpectedResponse)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/);
	/** Notification for when an AECP Response is received (not an Unsolicited one) along with the time elapsed between the send and the receive. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onAecpResponseTime)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, unsigned long long /*responseTimeMsec*/);
	/** Notification for when an AEM-AECP Unsolicited Response was received. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onAemAecpUnsolicitedReceived)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_protocol_aecp_sequence_id_t const /*sequenceID*/);
} avdecc_local_entity_controller_delegate_t, *avdecc_local_entity_controller_delegate_p;

/* Enumeration and Control Protocol (AECP) AEM handlers */
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_acquire_entity_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_unique_identifier_t const /*owningEntity*/, avdecc_entity_model_descriptor_type_t const /*descriptorType*/, avdecc_entity_model_descriptor_index_t const /*descriptorIndex*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_release_entity_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_unique_identifier_t const /*owningEntity*/, avdecc_entity_model_descriptor_type_t const /*descriptorType*/, avdecc_entity_model_descriptor_index_t const /*descriptorIndex*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_lock_entity_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_unique_identifier_t const /*lockingEntity*/, avdecc_entity_model_descriptor_type_t const /*descriptorType*/, avdecc_entity_model_descriptor_index_t const /*descriptorIndex*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_unlock_entity_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_unique_identifier_t const /*lockingEntity*/, avdecc_entity_model_descriptor_type_t const /*descriptorType*/, avdecc_entity_model_descriptor_index_t const /*descriptorIndex*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_query_entity_available_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_query_controller_available_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_register_unsolicited_notifications_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_unregister_unsolicited_notifications_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_read_entity_descriptor_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_entity_descriptor_cp const /*descriptor*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_read_configuration_descriptor_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_configuration_descriptor_cp const /*descriptor*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_read_audio_unit_descriptor_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*audioUnitIndex*/, avdecc_entity_model_audio_unit_descriptor_cp const /*descriptor*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_read_stream_input_descriptor_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*streamIndex*/, avdecc_entity_model_stream_descriptor_cp const /*descriptor*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_read_stream_output_descriptor_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*streamIndex*/, avdecc_entity_model_stream_descriptor_cp const /*descriptor*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_read_jack_input_descriptor_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*jackIndex*/, avdecc_entity_model_jack_descriptor_cp const /*descriptor*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_read_jack_output_descriptor_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*jackIndex*/, avdecc_entity_model_jack_descriptor_cp const /*descriptor*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_read_avb_interface_descriptor_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*avbInterfaceIndex*/, avdecc_entity_model_avb_interface_descriptor_cp const /*descriptor*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_read_clock_source_descriptor_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*clockSourceIndex*/, avdecc_entity_model_clock_source_descriptor_cp const /*descriptor*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_read_memory_object_descriptor_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*memoryObjectIndex*/, avdecc_entity_model_memory_object_descriptor_cp const /*descriptor*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_read_locale_descriptor_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*localeIndex*/, avdecc_entity_model_locale_descriptor_cp const /*descriptor*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_read_strings_descriptor_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*stringsIndex*/, avdecc_entity_model_strings_descriptor_cp const /*descriptor*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_read_stream_port_input_descriptor_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*streamPortIndex*/, avdecc_entity_model_stream_port_descriptor_cp const /*descriptor*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_read_stream_port_output_descriptor_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*streamPortIndex*/, avdecc_entity_model_stream_port_descriptor_cp const /*descriptor*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_read_external_port_input_descriptor_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*externalPortIndex*/, avdecc_entity_model_external_port_descriptor_cp const /*descriptor*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_read_external_port_output_descriptor_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*externalPortIndex*/, avdecc_entity_model_external_port_descriptor_cp const /*descriptor*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_read_internal_port_input_descriptor_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*internalPortIndex*/, avdecc_entity_model_internal_port_descriptor_cp const /*descriptor*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_read_internal_port_output_descriptor_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*internalPortIndex*/, avdecc_entity_model_internal_port_descriptor_cp const /*descriptor*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_read_audio_cluster_descriptor_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*clusterIndex*/, avdecc_entity_model_audio_cluster_descriptor_cp const /*descriptor*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_read_audio_map_descriptor_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*mapIndex*/, avdecc_entity_model_audio_map_descriptor_cp const /*descriptor*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_read_clock_domain_descriptor_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*clockDomainIndex*/, avdecc_entity_model_clock_domain_descriptor_cp const /*descriptor*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_set_configuration_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_get_configuration_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_set_stream_input_format_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*streamIndex*/, avdecc_entity_model_stream_format_t const /*streamFormat*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_get_stream_input_format_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*streamIndex*/, avdecc_entity_model_stream_format_t const /*streamFormat*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_set_stream_output_format_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*streamIndex*/, avdecc_entity_model_stream_format_t const /*streamFormat*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_get_stream_output_format_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*streamIndex*/, avdecc_entity_model_stream_format_t const /*streamFormat*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_get_stream_port_input_audio_map_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*streamPortIndex*/, avdecc_entity_model_descriptor_index_t const /*numberOfMaps*/, avdecc_entity_model_descriptor_index_t const /*mapIndex*/, avdecc_entity_model_audio_mapping_cp const* const /*mappings*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_get_stream_port_output_audio_map_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*streamPortIndex*/, avdecc_entity_model_descriptor_index_t const /*numberOfMaps*/, avdecc_entity_model_descriptor_index_t const /*mapIndex*/, avdecc_entity_model_audio_mapping_cp const* const /*mappings*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_add_stream_port_input_audio_mappings_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*streamPortIndex*/, avdecc_entity_model_audio_mapping_cp const* const /*mappings*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_add_stream_port_output_audio_mappings_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*streamPortIndex*/, avdecc_entity_model_audio_mapping_cp const* const /*mappings*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_remove_stream_port_input_audio_mappings_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*streamPortIndex*/, avdecc_entity_model_audio_mapping_cp const* const /*mappings*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_remove_stream_port_output_audio_mappings_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*streamPortIndex*/, avdecc_entity_model_audio_mapping_cp const* const /*mappings*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_set_stream_input_info_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*streamIndex*/, avdecc_entity_model_stream_info_cp const /*info*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_set_stream_output_info_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*streamIndex*/, avdecc_entity_model_stream_info_cp const /*info*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_get_stream_input_info_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*streamIndex*/, avdecc_entity_model_stream_info_cp const /*info*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_get_stream_output_info_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*streamIndex*/, avdecc_entity_model_stream_info_cp const /*info*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_set_entity_name_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_fixed_string_t const /*entityName*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_get_entity_name_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_fixed_string_t const /*entityName*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_set_entity_group_name_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_fixed_string_t const /*entityGroupName*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_get_entity_group_name_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_fixed_string_t const /*entityGroupName*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_set_configuration_name_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_fixed_string_t const /*configurationName*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_get_configuration_name_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_fixed_string_t const /*configurationName*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_set_audio_unit_name_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*audioUnitIndex*/, avdecc_fixed_string_t const /*audioUnitName*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_get_audio_unit_name_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*audioUnitIndex*/, avdecc_fixed_string_t const /*audioUnitName*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_set_stream_input_name_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*streamIndex*/, avdecc_fixed_string_t const /*streamInputName*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_get_stream_input_name_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*streamIndex*/, avdecc_fixed_string_t const /*streamInputName*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_set_stream_output_name_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*streamIndex*/, avdecc_fixed_string_t const /*streamOutputName*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_get_stream_output_name_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*streamIndex*/, avdecc_fixed_string_t const /*streamOutputName*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_set_avb_interface_name_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*avbInterfaceIndex*/, avdecc_fixed_string_t const /*avbInterfaceName*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_get_avb_interface_name_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*avbInterfaceIndex*/, avdecc_fixed_string_t const /*avbInterfaceName*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_set_clock_source_name_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*clockSourceIndex*/, avdecc_fixed_string_t const /*clockSourceName*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_get_clock_source_name_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*clockSourceIndex*/, avdecc_fixed_string_t const /*clockSourceName*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_set_memory_object_name_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*memoryObjectIndex*/, avdecc_fixed_string_t const /*memoryObjectName*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_get_memory_object_name_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*memoryObjectIndex*/, avdecc_fixed_string_t const /*memoryObjectName*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_set_audio_cluster_name_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*audioClusterIndex*/, avdecc_fixed_string_t const /*audioClusterName*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_get_audio_cluster_name_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*audioClusterIndex*/, avdecc_fixed_string_t const /*audioClusterName*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_set_clock_domain_name_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*clockDomainIndex*/, avdecc_fixed_string_t const /*clockDomainName*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_get_clock_domain_name_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*clockDomainIndex*/, avdecc_fixed_string_t const /*clockDomainName*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_set_audio_unit_sampling_rate_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*audioUnitIndex*/, avdecc_entity_model_sampling_rate_t const /*samplingRate*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_get_audio_unit_sampling_rate_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*audioUnitIndex*/, avdecc_entity_model_sampling_rate_t const /*samplingRate*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_set_video_cluster_sampling_rate_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*videoClusterIndex*/, avdecc_entity_model_sampling_rate_t const /*samplingRate*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_get_video_cluster_sampling_rate_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*videoClusterIndex*/, avdecc_entity_model_sampling_rate_t const /*samplingRate*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_set_sensor_cluster_sampling_rate_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*sensorClusterIndex*/, avdecc_entity_model_sampling_rate_t const /*samplingRate*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_get_sensor_cluster_sampling_rate_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*sensorClusterIndex*/, avdecc_entity_model_sampling_rate_t const /*samplingRate*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_set_clock_source_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*clockDomainIndex*/, avdecc_entity_model_descriptor_index_t const /*clockSourceIndex*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_get_clock_source_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*clockDomainIndex*/, avdecc_entity_model_descriptor_index_t const /*clockSourceIndex*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_start_stream_input_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*streamIndex*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_start_stream_output_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*streamIndex*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_stop_stream_input_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*streamIndex*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_stop_stream_output_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*streamIndex*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_get_avb_info_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*avbInterfaceIndex*/, avdecc_entity_model_avb_info_cp const /*info*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_get_as_path_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*avbInterfaceIndex*/, avdecc_entity_model_as_path_cp const /*asPath*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_get_entity_counters_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_entity_counter_valid_flags_t const /*validCounters*/, avdecc_entity_model_descriptor_counters_t const /*counters*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_get_avb_interface_counters_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*avbInterfaceIndex*/, avdecc_entity_avb_interface_counter_valid_flags_t const /*validCounters*/, avdecc_entity_model_descriptor_counters_t const /*counters*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_get_clock_domain_counters_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*clockDomainIndex*/, avdecc_entity_clock_domain_counter_valid_flags_t const /*validCounters*/, avdecc_entity_model_descriptor_counters_t const /*counters*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_get_stream_input_counters_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*streamIndex*/, avdecc_entity_stream_input_counter_valid_flags_t const /*validCounters*/, avdecc_entity_model_descriptor_counters_t const /*counters*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_get_stream_output_counters_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*streamIndex*/, avdecc_entity_stream_output_counter_valid_flags_t const /*validCounters*/, avdecc_entity_model_descriptor_counters_t const /*counters*/);
//using StartOperationHandler)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex, la::avdecc::entity::model::OperationID const operationID, la::avdecc::entity::model::MemoryObjectOperationType const operationType, la::avdecc::MemoryBuffer const& memoryBuffer);
//using AbortOperationHandler)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, la::avdecc::entity::model::DescriptorType const descriptorType, la::avdecc::entity::model::DescriptorIndex const descriptorIndex, la::avdecc::entity::model::OperationID const operationID);
//using SetMemoryObjectLengthHandler)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*memoryObjectIndex*/, unsigned long long const length);
//using GetMemoryObjectLengthHandler)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const /*status*/, avdecc_entity_model_descriptor_index_t const /*configurationIndex*/, avdecc_entity_model_descriptor_index_t const /*memoryObjectIndex*/, unsigned long long const length);
///* Enumeration and Control Protocol (AECP) AA handlers */
//typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_address_access_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aa_command_status_t const /*status*/, avdecc_entity_address_access_tlvs_cp const /*tlvs*/);
///* Enumeration and Control Protocol (AECP) MVU handlers (Milan Vendor Unique) */
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_get_milan_info_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_mvu_command_status_t const /*status*/, avdecc_entity_model_milan_info_cp const /*info*/);
///* Connection Management Protocol (ACMP) handlers */
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_connect_stream_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_entity_model_stream_identification_cp const /*talkerStream*/, avdecc_entity_model_stream_identification_cp const /*listenerStream*/, unsigned short const /*connectionCount*/, avdecc_entity_connection_flags_t const /*flags*/, avdecc_local_entity_control_status_t const /*status*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_disconnect_stream_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_entity_model_stream_identification_cp const /*talkerStream*/, avdecc_entity_model_stream_identification_cp const /*listenerStream*/, unsigned short const /*connectionCount*/, avdecc_entity_connection_flags_t const /*flags*/, avdecc_local_entity_control_status_t const /*status*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_disconnect_talker_stream_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_entity_model_stream_identification_cp const /*talkerStream*/, avdecc_entity_model_stream_identification_cp const /*listenerStream*/, unsigned short const /*connectionCount*/, avdecc_entity_connection_flags_t const /*flags*/, avdecc_local_entity_control_status_t const /*status*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_get_talker_stream_state_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_entity_model_stream_identification_cp const /*talkerStream*/, avdecc_entity_model_stream_identification_cp const /*listenerStream*/, unsigned short const /*connectionCount*/, avdecc_entity_connection_flags_t const /*flags*/, avdecc_local_entity_control_status_t const /*status*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_get_listener_stream_state_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_entity_model_stream_identification_cp const /*talkerStream*/, avdecc_entity_model_stream_identification_cp const /*listenerStream*/, unsigned short const /*connectionCount*/, avdecc_entity_connection_flags_t const /*flags*/, avdecc_local_entity_control_status_t const /*status*/);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_local_entity_get_talker_stream_connection_cb)(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_entity_model_stream_identification_cp const /*talkerStream*/, avdecc_entity_model_stream_identification_cp const /*listenerStream*/, unsigned short const /*connectionCount*/, avdecc_entity_connection_flags_t const /*flags*/, avdecc_local_entity_control_status_t const /*status*/);

// LocalEntity APIs
/** Creates a new LocalEntity attached to the specified ProtocolInterfaceHandle. avdecc_local_entity_error_no_error is returned in case of success and createdLocalEntityHandle is initialized with the newly created LocalEntityHandle. LA_AVDECC_LocalEntity_destroy must be called when the LocalEntity is no longer in use. */
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_create(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, avdecc_entity_cp const entity, avdecc_local_entity_controller_delegate_p const delegate, LA_AVDECC_LOCAL_ENTITY_HANDLE* const createdLocalEntityHandle);
/** Destroys a previously created LocalEntity. */
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_destroy(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle);
/** Enables entity advertising with available duration included between 2-62 seconds. */
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_enableEntityAdvertising(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, unsigned int const availableDuration);
/** Disables entity advertising. */
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_disableEntityAdvertising(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle);
/** Requests a remote entities discovery. */
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_discoverRemoteEntities(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle);
/** Requests a targetted remote entity discovery. */
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_discoverRemoteEntity(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID);
/** Sets automatic discovery delay (in milliseconds). 0 (default) for no automatic discovery. */
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setAutomaticDiscoveryDelay(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, unsigned int const millisecondsDelay);

/* Enumeration and Control Protocol (AECP) AEM */
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_acquireEntity(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_bool_t const isPersistent, avdecc_entity_model_descriptor_type_t const descriptorType, avdecc_entity_model_descriptor_index_t const descriptorIndex, avdecc_local_entity_acquire_entity_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_releaseEntity(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const descriptorType, avdecc_entity_model_descriptor_index_t const descriptorIndex, avdecc_local_entity_release_entity_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_lockEntity(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const descriptorType, avdecc_entity_model_descriptor_index_t const descriptorIndex, avdecc_local_entity_lock_entity_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_unlockEntity(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const descriptorType, avdecc_entity_model_descriptor_index_t const descriptorIndex, avdecc_local_entity_unlock_entity_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_queryEntityAvailable(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_local_entity_query_entity_available_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_queryControllerAvailable(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_local_entity_query_controller_available_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_registerUnsolicitedNotifications(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_local_entity_register_unsolicited_notifications_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_unregisterUnsolicitedNotifications(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_local_entity_unregister_unsolicited_notifications_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readEntityDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_local_entity_read_entity_descriptor_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readConfigurationDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_local_entity_read_configuration_descriptor_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readAudioUnitDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const audioUnitIndex, avdecc_local_entity_read_audio_unit_descriptor_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readStreamInputDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_local_entity_read_stream_input_descriptor_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readStreamOutputDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_local_entity_read_stream_output_descriptor_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readJackInputDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const jackIndex, avdecc_local_entity_read_jack_input_descriptor_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readJackOutputDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const jackIndex, avdecc_local_entity_read_jack_output_descriptor_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readAvbInterfaceDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const avbInterfaceIndex, avdecc_local_entity_read_avb_interface_descriptor_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readClockSourceDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const clockSourceIndex, avdecc_local_entity_read_clock_source_descriptor_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readMemoryObjectDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const memoryObjectIndex, avdecc_local_entity_read_memory_object_descriptor_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readLocaleDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const localeIndex, avdecc_local_entity_read_locale_descriptor_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readStringsDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const stringsIndex, avdecc_local_entity_read_strings_descriptor_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readStreamPortInputDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const streamPortIndex, avdecc_local_entity_read_stream_port_input_descriptor_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readStreamPortOutputDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const streamPortIndex, avdecc_local_entity_read_stream_port_output_descriptor_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readExternalPortInputDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const externalPortIndex, avdecc_local_entity_read_external_port_input_descriptor_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readExternalPortOutputDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const externalPortIndex, avdecc_local_entity_read_external_port_output_descriptor_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readInternalPortInputDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const internalPortIndex, avdecc_local_entity_read_internal_port_input_descriptor_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readInternalPortOutputDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const internalPortIndex, avdecc_local_entity_read_internal_port_output_descriptor_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readAudioClusterDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const clusterIndex, avdecc_local_entity_read_audio_cluster_descriptor_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readAudioMapDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const mapIndex, avdecc_local_entity_read_audio_map_descriptor_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_readClockDomainDescriptor(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const clockDomainIndex, avdecc_local_entity_read_clock_domain_descriptor_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setConfiguration(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_local_entity_set_configuration_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getConfiguration(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_local_entity_get_configuration_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setStreamInputFormat(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_entity_model_stream_format_t const streamFormat, avdecc_local_entity_set_stream_input_format_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getStreamInputFormat(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_local_entity_get_stream_input_format_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setStreamOutputFormat(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_entity_model_stream_format_t const streamFormat, avdecc_local_entity_set_stream_output_format_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getStreamOutputFormat(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_local_entity_get_stream_output_format_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getStreamPortInputAudioMap(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamPortIndex, avdecc_entity_model_descriptor_index_t const mapIndex, avdecc_local_entity_get_stream_port_input_audio_map_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getStreamPortOutputAudioMap(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamPortIndex, avdecc_entity_model_descriptor_index_t const mapIndex, avdecc_local_entity_get_stream_port_output_audio_map_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_addStreamPortInputAudioMappings(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamPortIndex, avdecc_entity_model_audio_mapping_cp const* const mappings, avdecc_local_entity_add_stream_port_input_audio_mappings_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_addStreamPortOutputAudioMappings(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamPortIndex, avdecc_entity_model_audio_mapping_cp const* const mappings, avdecc_local_entity_add_stream_port_output_audio_mappings_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_removeStreamPortInputAudioMappings(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamPortIndex, avdecc_entity_model_audio_mapping_cp const* const mappings, avdecc_local_entity_remove_stream_port_input_audio_mappings_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_removeStreamPortOutputAudioMappings(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamPortIndex, avdecc_entity_model_audio_mapping_cp const* const mappings, avdecc_local_entity_remove_stream_port_output_audio_mappings_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setStreamInputInfo(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_entity_model_stream_info_cp const info, avdecc_local_entity_set_stream_input_info_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setStreamOutputInfo(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_entity_model_stream_info_cp const info, avdecc_local_entity_set_stream_output_info_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getStreamInputInfo(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_local_entity_get_stream_input_info_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getStreamOutputInfo(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_local_entity_get_stream_output_info_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setEntityName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_fixed_string_t const entityName, avdecc_local_entity_set_entity_name_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getEntityName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_local_entity_get_entity_name_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setEntityGroupName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_fixed_string_t const entityGroupName, avdecc_local_entity_set_entity_group_name_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getEntityGroupName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_local_entity_get_entity_group_name_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setConfigurationName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_fixed_string_t const configurationName, avdecc_local_entity_set_configuration_name_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getConfigurationName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_local_entity_get_configuration_name_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setAudioUnitName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const audioUnitIndex, avdecc_fixed_string_t const audioUnitName, avdecc_local_entity_set_audio_unit_name_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getAudioUnitName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const audioUnitIndex, avdecc_local_entity_get_audio_unit_name_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setStreamInputName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_fixed_string_t const streamInputName, avdecc_local_entity_set_stream_input_name_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getStreamInputName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_local_entity_get_stream_input_name_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setStreamOutputName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_fixed_string_t const streamOutputName, avdecc_local_entity_set_stream_output_name_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getStreamOutputName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_local_entity_get_stream_output_name_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setAvbInterfaceName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const avbInterfaceIndex, avdecc_fixed_string_t const avbInterfaceName, avdecc_local_entity_set_avb_interface_name_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getAvbInterfaceName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const avbInterfaceIndex, avdecc_local_entity_get_avb_interface_name_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setClockSourceName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const clockSourceIndex, avdecc_fixed_string_t const clockSourceName, avdecc_local_entity_set_clock_source_name_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getClockSourceName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const clockSourceIndex, avdecc_local_entity_get_clock_source_name_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setMemoryObjectName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const memoryObjectIndex, avdecc_fixed_string_t const memoryObjectName, avdecc_local_entity_set_memory_object_name_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getMemoryObjectName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const memoryObjectIndex, avdecc_local_entity_get_memory_object_name_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setAudioClusterName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const audioClusterIndex, avdecc_fixed_string_t const audioClusterName, avdecc_local_entity_set_audio_cluster_name_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getAudioClusterName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const audioClusterIndex, avdecc_local_entity_get_audio_cluster_name_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setClockDomainName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const clockDomainIndex, avdecc_fixed_string_t const clockDomainName, avdecc_local_entity_set_clock_domain_name_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getClockDomainName(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const clockDomainIndex, avdecc_local_entity_get_clock_domain_name_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setAudioUnitSamplingRate(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const audioUnitIndex, avdecc_entity_model_sampling_rate_t const samplingRate, avdecc_local_entity_set_audio_unit_sampling_rate_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getAudioUnitSamplingRate(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const audioUnitIndex, avdecc_local_entity_get_audio_unit_sampling_rate_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setVideoClusterSamplingRate(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const videoClusterIndex, avdecc_entity_model_sampling_rate_t const samplingRate, avdecc_local_entity_set_video_cluster_sampling_rate_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getVideoClusterSamplingRate(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const videoClusterIndex, avdecc_local_entity_get_video_cluster_sampling_rate_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setSensorClusterSamplingRate(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const sensorClusterIndex, avdecc_entity_model_sampling_rate_t const samplingRate, avdecc_local_entity_set_sensor_cluster_sampling_rate_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getSensorClusterSamplingRate(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const sensorClusterIndex, avdecc_local_entity_get_sensor_cluster_sampling_rate_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setClockSource(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const clockDomainIndex, avdecc_entity_model_descriptor_type_t const clockSourceIndex, avdecc_local_entity_set_clock_source_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getClockSource(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const clockDomainIndex, avdecc_local_entity_get_clock_source_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_startStreamInput(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_local_entity_start_stream_input_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_startStreamOutput(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_local_entity_start_stream_output_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_stopStreamInput(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_local_entity_stop_stream_input_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_stopStreamOutput(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_local_entity_stop_stream_output_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getAvbInfo(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const avbInterfaceIndex, avdecc_local_entity_get_avb_info_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getAsPath(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const avbInterfaceIndex, avdecc_local_entity_get_as_path_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getEntityCounters(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_local_entity_get_entity_counters_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getAvbInterfaceCounters(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const avbInterfaceIndex, avdecc_local_entity_get_avb_interface_counters_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getClockDomainCounters(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const clockDomainIndex, avdecc_local_entity_get_clock_domain_counters_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getStreamInputCounters(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_local_entity_get_stream_input_counters_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getStreamOutputCounters(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const streamIndex, avdecc_local_entity_get_stream_output_counters_cb const onResult);
//LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_startOperation(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, model::MemoryObjectOperationType const operationType, MemoryBuffer const& memoryBuffer, StartOperationHandler const onResult);
//LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_abortOperation(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, model::DescriptorType const descriptorType, model::DescriptorIndex const descriptorIndex, model::OperationID const operationID, AbortOperationHandler const onResult);
//LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_setMemoryObjectLength(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const memoryObjectIndex, unsigned long long const length, SetMemoryObjectLengthHandler const onResult);
//LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getMemoryObjectLength(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_entity_model_descriptor_type_t const configurationIndex, avdecc_entity_model_descriptor_type_t const memoryObjectIndex, GetMemoryObjectLengthHandler const onResult);

/* Enumeration and Control Protocol (AECP) AA */
//LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_addressAccess(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, addressAccess::Tlvs const& tlvs, avdecc_local_entity_address_access_cb const onResult);

/* Enumeration and Control Protocol (AECP) MVU (Milan Vendor Unique) */
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getMilanInfo(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_unique_identifier_t const entityID, avdecc_local_entity_get_milan_info_cb const onResult);

/* Connection Management Protocol (ACMP) */
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_connectStream(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_entity_model_stream_identification_cp const talkerStream, avdecc_entity_model_stream_identification_cp const listenerStream, avdecc_local_entity_connect_stream_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_disconnectStream(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_entity_model_stream_identification_cp const talkerStream, avdecc_entity_model_stream_identification_cp const listenerStream, avdecc_local_entity_disconnect_stream_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_disconnectTalkerStream(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_entity_model_stream_identification_cp const talkerStream, avdecc_entity_model_stream_identification_cp const listenerStream, avdecc_local_entity_disconnect_talker_stream_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getTalkerStreamState(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_entity_model_stream_identification_cp const talkerStream, avdecc_local_entity_get_talker_stream_state_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getListenerStreamState(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_entity_model_stream_identification_cp const listenerStream, avdecc_local_entity_get_listener_stream_state_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_local_entity_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_getTalkerStreamConnection(LA_AVDECC_LOCAL_ENTITY_HANDLE const handle, avdecc_entity_model_stream_identification_cp const talkerStream, unsigned short const connectionIndex, avdecc_local_entity_get_talker_stream_connection_cb const onResult);


/* ************************************************************************** */
/* Adpdu                                                                      */
/* ************************************************************************** */

typedef struct avdecc_protocol_adpdu_s
{
	// EtherII fields
	avdecc_mac_address_t dest_address;
	avdecc_mac_address_t src_address;
	// Adpdu fields
	avdecc_protocol_adp_message_type_t message_type;
	unsigned char valid_time;
	avdecc_unique_identifier_t entity_id;
	avdecc_unique_identifier_t entity_model_id;
	avdecc_entity_entity_capabilities_t entity_capabilities;
	unsigned short talker_stream_sources;
	avdecc_entity_talker_capabilities_t talker_capabilities;
	unsigned short listener_stream_sinks;
	avdecc_entity_listener_capabilities_t listener_capabilities;
	avdecc_entity_controller_capabilities_t controller_capabilities;
	unsigned int available_index;
	avdecc_unique_identifier_t gptp_grandmaster_id;
	unsigned char gptp_domain_number;
	avdecc_entity_model_descriptor_index_t identify_control_index;
	avdecc_entity_model_descriptor_index_t interface_index;
	avdecc_unique_identifier_t association_id;
} avdecc_protocol_adpdu_t, *avdecc_protocol_adpdu_p;
typedef avdecc_protocol_adpdu_t const* avdecc_protocol_adpdu_cp;

LA_AVDECC_BINDINGS_C_API avdecc_mac_address_t const* LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_Protocol_Adpdu_getMulticastMacAddress();


/* ************************************************************************** */
/* Aem-Aecpdu                                                                 */
/* ************************************************************************** */

typedef struct avdecc_protocol_aem_aecpdu_s
{
	// EtherII fields
	avdecc_mac_address_t dest_address;
	avdecc_mac_address_t src_address;
	// Aecpdu fields
	avdecc_protocol_aecp_message_type_t message_type;
	avdecc_protocol_aecp_status_t status;
	avdecc_unique_identifier_t target_entity_id;
	avdecc_unique_identifier_t controller_entity_id;
	avdecc_protocol_aecp_sequence_id_t sequence_id;
	// Aem fields
	avdecc_bool_t unsolicited;
	avdecc_protocol_aem_command_type_t command_type;
	unsigned short command_specific_length;
	unsigned char command_specific[524 - sizeof(avdecc_unique_identifier_t) - sizeof(avdecc_protocol_aecp_sequence_id_t) - sizeof(avdecc_protocol_aem_command_type_t)];
} avdecc_protocol_aem_aecpdu_t, *avdecc_protocol_aem_aecpdu_p;
typedef avdecc_protocol_aem_aecpdu_t const* avdecc_protocol_aem_aecpdu_cp;


/* ************************************************************************** */
/* Mvu-Aecpdu                                                                 */
/* ************************************************************************** */

typedef struct avdecc_protocol_mvu_aecpdu_s
{
	// EtherII fields
	avdecc_mac_address_t dest_address;
	avdecc_mac_address_t src_address;
	// Aecpdu fields
	avdecc_protocol_aecp_message_type_t message_type;
	avdecc_protocol_aecp_status_t status;
	avdecc_unique_identifier_t target_entity_id;
	avdecc_unique_identifier_t controller_entity_id;
	avdecc_protocol_aecp_sequence_id_t sequence_id;
	// Mvu fields
	avdecc_protocol_mvu_command_type_t command_type;
	unsigned short command_specific_length;
	unsigned char command_specific[524 - sizeof(avdecc_unique_identifier_t) - sizeof(avdecc_protocol_aecp_sequence_id_t) - sizeof(avdecc_protocol_vu_protocol_id) - sizeof(avdecc_protocol_mvu_command_type_t)];
} avdecc_protocol_mvu_aecpdu_t, *avdecc_protocol_mvu_aecpdu_p;
typedef avdecc_protocol_mvu_aecpdu_t const* avdecc_protocol_mvu_aecpdu_cp;


/* ************************************************************************** */
/* Acmpdu                                                                     */
/* ************************************************************************** */

typedef struct avdecc_protocol_acmpdu_s
{
	// EtherII fields
	avdecc_mac_address_t dest_address;
	avdecc_mac_address_t src_address;
	// Avtpdu fields
	avdecc_unique_identifier_t stream_id;
	// Acmpdu fields
	avdecc_protocol_acmp_message_type_t message_type;
	avdecc_protocol_acmp_status_t status;
	avdecc_unique_identifier_t controller_entity_id;
	avdecc_unique_identifier_t talker_entity_id;
	avdecc_unique_identifier_t listener_entity_id;
	avdecc_protocol_acmp_unique_id_t talker_unique_id;
	avdecc_protocol_acmp_unique_id_t listener_unique_id;
	avdecc_mac_address_t stream_dest_address;
	unsigned short connection_count;
	avdecc_protocol_acmp_sequence_id_t sequence_id;
	avdecc_entity_connection_flags_t flags;
	unsigned short stream_vlan_id;
} avdecc_protocol_acmpdu_t, *avdecc_protocol_acmpdu_p;
typedef avdecc_protocol_acmpdu_t const* avdecc_protocol_acmpdu_cp;

LA_AVDECC_BINDINGS_C_API avdecc_mac_address_t const* LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_Protocol_Acmpdu_getMulticastMacAddress();


/* ************************************************************************** */
/* Executor                                                                   */
/* ************************************************************************** */

// Executor APIs
/** Creates a new Queue Executor. avdecc_executor_error_no_error is returned in case of success and createdExecutorHandle is initialized with the newly created ExecutorHandle. LA_AVDECC_ProtocolInterface_destroy must be called when the ProtocolInterface is no longer in use. */
LA_AVDECC_BINDINGS_C_API avdecc_executor_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_Executor_createQueueExecutor(avdecc_const_string_t executorName, LA_AVDECC_EXECUTOR_WRAPPER_HANDLE* const createdExecutorHandle);
/** Destroys a previously created Executor. */
LA_AVDECC_BINDINGS_C_API avdecc_executor_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_Executor_destroy(LA_AVDECC_EXECUTOR_WRAPPER_HANDLE const handle);


/* ************************************************************************** */
/* ProtocolInterface                                                          */
/* ************************************************************************** */

// Observer
typedef struct avdecc_protocol_interface_observer_s
{
	/* **** Global notifications **** */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onTransportError)(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const /*handle*/);

	/* **** Discovery notifications **** */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onLocalEntityOnline)(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const /*handle*/, avdecc_entity_cp const /*entity*/);
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onLocalEntityOffline)(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/);
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onLocalEntityUpdated)(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const /*handle*/, avdecc_entity_cp const /*entity*/);
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onRemoteEntityOnline)(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const /*handle*/, avdecc_entity_cp const /*entity*/);
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onRemoteEntityOffline)(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/);
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onRemoteEntityUpdated)(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const /*handle*/, avdecc_entity_cp const /*entity*/);

	/* **** AECP notifications **** */
	/** Notification for when an AECP-AEM Command is received (for a locally registered entity). */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onAecpAemCommand)(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const /*handle*/, avdecc_protocol_aem_aecpdu_cp const /*aecpdu*/);
	/** Notification for when an unsolicited AECP-AEM Response is received (for a locally registered entity). */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onAecpAemUnsolicitedResponse)(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const /*handle*/, avdecc_protocol_aem_aecpdu_cp const /*aecpdu*/);
	/** Notification for when an identify notification is received (the notification being a multicast message, the notification is triggered even if there are no locally registered entities). */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onAecpAemIdentifyNotification)(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const /*handle*/, avdecc_protocol_aem_aecpdu_cp const /*aecpdu*/);

	/* **** ACMP notifications **** */
	/** Notification for when an ACMP Command is received, even for none of the locally registered entities. */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onAcmpCommand)(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const /*handle*/, avdecc_protocol_acmpdu_cp const /*acmpdu*/);
	/** Notification for when an ACMP Response is received, even for none of the locally registered entities and for responses already processed by the CommandStateMachine (meaning the sendAcmpCommand result handler have already been called). */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onAcmpResponse)(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const /*handle*/, avdecc_protocol_acmpdu_cp const /*acmpdu*/);

	/* **** Low level notifications (not supported by all kinds of ProtocolInterface), triggered before processing the pdu **** */
	/** Notification for when an ADPDU is received (might be a message that was sent by self as this event might be triggered for outgoing messages) */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onAdpduReceived)(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const /*handle*/, avdecc_protocol_adpdu_cp const /*adpdu*/);
	/** Notification for when an AEM-AECPDU is received (might be a message that was sent by self as this event might be triggered for outgoing messages) */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onAemAecpduReceived)(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const /*handle*/, avdecc_protocol_aem_aecpdu_cp const /*aecpdu*/);
	/** Notification for when an AA-AECPDU is received (might be a message that was sent by self as this event might be triggered for outgoing messages) */
	//void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onAaAecpduReceived)(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const /*handle*/, avdecc_protocol_aa_aecpdu_cp const /*aecpdu*/);
	/** Notification for when an MVU-AECPDU is received (might be a message that was sent by self as this event might be triggered for outgoing messages) */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onMvuAecpduReceived)(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const /*handle*/, avdecc_protocol_mvu_aecpdu_cp const /*aecpdu*/);
	/** Notification for when an ACMPDU is received (might be a message that was sent by self as this event might be triggered for outgoing messages) */
	void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* onAcmpduReceived)(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const /*handle*/, avdecc_protocol_acmpdu_cp const /*acmpdu*/);
} avdecc_protocol_interface_observer_t, *avdecc_protocol_interface_observer_p;

// Result Handlers
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_protocol_interfaces_send_aem_aecp_command_cb)(avdecc_protocol_aem_aecpdu_cp const response, avdecc_protocol_interface_error_t const error);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_protocol_interfaces_send_mvu_aecp_command_cb)(avdecc_protocol_mvu_aecpdu_cp const response, avdecc_protocol_interface_error_t const error);
typedef void(LA_AVDECC_BINDINGS_C_CALL_CONVENTION* avdecc_protocol_interfaces_send_acmp_command_cb)(avdecc_protocol_acmpdu_cp const response, avdecc_protocol_interface_error_t const error);

// Static APIs
LA_AVDECC_BINDINGS_C_API avdecc_bool_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_isSupportedProtocolInterfaceType(avdecc_protocol_interface_type_t const protocolInterfaceType);
LA_AVDECC_BINDINGS_C_API avdecc_string_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_typeToString(avdecc_protocol_interface_type_t const protocolInterfaceType);
LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_types_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_getSupportedProtocolInterfaceTypes();

// ProtocolInterface APIs
LA_AVDECC_BINDINGS_C_API avdecc_const_string_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_getDefaultExecutorName();
/** Creates a new ProtocolInterface. avdecc_protocol_interface_error_no_error is returned in case of success and createdProtocolInterfaceHandle is initialized with the newly created ProtocolInterfaceHandle. LA_AVDECC_ProtocolInterface_destroy must be called when the ProtocolInterface is no longer in use. */
LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_create(avdecc_protocol_interface_type_t const protocolInterfaceType, avdecc_const_string_t interfaceName, LA_AVDECC_PROTOCOL_INTERFACE_HANDLE* const createdProtocolInterfaceHandle);
/** Destroys a previously created ProtocolInterface. */
LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_destroy(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle);
LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_getMacAddress(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, avdecc_mac_address_t* const address);
LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_shutdown(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle);
LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_registerObserver(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, avdecc_protocol_interface_observer_p const observer);
LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_unregisterObserver(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, avdecc_protocol_interface_observer_p const observer);
LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_getDynamicEID(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, avdecc_unique_identifier_t* const generatedEntityID);
LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_releaseDynamicEID(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, avdecc_unique_identifier_t const entityID);
LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_registerLocalEntity(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, LA_AVDECC_LOCAL_ENTITY_HANDLE const localEntityHandle);
LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_unregisterLocalEntity(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, LA_AVDECC_LOCAL_ENTITY_HANDLE const localEntityHandle);
LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_enableEntityAdvertising(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, LA_AVDECC_LOCAL_ENTITY_HANDLE const localEntityHandle);
LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_disableEntityAdvertising(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, LA_AVDECC_LOCAL_ENTITY_HANDLE const localEntityHandle);
LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_setEntityNeedsAdvertise(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, LA_AVDECC_LOCAL_ENTITY_HANDLE const localEntityHandle, avdecc_local_entity_advertise_flags_t const flags);
LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_discoverRemoteEntities(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle);
LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_discoverRemoteEntity(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, avdecc_unique_identifier_t const entityID);
LA_AVDECC_BINDINGS_C_API avdecc_bool_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_isDirectMessageSupported(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle);
LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_sendAdpMessage(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, avdecc_protocol_adpdu_cp const adpdu);
LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_sendAemAecpMessage(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, avdecc_protocol_aem_aecpdu_cp const aecpdu);
LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_sendAcmpMessage(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, avdecc_protocol_acmpdu_cp const acmpdu);
/** Requires a Controller type LocalEntity to have been created and registered (using LA_AVDECC_LocalEntity_create and LA_AVDECC_ProtocolInterface_registerLocalEntity) for the specified aecpdu.controller_entity_id. */
LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_sendAemAecpCommand(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, avdecc_protocol_aem_aecpdu_cp const aecpdu, avdecc_protocol_interfaces_send_aem_aecp_command_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_sendAemAecpResponse(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, avdecc_protocol_aem_aecpdu_cp const aecpdu);
/** Requires a Controller type LocalEntity to have been created and registered (using LA_AVDECC_LocalEntity_create and LA_AVDECC_ProtocolInterface_registerLocalEntity) for the specified aecpdu.controller_entity_id. */
LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_sendMvuAecpCommand(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, avdecc_protocol_mvu_aecpdu_cp const aecpdu, avdecc_protocol_interfaces_send_mvu_aecp_command_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_sendMvuAecpResponse(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, avdecc_protocol_mvu_aecpdu_cp const aecpdu);
/** Requires a Controller type LocalEntity to have been created and registered (using LA_AVDECC_LocalEntity_create and LA_AVDECC_ProtocolInterface_registerLocalEntity) for the specified acmpdu.controller_entity_id. */
LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_sendAcmpCommand(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, avdecc_protocol_acmpdu_cp const acmpdu, avdecc_protocol_interfaces_send_acmp_command_cb const onResult);
LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_sendAcmpResponse(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle, avdecc_protocol_acmpdu_cp const acmpdu);
LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_lock(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle);
LA_AVDECC_BINDINGS_C_API avdecc_protocol_interface_error_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_unlock(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle);
LA_AVDECC_BINDINGS_C_API avdecc_bool_t LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_ProtocolInterface_isSelfLocked(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle);
