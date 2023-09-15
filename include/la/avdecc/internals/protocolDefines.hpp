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
* @file protocolDefines.hpp
* @author Christophe Calmejane
* @brief Avdecc protocol (IEEE Std 1722.1) types and constants.
*/

#pragma once

#include "la/avdecc/utils.hpp"

#include "exports.hpp"

#include <cstdint>

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
extern LA_AVDECC_API std::uint16_t const AaAecpMaxSingleTlvMemoryDataLength; /* Maximum individual TLV memory_data length in commands */

/** ADP Message Type - IEEE1722.1-2013 Clause 6.2.1.5 */
class AdpMessageType : public utils::TypedDefine<AdpMessageType, std::uint8_t>
{
public:
	using TypedDefine::TypedDefine;

	static LA_AVDECC_API AdpMessageType const EntityAvailable;
	static LA_AVDECC_API AdpMessageType const EntityDeparting;
	static LA_AVDECC_API AdpMessageType const EntityDiscover;

	LA_AVDECC_API operator std::string() const noexcept;
};

/** AECP Message Type - IEEE1722.1-2013 Clause 9.2.1.1.5 */
class AecpMessageType : public utils::TypedDefine<AecpMessageType, std::uint8_t>
{
public:
	using TypedDefine::TypedDefine;

	static LA_AVDECC_API AecpMessageType const AemCommand;
	static LA_AVDECC_API AecpMessageType const AemResponse;
	static LA_AVDECC_API AecpMessageType const AddressAccessCommand;
	static LA_AVDECC_API AecpMessageType const AddressAccessResponse;
	static LA_AVDECC_API AecpMessageType const AvcCommand;
	static LA_AVDECC_API AecpMessageType const AvcResponse;
	static LA_AVDECC_API AecpMessageType const VendorUniqueCommand;
	static LA_AVDECC_API AecpMessageType const VendorUniqueResponse;
	static LA_AVDECC_API AecpMessageType const HdcpAemCommand;
	static LA_AVDECC_API AecpMessageType const HdcpAemResponse;
	static LA_AVDECC_API AecpMessageType const ExtendedCommand;
	static LA_AVDECC_API AecpMessageType const ExtendedResponse;

	LA_AVDECC_API operator std::string() const noexcept;
};

/** AECP Status - IEEE1722.1-2013 Clause 9.2.1.1.6 */
class AecpStatus : public utils::TypedDefine<AecpStatus, std::uint8_t>
{
public:
	using TypedDefine::TypedDefine;

	static LA_AVDECC_API AecpStatus const Success;
	static LA_AVDECC_API AecpStatus const NotImplemented;

	LA_AVDECC_API operator std::string() const noexcept;
};

/** AECP SequenceID - IEEE1722.1-2013 Clause 9.2.1.1.10 */
using AecpSequenceID = std::uint16_t;

/** AEM AECP Status - IEEE1722.1-2013 Clause 7.4 */
class AemAecpStatus : public AecpStatus
{
public:
	using AecpStatus::AecpStatus;
	LA_AVDECC_API explicit AemAecpStatus(AecpStatus const status) noexcept;

	static LA_AVDECC_API AemAecpStatus const NoSuchDescriptor;
	static LA_AVDECC_API AemAecpStatus const EntityLocked;
	static LA_AVDECC_API AemAecpStatus const EntityAcquired;
	static LA_AVDECC_API AemAecpStatus const NotAuthenticated;
	static LA_AVDECC_API AemAecpStatus const AuthenticationDisabled;
	static LA_AVDECC_API AemAecpStatus const BadArguments;
	static LA_AVDECC_API AemAecpStatus const NoResources;
	static LA_AVDECC_API AemAecpStatus const InProgress;
	static LA_AVDECC_API AemAecpStatus const EntityMisbehaving;
	static LA_AVDECC_API AemAecpStatus const NotSupported;
	static LA_AVDECC_API AemAecpStatus const StreamIsRunning;

	LA_AVDECC_API operator std::string() const noexcept;
};

