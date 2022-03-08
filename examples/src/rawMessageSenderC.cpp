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
* @file rawMessageSenderC.cpp
* @author Christophe Calmejane
*/

/** *********************************************************************************** **/
/** Example sending raw messages using a ProtocolInterface C bindings (very low level)  **/
/** *********************************************************************************** **/

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <vector>
#include <thread>
#include <chrono>
#include <future>
#include <cstdint>
#include <cstring> // memcpy
#include <la/avdecc/avdecc.h>
#include <la/avdecc/internals/protocolAvtpdu.hpp> // SerializationBuffer
#include "utils.hpp"

static auto constexpr s_ProgID = std::uint16_t{ 5 };
static auto const s_TargetEntityID = la::avdecc::UniqueIdentifier{ 0x001b92fffe01b930 };
static auto const s_ListenerEntityID = la::avdecc::UniqueIdentifier{ 0x001b92fffe01b930 };
static auto const s_TalkerEntityID = la::avdecc::UniqueIdentifier{ 0x1b92fffe02233b };
static auto const s_TargetMacAddress = la::networkInterface::MacAddress{ 0x00, 0x1b, 0x92, 0x01, 0xb9, 0x30 };

inline void protocolInterface_sendRawMessages(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle)
{
	avdecc_mac_address_t piAddress{};
	LA_AVDECC_ProtocolInterface_getMacAddress(handle, &piAddress);
	auto const controllerID = LA_AVDECC_generateEID(&piAddress, s_ProgID);

	// Send raw ADP message (Entity Available message)
	{
		auto adpdu = avdecc_protocol_adpdu_t{};

		// Set Ether2 fields
		LA_AVDECC_ProtocolInterface_getMacAddress(handle, &adpdu.src_address);
		std::memcpy(&adpdu.dest_address, LA_AVDECC_Protocol_Adpdu_getMulticastMacAddress(), sizeof(adpdu.dest_address));
		// Set ADP fields
		adpdu.message_type = avdecc_protocol_adp_message_type_entity_available;
		adpdu.valid_time = 10;
		adpdu.entity_id = controllerID;
		adpdu.entity_model_id = LA_AVDECC_getNullUniqueIdentifier();
		adpdu.entity_capabilities = avdecc_entity_entity_capabilities_none;
		adpdu.talker_stream_sources = 0u;
		adpdu.talker_capabilities = avdecc_entity_talker_capabilities_none;
		adpdu.listener_stream_sinks = 0u;
		adpdu.listener_capabilities = avdecc_entity_listener_capabilities_none;
		adpdu.controller_capabilities = avdecc_entity_controller_capabilities_implemented;
		adpdu.available_index = 0u;
		adpdu.gptp_grandmaster_id = LA_AVDECC_getNullUniqueIdentifier();
		adpdu.gptp_domain_number = 0u;
		adpdu.identify_control_index = 0u;
		adpdu.interface_index = 0u;
		adpdu.association_id = LA_AVDECC_getNullUniqueIdentifier();

		// Send the message
		LA_AVDECC_ProtocolInterface_sendAdpMessage(handle, &adpdu);
	}

	// Send raw ACMP message (Connect Stream Command)
	{
		auto acmpdu = avdecc_protocol_acmpdu_t{};

		// Set Ether2 fields
		LA_AVDECC_ProtocolInterface_getMacAddress(handle, &acmpdu.src_address);
		std::memcpy(&acmpdu.dest_address, LA_AVDECC_Protocol_Acmpdu_getMulticastMacAddress(), sizeof(acmpdu.dest_address));
		// Set AVTPControl fields
		acmpdu.stream_id = LA_AVDECC_getNullUniqueIdentifier();
		// Set ACMP fields
		acmpdu.message_type = avdecc_protocol_acmp_message_type_connect_rx_command;
		acmpdu.status = avdecc_protocol_acmp_status_success;
		acmpdu.controller_entity_id = controllerID;
		acmpdu.talker_entity_id = s_TalkerEntityID;
		acmpdu.listener_entity_id = s_ListenerEntityID;
		acmpdu.talker_unique_id = 0u;
		acmpdu.listener_unique_id = 0u;
		memset(&acmpdu.stream_dest_address, 0, sizeof(acmpdu.stream_dest_address));
		acmpdu.connection_count = 0u;
		acmpdu.sequence_id = 0u;
		acmpdu.flags = avdecc_entity_connection_flags_streaming_wait;
		acmpdu.stream_vlan_id = 0u;

		// Send the message
		LA_AVDECC_ProtocolInterface_sendAcmpMessage(handle, &acmpdu);
	}

	// Send raw AEM AECP message (Lock Command)
	{
		auto aecpdu = avdecc_protocol_aem_aecpdu_t{};

		// Set Ether2 fields
		LA_AVDECC_ProtocolInterface_getMacAddress(handle, &aecpdu.src_address);
		std::memcpy(&aecpdu.dest_address, &s_TargetMacAddress, sizeof(aecpdu.dest_address));
		// Set AECP fields
		aecpdu.message_type = avdecc_protocol_aecp_message_type_aem_command;
		aecpdu.status = avdecc_protocol_aem_status_success;
		aecpdu.target_entity_id = s_TargetEntityID;
		aecpdu.controller_entity_id = controllerID;
		aecpdu.sequence_id = 0u;
		// Set AEM fields
		aecpdu.unsolicited = avdecc_bool_false;
		aecpdu.command_type = avdecc_protocol_aem_command_type_lock_entity;
		{
			auto buffer = la::avdecc::protocol::SerializationBuffer{};

			// Manually fill the AEM payload
			buffer << static_cast<std::uint32_t>(0u /* Lock Flags */) << static_cast<std::uint64_t>(0u /* LockedID */) << static_cast<std::uint16_t>(0u /* DescriptorType */) << static_cast<std::uint16_t>(0u /* DescriptorIndex */); // Lock payload

			std::memcpy(aecpdu.command_specific, buffer.data(), std::min(sizeof(aecpdu.command_specific), buffer.size()));
			aecpdu.command_specific_length = static_cast<decltype(aecpdu.command_specific_length)>(buffer.size());
		}

		// Send the message
		LA_AVDECC_ProtocolInterface_sendAemAecpMessage(handle, &aecpdu);
	}
}

