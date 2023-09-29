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
* @file utils.cpp
* @author Christophe Calmejane
* @brief C bindings utility functions
*/

#include "utils.hpp"
#include <cstring> // memcpy

namespace la
{
namespace avdecc
{
namespace bindings
{
namespace fromCppToC
{
void set_macAddress(networkInterface::MacAddress const& source, avdecc_mac_address_t& macAddress) noexcept
{
	std::memcpy(macAddress, source.data(), sizeof(macAddress));
}

avdecc_entity_common_information_t make_entity_common_information(entity::Entity::CommonInformation const& source) noexcept
{
	auto commonInfo = avdecc_entity_common_information_t{};

	commonInfo.entity_id = source.entityID;
	commonInfo.entity_model_id = source.entityModelID;
	commonInfo.entity_capabilities = static_cast<avdecc_entity_entity_capabilities_t>(source.entityCapabilities.value());
	commonInfo.talker_stream_sources = source.talkerStreamSources;
	commonInfo.talker_capabilities = static_cast<avdecc_entity_talker_capabilities_t>(source.talkerCapabilities.value());
	commonInfo.listener_stream_sinks = source.listenerStreamSinks;
	commonInfo.listener_capabilities = static_cast<avdecc_entity_listener_capabilities_t>(source.listenerCapabilities.value());
	commonInfo.controller_capabilities = static_cast<avdecc_entity_controller_capabilities_t>(source.controllerCapabilities.value());
	commonInfo.identify_control_index_valid = !!source.identifyControlIndex;
	commonInfo.identify_control_index = static_cast<avdecc_entity_model_descriptor_index_t>(source.identifyControlIndex ? *source.identifyControlIndex : 0u);
	commonInfo.association_id_valid = !!source.associationID;
	commonInfo.association_id = source.associationID ? *source.associationID : UniqueIdentifier::getNullUniqueIdentifier();

	return commonInfo;
}

avdecc_entity_interface_information_t make_entity_interface_information(entity::model::AvbInterfaceIndex const index, entity::Entity::InterfaceInformation const& source) noexcept
{
	auto interfaceInfo = avdecc_entity_interface_information_t{};

	interfaceInfo.interface_index = index;
	std::memcpy(interfaceInfo.mac_address, source.macAddress.data(), sizeof(interfaceInfo.mac_address));
	interfaceInfo.valid_time = source.validTime;
	interfaceInfo.available_index = source.availableIndex;
	interfaceInfo.gptp_grandmaster_id_valid = !!source.gptpGrandmasterID;
	interfaceInfo.gptp_grandmaster_id = source.gptpGrandmasterID ? *source.gptpGrandmasterID : UniqueIdentifier::getNullUniqueIdentifier();
	interfaceInfo.gptp_domain_number_valid = !!source.gptpDomainNumber;
	interfaceInfo.gptp_domain_number = static_cast<std::uint8_t>(source.gptpDomainNumber ? *source.gptpDomainNumber : 0u);
	interfaceInfo.next = nullptr;

	return interfaceInfo;
}

scoped_avdecc_entity make_entity(entity::Entity const& source) noexcept
{
	auto entity = scoped_avdecc_entity{};

	entity.entity.common_information = make_entity_common_information(source.getCommonInformation());

	avdecc_entity_interface_information_p previous = nullptr;
	avdecc_entity_interface_information_p current = &entity.entity.interfaces_information;
	for (auto const& [interfaceIndex, interfaceInfo] : source.getInterfacesInformation())
	{
		if (previous)
		{
			entity.nextInterfaces.push_front({});
			current = &entity.nextInterfaces.front();
			previous->next = current;
		}
		*current = make_entity_interface_information(interfaceIndex, interfaceInfo);
		previous = current;
	}

	return entity;
}

avdecc_protocol_adpdu_t make_adpdu(protocol::Adpdu const& source) noexcept
{
	auto adpdu = avdecc_protocol_adpdu_t{};

	// Set Ether2 fields
	{
		auto const& ether2 = static_cast<protocol::EtherLayer2 const&>(source);
		set_macAddress(ether2.getSrcAddress(), adpdu.src_address);
		set_macAddress(ether2.getDestAddress(), adpdu.dest_address);
	}
	// Set ADP fields
	{
		auto const& frame = source;
		adpdu.message_type = static_cast<avdecc_protocol_adp_message_type_t>(frame.getMessageType());
		adpdu.valid_time = frame.getValidTime();
		adpdu.entity_id = frame.getEntityID();
		adpdu.entity_model_id = frame.getEntityModelID();
		adpdu.entity_capabilities = static_cast<avdecc_entity_entity_capabilities_t>(frame.getEntityCapabilities().value());
		adpdu.talker_stream_sources = frame.getTalkerStreamSources();
		adpdu.talker_capabilities = static_cast<avdecc_entity_talker_capabilities_t>(frame.getTalkerCapabilities().value());
		adpdu.listener_capabilities = frame.getListenerStreamSinks();
		adpdu.listener_capabilities = static_cast<avdecc_entity_listener_capabilities_t>(frame.getListenerCapabilities().value());
		adpdu.controller_capabilities = static_cast<avdecc_entity_controller_capabilities_t>(frame.getControllerCapabilities().value());
		adpdu.available_index = frame.getAvailableIndex();
		adpdu.gptp_grandmaster_id = frame.getGptpGrandmasterID();
		adpdu.gptp_domain_number = frame.getGptpDomainNumber();
		adpdu.identify_control_index = frame.getIdentifyControlIndex();
		adpdu.interface_index = frame.getInterfaceIndex();
		adpdu.association_id = frame.getAssociationID();
	}

	return adpdu;
}

avdecc_protocol_aem_aecpdu_t make_aem_aecpdu(protocol::AemAecpdu const& source) noexcept
{
	auto aecpdu = avdecc_protocol_aem_aecpdu_t{};

	// Set Ether2 fields
	{
		auto const& ether2 = static_cast<protocol::EtherLayer2 const&>(source);
		set_macAddress(ether2.getSrcAddress(), aecpdu.src_address);
		set_macAddress(ether2.getDestAddress(), aecpdu.dest_address);
	}
	// Set AECP and AEM fields
	{
		auto const& frame = source;
		// AECP fields
		aecpdu.message_type = static_cast<avdecc_protocol_aecp_message_type_t>(frame.getMessageType());
		aecpdu.status = static_cast<avdecc_protocol_aecp_status_t>(frame.getStatus());
		aecpdu.target_entity_id = frame.getTargetEntityID();
		aecpdu.controller_entity_id = frame.getControllerEntityID();
		aecpdu.sequence_id = frame.getSequenceID();
		// AEM fields
		auto const [payload, payloadLength] = frame.getPayload();
		aecpdu.command_specific_length = static_cast<decltype(aecpdu.command_specific_length)>(std::min(payloadLength, sizeof(aecpdu.command_specific)));
		std::memcpy(aecpdu.command_specific, payload, aecpdu.command_specific_length);
	}

	return aecpdu;
}

avdecc_protocol_mvu_aecpdu_t make_mvu_aecpdu(protocol::MvuAecpdu const& source) noexcept
{
	auto aecpdu = avdecc_protocol_mvu_aecpdu_t{};

	// Set Ether2 fields
	{
		auto const& ether2 = static_cast<protocol::EtherLayer2 const&>(source);
		set_macAddress(ether2.getSrcAddress(), aecpdu.src_address);
		set_macAddress(ether2.getDestAddress(), aecpdu.dest_address);
	}
	// Set AECP fields
	{
		auto const& frame = source;
		// AECP fields
		aecpdu.message_type = static_cast<avdecc_protocol_aecp_message_type_t>(frame.getMessageType());
		aecpdu.status = static_cast<avdecc_protocol_aecp_status_t>(frame.getStatus());
		aecpdu.target_entity_id = frame.getTargetEntityID();
		aecpdu.controller_entity_id = frame.getControllerEntityID();
		aecpdu.sequence_id = frame.getSequenceID();
	}
	// Set MVU fields
	{
		auto const& frame = source;
		auto const [payload, payloadLength] = frame.getPayload();
		aecpdu.command_specific_length = static_cast<decltype(aecpdu.command_specific_length)>(std::min(payloadLength, sizeof(aecpdu.command_specific)));
		std::memcpy(aecpdu.command_specific, payload, aecpdu.command_specific_length);
	}

	return aecpdu;
}

avdecc_protocol_acmpdu_t make_acmpdu(protocol::Acmpdu const& source) noexcept
{
	auto acmpdu = avdecc_protocol_acmpdu_t{};

	// Set Ether2 fields
	{
		auto const& ether2 = static_cast<protocol::EtherLayer2 const&>(source);
		set_macAddress(ether2.getSrcAddress(), acmpdu.src_address);
		set_macAddress(ether2.getDestAddress(), acmpdu.dest_address);
	}
	// Set AVTPControl fields
	{
		auto const& avtp = static_cast<protocol::AvtpduControl const&>(source);
		acmpdu.stream_id = avtp.getStreamID();
	}
	// Set ACMP fields
	{
		auto const& frame = source;
		acmpdu.message_type = static_cast<avdecc_protocol_acmp_message_type_t>(frame.getMessageType());
		acmpdu.status = static_cast<avdecc_protocol_acmp_status_t>(frame.getStatus());
		acmpdu.controller_entity_id = frame.getControllerEntityID();
		acmpdu.talker_entity_id = frame.getTalkerEntityID();
		acmpdu.listener_entity_id = frame.getListenerEntityID();
		acmpdu.talker_unique_id = frame.getTalkerUniqueID();
		acmpdu.listener_unique_id = frame.getListenerUniqueID();
		set_macAddress(frame.getStreamDestAddress(), acmpdu.stream_dest_address);
		acmpdu.connection_count = frame.getConnectionCount();
		acmpdu.sequence_id = frame.getSequenceID();
		acmpdu.flags = static_cast<avdecc_entity_connection_flags_t>(frame.getFlags().value());
		acmpdu.stream_vlan_id = frame.getStreamVlanID();
	}

	return acmpdu;
}

avdecc_protocol_interface_error_t convertProtocolInterfaceErrorCode(protocol::ProtocolInterface::Error const error) noexcept
{
	switch (error)
	{
		case protocol::ProtocolInterface::Error::NoError:
			return avdecc_protocol_interface_error_no_error;
		case protocol::ProtocolInterface::Error::TransportError:
			return avdecc_protocol_interface_error_transport_error;
		case protocol::ProtocolInterface::Error::Timeout:
			return avdecc_protocol_interface_error_timeout;
		case protocol::ProtocolInterface::Error::UnknownRemoteEntity:
			return avdecc_protocol_interface_error_unknown_remote_entity;
		case protocol::ProtocolInterface::Error::UnknownLocalEntity:
			return avdecc_protocol_interface_error_unknown_local_entity;
		case protocol::ProtocolInterface::Error::InvalidEntityType:
			return avdecc_protocol_interface_error_invalid_entity_type;
		case protocol::ProtocolInterface::Error::DuplicateLocalEntityID:
			return avdecc_protocol_interface_error_duplicate_local_entity_id;
		case protocol::ProtocolInterface::Error::InterfaceNotFound:
			return avdecc_protocol_interface_error_interface_not_found;
		case protocol::ProtocolInterface::Error::InvalidParameters:
			return avdecc_protocol_interface_error_invalid_parameters;
		case protocol::ProtocolInterface::Error::InterfaceNotSupported:
			return avdecc_protocol_interface_error_interface_not_supported;
		case protocol::ProtocolInterface::Error::MessageNotSupported:
			return avdecc_protocol_interface_error_message_not_supported;
		case protocol::ProtocolInterface::Error::ExecutorNotInitialized:
			return avdecc_protocol_interface_error_executor_not_initialized;
		case protocol::ProtocolInterface::Error::InternalError:
			return avdecc_protocol_interface_error_internal_error;
		default:
			AVDECC_ASSERT(false, "Unhandled error");
			return avdecc_protocol_interface_error_internal_error;
	}
}

avdecc_local_entity_advertise_flags_t convertLocalEntityAdvertiseFlags(entity::LocalEntity::AdvertiseFlags const flags) noexcept
{
	auto f = avdecc_local_entity_advertise_flags_t{};

	if (flags.test(entity::LocalEntity::AdvertiseFlag::EntityCapabilities))
	{
		f |= avdecc_local_entity_advertise_flags_entity_capabilities;
	}
	if (flags.test(entity::LocalEntity::AdvertiseFlag::AssociationID))
	{
		f |= avdecc_local_entity_advertise_flags_association_id;
	}
	if (flags.test(entity::LocalEntity::AdvertiseFlag::ValidTime))
	{
		f |= avdecc_local_entity_advertise_flags_valid_time;
	}
	if (flags.test(entity::LocalEntity::AdvertiseFlag::GptpGrandmasterID))
	{
		f |= avdecc_local_entity_advertise_flags_gptp_grandmaster_id;
	}
	if (flags.test(entity::LocalEntity::AdvertiseFlag::GptpDomainNumber))
	{
		f |= avdecc_local_entity_advertise_flags_gptp_domain_number;
	}

	return f;
}

avdecc_entity_model_stream_identification_t make_stream_identification(entity::model::StreamIdentification const& source) noexcept
{
	auto stream = avdecc_entity_model_stream_identification_t{};

	stream.entity_id = source.entityID;
	stream.stream_index = source.streamIndex;

	return stream;
}

std::vector<avdecc_entity_model_audio_mapping_t> make_audio_mappings(entity::model::AudioMappings const& mappings) noexcept
{
	auto m = std::vector<avdecc_entity_model_audio_mapping_t>{};

	for (auto const& mapping : mappings)
	{
		m.emplace_back(avdecc_entity_model_audio_mapping_t{ mapping.streamIndex, mapping.streamChannel, mapping.clusterOffset, mapping.clusterChannel });
	}

	return m;
}

std::vector<avdecc_entity_model_audio_mapping_p> make_audio_mappings_pointer(std::vector<avdecc_entity_model_audio_mapping_t>& mappings) noexcept
{
	auto m = std::vector<avdecc_entity_model_audio_mapping_p>{};

	for (auto& mapping : mappings)
	{
		m.push_back(&mapping);
	}

	// NULL terminated list
	m.push_back(nullptr);

	return m;
}

avdecc_entity_model_stream_info_t make_stream_info(entity::model::StreamInfo const& info) noexcept
{
	auto i = avdecc_entity_model_stream_info_t{};

	i.stream_info_flags = info.streamInfoFlags.value();
	i.stream_format = info.streamFormat;
	i.stream_id = info.streamID;
	i.msrp_accumulated_latency = info.msrpAccumulatedLatency;
	std::memcpy(i.stream_dest_mac, info.streamDestMac.data(), sizeof(i.stream_dest_mac));
	i.msrp_failure_code = static_cast<avdecc_entity_model_msrp_failure_code_t>(info.msrpFailureCode);
	i.msrp_failure_bridge_id = info.msrpFailureBridgeID;
	i.stream_vlan_id = info.streamVlanID;
	// Milan additions
	i.stream_info_flags_ex_valid = !!info.streamInfoFlagsEx;
	i.stream_info_flags_ex = static_cast<avdecc_entity_stream_info_flags_ex_t>(info.streamInfoFlagsEx ? (*info.streamInfoFlagsEx).value() : 0u);
	i.probing_status_valid = !!info.probingStatus;
	i.probing_status = info.probingStatus ? static_cast<avdecc_entity_model_probing_status_t>(*info.probingStatus) : avdecc_entity_model_probing_status_t{ 0u };
	i.acmp_status_valid = !!info.acmpStatus;
	i.acmp_status = static_cast<avdecc_protocol_acmp_status_t>(info.acmpStatus ? (*info.acmpStatus).getValue() : 0u);

	return i;
}

avdecc_entity_model_avb_info_t make_avb_info(entity::model::AvbInfo const& info) noexcept
{
	auto i = avdecc_entity_model_avb_info_t{};

	i.gptp_grandmaster_id = info.gptpGrandmasterID;
	i.propagation_delay = info.propagationDelay;
	i.gptp_domain_number = info.gptpDomainNumber;
	i.flags = static_cast<avdecc_entity_avb_info_flags_t>(info.flags.value());
	i.mappings = nullptr;

	return i;
}

std::vector<avdecc_entity_model_msrp_mapping_t> make_msrp_mappings(entity::model::MsrpMappings const& mappings) noexcept
{
	auto m = std::vector<avdecc_entity_model_msrp_mapping_t>{};

	for (auto const& mapping : mappings)
	{
		m.emplace_back(avdecc_entity_model_msrp_mapping_t{ mapping.trafficClass, mapping.priority, mapping.vlanID });
	}

	return m;
}

std::vector<avdecc_entity_model_msrp_mapping_p> make_msrp_mappings_pointer(std::vector<avdecc_entity_model_msrp_mapping_t>& mappings) noexcept
{
	auto m = std::vector<avdecc_entity_model_msrp_mapping_p>{};

	for (auto& mapping : mappings)
	{
		m.push_back(&mapping);
	}

	// NULL terminated list
	m.push_back(nullptr);

	return m;
}

avdecc_entity_model_as_path_t make_as_path(entity::model::AsPath const& /*path*/) noexcept
{
	auto p = avdecc_entity_model_as_path_t{};

	p.sequence = nullptr;

	return p;
}

std::vector<avdecc_unique_identifier_t> make_path_sequence(entity::model::PathSequence const& path) noexcept
{
	auto m = std::vector<avdecc_unique_identifier_t>{};

	for (auto const& p : path)
	{
		m.emplace_back(p);
	}

	return m;
}

std::vector<avdecc_unique_identifier_t*> make_path_sequence_pointer(std::vector<avdecc_unique_identifier_t>& path) noexcept
{
	auto m = std::vector<avdecc_unique_identifier_t*>{};

	for (auto& p : path)
	{
		m.push_back(&p);
	}

	// NULL terminated list
	m.push_back(nullptr);

	return m;
}

avdecc_entity_model_milan_info_t make_milan_info(entity::model::MilanInfo const& info) noexcept
{
	auto i = avdecc_entity_model_milan_info_t{};

	i.protocol_version = info.protocolVersion;
	i.features_flags = static_cast<avdecc_entity_avb_info_flags_t>(info.featuresFlags.value());
	i.certification_version = info.certificationVersion;

	return i;
}

avdecc_entity_model_entity_descriptor_t make_entity_descriptor(entity::model::EntityDescriptor const& descriptor) noexcept
{
	auto d = avdecc_entity_model_entity_descriptor_t{};

	d.entity_id = descriptor.entityID;
	d.entity_model_id = descriptor.entityModelID;
	d.entity_capabilities = static_cast<avdecc_entity_entity_capabilities_t>(descriptor.entityCapabilities.value());
	d.talker_stream_sources = descriptor.talkerStreamSources;
	d.talker_capabilities = static_cast<avdecc_entity_talker_capabilities_t>(descriptor.talkerCapabilities.value());
	d.listener_stream_sinks = descriptor.listenerStreamSinks;
	d.listener_capabilities = static_cast<avdecc_entity_listener_capabilities_t>(descriptor.listenerCapabilities.value());
	d.controller_capabilities = static_cast<avdecc_entity_controller_capabilities_t>(descriptor.controllerCapabilities.value());
	d.available_index = descriptor.availableIndex;
	d.association_id = descriptor.associationID;
	std::memcpy(d.entity_name, descriptor.entityName.data(), sizeof(avdecc_fixed_string_t));
	d.vendor_name_string = descriptor.vendorNameString;
	d.model_name_string = descriptor.modelNameString;
	std::memcpy(d.firmware_version, descriptor.firmwareVersion.data(), sizeof(avdecc_fixed_string_t));
	std::memcpy(d.group_name, descriptor.groupName.data(), sizeof(avdecc_fixed_string_t));
	std::memcpy(d.serial_number, descriptor.serialNumber.data(), sizeof(avdecc_fixed_string_t));
	d.configurations_count = descriptor.configurationsCount;
	d.current_configuration = descriptor.currentConfiguration;

	return d;
}

avdecc_entity_model_configuration_descriptor_t make_configuration_descriptor(entity::model::ConfigurationDescriptor const& descriptor) noexcept
{
	auto d = avdecc_entity_model_configuration_descriptor_t{};

	std::memcpy(d.object_name, descriptor.objectName.data(), sizeof(avdecc_fixed_string_t));
	d.localized_description = descriptor.localizedDescription;
	d.counts = nullptr;

	return d;
}

std::vector<avdecc_entity_model_descriptors_count_t> make_descriptors_count(std::unordered_map<entity::model::DescriptorType, std::uint16_t, utils::EnumClassHash> const& counts) noexcept
{
	auto m = std::vector<avdecc_entity_model_descriptors_count_t>{};

	for (auto const& [descriptorType, count] : counts)
	{
		m.emplace_back(avdecc_entity_model_descriptors_count_t{ static_cast<avdecc_entity_model_descriptor_type_t>(descriptorType), count });
	}

	return m;
}

std::vector<avdecc_entity_model_descriptors_count_p> make_descriptors_count_pointer(std::vector<avdecc_entity_model_descriptors_count_t>& counts) noexcept
{
	auto m = std::vector<avdecc_entity_model_descriptors_count_t*>{};

	for (auto& p : counts)
	{
		m.push_back(&p);
	}

	// NULL terminated list
	m.push_back(nullptr);

	return m;
}

avdecc_entity_model_audio_unit_descriptor_t make_audio_unit_descriptor(entity::model::AudioUnitDescriptor const& descriptor) noexcept
{
	auto d = avdecc_entity_model_audio_unit_descriptor_t{};

	std::memcpy(d.object_name, descriptor.objectName.data(), sizeof(avdecc_fixed_string_t));
	d.localized_description = descriptor.localizedDescription;
	d.clock_domain_index = descriptor.clockDomainIndex;
	d.number_of_stream_input_ports = descriptor.numberOfStreamInputPorts;
	d.base_stream_input_port = descriptor.baseStreamInputPort;
	d.number_of_stream_output_ports = descriptor.numberOfStreamOutputPorts;
	d.base_stream_output_port = descriptor.baseStreamOutputPort;
	d.number_of_external_input_ports = descriptor.numberOfExternalInputPorts;
	d.base_external_input_port = descriptor.baseExternalInputPort;
	d.number_of_external_output_ports = descriptor.numberOfExternalOutputPorts;
	d.base_external_output_port = descriptor.baseExternalOutputPort;
	d.number_of_internal_input_ports = descriptor.numberOfInternalInputPorts;
	d.base_internal_input_port = descriptor.baseInternalInputPort;
	d.number_of_internal_output_ports = descriptor.numberOfInternalOutputPorts;
	d.base_internal_output_port = descriptor.baseInternalOutputPort;
	d.number_of_controls = descriptor.numberOfControls;
	d.base_control = descriptor.baseControl;
	d.number_of_signal_selectors = descriptor.numberOfSignalSelectors;
	d.base_signal_selector = descriptor.baseSignalSelector;
	d.number_of_mixers = descriptor.numberOfMixers;
	d.base_mixer = descriptor.baseMixer;
	d.number_of_matrices = descriptor.numberOfMatrices;
	d.base_matrix = descriptor.baseMatrix;
	d.number_of_splitters = descriptor.numberOfSplitters;
	d.base_splitter = descriptor.baseSplitter;
	d.number_of_combiners = descriptor.numberOfCombiners;
	d.base_combiner = descriptor.baseCombiner;
	d.number_of_demultiplexers = descriptor.numberOfDemultiplexers;
	d.base_demultiplexer = descriptor.baseDemultiplexer;
	d.number_of_multiplexers = descriptor.numberOfMultiplexers;
	d.base_multiplexer = descriptor.baseMultiplexer;
	d.number_of_transcoders = descriptor.numberOfTranscoders;
	d.base_transcoder = descriptor.baseTranscoder;
	d.number_of_control_blocks = descriptor.numberOfControlBlocks;
	d.base_control_block = descriptor.baseControlBlock;
	d.current_sampling_rate = descriptor.currentSamplingRate;
	d.sampling_rates = nullptr;

	return d;
}

std::vector<avdecc_entity_model_sampling_rate_t> make_sampling_rates(std::set<entity::model::SamplingRate> const& samplingRates) noexcept
{
	auto rates = std::vector<avdecc_entity_model_sampling_rate_t>{};

	for (auto const& r : samplingRates)
	{
		rates.emplace_back(r);
	}

	return rates;
}

std::vector<avdecc_entity_model_sampling_rate_t*> make_sampling_rates_pointer(std::vector<avdecc_entity_model_sampling_rate_t>& samplingRates) noexcept
{
	auto rates = std::vector<avdecc_entity_model_sampling_rate_t*>{};

	for (auto& r : samplingRates)
	{
		rates.push_back(&r);
	}

	// NULL terminated list
	rates.push_back(nullptr);

	return rates;
}

avdecc_entity_model_stream_descriptor_t make_stream_descriptor(entity::model::StreamDescriptor const& descriptor) noexcept
{
	auto d = avdecc_entity_model_stream_descriptor_t{};

	std::memcpy(d.object_name, descriptor.objectName.data(), sizeof(avdecc_fixed_string_t));
	d.localized_description = descriptor.localizedDescription;
	d.clock_domain_index = descriptor.clockDomainIndex;
	d.stream_flags = static_cast<avdecc_entity_stream_flags_t>(descriptor.streamFlags.value());
	d.current_format = descriptor.currentFormat;
	d.backupTalker_entity_id_0 = descriptor.backupTalkerEntityID_0;
	d.backup_talker_unique_id_0 = descriptor.backupTalkerUniqueID_0;
	d.backup_talker_entity_id_1 = descriptor.backupTalkerEntityID_1;
	d.backup_talker_unique_id_1 = descriptor.backupTalkerUniqueID_1;
	d.backup_talker_entity_id_2 = descriptor.backupTalkerEntityID_2;
	d.backup_talker_unique_id_2 = descriptor.backupTalkerUniqueID_2;
	d.backedup_talker_entity_id = descriptor.backedupTalkerEntityID;
	d.backedup_talker_unique = descriptor.backedupTalkerUnique;
	d.avb_interface_index = descriptor.avbInterfaceIndex;
	d.buffer_length = descriptor.bufferLength;
	d.formats = nullptr;
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
	d.redundant_streams = nullptr;
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

	return d;
}

std::vector<avdecc_entity_model_stream_format_t> make_stream_formats(std::set<entity::model::StreamFormat> const& streamFormats) noexcept
{
	auto formats = std::vector<avdecc_entity_model_stream_format_t>{};

	for (auto const& f : streamFormats)
	{
		formats.emplace_back(f);
	}

	return formats;
}

std::vector<avdecc_entity_model_stream_format_t*> make_stream_formats_pointer(std::vector<avdecc_entity_model_stream_format_t>& streamFormats) noexcept
{
	auto formats = std::vector<avdecc_entity_model_stream_format_t*>{};

	for (auto& f : streamFormats)
	{
		formats.push_back(&f);
	}

	// NULL terminated list
	formats.push_back(nullptr);

	return formats;
}

std::vector<avdecc_entity_model_descriptor_index_t> make_redundant_stream_indexes(std::set<entity::model::StreamIndex> const& streamIndexes) noexcept
{
	auto indexes = std::vector<avdecc_entity_model_descriptor_index_t>{};

	for (auto const& i : streamIndexes)
	{
		indexes.emplace_back(i);
	}

	return indexes;
}

std::vector<avdecc_entity_model_descriptor_index_t*> make_redundant_stream_indexes_pointer(std::vector<avdecc_entity_model_descriptor_index_t>& streamIndexes) noexcept
{
	auto indexes = std::vector<avdecc_entity_model_descriptor_index_t*>{};

	for (auto& i : streamIndexes)
	{
		indexes.push_back(&i);
	}

	// NULL terminated list
	indexes.push_back(nullptr);

	return indexes;
}

avdecc_entity_model_jack_descriptor_t make_jack_descriptor(entity::model::JackDescriptor const& descriptor) noexcept
{
	auto d = avdecc_entity_model_jack_descriptor_t{};

	std::memcpy(d.object_name, descriptor.objectName.data(), sizeof(avdecc_fixed_string_t));
	d.localized_description = descriptor.localizedDescription;
	d.jack_flags = static_cast<avdecc_entity_jack_flags_t>(descriptor.jackFlags.value());
	d.jack_type = static_cast<avdecc_entity_model_jack_type_t>(descriptor.jackType);
	d.number_of_controls = descriptor.numberOfControls;
	d.base_control = descriptor.baseControl;

	return d;
}

avdecc_entity_model_avb_interface_descriptor_t make_avb_interface_descriptor(entity::model::AvbInterfaceDescriptor const& descriptor) noexcept
{
	auto d = avdecc_entity_model_avb_interface_descriptor_t{};

	std::memcpy(d.object_name, descriptor.objectName.data(), sizeof(avdecc_fixed_string_t));
	d.localized_description = descriptor.localizedDescription;
	std::memcpy(d.mac_address, descriptor.macAddress.data(), sizeof(d.mac_address));
	d.interface_flags = static_cast<avdecc_entity_avb_interface_flags_t>(descriptor.interfaceFlags.value());
	d.clock_identity = descriptor.clockIdentity;
	d.priority1 = descriptor.priority1;
	d.clock_class = descriptor.clockClass;
	d.offset_scaled_log_variance = descriptor.offsetScaledLogVariance;
	d.clock_accuracy = descriptor.clockAccuracy;
	d.priority2 = descriptor.priority2;
	d.domain_number = descriptor.domainNumber;
	d.log_sync_interval = descriptor.logSyncInterval;
	d.log_announce_interval = descriptor.logAnnounceInterval;
	d.log_p_delay_interval = descriptor.logPDelayInterval;
	d.port_number = descriptor.portNumber;

	return d;
}

avdecc_entity_model_clock_source_descriptor_t make_clock_source_descriptor(entity::model::ClockSourceDescriptor const& descriptor) noexcept
{
	auto d = avdecc_entity_model_clock_source_descriptor_t{};

	std::memcpy(d.object_name, descriptor.objectName.data(), sizeof(avdecc_fixed_string_t));
	d.localized_description = descriptor.localizedDescription;
	d.clock_source_flags = static_cast<avdecc_entity_clock_source_flags_t>(descriptor.clockSourceFlags.value());
	d.clock_source_type = static_cast<avdecc_entity_model_clock_source_type_t>(descriptor.clockSourceType);
	d.clock_source_identifier = descriptor.clockSourceIdentifier;
	d.clock_source_location_type = static_cast<avdecc_entity_model_descriptor_type_t>(descriptor.clockSourceLocationType);
	d.clock_source_location_index = descriptor.clockSourceLocationIndex;

	return d;
}

avdecc_entity_model_memory_object_descriptor_t make_memory_object_descriptor(entity::model::MemoryObjectDescriptor const& descriptor) noexcept
{
	auto d = avdecc_entity_model_memory_object_descriptor_t{};

	std::memcpy(d.object_name, descriptor.objectName.data(), sizeof(avdecc_fixed_string_t));
	d.localized_description = descriptor.localizedDescription;
	d.memory_object_type = static_cast<avdecc_entity_model_memory_object_type_t>(descriptor.memoryObjectType);
	d.target_descriptor_type = static_cast<avdecc_entity_model_descriptor_type_t>(descriptor.targetDescriptorType);
	d.target_descriptor_index = descriptor.targetDescriptorIndex;
	d.start_address = descriptor.startAddress;
	d.maximum_length = descriptor.maximumLength;
	d.length = descriptor.length;

	return d;
}

avdecc_entity_model_locale_descriptor_t make_locale_descriptor(entity::model::LocaleDescriptor const& descriptor) noexcept
{
	auto d = avdecc_entity_model_locale_descriptor_t{};

	std::memcpy(d.locale_id, descriptor.localeID.data(), sizeof(avdecc_fixed_string_t));
	d.number_of_string_descriptors = descriptor.numberOfStringDescriptors;
	d.base_string_descriptor_index = descriptor.baseStringDescriptorIndex;

	return d;
}

avdecc_entity_model_strings_descriptor_t make_strings_descriptor(entity::model::StringsDescriptor const& descriptor) noexcept
{
	auto d = avdecc_entity_model_strings_descriptor_t{};

	for (auto i = 0u; i < descriptor.strings.size(); ++i)
	{
		std::memcpy(d.strings[i], descriptor.strings[i].data(), sizeof(avdecc_fixed_string_t));
	}

	return d;
}

avdecc_entity_model_stream_port_descriptor_t make_stream_port_descriptor(entity::model::StreamPortDescriptor const& descriptor) noexcept
{
	auto d = avdecc_entity_model_stream_port_descriptor_t{};

	d.clock_domain_index = descriptor.clockDomainIndex;
	d.port_flags = static_cast<avdecc_entity_port_flags_t>(descriptor.portFlags.value());
	d.number_of_controls = descriptor.numberOfControls;
	d.base_control = descriptor.baseControl;
	d.number_of_clusters = descriptor.numberOfClusters;
	d.base_cluster = descriptor.baseCluster;
	d.number_of_maps = descriptor.numberOfMaps;
	d.base_map = descriptor.baseMap;

	return d;
}

avdecc_entity_model_external_port_descriptor_t make_external_port_descriptor(entity::model::ExternalPortDescriptor const& descriptor) noexcept
{
	auto d = avdecc_entity_model_external_port_descriptor_t{};

	d.clock_domain_index = descriptor.clockDomainIndex;
	d.port_flags = static_cast<avdecc_entity_port_flags_t>(descriptor.portFlags.value());
	d.number_of_controls = descriptor.numberOfControls;
	d.base_control = descriptor.baseControl;
	d.signal_type = static_cast<avdecc_entity_model_descriptor_type_t>(descriptor.signalType);
	d.signal_index = descriptor.signalIndex;
	d.signal_output = descriptor.signalOutput;
	d.block_latency = descriptor.blockLatency;
	d.jack_index = descriptor.jackIndex;

	return d;
}

avdecc_entity_model_internal_port_descriptor_t make_internal_port_descriptor(entity::model::InternalPortDescriptor const& descriptor) noexcept
{
	auto d = avdecc_entity_model_internal_port_descriptor_t{};

	d.clock_domain_index = descriptor.clockDomainIndex;
	d.port_flags = static_cast<avdecc_entity_port_flags_t>(descriptor.portFlags.value());
	d.number_of_controls = descriptor.numberOfControls;
	d.base_control = descriptor.baseControl;
	d.signal_type = static_cast<avdecc_entity_model_descriptor_type_t>(descriptor.signalType);
	d.signal_index = descriptor.signalIndex;
	d.signal_output = descriptor.signalOutput;
	d.block_latency = descriptor.blockLatency;
	d.internal_index = descriptor.internalIndex;

	return d;
}

avdecc_entity_model_audio_cluster_descriptor_t make_audio_cluster_descriptor(entity::model::AudioClusterDescriptor const& descriptor) noexcept
{
	auto d = avdecc_entity_model_audio_cluster_descriptor_t{};

	std::memcpy(d.object_name, descriptor.objectName.data(), sizeof(avdecc_fixed_string_t));
	d.localized_description = descriptor.localizedDescription;
	d.signal_type = static_cast<avdecc_entity_model_descriptor_type_t>(descriptor.signalType);
	d.signal_index = descriptor.signalIndex;
	d.signal_output = descriptor.signalOutput;
	d.path_latency = descriptor.pathLatency;
	d.block_latency = descriptor.blockLatency;
	d.channel_count = descriptor.channelCount;
	d.format = static_cast<avdecc_entity_model_audio_cluster_format_t>(descriptor.format);

	return d;
}

avdecc_entity_model_audio_map_descriptor_t make_audio_map_descriptor(entity::model::AudioMapDescriptor const& /*descriptor*/) noexcept
{
	auto d = avdecc_entity_model_audio_map_descriptor_t{};

	d.mappings = nullptr;

	return d;
}

avdecc_entity_model_clock_domain_descriptor_t make_clock_domain_descriptor(entity::model::ClockDomainDescriptor const& descriptor) noexcept
{
	auto d = avdecc_entity_model_clock_domain_descriptor_t{};

	std::memcpy(d.object_name, descriptor.objectName.data(), sizeof(avdecc_fixed_string_t));
	d.localized_description = descriptor.localizedDescription;
	d.clock_source_index = descriptor.clockSourceIndex;
	d.clock_sources = nullptr;

	return d;
}

std::vector<avdecc_entity_model_descriptor_index_t> make_clock_sources(std::vector<entity::model::ClockSourceIndex> const& clockSources) noexcept
{
	auto sources = std::vector<avdecc_entity_model_descriptor_index_t>{};

	for (auto const& s : clockSources)
	{
		sources.emplace_back(s);
	}

	return sources;
}

std::vector<avdecc_entity_model_descriptor_index_t*> make_clock_sources_pointer(std::vector<avdecc_entity_model_descriptor_index_t>& clockSources) noexcept
{
	auto sources = std::vector<avdecc_entity_model_descriptor_index_t*>{};

	for (auto& s : clockSources)
	{
		sources.push_back(&s);
	}

	// NULL terminated list
	sources.push_back(nullptr);

	return sources;
}

} // namespace fromCppToC

namespace fromCToCpp
{
networkInterface::MacAddress make_macAddress(avdecc_mac_address_cp const macAddress) noexcept
{
	auto adrs = networkInterface::MacAddress{};

	std::memcpy(adrs.data(), macAddress, adrs.size());

	return adrs;
}

entity::Entity::CommonInformation make_entity_common_information(avdecc_entity_common_information_t const& commonInfo) noexcept
{
	auto info = entity::Entity::CommonInformation{};

	info.entityID = UniqueIdentifier{ commonInfo.entity_id };
	info.entityModelID = UniqueIdentifier{ commonInfo.entity_model_id };
	info.entityCapabilities.assign(commonInfo.entity_capabilities);
	info.talkerStreamSources = commonInfo.talker_stream_sources;
	info.talkerCapabilities.assign(commonInfo.talker_capabilities);
	info.listenerStreamSinks = commonInfo.listener_stream_sinks;
	info.listenerCapabilities.assign(commonInfo.listener_capabilities);
	info.controllerCapabilities.assign(commonInfo.controller_capabilities);
	if (commonInfo.identify_control_index_valid)
	{
		info.identifyControlIndex = commonInfo.identify_control_index;
	}
	if (commonInfo.association_id_valid)
	{
		info.associationID = UniqueIdentifier{ commonInfo.association_id };
	}

	return info;
}

entity::Entity::InterfaceInformation make_entity_interface_information(avdecc_entity_interface_information_t const& interfaceInfo) noexcept
{
	auto info = entity::Entity::InterfaceInformation{};

	info.macAddress = make_macAddress(&interfaceInfo.mac_address);
	info.validTime = interfaceInfo.valid_time;
	info.availableIndex = interfaceInfo.available_index;
	if (interfaceInfo.gptp_grandmaster_id_valid)
	{
		info.gptpGrandmasterID = UniqueIdentifier{ interfaceInfo.gptp_grandmaster_id };
	}
	if (interfaceInfo.gptp_domain_number_valid)
	{
		info.gptpDomainNumber = interfaceInfo.gptp_domain_number;
	}

	return info;
}
/*
entity::AggregateEntity::UniquePointer make_local_entity(avdecc_entity_t const& entity)
{
	auto const entityCaps = static_cast<entity::EntityCapabilities>(entity.common_information.entity_capabilities);
	auto controlIndex{ std::optional<entity::model::ControlIndex>{} };
	auto associationID{ std::optional<UniqueIdentifier>{} };
	auto avbInterfaceIndex{ entity::Entity::GlobalAvbInterfaceIndex };
	auto gptpGrandmasterID{ std::optional<UniqueIdentifier>{} };
	auto gptpDomainNumber{ std::optional<std::uint8_t>{} };

	if (hasFlag(entityCaps, entity::EntityCapabilities::AemIdentifyControlIndexValid))
	{
		controlIndex = entity.common_information.identify_control_index;
	}
	if (hasFlag(entityCaps, entity::EntityCapabilities::AssociationIDValid))
	{
		associationID = entity.common_information.association_id;
	}
	if (hasFlag(entityCaps, entity::EntityCapabilities::AemInterfaceIndexValid))
	{
		avbInterfaceIndex = entity.interfaces_information.interface_index;
	}
	if (hasFlag(entityCaps, entity::EntityCapabilities::GptpSupported))
	{
		gptpGrandmasterID = entity.interfaces_information.gptp_grandmaster_id;
		gptpDomainNumber = entity.interfaces_information.gptp_domain_number;
	}

	auto const commonInfo{ entity::Entity::CommonInformation{ entity.common_information.entity_id, entity.common_information.entity_model_id, entityCaps, entity.common_information.talker_stream_sources, static_cast<entity::TalkerCapabilities>(entity.common_information.talker_capabilities), entity.common_information.listener_stream_sinks, static_cast<entity::ListenerCapabilities>(entity.common_information.listener_capabilities), static_cast<entity::ControllerCapabilities>(entity.common_information.controller_capabilities), controlIndex, associationID } };
	auto interfacesInfo{ entity::Entity::InterfacesInformation{ { entity.interfaces_information.interface_index, make_interface_information(entity.interfaces_information) } } };

	for (auto* interfaceInfo = entity.interfaces_information.next; interfaceInfo == nullptr; interfaceInfo = interfaceInfo->next)
	{
		interfacesInfo.insert({ interfaceInfo->interface_index, make_interface_information(*interfaceInfo) });
	}

	return entity::AggregateEntity::create(nullptr, commonInfo, interfacesInfo, nullptr);
}
*/
protocol::Adpdu make_adpdu(avdecc_protocol_adpdu_cp const adpdu) noexcept
{
	auto adp = protocol::Adpdu{};