/** AEM Command Type - IEEE1722.1-2013 Clause 7.4 */
class AemCommandType : public utils::TypedDefine<AemCommandType, std::uint16_t>
{
public:
	using TypedDefine::TypedDefine;

	static LA_AVDECC_API AemCommandType const AcquireEntity;
	static LA_AVDECC_API AemCommandType const LockEntity;
	static LA_AVDECC_API AemCommandType const EntityAvailable;
	static LA_AVDECC_API AemCommandType const ControllerAvailable;
	static LA_AVDECC_API AemCommandType const ReadDescriptor;
	static LA_AVDECC_API AemCommandType const WriteDescriptor;
	static LA_AVDECC_API AemCommandType const SetConfiguration;
	static LA_AVDECC_API AemCommandType const GetConfiguration;
	static LA_AVDECC_API AemCommandType const SetStreamFormat;
	static LA_AVDECC_API AemCommandType const GetStreamFormat;
	static LA_AVDECC_API AemCommandType const SetVideoFormat;
	static LA_AVDECC_API AemCommandType const GetVideoFormat;
	static LA_AVDECC_API AemCommandType const SetSensorFormat;
	static LA_AVDECC_API AemCommandType const GetSensorFormat;
	static LA_AVDECC_API AemCommandType const SetStreamInfo;
	static LA_AVDECC_API AemCommandType const GetStreamInfo;
	static LA_AVDECC_API AemCommandType const SetName;
	static LA_AVDECC_API AemCommandType const GetName;
	static LA_AVDECC_API AemCommandType const SetAssociationID;
	static LA_AVDECC_API AemCommandType const GetAssociationID;
	static LA_AVDECC_API AemCommandType const SetSamplingRate;
	static LA_AVDECC_API AemCommandType const GetSamplingRate;
	static LA_AVDECC_API AemCommandType const SetClockSource;
	static LA_AVDECC_API AemCommandType const GetClockSource;
	static LA_AVDECC_API AemCommandType const SetControl;
	static LA_AVDECC_API AemCommandType const GetControl;
	static LA_AVDECC_API AemCommandType const IncrementControl;
	static LA_AVDECC_API AemCommandType const DecrementControl;
	static LA_AVDECC_API AemCommandType const SetSignalSelector;
	static LA_AVDECC_API AemCommandType const GetSignalSelector;
	static LA_AVDECC_API AemCommandType const SetMixer;
	static LA_AVDECC_API AemCommandType const GetMixer;
	static LA_AVDECC_API AemCommandType const SetMatrix;
	static LA_AVDECC_API AemCommandType const GetMatrix;
	static LA_AVDECC_API AemCommandType const StartStreaming;
	static LA_AVDECC_API AemCommandType const StopStreaming;
	static LA_AVDECC_API AemCommandType const RegisterUnsolicitedNotification;
	static LA_AVDECC_API AemCommandType const DeregisterUnsolicitedNotification;
	static LA_AVDECC_API AemCommandType const IdentifyNotification;
	static LA_AVDECC_API AemCommandType const GetAvbInfo;
	static LA_AVDECC_API AemCommandType const GetAsPath;
	static LA_AVDECC_API AemCommandType const GetCounters;
	static LA_AVDECC_API AemCommandType const Reboot;
	static LA_AVDECC_API AemCommandType const GetAudioMap;
	static LA_AVDECC_API AemCommandType const AddAudioMappings;
	static LA_AVDECC_API AemCommandType const RemoveAudioMappings;
	static LA_AVDECC_API AemCommandType const GetVideoMap;
	static LA_AVDECC_API AemCommandType const AddVideoMappings;
	static LA_AVDECC_API AemCommandType const RemoveVideoMappings;
	static LA_AVDECC_API AemCommandType const GetSensorMap;
	static LA_AVDECC_API AemCommandType const AddSensorMappings;
	static LA_AVDECC_API AemCommandType const RemoveSensorMappings;
	static LA_AVDECC_API AemCommandType const StartOperation;
	static LA_AVDECC_API AemCommandType const AbortOperation;
	static LA_AVDECC_API AemCommandType const OperationStatus;
	static LA_AVDECC_API AemCommandType const AuthAddKey;
	static LA_AVDECC_API AemCommandType const AuthDeleteKey;
	static LA_AVDECC_API AemCommandType const AuthGetKeyList;
	static LA_AVDECC_API AemCommandType const AuthGetKey;
	static LA_AVDECC_API AemCommandType const AuthAddKeyToChain;
	static LA_AVDECC_API AemCommandType const AuthDeleteKeyFromChain;
	static LA_AVDECC_API AemCommandType const AuthGetKeychainList;
	static LA_AVDECC_API AemCommandType const AuthGetIdentity;
	static LA_AVDECC_API AemCommandType const AuthAddToken;
	static LA_AVDECC_API AemCommandType const AuthDeleteToken;
	static LA_AVDECC_API AemCommandType const Authenticate;
	static LA_AVDECC_API AemCommandType const Deauthenticate;
	static LA_AVDECC_API AemCommandType const EnableTransportSecurity;
	static LA_AVDECC_API AemCommandType const DisableTransportSecurity;
	static LA_AVDECC_API AemCommandType const EnableStreamEncryption;
	static LA_AVDECC_API AemCommandType const DisableStreamEncryption;
	static LA_AVDECC_API AemCommandType const SetMemoryObjectLength;
	static LA_AVDECC_API AemCommandType const GetMemoryObjectLength;
	static LA_AVDECC_API AemCommandType const SetStreamBackup;
	static LA_AVDECC_API AemCommandType const GetStreamBackup;
	static LA_AVDECC_API AemCommandType const Expansion;