static auto protocolInterface_sendControllerCommands_AcmpCommandResultPromise = std::promise<void>{};
inline void LA_AVDECC_BINDINGS_C_CALL_CONVENTION protocolInterface_sendControllerCommands_AcmpResponse(avdecc_protocol_acmpdu_cp const /*response*/, avdecc_protocol_interface_error_t const error)
{
	outputText("Got ACMP response with status: " + std::to_string(error) + "\n");
	protocolInterface_sendControllerCommands_AcmpCommandResultPromise.set_value();
}

static auto protocolInterface_sendControllerCommands_AemAecpCommandResultPromise = std::promise<void>{};
inline void LA_AVDECC_BINDINGS_C_CALL_CONVENTION protocolInterface_sendControllerCommands_AemAecpResponse(avdecc_protocol_aem_aecpdu_cp const /*response*/, avdecc_protocol_interface_error_t const error)
{
	outputText("Got AECP response with status: " + std::to_string(error) + "\n");
	protocolInterface_sendControllerCommands_AemAecpCommandResultPromise.set_value();
}

static auto protocolInterface_sendControllerCommands_MvuAecpCommandResultPromise = std::promise<void>{};
inline void LA_AVDECC_BINDINGS_C_CALL_CONVENTION protocolInterface_sendControllerCommands_MvuAecpResponse(avdecc_protocol_mvu_aecpdu_cp const /*response*/, avdecc_protocol_interface_error_t const error)
{
	outputText("Got AECP response with status: " + std::to_string(error) + "\n");
	protocolInterface_sendControllerCommands_MvuAecpCommandResultPromise.set_value();
}

