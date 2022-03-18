/*
* Copyright (C) 2016-2022, L-Acoustics and its contributors

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
* @file utils.hpp
* @author Christophe Calmejane
* @brief C bindings utility templates
*/

#pragma once

#include "la/avdecc/avdecc.h"
#include <la/avdecc/avdecc.hpp>
#include <unordered_map>
#include <memory>
#include <list>
#include <type_traits>

#ifdef _WIN32
#	define strdup _strdup
#endif // _WIN32

namespace la
{
namespace avdecc
{
namespace bindings
{
template<class Object>
class HandleManager
{
public:
	using Handle = void*;
	using StoredObject = std::conditional_t<std::is_pointer_v<Object>, std::unique_ptr<std::remove_pointer_t<Object>>, Object>;
	using Objects = std::unordered_map<Handle, StoredObject>;

	template<typename Sfinae = Object, typename... Ts>
	std::enable_if_t<!std::is_pointer_v<Sfinae>, Handle> createObject(Ts&&... params)
	{
		auto obj = Object::element_type::create(std::forward<Ts>(params)...);
		auto const handle = static_cast<Handle>(obj.get());
		_objects.emplace(std::make_pair(handle, std::move(obj)));
		return handle;
	}
	template<typename Sfinae = Object, typename... Ts>
	std::enable_if_t<std::is_pointer_v<Sfinae>, Handle> createObject(Ts&&... params)
	{
		auto obj = std::make_unique<typename StoredObject::element_type>(std::forward<Ts>(params)...);
		auto const handle = static_cast<Handle>(obj.get());
		_objects.emplace(std::make_pair(handle, std::move(obj)));
		return handle;
	}
	template<typename Sfinae = Object>
	std::enable_if_t<!std::is_pointer_v<Sfinae>, Handle> setObject(StoredObject&& obj)
	{
		auto const handle = static_cast<Handle>(obj.get());
		_objects.emplace(std::make_pair(handle, std::move(obj)));
		return handle;
	}

	typename StoredObject::element_type& getObject(Handle const handle)
	{
		auto const it = _objects.find(handle);
		if (it == _objects.end())
		{
			throw std::invalid_argument("Object not found");
		}
		return *it->second;
	}

	void destroyObject(Handle const handle) noexcept
	{
		_objects.erase(handle);
	}

	void clear() noexcept
	{
		_objects.clear();
	}

