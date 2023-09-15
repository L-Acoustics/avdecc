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
* @file protocolAcmpdu.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "protocolAvtpdu.hpp"

#include <memory>

namespace la
{
namespace avdecc
{
namespace protocol
{
/** Acmpdu message */
class Acmpdu final : public AvtpduControl
{
public:
	static constexpr size_t Length = 44; /* ACMPDU size - IEEE1722.1-2013 Clause 8.2.1.7 */
	using UniquePointer = std::unique_ptr<Acmpdu, void (*)(Acmpdu*)>;
	static LA_AVDECC_API networkInterface::MacAddress const Multicast_Mac_Address; /* Annex B */

	/**
	* @brief Factory method to create a new Acmpdu.
	* @details Creates a new Acmpdu as a unique pointer.
	* @return A new Acmpdu as a Acmpdu::UniquePointer.
	*/
	static UniquePointer create() noexcept
	{
		auto deleter = [](Acmpdu* self)
		{
			self->destroy();
		};
		return UniquePointer(createRawAcmpdu(), deleter);
	}

	/** Constructor for heap Acmpdu */
	LA_AVDECC_API Acmpdu() noexcept;

	/** Destructor (for some reason we have to define it in the cpp file or clang complains about missing vtable, using = default or inline not working) */
	virtual LA_AVDECC_API ~Acmpdu() noexcept override;

	// Setters
	void LA_AVDECC_CALL_CONVENTION setMessageType(AcmpMessageType const messageType) noexcept
	{
		AvtpduControl::setControlData(messageType.getValue());
	}
	void LA_AVDECC_CALL_CONVENTION setStatus(AcmpStatus const status) noexcept
	{
		AvtpduControl::setStatus(status.getValue());
	}
	void LA_AVDECC_CALL_CONVENTION setControllerEntityID(UniqueIdentifier const controllerEntityID) noexcept
	{
		_controllerEntityID = controllerEntityID;
	}
	void LA_AVDECC_CALL_CONVENTION setTalkerEntityID(UniqueIdentifier const talkerEntityID) noexcept
	{
		_talkerEntityID = talkerEntityID;
	}
	void LA_AVDECC_CALL_CONVENTION setListenerEntityID(UniqueIdentifier const listenerEntityID) noexcept
	{
		_listenerEntityID = listenerEntityID;
	}
	void LA_AVDECC_CALL_CONVENTION setTalkerUniqueID(AcmpUniqueID const talkerUniqueID) noexcept
	{
		_talkerUniqueID = talkerUniqueID;
	}
	void LA_AVDECC_CALL_CONVENTION setListenerUniqueID(AcmpUniqueID const listenerUniqueID) noexcept
	{
		_listenerUniqueID = listenerUniqueID;
	}
	void LA_AVDECC_CALL_CONVENTION setStreamDestAddress(networkInterface::MacAddress const& streamDestAddress) noexcept
	{
		_streamDestAddress = streamDestAddress;
	}
	void LA_AVDECC_CALL_CONVENTION setConnectionCount(std::uint16_t const connectionCount) noexcept
	{
		_connectionCount = connectionCount;
	}
	void LA_AVDECC_CALL_CONVENTION setSequenceID(AcmpSequenceID const sequenceID) noexcept
	{
		_sequenceID = sequenceID;
	}
	void LA_AVDECC_CALL_CONVENTION setFlags(entity::ConnectionFlags const flags) noexcept
	{
		_flags = flags;
	}
	void LA_AVDECC_CALL_CONVENTION setStreamVlanID(std::uint16_t const streamVlanID) noexcept
	{
		_streamVlanID = streamVlanID;
	}

	// Getters
	AcmpMessageType LA_AVDECC_CALL_CONVENTION getMessageType() const noexcept
	{
		return AcmpMessageType(AvtpduControl::getControlData());
	}
	AcmpStatus LA_AVDECC_CALL_CONVENTION getStatus() const noexcept
	{
		return AcmpStatus(AvtpduControl::getStatus());
	}
	UniqueIdentifier LA_AVDECC_CALL_CONVENTION getControllerEntityID() const noexcept
	{
		return _controllerEntityID;
	}
	UniqueIdentifier LA_AVDECC_CALL_CONVENTION getTalkerEntityID() const noexcept
	{
		return _talkerEntityID;
	}
	UniqueIdentifier LA_AVDECC_CALL_CONVENTION getListenerEntityID() const noexcept
	{
		return _listenerEntityID;
	}
	AcmpUniqueID LA_AVDECC_CALL_CONVENTION getTalkerUniqueID() const noexcept
	{
		return _talkerUniqueID;
	}
	AcmpUniqueID LA_AVDECC_CALL_CONVENTION getListenerUniqueID() const noexcept
	{
		return _listenerUniqueID;
	}
	networkInterface::MacAddress LA_AVDECC_CALL_CONVENTION getStreamDestAddress() const noexcept
	{
		return _streamDestAddress;
	}
	std::uint16_t LA_AVDECC_CALL_CONVENTION getConnectionCount() const noexcept
	{
		return _connectionCount;
	}
	AcmpSequenceID LA_AVDECC_CALL_CONVENTION getSequenceID() const noexcept
	{
		return _sequenceID;
	}
	entity::ConnectionFlags LA_AVDECC_CALL_CONVENTION getFlags() const noexcept
	{
		return _flags;
	}
	std::uint16_t LA_AVDECC_CALL_CONVENTION getStreamVlanID() const noexcept
	{
		return _streamVlanID;
	}

	/** Serialization method */
	LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION serialize(SerializationBuffer& buffer) const;

	/** Deserialization method */
	LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION deserialize(DeserializationBuffer& buffer);

	/** Copy method */
	LA_AVDECC_API UniquePointer LA_AVDECC_CALL_CONVENTION copy() const;

	// Defaulted compiler auto-generated methods
	LA_AVDECC_API Acmpdu(Acmpdu&&);
	LA_AVDECC_API Acmpdu(Acmpdu const&);
	LA_AVDECC_API Acmpdu& LA_AVDECC_CALL_CONVENTION operator=(Acmpdu const&);
	LA_AVDECC_API Acmpdu& LA_AVDECC_CALL_CONVENTION operator=(Acmpdu&&);

private:
	/** Entry point */
	static LA_AVDECC_API Acmpdu* LA_AVDECC_CALL_CONVENTION createRawAcmpdu() noexcept;

	/** Destroy method for COM-like interface */
	LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION destroy() noexcept;

	// Acmpdu header data
	UniqueIdentifier _controllerEntityID{};
	UniqueIdentifier _talkerEntityID{};
	UniqueIdentifier _listenerEntityID{};
	AcmpUniqueID _talkerUniqueID{ 0u };
	AcmpUniqueID _listenerUniqueID{ 0u };
	networkInterface::MacAddress _streamDestAddress{};
	std::uint16_t _connectionCount{ 0u };
	AcmpSequenceID _sequenceID{ 0u };
	entity::ConnectionFlags _flags{};
	std::uint16_t _streamVlanID{ 0u };

private:
	// Hide renamed AvtpduControl data
	using AvtpduControl::setControlData;
	using AvtpduControl::setControlDataLength;
	using AvtpduControl::getControlData;
	using AvtpduControl::getControlDataLength;
	// Hide Avtpdu const data
	using Avtpdu::setSubType;
	using Avtpdu::getSubType;
	// Hide AvtpduControl const data
	using AvtpduControl::setStreamValid;
	using AvtpduControl::getStreamValid;
};

} // namespace protocol
} // namespace avdecc
} // namespace la
