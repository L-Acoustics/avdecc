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

/** Acmpdu common header */
class Acmpdu final : public AvtpduControl
{
public:
	static constexpr size_t Length = 44; /* ACMPDU size - Clause 8.2.1.7 */
	using UniquePointer = std::unique_ptr<Acmpdu, void(*)(Acmpdu*)>;

	/**
	* @brief Factory method to create a new Acmpdu.
	* @details Creates a new Acmpdu as a unique pointer.
	* @return A new Acmpdu as a Acmpdu::UniquePointer.
	*/
	static UniquePointer create()
	{
		auto deleter = [](Acmpdu* self)
		{
			self->destroy();
		};
		return UniquePointer(createRawAcmpdu(), deleter);
	}

	// Setters
	void setMessageType(AcmpMessageType const messageType) noexcept
	{
		AvtpduControl::setControlData(messageType.getValue());
	}
	void setStatus(AcmpStatus const status) noexcept
	{
		AvtpduControl::setStatus(status.getValue());
	}
	void setControllerEntityID(UniqueIdentifier const controllerEntityID) noexcept
	{
		_controllerEntityID = controllerEntityID;
	}
	void setTalkerEntityID(UniqueIdentifier const talkerEntityID) noexcept
	{
		_talkerEntityID = talkerEntityID;
	}
	void setListenerEntityID(UniqueIdentifier const listenerEntityID) noexcept
	{
		_listenerEntityID = listenerEntityID;
	}
	void setTalkerUniqueID(AcmpUniqueID const talkerUniqueID) noexcept
	{
		_talkerUniqueID = talkerUniqueID;
	}
	void setListenerUniqueID(AcmpUniqueID const listenerUniqueID) noexcept
	{
		_listenerUniqueID = listenerUniqueID;
	}
	void setStreamDestAddress(networkInterface::MacAddress const& streamDestAddress) noexcept
	{
		_streamDestAddress = streamDestAddress;
	}
	void setConnectionCount(std::uint16_t const connectionCount) noexcept
	{
		_connectionCount = connectionCount;
	}
	void setSequenceID(AcmpSequenceID const sequenceID) noexcept
	{
		_sequenceID = sequenceID;
	}
	void setFlags(entity::ConnectionFlags const flags) noexcept
	{
		_flags = flags;
	}
	void setStreamVlanID(std::uint16_t const streamVlanID) noexcept
	{
		_streamVlanID = streamVlanID;
	}

	// Getters
	AcmpMessageType getMessageType() const noexcept
	{
		return AcmpMessageType(AvtpduControl::getControlData());
	}
	AcmpStatus getStatus() const noexcept
	{
		return AcmpStatus(AvtpduControl::getStatus());
	}
	UniqueIdentifier getControllerEntityID() const noexcept
	{
		return _controllerEntityID;
	}
	UniqueIdentifier getTalkerEntityID() const noexcept
	{
		return _talkerEntityID;
	}
	UniqueIdentifier getListenerEntityID() const noexcept
	{
		return _listenerEntityID;
	}
	AcmpUniqueID getTalkerUniqueID() const noexcept
	{
		return _talkerUniqueID;
	}
	AcmpUniqueID getListenerUniqueID() const noexcept
	{
		return _listenerUniqueID;
	}
	networkInterface::MacAddress getStreamDestAddress() const noexcept
	{
		return _streamDestAddress;
	}
	std::uint16_t getConnectionCount() const noexcept
	{
		return _connectionCount;
	}
	AcmpSequenceID getSequenceID() const noexcept
	{
		return _sequenceID;
	}
	entity::ConnectionFlags getFlags() const noexcept
	{
		return _flags;
	}
	std::uint16_t getStreamVlanID() const noexcept
	{
		return _streamVlanID;
	}

	/** Serialization method */
	void serialize(SerializationBuffer& buffer) const;

	/** Deserialization method */
	void deserialize(DeserializationBuffer& buffer);

	/** Copy method */
	UniquePointer copy() const;

	// Defaulted compiler auto-generated methods
	Acmpdu(Acmpdu&&) = default;
	Acmpdu(Acmpdu const&) = default;
	Acmpdu& operator=(Acmpdu const&) = default;
	Acmpdu& operator=(Acmpdu&&) = default;

private:
	/** Constructor */
	Acmpdu() noexcept;

	/** Destructor */
	virtual ~Acmpdu() noexcept override = default;

	/** Entry point */
	static Acmpdu* createRawAcmpdu();

	/** Destroy method for COM-like interface */
	void destroy() noexcept;

	// Acmpdu header data
	UniqueIdentifier _controllerEntityID{ getNullIdentifier() };
	UniqueIdentifier _talkerEntityID{ getNullIdentifier() };
	UniqueIdentifier _listenerEntityID{ getNullIdentifier() };
	AcmpUniqueID _talkerUniqueID{ 0 };
	AcmpUniqueID _listenerUniqueID{ 0 };
	networkInterface::MacAddress _streamDestAddress{};
	std::uint16_t _connectionCount{ 0 };
	AcmpSequenceID _sequenceID{ 0 };
	entity::ConnectionFlags _flags{ entity::ConnectionFlags::None };
	std::uint16_t _streamVlanID{ 0 };

private:
	// Hide renamed AvtpduControl data
	using AvtpduControl::setControlData;
	using AvtpduControl::setControlDataLength;
	using AvtpduControl::getControlData;
	using AvtpduControl::getControlDataLength;
	// Hide EtherLayer2 const data
	using EtherLayer2::setDestAddress;
	using EtherLayer2::getDestAddress;
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
