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
* @file protocolDefines.hpp
* @author Christophe Calmejane
* @brief Avdecc protocol (IEEE Std 1722.1) types and constants.
*/

#pragma once

#include <cstdint>
#include "la/avdecc/utils.hpp"
#include "exports.hpp"

namespace la
{
namespace avdecc
{
namespace protocol
{

/* Global protocol defines */
static constexpr std::uint16_t EthernetMaxFrameSize{ 1522 };
static constexpr std::uint16_t AvtpEtherType{ 0x22f0 };
static constexpr std::uint16_t AvtpMaxPayloadLength{ 1500 };
static constexpr std::uint8_t AvtpVersion{ 0x00 };
static constexpr std::uint8_t AvtpSubType_Adp{ 0x7a };
static constexpr std::uint8_t AvtpSubType_Aecp{ 0x7b };
static constexpr std::uint8_t AvtpSubType_Acmp{ 0x7c };
static constexpr std::uint8_t AvtpSubType_Maap{ 0x7e };
static constexpr std::uint8_t AvtpSubType_Experimental{ 0x7f };

/** ADP Message Type - Clause 6.2.1.5 */
class AdpMessageType : public TypedDefine<std::uint8_t>
{
public:
	using TypedDefine::TypedDefine;

	static AdpMessageType const EntityAvailable;
	static AdpMessageType const EntityDeparting;
	static AdpMessageType const EntityDiscover;

	operator std::string() const noexcept;
};

/** AECP Message Type - Clause 9.2.1.1.5 */
class AecpMessageType : public TypedDefine<std::uint8_t>
{
public:
	using TypedDefine::TypedDefine;

	static AecpMessageType const AemCommand;
	static AecpMessageType const AemResponse;
	static AecpMessageType const AddressAccessCommand;
	static AecpMessageType const AddressAccessResponse;
	static AecpMessageType const AvcCommand;
	static AecpMessageType const AvcResponse;
	static AecpMessageType const VendorUniqueCommand;
	static AecpMessageType const VendorUniqueResponse;
	static AecpMessageType const HdcpAemCommand;
	static AecpMessageType const HdcpAemResponse;
	static AecpMessageType const ExtendedCommand;
	static AecpMessageType const ExtendedResponse;

	operator std::string() const noexcept;
};

/** AECP Status - Clause 9.2.1.1.6 */
class AecpStatus : public TypedDefine<std::uint8_t>
{
public:
	using TypedDefine::TypedDefine;

	static AecpStatus const Success;
	static AecpStatus const NotImplemented;
};

/** AECP SequenceID - Clause 9.2.1.1.10 */
using AecpSequenceID = std::uint16_t;

/** AEM AECP Status - Clause 7.4 */
class AemAecpStatus : public AecpStatus
{
public:
	using AecpStatus::AecpStatus;

	static AemAecpStatus const NoSuchDescriptor;
	static AemAecpStatus const EntityLocked;
	static AemAecpStatus const EntityAcquired;
	static AemAecpStatus const NotAuthenticated;
	static AemAecpStatus const AuthenticationDisabled;
	static AemAecpStatus const BadArguments;
	static AemAecpStatus const NoResources;
	static AemAecpStatus const InProgress;
	static AemAecpStatus const EntityMisbehaving;
	static AemAecpStatus const NotSupported;
	static AemAecpStatus const StreamIsRunning;
};

/** AEM Command Type - Clause 7.4 */
class AemCommandType : public TypedDefine<std::uint16_t>
{
public:
	using TypedDefine::TypedDefine;

	static AemCommandType const AcquireEntity;
	static AemCommandType const LockEntity;
	static AemCommandType const EntityAvailable;
	static AemCommandType const ControllerAvailable;
	static AemCommandType const ReadDescriptor;
	static AemCommandType const WriteDescriptor;
	static AemCommandType const SetConfiguration;
	static AemCommandType const GetConfiguration;
	static AemCommandType const SetStreamFormat;
	static AemCommandType const GetStreamFormat;
	static AemCommandType const SetVideoFormat;
	static AemCommandType const GetVideoFormat;
	static AemCommandType const SetSensorFormat;
	static AemCommandType const GetSensorFormat;
	static AemCommandType const SetStreamInfo;
	static AemCommandType const GetStreamInfo;
	static AemCommandType const SetName;
	static AemCommandType const GetName;
	static AemCommandType const SetAssociationID;
	static AemCommandType const GetAssociationID;
	static AemCommandType const SetSamplingRate;
	static AemCommandType const GetSamplingRate;
	static AemCommandType const SetClockSource;
	static AemCommandType const GetClockSource;
	static AemCommandType const SetControl;
	static AemCommandType const GetControl;
	static AemCommandType const IncrementControl;
	static AemCommandType const DecrementControl;
	static AemCommandType const SetSignalSelector;
	static AemCommandType const GetSignalSelector;
	static AemCommandType const SetMixer;
	static AemCommandType const GetMixer;
	static AemCommandType const SetMatrix;
	static AemCommandType const GetMatrix;
	static AemCommandType const StartStreaming;
	static AemCommandType const StopStreaming;
	static AemCommandType const RegisterUnsolicitedNotification;
	static AemCommandType const DeregisterUnsolicitedNotification;
	static AemCommandType const IdentifyNotification;
	static AemCommandType const GetAvbInfo;
	static AemCommandType const GetAsPath;
	static AemCommandType const GetCounters;
	static AemCommandType const Reboot;
	static AemCommandType const GetAudioMap;
	static AemCommandType const AddAudioMappings;
	static AemCommandType const RemoveAudioMappings;
	static AemCommandType const GetVideoMap;
	static AemCommandType const AddVideoMappings;
	static AemCommandType const RemoveVideoMappings;
	static AemCommandType const GetSensorMap;
	static AemCommandType const AddSensorMappings;
	static AemCommandType const RemoveSensorMappings;
	static AemCommandType const StartOperation;
	static AemCommandType const AbortOperation;
	static AemCommandType const OperationStatus;
	static AemCommandType const AuthAddKey;
	static AemCommandType const AuthDeleteKey;
	static AemCommandType const AuthGetKeyList;
	static AemCommandType const AuthGetKey;
	static AemCommandType const AuthAddKeyToChain;
	static AemCommandType const AuthDeleteKeyFromChain;
	static AemCommandType const AuthGetKeychainList;
	static AemCommandType const AuthGetIdentity;
	static AemCommandType const AuthAddToken;
	static AemCommandType const AuthDeleteToken;
	static AemCommandType const Authenticate;
	static AemCommandType const Deauthenticate;
	static AemCommandType const EnableTransportSecurity;
	static AemCommandType const DisableTransportSecurity;
	static AemCommandType const EnableStreamEncryption;
	static AemCommandType const DisableStreamEncryption;
	static AemCommandType const SetMemoryObjectLength;
	static AemCommandType const GetMemoryObjectLength;
	static AemCommandType const SetStreamBackup;
	static AemCommandType const GetStreamBackup;
	static AemCommandType const Expansion;

