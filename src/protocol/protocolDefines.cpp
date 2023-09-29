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
* @file protocolDefines.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/internals/protocolDefines.hpp"
#include "la/avdecc/internals/protocolAaAecpdu.hpp"

#include <unordered_map>

namespace la
{
namespace avdecc
{
namespace protocol
{
/* Global protocol defines */
std::uint16_t const AaAecpMaxSingleTlvMemoryDataLength{ Aecpdu::MaximumSendLength - Aecpdu::HeaderLength - AaAecpdu::HeaderLength - AaAecpdu::TlvHeaderLength }; /* Maximum individual TLV memory_data length in commands */

/** ADP Message Type - IEEE1722.1-2013 Clause 6.2.1.5 */
AdpMessageType const AdpMessageType::EntityAvailable{ 0 };
AdpMessageType const AdpMessageType::EntityDeparting{ 1 };
AdpMessageType const AdpMessageType::EntityDiscover{ 2 };
/* 3-15 reserved for future use */

AdpMessageType::operator std::string() const noexcept
{
	static std::unordered_map<AdpMessageType::value_type, std::string> s_AdpMessageTypeMapping = {
		{ AdpMessageType::EntityAvailable.getValue(), "ENTITY_AVAILABLE" },
		{ AdpMessageType::EntityDeparting.getValue(), "ENTITY_DEPARTING" },
		{ AdpMessageType::EntityDiscover.getValue(), "ENTITY_DISCOVER" },
	};

	auto const& it = s_AdpMessageTypeMapping.find(getValue());
	if (it == s_AdpMessageTypeMapping.end())
		return "INVALID_MESSAGE_TYPE";
	return it->second;
}

/** AECP Message Type - IEEE1722.1-2013 Clause 9.2.1.1.5 */
AecpMessageType const AecpMessageType::AemCommand{ 0 };
AecpMessageType const AecpMessageType::AemResponse{ 1 };
AecpMessageType const AecpMessageType::AddressAccessCommand{ 2 };
AecpMessageType const AecpMessageType::AddressAccessResponse{ 3 };
AecpMessageType const AecpMessageType::AvcCommand{ 4 };
AecpMessageType const AecpMessageType::AvcResponse{ 5 };
AecpMessageType const AecpMessageType::VendorUniqueCommand{ 6 };
AecpMessageType const AecpMessageType::VendorUniqueResponse{ 7 };
AecpMessageType const AecpMessageType::HdcpAemCommand{ 8 };
AecpMessageType const AecpMessageType::HdcpAemResponse{ 9 };
/* 10-13 reserved for future use */
AecpMessageType const AecpMessageType::ExtendedCommand{ 14 };
AecpMessageType const AecpMessageType::ExtendedResponse{ 15 };

AecpMessageType::operator std::string() const noexcept
{
	static std::unordered_map<AecpMessageType::value_type, std::string> s_AecpMessageTypeMapping = {
		{ AecpMessageType::AemCommand.getValue(), "AEM_COMMAND" },
		{ AecpMessageType::AemResponse.getValue(), "AEM_RESPONSE" },
		{ AecpMessageType::AddressAccessCommand.getValue(), "ADDRESS_ACCESS_COMMAND" },
		{ AecpMessageType::AddressAccessResponse.getValue(), "ADDRESS_ACCESS_RESPONSE" },
		{ AecpMessageType::AvcCommand.getValue(), "AVC_COMMAND" },
		{ AecpMessageType::AvcResponse.getValue(), "AVC_RESPONSE" },
		{ AecpMessageType::VendorUniqueCommand.getValue(), "VENDOR_UNIQUE_COMMAND" },
		{ AecpMessageType::VendorUniqueResponse.getValue(), "VENDOR_UNIQUE_RESPONSE" },
		{ AecpMessageType::HdcpAemCommand.getValue(), "HDCP_AEM_COMMAND" },
		{ AecpMessageType::HdcpAemResponse.getValue(), "HDCP_AEM_RESPONSE" },
		{ AecpMessageType::ExtendedCommand.getValue(), "EXTENDED_COMMAND" },
		{ AecpMessageType::ExtendedResponse.getValue(), "EXTENDED_RESPONSE" },
	};

	auto const& it = s_AecpMessageTypeMapping.find(getValue());
	if (it == s_AecpMessageTypeMapping.end())
		return "INVALID_MESSAGE_TYPE";
	return it->second;
}

/** AECP Status - IEEE1722.1-2013 Clause 9.2.1.1.6 */
AecpStatus const AecpStatus::Success{ 0 };
AecpStatus const AecpStatus::NotImplemented{ 1 };
/* 2-31 defined by message type */

AecpStatus::operator std::string() const noexcept
{
	static std::unordered_map<AecpStatus::value_type, std::string> s_AecpStatusMapping = {
		{ AecpStatus::Success.getValue(), "SUCCESS" },
		{ AecpStatus::NotImplemented.getValue(), "NOT_IMPLEMENTED" },
	};

	auto const& it = s_AecpStatusMapping.find(getValue());
	if (it == s_AecpStatusMapping.end())
		return "INVALID_STATUS";
	return it->second;
}

/** AEM AECP Status - IEEE1722.1-2013 Clause 7.4 */
LA_AVDECC_API AemAecpStatus::AemAecpStatus(AecpStatus const status) noexcept
	: AecpStatus{ status }
{
}

AemAecpStatus const AemAecpStatus::NoSuchDescriptor{ 2 };
AemAecpStatus const AemAecpStatus::EntityLocked{ 3 };
AemAecpStatus const AemAecpStatus::EntityAcquired{ 4 };
AemAecpStatus const AemAecpStatus::NotAuthenticated{ 5 };
AemAecpStatus const AemAecpStatus::AuthenticationDisabled{ 6 };
AemAecpStatus const AemAecpStatus::BadArguments{ 7 };
AemAecpStatus const AemAecpStatus::NoResources{ 8 };
AemAecpStatus const AemAecpStatus::InProgress{ 9 };
AemAecpStatus const AemAecpStatus::EntityMisbehaving{ 10 };
AemAecpStatus const AemAecpStatus::NotSupported{ 11 };
AemAecpStatus const AemAecpStatus::StreamIsRunning{ 12 };
/* 13-31 reserved for future use */

AemAecpStatus::operator std::string() const noexcept
{
	static std::unordered_map<AemAecpStatus::value_type, std::string> s_AemAecpStatusMapping = {
		{ AemAecpStatus::NoSuchDescriptor.getValue(), "NO_SUCH_DESCRIPTOR" },
		{ AemAecpStatus::EntityLocked.getValue(), "ENTITY_LOCKED" },
		{ AemAecpStatus::EntityAcquired.getValue(), "ENTITY_ACQUIRED" },
		{ AemAecpStatus::NotAuthenticated.getValue(), "NOT_AUTHENTICATED" },
		{ AemAecpStatus::AuthenticationDisabled.getValue(), "AUTHENTICATION_DISABLED" },
		{ AemAecpStatus::BadArguments.getValue(), "BAD_ARGUMENTS" },
		{ AemAecpStatus::NoResources.getValue(), "NO_RESOURCES" },
		{ AemAecpStatus::InProgress.getValue(), "IN_PROGRESS" },
		{ AemAecpStatus::EntityMisbehaving.getValue(), "ENTITY_MISBEHAVING" },
		{ AemAecpStatus::NotSupported.getValue(), "NOT_SUPPORTED" },
		{ AemAecpStatus::StreamIsRunning.getValue(), "STREAM_IS_RUNNING" },
	};

	auto const& it = s_AemAecpStatusMapping.find(getValue());
	if (it == s_AemAecpStatusMapping.end())
		return "INVALID_STATUS";
	return it->second;
}

/** AEM Command Type - IEEE1722.1-2013 Clause 7.4 */
AemCommandType const AemCommandType::AcquireEntity{ 0x0000 };
AemCommandType const AemCommandType::LockEntity{ 0x0001 };
AemCommandType const AemCommandType::EntityAvailable{ 0x0002 };
AemCommandType const AemCommandType::ControllerAvailable{ 0x0003 };
AemCommandType const AemCommandType::ReadDescriptor{ 0x0004 };
AemCommandType const AemCommandType::WriteDescriptor{ 0x0005 };
AemCommandType const AemCommandType::SetConfiguration{ 0x0006 };
AemCommandType const AemCommandType::GetConfiguration{ 0x0007 };
AemCommandType const AemCommandType::SetStreamFormat{ 0x0008 };
AemCommandType const AemCommandType::GetStreamFormat{ 0x0009 };
AemCommandType const AemCommandType::SetVideoFormat{ 0x000a };
AemCommandType const AemCommandType::GetVideoFormat{ 0x000b };
AemCommandType const AemCommandType::SetSensorFormat{ 0x000c };
AemCommandType const AemCommandType::GetSensorFormat{ 0x000d };
AemCommandType const AemCommandType::SetStreamInfo{ 0x000e };
AemCommandType const AemCommandType::GetStreamInfo{ 0x000f };
AemCommandType const AemCommandType::SetName{ 0x0010 };
AemCommandType const AemCommandType::GetName{ 0x0011 };
AemCommandType const AemCommandType::SetAssociationID{ 0x0012 };
AemCommandType const AemCommandType::GetAssociationID{ 0x0013 };
AemCommandType const AemCommandType::SetSamplingRate{ 0x0014 };
AemCommandType const AemCommandType::GetSamplingRate{ 0x0015 };
AemCommandType const AemCommandType::SetClockSource{ 0x0016 };
AemCommandType const AemCommandType::GetClockSource{ 0x0017 };
AemCommandType const AemCommandType::SetControl{ 0x0018 };
AemCommandType const AemCommandType::GetControl{ 0x0019 };
AemCommandType const AemCommandType::IncrementControl{ 0x001a };
AemCommandType const AemCommandType::DecrementControl{ 0x001b };
AemCommandType const AemCommandType::SetSignalSelector{ 0x001c };
AemCommandType const AemCommandType::GetSignalSelector{ 0x001d };
AemCommandType const AemCommandType::SetMixer{ 0x001e };
AemCommandType const AemCommandType::GetMixer{ 0x001f };
AemCommandType const AemCommandType::SetMatrix{ 0x0020 };
AemCommandType const AemCommandType::GetMatrix{ 0x0021 };
AemCommandType const AemCommandType::StartStreaming{ 0x0022 };
AemCommandType const AemCommandType::StopStreaming{ 0x0023 };
AemCommandType const AemCommandType::RegisterUnsolicitedNotification{ 0x0024 };
AemCommandType const AemCommandType::DeregisterUnsolicitedNotification{ 0x0025 };
AemCommandType const AemCommandType::IdentifyNotification{ 0x0026 };
AemCommandType const AemCommandType::GetAvbInfo{ 0x0027 };
AemCommandType const AemCommandType::GetAsPath{ 0x0028 };
AemCommandType const AemCommandType::GetCounters{ 0x0029 };
AemCommandType const AemCommandType::Reboot{ 0x002a };
AemCommandType const AemCommandType::GetAudioMap{ 0x002b };
AemCommandType const AemCommandType::AddAudioMappings{ 0x002c };
AemCommandType const AemCommandType::RemoveAudioMappings{ 0x002d };
AemCommandType const AemCommandType::GetVideoMap{ 0x002e };
AemCommandType const AemCommandType::AddVideoMappings{ 0x002f };
AemCommandType const AemCommandType::RemoveVideoMappings{ 0x0030 };
AemCommandType const AemCommandType::GetSensorMap{ 0x0031 };
AemCommandType const AemCommandType::AddSensorMappings{ 0x0032 };
AemCommandType const AemCommandType::RemoveSensorMappings{ 0x0033 };
AemCommandType const AemCommandType::StartOperation{ 0x0034 };
AemCommandType const AemCommandType::AbortOperation{ 0x0035 };
AemCommandType const AemCommandType::OperationStatus{ 0x0036 };
AemCommandType const AemCommandType::AuthAddKey{ 0x0037 };
AemCommandType const AemCommandType::AuthDeleteKey{ 0x0038 };
AemCommandType const AemCommandType::AuthGetKeyList{ 0x0039 };
AemCommandType const AemCommandType::AuthGetKey{ 0x003a };
AemCommandType const AemCommandType::AuthAddKeyToChain{ 0x003b };
AemCommandType const AemCommandType::AuthDeleteKeyFromChain{ 0x003c };
AemCommandType const AemCommandType::AuthGetKeychainList{ 0x003d };
AemCommandType const AemCommandType::AuthGetIdentity{ 0x003e };
AemCommandType const AemCommandType::AuthAddToken{ 0x003f };
AemCommandType const AemCommandType::AuthDeleteToken{ 0x0040 };
AemCommandType const AemCommandType::Authenticate{ 0x0041 };
AemCommandType const AemCommandType::Deauthenticate{ 0x0042 };
AemCommandType const AemCommandType::EnableTransportSecurity{ 0x0043 };
AemCommandType const AemCommandType::DisableTransportSecurity{ 0x0044 };
AemCommandType const AemCommandType::EnableStreamEncryption{ 0x0045 };
AemCommandType const AemCommandType::DisableStreamEncryption{ 0x0046 };
AemCommandType const AemCommandType::SetMemoryObjectLength{ 0x0047 };
AemCommandType const AemCommandType::GetMemoryObjectLength{ 0x0048 };
AemCommandType const AemCommandType::SetStreamBackup{ 0x0049 };
AemCommandType const AemCommandType::GetStreamBackup{ 0x004a };
/* 0x004b-0x7ffe reserved for future use */
AemCommandType const AemCommandType::Expansion{ 0x7fff };

AemCommandType const AemCommandType::InvalidCommandType{ 0xffff };

AemCommandType::operator std::string() const noexcept
{
	static std::unordered_map<AemCommandType::value_type, std::string> s_AemCommandTypeMapping = {
		{ AemCommandType::AcquireEntity.getValue(), "ACQUIRE_ENTITY" },
		{ AemCommandType::LockEntity.getValue(), "LOCK_ENTITY" },
		{ AemCommandType::EntityAvailable.getValue(), "ENTITY_AVAILABLE" },
		{ AemCommandType::ControllerAvailable.getValue(), "CONTROLLER_AVAILABLE" },
		{ AemCommandType::ReadDescriptor.getValue(), "READ_DESCRIPTOR" },
		{ AemCommandType::WriteDescriptor.getValue(), "WRITE_DESCRIPTOR" },
		{ AemCommandType::SetConfiguration.getValue(), "SET_CONFIGURATION" },
		{ AemCommandType::GetConfiguration.getValue(), "GET_CONFIGURATION" },
		{ AemCommandType::SetStreamFormat.getValue(), "SET_STREAM_FORMAT" },
		{ AemCommandType::GetStreamFormat.getValue(), "GET_STREAM_FORMAT" },
		{ AemCommandType::SetVideoFormat.getValue(), "SET_VIDEO_FORMAT" },
		{ AemCommandType::GetVideoFormat.getValue(), "GET_VIDEO_FORMAT" },
		{ AemCommandType::SetSensorFormat.getValue(), "SET_SENSOR_FORMAT" },
		{ AemCommandType::GetSensorFormat.getValue(), "GET_SENSOR_FORMAT" },
		{ AemCommandType::SetStreamInfo.getValue(), "SET_STREAM_INFO" },
		{ AemCommandType::GetStreamInfo.getValue(), "GET_STREAM_INFO" },
		{ AemCommandType::SetName.getValue(), "SET_NAME" },
		{ AemCommandType::GetName.getValue(), "GET_NAME" },
		{ AemCommandType::SetAssociationID.getValue(), "SET_ASSOCIATION_ID" },
		{ AemCommandType::GetAssociationID.getValue(), "GET_ASSOCIATION_ID" },
		{ AemCommandType::SetSamplingRate.getValue(), "SET_SAMPLING_RATE" },
		{ AemCommandType::GetSamplingRate.getValue(), "GET_SAMPLING_RATE" },
		{ AemCommandType::SetClockSource.getValue(), "SET_CLOCK_SOURCE" },
		{ AemCommandType::GetClockSource.getValue(), "GET_CLOCK_SOURCE" },
		{ AemCommandType::SetControl.getValue(), "SET_CONTROL" },
		{ AemCommandType::GetControl.getValue(), "GET_CONTROL" },
		{ AemCommandType::IncrementControl.getValue(), "INCREMENT_CONTROL" },
		{ AemCommandType::DecrementControl.getValue(), "DECREMENT_CONTROL" },
		{ AemCommandType::SetSignalSelector.getValue(), "SET_SIGNAL_SELECTOR" },
		{ AemCommandType::GetSignalSelector.getValue(), "GET_SIGNAL_SELECTOR" },
		{ AemCommandType::SetMixer.getValue(), "SET_MIXER" },
		{ AemCommandType::GetMixer.getValue(), "GET_MIXER" },
		{ AemCommandType::SetMatrix.getValue(), "SET_MATRIX" },
		{ AemCommandType::GetMatrix.getValue(), "GET_MATRIX" },
		{ AemCommandType::StartStreaming.getValue(), "START_STREAMING" },
		{ AemCommandType::StopStreaming.getValue(), "STOP_STREAMING" },
		{ AemCommandType::RegisterUnsolicitedNotification.getValue(), "REGISTER_UNSOLICITED_NOTIFICATION" },
		{ AemCommandType::DeregisterUnsolicitedNotification.getValue(), "DEREGISTER_UNSOLICITED_NOTIFICATION" },
		{ AemCommandType::IdentifyNotification.getValue(), "IDENTIFY_NOTIFICATION" },
		{ AemCommandType::GetAvbInfo.getValue(), "GET_AVB_INFO" },
		{ AemCommandType::GetAsPath.getValue(), "GET_AS_PATH" },
		{ AemCommandType::GetCounters.getValue(), "GET_COUNTERS" },
		{ AemCommandType::Reboot.getValue(), "REBOOT" },
		{ AemCommandType::GetAudioMap.getValue(), "GET_AUDIO_MAP" },
		{ AemCommandType::AddAudioMappings.getValue(), "ADD_AUDIO_MAPPINGS" },
		{ AemCommandType::RemoveAudioMappings.getValue(), "REMOVE_AUDIO_MAPPINGS" },
		{ AemCommandType::GetVideoMap.getValue(), "GET_VIDEO_MAP" },
		{ AemCommandType::AddVideoMappings.getValue(), "ADD_VIDEO_MAPPINGS" },
		{ AemCommandType::RemoveVideoMappings.getValue(), "REMOVE_VIDEO_MAPPINGS" },
		{ AemCommandType::GetSensorMap.getValue(), "GET_SENSOR_MAP" },
		{ AemCommandType::AddSensorMappings.getValue(), "ADD_SENSOR_MAPPINGS" },
		{ AemCommandType::RemoveSensorMappings.getValue(), "REMOVE_SENSOR_MAPPINGS" },
		{ AemCommandType::StartOperation.getValue(), "START_OPERATION" },
		{ AemCommandType::AbortOperation.getValue(), "ABORT_OPERATION" },
		{ AemCommandType::OperationStatus.getValue(), "OPERATION_STATUS" },
		{ AemCommandType::AuthAddKey.getValue(), "AUTH_ADD_KEY" },
		{ AemCommandType::AuthDeleteKey.getValue(), "AUTH_DELETE_KEY" },
		{ AemCommandType::AuthGetKeyList.getValue(), "AUTH_GET_KEY_LIST" },
		{ AemCommandType::AuthGetKey.getValue(), "AUTH_GET_KEY" },
		{ AemCommandType::AuthAddKeyToChain.getValue(), "AUTH_ADD_KEY_TO_CHAIN" },
		{ AemCommandType::AuthDeleteKeyFromChain.getValue(), "AUTH_DELETE_KEY_FROM_CHAIN" },
		{ AemCommandType::AuthGetKeychainList.getValue(), "AUTH_GET_KEYCHAIN_LIST" },
		{ AemCommandType::AuthGetIdentity.getValue(), "AUTH_GET_IDENTITY" },
		{ AemCommandType::AuthAddToken.getValue(), "AUTH_ADD_TOKEN" },
		{ AemCommandType::AuthDeleteToken.getValue(), "AUTH_DELETE_TOKEN" },
		{ AemCommandType::Authenticate.getValue(), "AUTHENTICATE" },
		{ AemCommandType::Deauthenticate.getValue(), "DEAUTHENTICATE" },
		{ AemCommandType::EnableTransportSecurity.getValue(), "ENABLE_TRANSPORT_SECURITY" },
		{ AemCommandType::DisableTransportSecurity.getValue(), "DISABLE_TRANSPORT_SECURITY" },
		{ AemCommandType::EnableStreamEncryption.getValue(), "ENABLE_STREAM_ENCRYPTION" },
		{ AemCommandType::DisableStreamEncryption.getValue(), "DISABLE_STREAM_ENCRYPTION" },
		{ AemCommandType::SetMemoryObjectLength.getValue(), "SET_MEMORY_OBJECT_LENGTH" },
		{ AemCommandType::GetMemoryObjectLength.getValue(), "GET_MEMORY_OBJECT_LENGTH" },
		{ AemCommandType::SetStreamBackup.getValue(), "SET_STREAM_BACKUP" },
		{ AemCommandType::GetStreamBackup.getValue(), "GET_STREAM_BACKUP" },
		{ AemCommandType::Expansion.getValue(), "EXPANSION" },
		{ AemCommandType::InvalidCommandType.getValue(), "INVALID_COMMAND_TYPE" },
	};

	auto const& it = s_AemCommandTypeMapping.find(getValue());
	if (it == s_AemCommandTypeMapping.end())
		return "INVALID_COMMAND_TYPE";
	return it->second;
}

/** AEM Acquire Entity Flags - IEEE1722.1-2013 Clause 7.4.1.1 */
AemAcquireEntityFlags const AemAcquireEntityFlags::None{ 0x00000000 };
AemAcquireEntityFlags const AemAcquireEntityFlags::Persistent{ 0x00000001 };
AemAcquireEntityFlags const AemAcquireEntityFlags::Release{ 0x80000000 };

AemAcquireEntityFlags::operator std::string() const noexcept
{
	static std::unordered_map<AemAcquireEntityFlags::value_type, std::string> s_AemAcquireEntityFlagsMapping = {
		{ AemAcquireEntityFlags::None.getValue(), "NONE" },
		{ AemAcquireEntityFlags::Persistent.getValue(), "PERSISTENT" },
		{ AemAcquireEntityFlags::Release.getValue(), "RELEASE" },
	};

	auto const& it = s_AemAcquireEntityFlagsMapping.find(getValue());
	if (it == s_AemAcquireEntityFlagsMapping.end())
		return "INVALID_FLAGS";
	return it->second;
}

/** AEM Lock Entity Flags - IEEE1722.1-2013 Clause 7.4.2.1 */
AemLockEntityFlags const AemLockEntityFlags::None{ 0x00000000 };
AemLockEntityFlags const AemLockEntityFlags::Unlock{ 0x00000001 };

AemLockEntityFlags::operator std::string() const noexcept
{
	static std::unordered_map<AemLockEntityFlags::value_type, std::string> s_AemLockEntityFlagsMapping = {
		{ AemLockEntityFlags::None.getValue(), "NONE" },
		{ AemLockEntityFlags::Unlock.getValue(), "UNLOCK" },
	};

	auto const& it = s_AemLockEntityFlagsMapping.find(getValue());
	if (it == s_AemLockEntityFlagsMapping.end())
		return "INVALID_FLAGS";
	return it->second;
}

/** Address Access Mode - IEEE1722.1-2013 Clause 9.2.1.3.3 */
AaMode const AaMode::Read{ 0x0 };
AaMode const AaMode::Write{ 0x1 };
AaMode const AaMode::Execute{ 0x2 };
/* 0x3-0xf reserved for future use */

AaMode::operator std::string() const noexcept
{
	static std::unordered_map<AaMode::value_type, std::string> s_AaModeMapping = {
		{ AaMode::Read.getValue(), "READ" },
		{ AaMode::Write.getValue(), "WRITE" },
		{ AaMode::Execute.getValue(), "EXECUTE" },
	};

	auto const& it = s_AaModeMapping.find(getValue());
	if (it == s_AaModeMapping.end())
		return "INVALID_ADDRESS_ACCESS_MODE";
	return it->second;
}

/** Address Access AECP Status - IEEE1722.1-2013 Clause 9.2.1.3.4 */
AaAecpStatus const AaAecpStatus::AddressTooLow{ 2 };
AaAecpStatus const AaAecpStatus::AddressTooHigh{ 3 };
AaAecpStatus const AaAecpStatus::AddressInvalid{ 4 };
AaAecpStatus const AaAecpStatus::TlvInvalid{ 5 };
AaAecpStatus const AaAecpStatus::DataInvalid{ 6 };
AaAecpStatus const AaAecpStatus::Unsupported{ 7 };
/* 8-31 reserved for future use */

AaAecpStatus::operator std::string() const noexcept
{
	static std::unordered_map<AaAecpStatus::value_type, std::string> s_AaAecpStatusMapping = {
		{ AaAecpStatus::AddressTooLow.getValue(), "ADDRESS_TOO_LOW" },
		{ AaAecpStatus::AddressTooHigh.getValue(), "ADDRESS_TOO_HIGH" },
		{ AaAecpStatus::AddressInvalid.getValue(), "ADDRESS_INVALID" },
		{ AaAecpStatus::TlvInvalid.getValue(), "TLV_INVALID" },
		{ AaAecpStatus::DataInvalid.getValue(), "DATA_INVALID" },
		{ AaAecpStatus::Unsupported.getValue(), "UNSUPPORTED" },
	};

	auto const& it = s_AaAecpStatusMapping.find(getValue());
	if (it == s_AaAecpStatusMapping.end())
		return "INVALID_STATUS";
	return it->second;
}

/** Milan Vendor Unique AECP Status - Milan-2019 Clause 7.2.3 */
MvuAecpStatus::operator std::string() const noexcept
{
	static std::unordered_map<MvuAecpStatus::value_type, std::string> s_MvuAecpStatusMapping = {
		{ MvuAecpStatus::Success.getValue(), "SUCCESS" },
		{ MvuAecpStatus::NotImplemented.getValue(), "NOT_IMPLEMENTED" },
	};

	auto const& it = s_MvuAecpStatusMapping.find(getValue());
	if (it == s_MvuAecpStatusMapping.end())
		return "INVALID_STATUS";
	return it->second;
}

/** Milan Vendor Unique Command Type - Milan-2019 Clause 7.2.2.3 */
MvuCommandType const MvuCommandType::GetMilanInfo{ 0 };
MvuCommandType const MvuCommandType::InvalidCommandType{ 0xffff };

MvuCommandType::operator std::string() const noexcept
{
	static std::unordered_map<MvuCommandType::value_type, std::string> s_MvuCommandTypeMapping = {
		{ MvuCommandType::GetMilanInfo.getValue(), "GET_MILAN_INFO" },
		{ MvuCommandType::InvalidCommandType.getValue(), "INVALID_COMMAND_TYPE" },
	};

	auto const& it = s_MvuCommandTypeMapping.find(getValue());
	if (it == s_MvuCommandTypeMapping.end())
		return "INVALID_COMMAND_TYPE";
	return it->second;
}

/** ACMP Message Type - IEEE1722.1-2013 Clause 8.2.1.5 */
AcmpMessageType const AcmpMessageType::ConnectTxCommand{ 0 };
AcmpMessageType const AcmpMessageType::ConnectTxResponse{ 1 };
AcmpMessageType const AcmpMessageType::DisconnectTxCommand{ 2 };
AcmpMessageType const AcmpMessageType::DisconnectTxResponse{ 3 };
AcmpMessageType const AcmpMessageType::GetTxStateCommand{ 4 };
AcmpMessageType const AcmpMessageType::GetTxStateResponse{ 5 };
AcmpMessageType const AcmpMessageType::ConnectRxCommand{ 6 };
AcmpMessageType const AcmpMessageType::ConnectRxResponse{ 7 };
AcmpMessageType const AcmpMessageType::DisconnectRxCommand{ 8 };
AcmpMessageType const AcmpMessageType::DisconnectRxResponse{ 9 };
AcmpMessageType const AcmpMessageType::GetRxStateCommand{ 10 };
AcmpMessageType const AcmpMessageType::GetRxStateResponse{ 11 };
AcmpMessageType const AcmpMessageType::GetTxConnectionCommand{ 12 };
AcmpMessageType const AcmpMessageType::GetTxConnectionResponse{ 13 };
/* 14-15 reserved for future use */

AcmpMessageType::operator std::string() const noexcept
{
	static std::unordered_map<AcmpMessageType::value_type, std::string> s_AcmpMessageTypeMapping = {
		{ AcmpMessageType::ConnectTxCommand.getValue(), "CONNECT_TX_COMMAND" },
		{ AcmpMessageType::ConnectTxResponse.getValue(), "CONNECT_TX_RESPONSE" },
		{ AcmpMessageType::DisconnectTxCommand.getValue(), "DISCONNECT_TX_COMMAND" },
		{ AcmpMessageType::DisconnectTxResponse.getValue(), "DISCONNECT_TX_RESPONSE" },
		{ AcmpMessageType::GetTxStateCommand.getValue(), "GET_TX_STATE_COMMAND" },
		{ AcmpMessageType::GetTxStateResponse.getValue(), "GET_TX_STATE_RESPONSE" },
		{ AcmpMessageType::ConnectRxCommand.getValue(), "CONNECT_RX_COMMAND" },
		{ AcmpMessageType::ConnectRxResponse.getValue(), "CONNECT_RX_RESPONSE" },
		{ AcmpMessageType::DisconnectRxCommand.getValue(), "DISCONNECT_RX_COMMAND" },
		{ AcmpMessageType::DisconnectRxResponse.getValue(), "DISCONNECT_RX_RESPONSE" },
		{ AcmpMessageType::GetRxStateCommand.getValue(), "GET_RX_STATE_COMMAND" },
		{ AcmpMessageType::GetRxStateResponse.getValue(), "GET_RX_STATE_RESPONSE" },
		{ AcmpMessageType::GetTxConnectionCommand.getValue(), "GET_TX_CONNECTION_COMMAND" },
		{ AcmpMessageType::GetTxConnectionResponse.getValue(), "GET_TX_CONNECTION_RESPONSE" },
	};

	auto const& it = s_AcmpMessageTypeMapping.find(getValue());
	if (it == s_AcmpMessageTypeMapping.end())
		return "INVALID_MESSAGE_TYPE";
	return it->second;
}

/** ACMP Status - IEEE1722.1-2013 Clause 8.2.1.6 */
AcmpStatus const AcmpStatus::Success{ 0 };
AcmpStatus const AcmpStatus::ListenerUnknownID{ 1 };
AcmpStatus const AcmpStatus::TalkerUnknownID{ 2 };
AcmpStatus const AcmpStatus::TalkerDestMacFail{ 3 };
AcmpStatus const AcmpStatus::TalkerNoStreamIndex{ 4 };
AcmpStatus const AcmpStatus::TalkerNoBandwidth{ 5 };
AcmpStatus const AcmpStatus::TalkerExclusive{ 6 };
AcmpStatus const AcmpStatus::ListenerTalkerTimeout{ 7 };
AcmpStatus const AcmpStatus::ListenerExclusive{ 8 };
AcmpStatus const AcmpStatus::StateUnavailable{ 9 };
AcmpStatus const AcmpStatus::NotConnected{ 10 };
AcmpStatus const AcmpStatus::NoSuchConnection{ 11 };
AcmpStatus const AcmpStatus::CouldNotSendMessage{ 12 };
AcmpStatus const AcmpStatus::TalkerMisbehaving{ 13 };
AcmpStatus const AcmpStatus::ListenerMisbehaving{ 14 };
/* 15 reserved for future use */
AcmpStatus const AcmpStatus::ControllerNotAuthorized{ 16 };
AcmpStatus const AcmpStatus::IncompatibleRequest{ 17 };
/* 18-30 reserved for future use */
AcmpStatus const AcmpStatus::NotSupported{ 31 };

static std::unordered_map<AcmpStatus::value_type, std::string> s_AcmpStatusMapping = {
	{ AcmpStatus::Success.getValue(), "SUCCESS" },
	{ AcmpStatus::ListenerUnknownID.getValue(), "LISTENER_UNKNOWN_ID" },
	{ AcmpStatus::TalkerUnknownID.getValue(), "TALKER_UNKNOWN_ID" },
	{ AcmpStatus::TalkerDestMacFail.getValue(), "TALKER_DEST_MAC_FAIL" },
	{ AcmpStatus::TalkerNoStreamIndex.getValue(), "TALKER_NO_STREAM_INDEX" },
	{ AcmpStatus::TalkerNoBandwidth.getValue(), "TALKER_NO_BANDWIDTH" },
	{ AcmpStatus::TalkerExclusive.getValue(), "TALKER_EXCLUSIVE" },
	{ AcmpStatus::ListenerTalkerTimeout.getValue(), "LISTENER_TALKER_TIMEOUT" },
	{ AcmpStatus::ListenerExclusive.getValue(), "LISTENER_EXCLUSIVE" },
	{ AcmpStatus::StateUnavailable.getValue(), "STATE_UNAVAILABLE" },
	{ AcmpStatus::NotConnected.getValue(), "NOT_CONNECTED" },
	{ AcmpStatus::NoSuchConnection.getValue(), "NO_SUCH_CONNECTION" },
	{ AcmpStatus::CouldNotSendMessage.getValue(), "COULD_NOT_SEND_MESSAGE" },
	{ AcmpStatus::TalkerMisbehaving.getValue(), "TALKER_MISBEHAVING" },
	{ AcmpStatus::ListenerMisbehaving.getValue(), "LISTENER_MISBEHAVING" },
	/* 15 reserved for future use */
	{ AcmpStatus::ControllerNotAuthorized.getValue(), "CONTROLLER_NOT_AUTHORIZED" },
	{ AcmpStatus::IncompatibleRequest.getValue(), "INCOMPATIBLE_REQUEST" },
	/* 18-30 reserved for future use */
	{ AcmpStatus::NotSupported.getValue(), "NOT_SUPPORTED" },
};

AcmpStatus::AcmpStatus() noexcept
	: TypedDefine(AcmpStatus::NotSupported)
{
}

AcmpStatus::operator std::string() const noexcept
{
	auto const& it = s_AcmpStatusMapping.find(getValue());
	if (it == s_AcmpStatusMapping.end())
		return "INVALID_STATUS";
	return it->second;
}

void LA_AVDECC_CALL_CONVENTION AcmpStatus::fromString(std::string const& stringValue)
{
	for (auto const& [key, value] : s_AcmpStatusMapping)
	{
		if (value == stringValue)
		{
			setValue(key);
			return;
		}
	}
	throw std::invalid_argument("Unknown AcmpStatus string representation: " + stringValue);
}

} // namespace protocol
} // namespace avdecc
} // namespace la