	// Set Ether2 fields
	{
		auto& ether2 = static_cast<protocol::EtherLayer2&>(adp);
		ether2.setSrcAddress(make_macAddress(&adpdu->src_address));
		ether2.setDestAddress(make_macAddress(&adpdu->dest_address));
	}
	// Set ADP fields
	{
		auto& frame = adp;
		frame.setMessageType(static_cast<protocol::AdpMessageType>(adpdu->message_type));
		frame.setValidTime(adpdu->valid_time);
		frame.setEntityID(UniqueIdentifier{ adpdu->entity_id });
		frame.setEntityModelID(UniqueIdentifier{ adpdu->entity_model_id });
		{
			auto caps = entity::EntityCapabilities{};
			caps.assign(adpdu->entity_capabilities);
			frame.setEntityCapabilities(caps);
		}
		frame.setTalkerStreamSources(adpdu->talker_stream_sources);
		{
			auto caps = entity::TalkerCapabilities{};
			caps.assign(adpdu->talker_capabilities);
			frame.setTalkerCapabilities(caps);
		}
		frame.setListenerStreamSinks(adpdu->listener_capabilities);
		{
			auto caps = entity::ListenerCapabilities{};
			caps.assign(adpdu->listener_capabilities);
			frame.setListenerCapabilities(caps);
		}
		{
			auto caps = entity::ControllerCapabilities{};
			caps.assign(adpdu->controller_capabilities);
			frame.setControllerCapabilities(caps);
		}
		frame.setAvailableIndex(adpdu->available_index);
		frame.setGptpGrandmasterID(UniqueIdentifier{ adpdu->gptp_grandmaster_id });
		frame.setGptpDomainNumber(adpdu->gptp_domain_number);
		frame.setIdentifyControlIndex(adpdu->identify_control_index);
		frame.setInterfaceIndex(adpdu->interface_index);
		frame.setAssociationID(UniqueIdentifier{ adpdu->association_id });
	}