	static AemCommandType const InvalidCommandType;

	operator std::string() const noexcept;
};

/** AEM Acquire Entity Flags - Clause 7.4.1.1 */
class AemAcquireEntityFlags : public TypedDefine<std::uint32_t>
{
public:
	using TypedDefine::TypedDefine;

	static AemAcquireEntityFlags const None;
	static AemAcquireEntityFlags const Persistent;
	static AemAcquireEntityFlags const Release;
};

/** AEM Lock Entity Flags - Clause 7.4.2.1 */
class AemLockEntityFlags : public TypedDefine<std::uint32_t>
{
public:
	using TypedDefine::TypedDefine;

	static AemLockEntityFlags const None;
	static AemLockEntityFlags const Unlock;
};

/** ACMP Message Type - Clause 8.2.1.5 */
class AcmpMessageType : public TypedDefine<std::uint8_t>
{
public:
	using TypedDefine::TypedDefine;

	static AcmpMessageType const ConnectTxCommand;
	static AcmpMessageType const ConnectTxResponse;
	static AcmpMessageType const DisconnectTxCommand;
	static AcmpMessageType const DisconnectTxResponse;
	static AcmpMessageType const GetTxStateCommand;
	static AcmpMessageType const GetTxStateResponse;
	static AcmpMessageType const ConnectRxCommand;
	static AcmpMessageType const ConnectRxResponse;
	static AcmpMessageType const DisconnectRxCommand;
	static AcmpMessageType const DisconnectRxResponse;
	static AcmpMessageType const GetRxStateCommand;
	static AcmpMessageType const GetRxStateResponse;
	static AcmpMessageType const GetTxConnectionCommand;
	static AcmpMessageType const GetTxConnectionResponse;

	operator std::string() const noexcept;
};

/** ACMP Status - Clause 8.2.1.6 */
class AcmpStatus : public TypedDefine<std::uint8_t>
{
public:
	using TypedDefine::TypedDefine;

	static AcmpStatus const Success;
	static AcmpStatus const ListenerUnknownID;
	static AcmpStatus const TalkerUnknownID;
	static AcmpStatus const TalkerDestMacFail;
	static AcmpStatus const TalkerNoStreamIndex;
	static AcmpStatus const TalkerNoBandwidth;
	static AcmpStatus const TalkerExclusive;
	static AcmpStatus const ListenerTalkerTimeout;
	static AcmpStatus const ListenerExclusive;
	static AcmpStatus const StateUnavailable;
	static AcmpStatus const NotConnected;
	static AcmpStatus const NoSuchConnection;
	static AcmpStatus const CouldNotSendMessage;
	static AcmpStatus const TalkerMisbehaving;
	static AcmpStatus const ListenerMisbehaving;
	static AcmpStatus const ControllerNotAuthorized;
	static AcmpStatus const IncompatibleRequest;
	static AcmpStatus const NotSupported;
};

/** ACMP UniqueID - Clause 8.2.1.12 and 8.2.1.13 */
using AcmpUniqueID = std::uint16_t;

/** ACMP SequenceID - Clause 8.2.1.16 */
using AcmpSequenceID = std::uint16_t;

} // namespace protocol

// Define bitfield TypedDefine traits for AemAcquireEntityFlags
template<>
struct typed_define_traits<protocol::AemAcquireEntityFlags>
{
	static constexpr bool is_bitfield = true;
};

// Define bitfield TypedDefine traits for AemLockEntityFlags
template<>
struct typed_define_traits<protocol::AemLockEntityFlags>
{
	static constexpr bool is_bitfield = true;
};

} // namespace avdecc
} // namespace la