	static LA_AVDECC_API AemCommandType const InvalidCommandType;

	LA_AVDECC_API operator std::string() const noexcept;
};

/** AEM Acquire Entity Flags - IEEE1722.1-2013 Clause 7.4.1.1 */
class AemAcquireEntityFlags : public utils::TypedDefine<AemAcquireEntityFlags, std::uint32_t>
{
public:
	using TypedDefine::TypedDefine;

	static LA_AVDECC_API AemAcquireEntityFlags const None;
	static LA_AVDECC_API AemAcquireEntityFlags const Persistent;
	static LA_AVDECC_API AemAcquireEntityFlags const Release;

	LA_AVDECC_API operator std::string() const noexcept;
};

/** AEM Lock Entity Flags - IEEE1722.1-2013 Clause 7.4.2.1 */
class AemLockEntityFlags : public utils::TypedDefine<AemLockEntityFlags, std::uint32_t>
{
public:
	using TypedDefine::TypedDefine;

	static LA_AVDECC_API AemLockEntityFlags const None;
	static LA_AVDECC_API AemLockEntityFlags const Unlock;

	LA_AVDECC_API operator std::string() const noexcept;
};

/** Address Access Mode - IEEE1722.1-2013 Clause 9.2.1.3.3 */
class AaMode : public utils::TypedDefine<AaMode, std::uint8_t>
{
public:
	using TypedDefine::TypedDefine;

	static LA_AVDECC_API AaMode const Read;
	static LA_AVDECC_API AaMode const Write;
	static LA_AVDECC_API AaMode const Execute;

	LA_AVDECC_API operator std::string() const noexcept;
};

/** Address Access AECP Status - IEEE1722.1-2013 Clause 9.2.1.3.4 */
class AaAecpStatus : public AecpStatus
{
public:
	using AecpStatus::AecpStatus;

	static LA_AVDECC_API AaAecpStatus const AddressTooLow;
	static LA_AVDECC_API AaAecpStatus const AddressTooHigh;
	static LA_AVDECC_API AaAecpStatus const AddressInvalid;
	static LA_AVDECC_API AaAecpStatus const TlvInvalid;
	static LA_AVDECC_API AaAecpStatus const DataInvalid;
	static LA_AVDECC_API AaAecpStatus const Unsupported;

	LA_AVDECC_API operator std::string() const noexcept;
};

/** Milan Vendor Unique AECP Status - Milan-2019 Clause 7.2.3 */
class MvuAecpStatus : public AecpStatus
{
public:
	using AecpStatus::AecpStatus;

	LA_AVDECC_API operator std::string() const noexcept;
};