	return adp;
}

static void set_aem_aecpdu(avdecc_protocol_aem_aecpdu_cp const aecpdu, protocol::AemAecpdu& aecp) noexcept
{
	// Set Ether2 fields
	{
		auto& ether2 = static_cast<protocol::EtherLayer2&>(aecp);
		ether2.setSrcAddress(make_macAddress(&aecpdu->src_address));
		ether2.setDestAddress(make_macAddress(&aecpdu->dest_address));
	}
	// Set AECP and AEM fields
	{
		auto& frame = static_cast<protocol::AemAecpdu&>(aecp);
		// AECP fields
		frame.setStatus(static_cast<protocol::AecpStatus>(aecpdu->status));
		frame.setTargetEntityID(UniqueIdentifier{ aecpdu->target_entity_id });
		frame.setControllerEntityID(UniqueIdentifier{ aecpdu->controller_entity_id });
		frame.setSequenceID(aecpdu->sequence_id);
		// AEM fields
		frame.setUnsolicited(aecpdu->unsolicited);
		frame.setCommandType(static_cast<protocol::AemCommandType>(aecpdu->command_type));
		frame.setCommandSpecificData(aecpdu->command_specific, aecpdu->command_specific_length);
	}
}

protocol::AemAecpdu make_aem_aecpdu(avdecc_protocol_aem_aecpdu_cp const aecpdu) noexcept
{
	auto const isResponse = (aecpdu->message_type % 2) == 1; // Odd numbers are responses (see IEEE1722.1-2013 Clause 8.2.1.5)
	auto aecp = protocol::AemAecpdu{ isResponse };