inline void protocolInterface_sendControllerCommands(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle)
{
	// Generate an EID
	avdecc_mac_address_t piAddress{};

	LA_AVDECC_ProtocolInterface_getMacAddress(handle, &piAddress);
	auto const controllerID = LA_AVDECC_generateEID(&piAddress, s_ProgID);

	// In order to be allowed to send Commands, we have to declare ourself as a Controller type LocalEntity
	auto const commonInformation = avdecc_entity_common_information_s{ controllerID, LA_AVDECC_getNullUniqueIdentifier(), avdecc_entity_entity_capabilities_none, 0u, avdecc_entity_talker_capabilities_none, 0u, avdecc_entity_listener_capabilities_none, avdecc_entity_controller_capabilities_implemented, avdecc_bool_false, 0u, avdecc_bool_false, LA_AVDECC_getNullUniqueIdentifier() };
	auto interfaceInfo = avdecc_entity_interface_information_s{ LA_AVDECC_getGlobalAvbInterfaceIndex(), {}, 31u, 0u, avdecc_bool_false, LA_AVDECC_getNullUniqueIdentifier(), avdecc_bool_false, 0u, nullptr };
	std::memcpy(interfaceInfo.mac_address, piAddress, sizeof(interfaceInfo.mac_address)); // Need to copy the macAddress, cannot be initialized statically
	auto const entity = avdecc_entity_s{ commonInformation, interfaceInfo };

	// Create a LocalEntity
	auto localEntityHandle = LA_AVDECC_INVALID_HANDLE;
	{
		auto const error = LA_AVDECC_LocalEntity_create(handle, &entity, nullptr, &localEntityHandle);
		if (error != avdecc_local_entity_error_no_error)
		{
			outputText("Error creating local entity: " + std::to_string(error) + "\n");
			return;
		}
	}
	auto localEntityHandleGuard = Guard<LA_AVDECC_LOCAL_ENTITY_HANDLE, avdecc_local_entity_error_t, LA_AVDECC_LocalEntity_destroy>{ localEntityHandle };

	// Send ACMP command (Disconnect Stream)
	{
		auto acmpdu = avdecc_protocol_acmpdu_t{};

		// Set Ether2 fields
		LA_AVDECC_ProtocolInterface_getMacAddress(handle, &acmpdu.src_address);
		std::memcpy(&acmpdu.dest_address, LA_AVDECC_Protocol_Acmpdu_getMulticastMacAddress(), sizeof(acmpdu.dest_address));
		// Set AVTPControl fields
		acmpdu.stream_id = LA_AVDECC_getNullUniqueIdentifier();
		// Set ACMP fields
		acmpdu.message_type = avdecc_protocol_acmp_message_type_disconnect_rx_command;
		acmpdu.status = avdecc_protocol_acmp_status_success;
		acmpdu.controller_entity_id = entity.common_information.entity_id;
		acmpdu.talker_entity_id = s_TalkerEntityID;
		acmpdu.listener_entity_id = s_ListenerEntityID;
		acmpdu.talker_unique_id = 0u;
		acmpdu.listener_unique_id = 0u;
		memset(&acmpdu.stream_dest_address, 0, sizeof(acmpdu.stream_dest_address));
		acmpdu.connection_count = 0u;
		acmpdu.sequence_id = 0u;
		acmpdu.flags = avdecc_entity_connection_flags_none;
		acmpdu.stream_vlan_id = 0u;

		// Send the message
		auto const error = LA_AVDECC_ProtocolInterface_sendAcmpCommand(handle, &acmpdu, protocolInterface_sendControllerCommands_AcmpResponse);
		if (error != avdecc_protocol_interface_error_no_error)
		{
			outputText("Error sending ACMP command: " + std::to_string(error) + "\n");
		}
		else
		{
			// Wait for the command result
			auto status = protocolInterface_sendControllerCommands_AcmpCommandResultPromise.get_future().wait_for(std::chrono::seconds(20));
			if (status == std::future_status::timeout)
			{
				outputText("ACMP command timed out\n");
			}
		}
	}

	// Send AEM AECP message (Release Command)
	{
		auto aecpdu = avdecc_protocol_aem_aecpdu_t{};

		// Set Ether2 fields
		LA_AVDECC_ProtocolInterface_getMacAddress(handle, &aecpdu.src_address);
		std::memcpy(&aecpdu.dest_address, &s_TargetMacAddress, sizeof(aecpdu.dest_address));
		// Set AECP fields
		aecpdu.message_type = avdecc_protocol_aecp_message_type_aem_command; // Optional, automatically forced
		aecpdu.status = avdecc_protocol_aem_status_success;
		aecpdu.target_entity_id = s_TargetEntityID;
		aecpdu.controller_entity_id = controllerID;
		aecpdu.sequence_id = 0u;
		// Set AEM fields
		aecpdu.unsolicited = avdecc_bool_false;
		aecpdu.command_type = avdecc_protocol_aem_command_type_lock_entity;
		{
			auto buffer = la::avdecc::protocol::SerializationBuffer{};

			// Manually fill the AEM payload
			buffer << static_cast<std::uint32_t>(0x00000001 /* Lock Flags: Release */) << static_cast<std::uint64_t>(0u /* LockedID */) << static_cast<std::uint16_t>(0u /* DescriptorType */) << static_cast<std::uint16_t>(0u /* DescriptorIndex */); // Lock payload

			std::memcpy(aecpdu.command_specific, buffer.data(), std::min(sizeof(aecpdu.command_specific), buffer.size()));
			aecpdu.command_specific_length = static_cast<decltype(aecpdu.command_specific_length)>(buffer.size());
		}

		// Send the message
		auto const error = LA_AVDECC_ProtocolInterface_sendAemAecpCommand(handle, &aecpdu, protocolInterface_sendControllerCommands_AemAecpResponse);
		if (error != avdecc_protocol_interface_error_no_error)
		{
			outputText("Error sending AECP command: " + std::to_string(error) + "\n");
		}
		else
		{
			// Wait for the command result
			auto status = protocolInterface_sendControllerCommands_AemAecpCommandResultPromise.get_future().wait_for(std::chrono::seconds(20));
			if (status == std::future_status::timeout)
			{
				outputText("AECP command timed out\n");
			}
		}
	}

	// Send MVU AECP message (GetMilanInfo)
	{
		auto aecpdu = avdecc_protocol_mvu_aecpdu_t{};

		// Set Ether2 fields
		LA_AVDECC_ProtocolInterface_getMacAddress(handle, &aecpdu.src_address);
		std::memcpy(&aecpdu.dest_address, &s_TargetMacAddress, sizeof(aecpdu.dest_address));
		// Set AECP fields
		aecpdu.message_type = avdecc_protocol_aecp_message_type_vendor_unique_command; // Optional, automatically forced
		aecpdu.status = avdecc_protocol_mvu_status_success;
		aecpdu.target_entity_id = s_TargetEntityID;
		aecpdu.controller_entity_id = controllerID;
		aecpdu.sequence_id = 0u;
		// Set MVU fields
		aecpdu.command_type = avdecc_protocol_mvu_command_type_get_milan_info;
		{
			auto buffer = la::avdecc::protocol::SerializationBuffer{};

			// Manually fill the MVU payload
			buffer << static_cast<std::uint16_t>(0x0000 /* Reserved Field */);

			std::memcpy(aecpdu.command_specific, buffer.data(), std::min(sizeof(aecpdu.command_specific), buffer.size()));
			aecpdu.command_specific_length = static_cast<decltype(aecpdu.command_specific_length)>(buffer.size());
		}

		// Send the message
		auto const error = LA_AVDECC_ProtocolInterface_sendMvuAecpCommand(handle, &aecpdu, protocolInterface_sendControllerCommands_MvuAecpResponse);
		if (error != avdecc_protocol_interface_error_no_error)
		{
			outputText("Error sending AECP command: " + std::to_string(error) + "\n");
		}
		else
		{
			// Wait for the command result
			auto status = protocolInterface_sendControllerCommands_MvuAecpCommandResultPromise.get_future().wait_for(std::chrono::seconds(20));
			if (status == std::future_status::timeout)
			{
				outputText("AECP command timed out\n");
			}
		}
	}

	// Enable Entity Advertising
	{
		auto const error = LA_AVDECC_ProtocolInterface_enableEntityAdvertising(handle, localEntityHandle);
		if (error != avdecc_protocol_interface_error_no_error)
		{
			outputText("Error enabling entity advertising: " + std::to_string(error) + "\n");
			return;
		}

		outputText("Advertising enabled, waiting a bit\n");
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}

	// Release the dynamic entityID we generated for our controller
	{
		auto const error = LA_AVDECC_ProtocolInterface_releaseDynamicEID(handle, controllerID);
		if (error != avdecc_protocol_interface_error_no_error)
		{
			outputText("Error releasing dynamic EID: " + std::to_string(error) + "\n");
			return;
		}
	}
}