	Objects const& getObjects() const noexcept
	{
		return _objects;
	}

private:
	Objects _objects{};
};

namespace fromCppToC
{
struct scoped_avdecc_entity
{
	avdecc_entity_t entity{};
	std::list<avdecc_entity_interface_information_t> nextInterfaces{};
};
void set_macAddress(networkInterface::MacAddress const& source, avdecc_mac_address_t& macAddress) noexcept;
avdecc_entity_common_information_t make_entity_common_information(entity::Entity::CommonInformation const& source) noexcept;
avdecc_entity_interface_information_t make_entity_interface_information(entity::model::AvbInterfaceIndex const index, entity::Entity::InterfaceInformation const& source) noexcept;
scoped_avdecc_entity make_entity(entity::Entity const& source) noexcept;
avdecc_protocol_adpdu_t make_adpdu(protocol::Adpdu const& source) noexcept;
avdecc_protocol_aem_aecpdu_t make_aem_aecpdu(protocol::AemAecpdu const& source) noexcept;
avdecc_protocol_mvu_aecpdu_t make_mvu_aecpdu(protocol::MvuAecpdu const& source) noexcept;
avdecc_protocol_acmpdu_t make_acmpdu(protocol::Acmpdu const& source) noexcept;
avdecc_protocol_interface_error_t convertProtocolInterfaceErrorCode(protocol::ProtocolInterface::Error const error) noexcept;
avdecc_local_entity_advertise_flags_t convertLocalEntityAdvertiseFlags(entity::LocalEntity::AdvertiseFlags const flags) noexcept;
avdecc_entity_model_stream_identification_t make_stream_identification(entity::model::StreamIdentification const& source) noexcept;
std::vector<avdecc_entity_model_audio_mapping_t> make_audio_mappings(entity::model::AudioMappings const& mappings) noexcept;
std::vector<avdecc_entity_model_audio_mapping_p> make_audio_mappings_pointer(std::vector<avdecc_entity_model_audio_mapping_t>& mappings) noexcept;
avdecc_entity_model_stream_info_t make_stream_info(entity::model::StreamInfo const& info) noexcept;
avdecc_entity_model_avb_info_t make_avb_info(entity::model::AvbInfo const& info) noexcept;
std::vector<avdecc_entity_model_msrp_mapping_t> make_msrp_mappings(entity::model::MsrpMappings const& mappings) noexcept;
std::vector<avdecc_entity_model_msrp_mapping_p> make_msrp_mappings_pointer(std::vector<avdecc_entity_model_msrp_mapping_t>& mappings) noexcept;
avdecc_entity_model_as_path_t make_as_path(entity::model::AsPath const& path) noexcept;
std::vector<avdecc_unique_identifier_t> make_path_sequence(entity::model::PathSequence const& path) noexcept;
std::vector<avdecc_unique_identifier_t*> make_path_sequence_pointer(std::vector<avdecc_unique_identifier_t>& path) noexcept;
avdecc_entity_model_milan_info_t make_milan_info(entity::model::MilanInfo const& info) noexcept;
avdecc_entity_model_entity_descriptor_t make_entity_descriptor(entity::model::EntityDescriptor const& descriptor) noexcept;
avdecc_entity_model_configuration_descriptor_t make_configuration_descriptor(entity::model::ConfigurationDescriptor const& descriptor) noexcept;
std::vector<avdecc_entity_model_descriptors_count_t> make_descriptors_count(std::unordered_map<entity::model::DescriptorType, std::uint16_t, utils::EnumClassHash> const& counts) noexcept;
std::vector<avdecc_entity_model_descriptors_count_p> make_descriptors_count_pointer(std::vector<avdecc_entity_model_descriptors_count_t>& counts) noexcept;
avdecc_entity_model_audio_unit_descriptor_t make_audio_unit_descriptor(entity::model::AudioUnitDescriptor const& descriptor) noexcept;
std::vector<avdecc_entity_model_sampling_rate_t> make_sampling_rates(std::set<entity::model::SamplingRate> const& samplingRates) noexcept;
std::vector<avdecc_entity_model_sampling_rate_t*> make_sampling_rates_pointer(std::vector<avdecc_entity_model_sampling_rate_t>& samplingRates) noexcept;
avdecc_entity_model_stream_descriptor_t make_stream_descriptor(entity::model::StreamDescriptor const& descriptor) noexcept;
std::vector<avdecc_entity_model_stream_format_t> make_stream_formats(std::set<entity::model::StreamFormat> const& streamFormats) noexcept;
std::vector<avdecc_entity_model_stream_format_t*> make_stream_formats_pointer(std::vector<avdecc_entity_model_stream_format_t>& streamFormats) noexcept;
std::vector<avdecc_entity_model_descriptor_index_t> make_redundant_stream_indexes(std::set<entity::model::StreamIndex> const& streamIndexes) noexcept;
std::vector<avdecc_entity_model_descriptor_index_t*> make_redundant_stream_indexes_pointer(std::vector<avdecc_entity_model_descriptor_index_t>& streamIndexes) noexcept;
avdecc_entity_model_jack_descriptor_t make_jack_descriptor(entity::model::JackDescriptor const& descriptor) noexcept;
avdecc_entity_model_avb_interface_descriptor_t make_avb_interface_descriptor(entity::model::AvbInterfaceDescriptor const& descriptor) noexcept;
avdecc_entity_model_clock_source_descriptor_t make_clock_source_descriptor(entity::model::ClockSourceDescriptor const& descriptor) noexcept;
avdecc_entity_model_memory_object_descriptor_t make_memory_object_descriptor(entity::model::MemoryObjectDescriptor const& descriptor) noexcept;
avdecc_entity_model_locale_descriptor_t make_locale_descriptor(entity::model::LocaleDescriptor const& descriptor) noexcept;
avdecc_entity_model_strings_descriptor_t make_strings_descriptor(entity::model::StringsDescriptor const& descriptor) noexcept;
avdecc_entity_model_stream_port_descriptor_t make_stream_port_descriptor(entity::model::StreamPortDescriptor const& descriptor) noexcept;
avdecc_entity_model_external_port_descriptor_t make_external_port_descriptor(entity::model::ExternalPortDescriptor const& descriptor) noexcept;
avdecc_entity_model_internal_port_descriptor_t make_internal_port_descriptor(entity::model::InternalPortDescriptor const& descriptor) noexcept;
avdecc_entity_model_audio_cluster_descriptor_t make_audio_cluster_descriptor(entity::model::AudioClusterDescriptor const& descriptor) noexcept;
avdecc_entity_model_audio_map_descriptor_t make_audio_map_descriptor(entity::model::AudioMapDescriptor const& descriptor) noexcept;
avdecc_entity_model_clock_domain_descriptor_t make_clock_domain_descriptor(entity::model::ClockDomainDescriptor const& descriptor) noexcept;
std::vector<avdecc_entity_model_descriptor_index_t> make_clock_sources(std::vector<entity::model::ClockSourceIndex> const& clockSources) noexcept;
std::vector<avdecc_entity_model_descriptor_index_t*> make_clock_sources_pointer(std::vector<avdecc_entity_model_descriptor_index_t>& clockSources) noexcept;

} // namespace fromCppToC

namespace fromCToCpp
{
networkInterface::MacAddress make_macAddress(avdecc_mac_address_cp const macAddress) noexcept;
entity::Entity::CommonInformation make_entity_common_information(avdecc_entity_common_information_t const& commonInfo) noexcept;
entity::Entity::InterfaceInformation make_entity_interface_information(avdecc_entity_interface_information_t const& interfaceInfo) noexcept;
//entity::AggregateEntity::UniquePointer make_local_entity(avdecc_entity_t const& entity);
protocol::Adpdu make_adpdu(avdecc_protocol_adpdu_cp const adpdu) noexcept;
protocol::AemAecpdu make_aem_aecpdu(avdecc_protocol_aem_aecpdu_cp const aecpdu) noexcept;
protocol::AemAecpdu::UniquePointer make_aem_aecpdu_unique(avdecc_protocol_aem_aecpdu_cp const aecpdu) noexcept;
protocol::MvuAecpdu make_mvu_aecpdu(avdecc_protocol_mvu_aecpdu_cp const aecpdu) noexcept;
protocol::MvuAecpdu::UniquePointer make_mvu_aecpdu_unique(avdecc_protocol_mvu_aecpdu_cp const aecpdu) noexcept;
protocol::Acmpdu make_acmpdu(avdecc_protocol_acmpdu_cp const acmpdu) noexcept;
protocol::Acmpdu::UniquePointer make_acmpdu_unique(avdecc_protocol_acmpdu_cp const acmpdu) noexcept;
entity::LocalEntity::AdvertiseFlags convertLocalEntityAdvertiseFlags(avdecc_local_entity_advertise_flags_t const flags) noexcept;
entity::model::StreamIdentification make_stream_identification(avdecc_entity_model_stream_identification_cp const stream) noexcept;
entity::model::AudioMappings make_audio_mappings(avdecc_entity_model_audio_mapping_cp const* const mappings) noexcept;
entity::model::StreamInfo make_stream_info(avdecc_entity_model_stream_info_cp const info) noexcept;

} // namespace fromCToCpp

} // namespace bindings
} // namespace avdecc
} // namespace la