	set_aem_aecpdu(aecpdu, aecp);

	return aecp;
}

protocol::AemAecpdu::UniquePointer make_aem_aecpdu_unique(avdecc_protocol_aem_aecpdu_cp const aecpdu) noexcept
{
	auto const isResponse = (aecpdu->message_type % 2) == 1; // Odd numbers are responses (see IEEE1722.1-2013 Clause 8.2.1.5)
	auto aecp = protocol::AemAecpdu::create(isResponse);

	set_aem_aecpdu(aecpdu, static_cast<protocol::AemAecpdu&>(*aecp));

	return aecp;
}

static void set_mvu_aecpdu(avdecc_protocol_mvu_aecpdu_cp const aecpdu, protocol::MvuAecpdu& aecp) noexcept
{
	// Set Ether2 fields
	{
		auto& ether2 = static_cast<protocol::EtherLayer2&>(aecp);
		ether2.setSrcAddress(make_macAddress(&aecpdu->src_address));
		ether2.setDestAddress(make_macAddress(&aecpdu->dest_address));
	}
	// Set AECP and VU fields
	{
		auto& frame = static_cast<protocol::VuAecpdu&>(aecp);
		// AECP fields
		frame.setStatus(static_cast<protocol::AecpStatus>(aecpdu->status));
		frame.setTargetEntityID(UniqueIdentifier{ aecpdu->target_entity_id });
		frame.setControllerEntityID(UniqueIdentifier{ aecpdu->controller_entity_id });
		frame.setSequenceID(aecpdu->sequence_id);
		// VU fields
		frame.setProtocolIdentifier(protocol::MvuAecpdu::ProtocolID);
	}
	// Set MVU fields
	{
		auto& frame = static_cast<protocol::MvuAecpdu&>(aecp);
		frame.setCommandType(static_cast<protocol::MvuCommandType>(aecpdu->command_type));
		frame.setCommandSpecificData(aecpdu->command_specific, aecpdu->command_specific_length);
	}
}

protocol::MvuAecpdu make_mvu_aecpdu(avdecc_protocol_mvu_aecpdu_cp const aecpdu) noexcept
{
	auto const isResponse = (aecpdu->message_type % 2) == 1; // Odd numbers are responses (see IEEE1722.1-2013 Clause 8.2.1.5)
	auto aecp = protocol::MvuAecpdu{ isResponse };