inline void LA_AVDECC_BINDINGS_C_CALL_CONVENTION onLocalEntityOnline(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const /*handle*/, avdecc_entity_cp const /*entity*/)
{
	outputText("Found a local entity\n");
}

inline void LA_AVDECC_BINDINGS_C_CALL_CONVENTION onRemoteEntityOnline(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const /*handle*/, avdecc_entity_cp const /*entity*/)
{
	outputText("Found a remote entity\n");
}

inline void protocolInterface_discovery(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle)
{
	auto obs = avdecc_protocol_interface_observer_s{ nullptr, &onLocalEntityOnline, nullptr, nullptr, &onRemoteEntityOnline, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

	// Register an observer
	LA_AVDECC_ProtocolInterface_registerObserver(handle, &obs);

	// Send a discovery message
	auto const error = LA_AVDECC_ProtocolInterface_discoverRemoteEntities(handle);
	if (error != avdecc_protocol_interface_error_no_error)
	{
		outputText("Error sending discover remote entities: " + std::to_string(error) + "\n");
		return;
	}

	// Wait a bit for an entity to respond
	outputText("Waiting a bit for entities to be discovered\n");
	std::this_thread::sleep_for(std::chrono::seconds(5));

	// Unregister observer
	LA_AVDECC_ProtocolInterface_unregisterObserver(handle, &obs);
}

inline void LA_AVDECC_BINDINGS_C_CALL_CONVENTION onEntityOnline(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_entity_cp const /*entity*/)
{
	outputText("Found an entity (either local or remote)\n");
}

static auto LA_AVDECC_LocalEntity_lockEntityCommandResultPromise = std::promise<void>{};
inline void LA_AVDECC_BINDINGS_C_CALL_CONVENTION LA_AVDECC_LocalEntity_lockEntityResponse(LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const status, avdecc_unique_identifier_t const /*lockingEntity*/, avdecc_entity_model_descriptor_type_t const /*descriptorType*/, avdecc_entity_model_descriptor_index_t const /*descriptorIndex*/)
{
	outputText("Got Lock Entity response with status: " + std::to_string(status) + "\n");
	LA_AVDECC_LocalEntity_lockEntityCommandResultPromise.set_value();
}

inline void localEntity_test(LA_AVDECC_PROTOCOL_INTERFACE_HANDLE const handle)
{
	auto delegate = avdecc_local_entity_controller_delegate_t{};
	delegate.onEntityOnline = onEntityOnline;

	// Generate an EID
	avdecc_mac_address_t piAddress{};

	LA_AVDECC_ProtocolInterface_getMacAddress(handle, &piAddress);
	auto const controllerID = LA_AVDECC_generateEID(&piAddress, s_ProgID);

	// In order to be allowed to send Commands, we have to declare ourself as a Controller type LocalEntity
	auto const commonInformation = avdecc_entity_common_information_s{ controllerID, LA_AVDECC_getNullUniqueIdentifier(), avdecc_entity_entity_capabilities_none, 0u, avdecc_entity_talker_capabilities_none, 0u, avdecc_entity_listener_capabilities_none, avdecc_entity_controller_capabilities_implemented, avdecc_bool_false, 0u, avdecc_bool_false, LA_AVDECC_getNullUniqueIdentifier() };
	auto interfaceInfo = avdecc_entity_interface_information_s{ LA_AVDECC_getGlobalAvbInterfaceIndex(), {}, 31u, 0u, avdecc_bool_false, LA_AVDECC_getNullUniqueIdentifier(), avdecc_bool_false, 0u, nullptr };
	std::memcpy(interfaceInfo.mac_address, piAddress, sizeof(interfaceInfo.mac_address)); // Need to copy the macAddress, cannot be initialized statically
	auto const entity = avdecc_entity_s{ commonInformation, interfaceInfo };

	// Create a LocalEntity with a delegate
	auto localEntityHandle = LA_AVDECC_INVALID_HANDLE;
	{
		auto const error = LA_AVDECC_LocalEntity_create(handle, &entity, &delegate, &localEntityHandle);
		if (error != avdecc_local_entity_error_no_error)
		{
			outputText("Error creating local entity: " + std::to_string(error) + "\n");
			return;
		}
	}
	auto localEntityHandleGuard = Guard<LA_AVDECC_LOCAL_ENTITY_HANDLE, avdecc_local_entity_error_t, LA_AVDECC_LocalEntity_destroy>{ localEntityHandle };

	// Send a discovery message
	{
		auto const error = LA_AVDECC_ProtocolInterface_discoverRemoteEntities(handle);
		if (error != avdecc_protocol_interface_error_no_error)
		{
			outputText("Error sending discover remote entities: " + std::to_string(error) + "\n");
			return;
		}

		// Wait a bit for an entity to respond
		outputText("Waiting a bit for entities to be discovered\n");
		std::this_thread::sleep_for(std::chrono::seconds(5));
	}

	// Send an Lock command
	{
		auto const error = LA_AVDECC_LocalEntity_lockEntity(localEntityHandle, s_TargetEntityID, avdecc_entity_model_descriptor_type_entity, 0u, LA_AVDECC_LocalEntity_lockEntityResponse);
		if (error != avdecc_local_entity_error_no_error)
		{
			outputText("Error sending Lock entity command: " + std::to_string(error) + "\n");
		}
		else
		{
			// Wait for the command result
			auto status = LA_AVDECC_LocalEntity_lockEntityCommandResultPromise.get_future().wait_for(std::chrono::seconds(2));
			if (status == std::future_status::timeout)
			{
				outputText("Lock Entity command timed out\n");
			}
		}
	}
#if defined(_WIN32) && !defined(__clang__)
	// Too lazy to use a static method for these tests' result handlers (only way for it to properly compile on linux/macOS)
	{
		static auto commandResultPromise = std::promise<void>{};
		auto const error = LA_AVDECC_LocalEntity_getMilanInfo(localEntityHandle, s_TargetEntityID,
			[](LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_mvu_command_status_t const status, avdecc_entity_model_milan_info_cp const info)
			{
				outputText("GetMilanInfo response: " + std::to_string(status) + "\n");
				outputText(" - Proto: " + std::to_string(info->protocol_version) + "\n");
				outputText(" - Flags: " + std::to_string(info->features_flags) + "\n");
				outputText(" - Cert: " + la::avdecc::utils::toHexString(info->certification_version, true) + "\n");
				commandResultPromise.set_value();
			});
		if (error != avdecc_local_entity_error_no_error)
		{
			outputText("Error sending GetMilanInfo command: " + std::to_string(error) + "\n");
		}
		else
		{
			auto status = commandResultPromise.get_future().wait_for(std::chrono::seconds(2));
			if (status == std::future_status::timeout)
			{
				outputText("GetMilanInfo command timed out\n");
			}
		}
	}
	{
		static auto commandResultPromise = std::promise<void>{};
		auto const error = LA_AVDECC_LocalEntity_readEntityDescriptor(localEntityHandle, s_TargetEntityID,
			[](LA_AVDECC_LOCAL_ENTITY_HANDLE const /*handle*/, avdecc_unique_identifier_t const /*entityID*/, avdecc_local_entity_aem_command_status_t const status, avdecc_entity_model_entity_descriptor_cp const descriptor)
			{
				outputText("ReadEntityDescriptor response: " + std::to_string(status) + "\n");
				outputText(" - entity_id: " + la::avdecc::utils::toHexString(descriptor->entity_id, true) + "\n");
				outputText(" - entity_model_id: " + la::avdecc::utils::toHexString(descriptor->entity_model_id, true) + "\n");
				outputText(" - entity_capabilities: " + la::avdecc::utils::toHexString(descriptor->entity_capabilities, true) + "\n");
				outputText(" - talker_stream_sources: " + std::to_string(descriptor->talker_stream_sources) + "\n");
				outputText(" - talker_capabilities: " + la::avdecc::utils::toHexString(descriptor->talker_capabilities, true) + "\n");
				outputText(" - listener_stream_sinks: " + std::to_string(descriptor->listener_stream_sinks) + "\n");
				outputText(" - listener_capabilities: " + la::avdecc::utils::toHexString(descriptor->listener_capabilities, true) + "\n");
				outputText(" - controller_capabilities: " + la::avdecc::utils::toHexString(descriptor->controller_capabilities, true) + "\n");
				outputText(" - available_index: " + std::to_string(descriptor->available_index) + "\n");
				outputText(" - association_id: " + la::avdecc::utils::toHexString(descriptor->association_id, true) + "\n");
				outputText(" - entity_name: " + std::to_string(*descriptor->entity_name) + "\n");
				outputText(" - vendor_name_string: " + std::to_string(descriptor->vendor_name_string) + "\n");
				outputText(" - model_name_string: " + std::to_string(descriptor->model_name_string) + "\n");
				outputText(" - firmware_version: " + std::to_string(*descriptor->firmware_version) + "\n");
				outputText(" - group_name: " + std::to_string(*descriptor->group_name) + "\n");
				outputText(" - serial_number: " + std::to_string(*descriptor->serial_number) + "\n");
				outputText(" - configurations_count: " + std::to_string(descriptor->configurations_count) + "\n");
				outputText(" - current_configuration: " + std::to_string(descriptor->current_configuration) + "\n");
				commandResultPromise.set_value();
			});
		if (error != avdecc_local_entity_error_no_error)
		{
			outputText("Error sending ReadEntityDescriptor command: " + std::to_string(error) + "\n");
		}
		else
		{
			auto status = commandResultPromise.get_future().wait_for(std::chrono::seconds(2));
			if (status == std::future_status::timeout)
			{
				outputText("ReadEntityDescriptor command timed out\n");
			}
		}
	}
#endif // _WIN32

	// Let our local entity be destroyed
}

static int doJob()
{
	auto const protocolInterfaceType = chooseProtocolInterfaceType();
	auto intfc = chooseNetworkInterface();

	if (intfc.type == la::networkInterface::Interface::Type::None || protocolInterfaceType == avdecc_protocol_interface_type_none)
	{
		return 1;
	}

	// Create an Executor
	auto executorHandle = LA_AVDECC_INVALID_HANDLE;
	{
		auto const error = LA_AVDECC_Executor_createQueueExecutor(LA_AVDECC_ProtocolInterface_getDefaultExecutorName(), &executorHandle);
		if (error != avdecc_executor_error_no_error)
		{
			outputText("Error creating executor: " + std::to_string(error) + "\n");
			return 1;
		}
	}
	auto executorHandleGuard = Guard<LA_AVDECC_EXECUTOR_WRAPPER_HANDLE, avdecc_executor_error_t, &LA_AVDECC_Executor_destroy>{ executorHandle };

	// We need to create/destroy the protocol interface for each test, as the protocol interface will not trigger events for already discovered entities
	{
		// Create a ProtocolInterface
		auto protocolInterfaceHandle = LA_AVDECC_INVALID_HANDLE;
		{
			auto const error = LA_AVDECC_ProtocolInterface_create(protocolInterfaceType, intfc.id.c_str(), &protocolInterfaceHandle);
			if (error != avdecc_protocol_interface_error_no_error)
			{
				outputText("Error creating protocol interface: " + std::to_string(error) + "\n");
				return 1;
			}
		}
		auto protocolInterfaceHandleGuard = Guard<LA_AVDECC_PROTOCOL_INTERFACE_HANDLE, avdecc_protocol_interface_error_t, &LA_AVDECC_ProtocolInterface_destroy>{ protocolInterfaceHandle };

		// Test ProtocolInterface discovery messages
		protocolInterface_discovery(protocolInterfaceHandle);
	}

	{
		// Create a ProtocolInterface
		auto protocolInterfaceHandle = LA_AVDECC_INVALID_HANDLE;
		{
			auto const error = LA_AVDECC_ProtocolInterface_create(protocolInterfaceType, intfc.id.c_str(), &protocolInterfaceHandle);
			if (error != avdecc_protocol_interface_error_no_error)
			{
				outputText("Error creating protocol interface: " + std::to_string(error) + "\n");
				return 1;
			}
		}
		auto protocolInterfaceHandleGuard = Guard<LA_AVDECC_PROTOCOL_INTERFACE_HANDLE, avdecc_protocol_interface_error_t, &LA_AVDECC_ProtocolInterface_destroy>{ protocolInterfaceHandle };

		// Test sending raw messages
		protocolInterface_sendRawMessages(protocolInterfaceHandle);
	}

	{
		// Create a ProtocolInterface
		auto protocolInterfaceHandle = LA_AVDECC_INVALID_HANDLE;
		{
			auto const error = LA_AVDECC_ProtocolInterface_create(protocolInterfaceType, intfc.id.c_str(), &protocolInterfaceHandle);
			if (error != avdecc_protocol_interface_error_no_error)
			{
				outputText("Error creating protocol interface: " + std::to_string(error) + "\n");
				return 1;
			}
		}
		auto protocolInterfaceHandleGuard = Guard<LA_AVDECC_PROTOCOL_INTERFACE_HANDLE, avdecc_protocol_interface_error_t, &LA_AVDECC_ProtocolInterface_destroy>{ protocolInterfaceHandle };

		// Test sending controller type messages (commands)
		protocolInterface_sendControllerCommands(protocolInterfaceHandle);
	}

	{
		// Create a ProtocolInterface
		auto protocolInterfaceHandle = LA_AVDECC_INVALID_HANDLE;
		{
			auto const error = LA_AVDECC_ProtocolInterface_create(protocolInterfaceType, intfc.id.c_str(), &protocolInterfaceHandle);
			if (error != avdecc_protocol_interface_error_no_error)
			{
				outputText("Error creating protocol interface: " + std::to_string(error) + "\n");
				return 1;
			}
		}
		auto protocolInterfaceHandleGuard = Guard<LA_AVDECC_PROTOCOL_INTERFACE_HANDLE, avdecc_protocol_interface_error_t, &LA_AVDECC_ProtocolInterface_destroy>{ protocolInterfaceHandle };

		// Test LocalEntity messages
		localEntity_test(protocolInterfaceHandle);
	}

	return 0;
}

int main()
{
	// Check avdecc library interface version
	if (!LA_AVDECC_isCompatibleWithInterfaceVersion(LA_AVDECC_InterfaceVersion))
	{
		outputText(std::string("Avdecc shared library interface version invalid:\nCompiled with interface ") + std::to_string(LA_AVDECC_InterfaceVersion) + " (v" + std::string(LA_AVDECC_getVersion()) + "), but running interface " + std::to_string(LA_AVDECC_getInterfaceVersion()) + "\n");
		getch();
		return -1;
	}

	LA_AVDECC_initialize();

	initOutput();

	outputText(std::string("Using Avdecc C Wrapper Library v") + LA_AVDECC_getVersion() + "\n");

	auto ret = doJob();
	if (ret != 0)
	{
		outputText("\nTerminating with an error. Press any key to close\n");
		getch();
	}

	deinitOutput();

	LA_AVDECC_uninitialize();

	return ret;
}