/** Milan Vendor Unique Command Type - Milan-2019 Clause 7.2.2.3 */
class MvuCommandType : public utils::TypedDefine<MvuCommandType, std::uint16_t>
{
public:
	using TypedDefine::TypedDefine;

	static LA_AVDECC_API MvuCommandType const GetMilanInfo;

	static LA_AVDECC_API MvuCommandType const InvalidCommandType;

	LA_AVDECC_API operator std::string() const noexcept;
};

/** ACMP Message Type - IEEE1722.1-2013 Clause 8.2.1.5 */
class AcmpMessageType : public utils::TypedDefine<AcmpMessageType, std::uint8_t>
{
public:
	using TypedDefine::TypedDefine;

	static LA_AVDECC_API AcmpMessageType const ConnectTxCommand;
	static LA_AVDECC_API AcmpMessageType const ConnectTxResponse;
	static LA_AVDECC_API AcmpMessageType const DisconnectTxCommand;
	static LA_AVDECC_API AcmpMessageType const DisconnectTxResponse;
	static LA_AVDECC_API AcmpMessageType const GetTxStateCommand;
	static LA_AVDECC_API AcmpMessageType const GetTxStateResponse;
	static LA_AVDECC_API AcmpMessageType const ConnectRxCommand;
	static LA_AVDECC_API AcmpMessageType const ConnectRxResponse;
	static LA_AVDECC_API AcmpMessageType const DisconnectRxCommand;
	static LA_AVDECC_API AcmpMessageType const DisconnectRxResponse;
	static LA_AVDECC_API AcmpMessageType const GetRxStateCommand;
	static LA_AVDECC_API AcmpMessageType const GetRxStateResponse;
	static LA_AVDECC_API AcmpMessageType const GetTxConnectionCommand;
	static LA_AVDECC_API AcmpMessageType const GetTxConnectionResponse;

	LA_AVDECC_API operator std::string() const noexcept;
};

/** ACMP Status - IEEE1722.1-2013 Clause 8.2.1.6 */
class AcmpStatus : public utils::TypedDefine<AcmpStatus, std::uint8_t>
{
public:
	using TypedDefine::TypedDefine;

	LA_AVDECC_API AcmpStatus() noexcept;

	static LA_AVDECC_API AcmpStatus const Success;
	static LA_AVDECC_API AcmpStatus const ListenerUnknownID;
	static LA_AVDECC_API AcmpStatus const TalkerUnknownID;
	static LA_AVDECC_API AcmpStatus const TalkerDestMacFail;
	static LA_AVDECC_API AcmpStatus const TalkerNoStreamIndex;
	static LA_AVDECC_API AcmpStatus const TalkerNoBandwidth;
	static LA_AVDECC_API AcmpStatus const TalkerExclusive;
	static LA_AVDECC_API AcmpStatus const ListenerTalkerTimeout;
	static LA_AVDECC_API AcmpStatus const ListenerExclusive;
	static LA_AVDECC_API AcmpStatus const StateUnavailable;
	static LA_AVDECC_API AcmpStatus const NotConnected;
	static LA_AVDECC_API AcmpStatus const NoSuchConnection;
	static LA_AVDECC_API AcmpStatus const CouldNotSendMessage;
	static LA_AVDECC_API AcmpStatus const TalkerMisbehaving;
	static LA_AVDECC_API AcmpStatus const ListenerMisbehaving;
	static LA_AVDECC_API AcmpStatus const ControllerNotAuthorized;
	static LA_AVDECC_API AcmpStatus const IncompatibleRequest;
	static LA_AVDECC_API AcmpStatus const NotSupported;

	LA_AVDECC_API operator std::string() const noexcept;
	LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION fromString(std::string const& stringValue); // Throws std::invalid_argument if value does not exist
};

/** ACMP UniqueID - IEEE1722.1-2013 Clause 8.2.1.12 and 8.2.1.13 */
using AcmpUniqueID = std::uint16_t;

/** ACMP SequenceID - IEEE1722.1-2013 Clause 8.2.1.16 */
using AcmpSequenceID = std::uint16_t;

} // namespace protocol
} // namespace avdecc
} // namespace la