	set_mvu_aecpdu(aecpdu, aecp);

	return aecp;
}

protocol::MvuAecpdu::UniquePointer make_mvu_aecpdu_unique(avdecc_protocol_mvu_aecpdu_cp const aecpdu) noexcept
{
	auto const isResponse = (aecpdu->message_type % 2) == 1; // Odd numbers are responses (see IEEE1722.1-2013 Clause 8.2.1.5)
	auto aecp = protocol::MvuAecpdu::create(isResponse);

	set_mvu_aecpdu(aecpdu, static_cast<protocol::MvuAecpdu&>(*aecp));

	return aecp;
}

static void set_acmpdu(avdecc_protocol_acmpdu_cp const acmpdu, protocol::Acmpdu& acmp) noexcept
{
	// Set Ether2 fields
	{
		auto& ether2 = static_cast<protocol::EtherLayer2&>(acmp);
		ether2.setSrcAddress(make_macAddress(&acmpdu->src_address));
		ether2.setDestAddress(make_macAddress(&acmpdu->dest_address));
	}
	// Set AVTPControl fields
	{
		auto& avtp = static_cast<protocol::AvtpduControl&>(acmp);
		avtp.setStreamID(acmpdu->stream_id);
	}
	// Set ACMP fields
	{
		auto& frame = acmp;
		frame.setMessageType(static_cast<protocol::AcmpMessageType>(acmpdu->message_type));
		frame.setStatus(static_cast<protocol::AcmpStatus>(acmpdu->status));
		frame.setControllerEntityID(UniqueIdentifier{ acmpdu->controller_entity_id });
		frame.setTalkerEntityID(UniqueIdentifier{ acmpdu->talker_entity_id });
		frame.setListenerEntityID(UniqueIdentifier{ acmpdu->listener_entity_id });
		frame.setTalkerUniqueID(acmpdu->talker_unique_id);
		frame.setListenerUniqueID(acmpdu->listener_unique_id);
		frame.setStreamDestAddress(make_macAddress(&acmpdu->stream_dest_address));
		frame.setConnectionCount(acmpdu->connection_count);
		frame.setSequenceID(acmpdu->sequence_id);
		{
			auto flags = entity::ConnectionFlags{};
			flags.assign(acmpdu->flags);
			frame.setFlags(flags);
		}
		frame.setStreamVlanID(acmpdu->stream_vlan_id);
	}
}

protocol::Acmpdu make_acmpdu(avdecc_protocol_acmpdu_cp const acmpdu) noexcept
{
	auto acmp = protocol::Acmpdu{};

	set_acmpdu(acmpdu, acmp);

	return acmp;
}

protocol::Acmpdu::UniquePointer make_acmpdu_unique(avdecc_protocol_acmpdu_cp const acmpdu) noexcept
{
	auto acmp = protocol::Acmpdu::create();

	set_acmpdu(acmpdu, *acmp);

	return acmp;
}

entity::LocalEntity::AdvertiseFlags convertLocalEntityAdvertiseFlags(avdecc_local_entity_advertise_flags_t const flags) noexcept
{
	auto f = entity::LocalEntity::AdvertiseFlags{};

	if (flags & avdecc_local_entity_advertise_flags_entity_capabilities)
	{
		f.set(entity::LocalEntity::AdvertiseFlag::EntityCapabilities);
	}
	if (flags & avdecc_local_entity_advertise_flags_association_id)
	{
		f.set(entity::LocalEntity::AdvertiseFlag::AssociationID);
	}
	if (flags & avdecc_local_entity_advertise_flags_valid_time)
	{
		f.set(entity::LocalEntity::AdvertiseFlag::ValidTime);
	}
	if (flags & avdecc_local_entity_advertise_flags_gptp_grandmaster_id)
	{
		f.set(entity::LocalEntity::AdvertiseFlag::GptpGrandmasterID);
	}
	if (flags & avdecc_local_entity_advertise_flags_gptp_domain_number)
	{
		f.set(entity::LocalEntity::AdvertiseFlag::GptpDomainNumber);
	}

	return f;
}

entity::model::StreamIdentification make_stream_identification(avdecc_entity_model_stream_identification_cp const stream) noexcept
{
	auto s = entity::model::StreamIdentification{};

	s.entityID = UniqueIdentifier{ stream->entity_id };
	s.streamIndex = stream->stream_index;

	return s;
}

entity::model::AudioMappings make_audio_mappings(avdecc_entity_model_audio_mapping_cp const* const mappings) noexcept
{
	auto m = entity::model::AudioMappings{};

	auto* p = *mappings;
	while (p != nullptr)
	{
		m.emplace_back(entity::model::AudioMapping{ p->stream_index, p->stream_channel, p->cluster_offset, p->cluster_channel });
		++p;
	}

	return m;
}

entity::model::StreamInfo make_stream_info(avdecc_entity_model_stream_info_cp const info) noexcept
{
	auto i = entity::model::StreamInfo{};

	i.streamInfoFlags.assign(info->stream_info_flags);
	i.streamFormat = entity::model::StreamFormat{ info->stream_format };
	i.streamID = UniqueIdentifier{ info->stream_id };
	i.msrpAccumulatedLatency = info->msrp_accumulated_latency;
	i.streamDestMac = make_macAddress(&info->stream_dest_mac);
	i.msrpFailureCode = static_cast<entity::model::MsrpFailureCode>(info->msrp_failure_code);
	i.msrpFailureBridgeID = info->msrp_failure_bridge_id;
	i.streamVlanID = info->stream_vlan_id;
	// Milan additions
	if (info->stream_info_flags_ex_valid)
	{
		auto f = entity::StreamInfoFlagsEx{};
		f.assign(info->stream_info_flags_ex);
		i.streamInfoFlagsEx = f;
	}
	if (info->probing_status_valid)
	{
		i.probingStatus = static_cast<entity::model::ProbingStatus>(info->probing_status);
	}
	if (info->acmp_status_valid)
	{
		i.acmpStatus = static_cast<protocol::AcmpStatus>(info->acmp_status);
	}

	return i;
}


} // namespace fromCToCpp
} // namespace bindings
} // namespace avdecc
} // namespace la
