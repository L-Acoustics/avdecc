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
* @file protocolAecpdu.hpp
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

/** Aecpdu common header */
class Aecpdu : public AvtpduControl
{
public:
	static constexpr size_t DefaultMaxInflightCommands = 1;
	static constexpr size_t HeaderLength = 10; /* ControllerEID + SequenceID */
	static constexpr size_t MaximumLength = 524; /* AECPDU maximum size - Clause 9.2.1.1.7 */
#if defined(ALLOW_BIG_AEM_PAYLOADS)
	static constexpr size_t MaximumBigPayloadLength = MaximumLength * 2;
#endif // !ALLOW_BIG_AEM_PAYLOADS
	using UniquePointer = std::unique_ptr<Aecpdu, void(*)(Aecpdu*)>;

	// Setters
	void setMessageType(AecpMessageType const messageType) noexcept
	{
		AvtpduControl::setControlData(messageType.getValue());
	}
	void setStatus(AecpStatus const status) noexcept
	{
		AvtpduControl::setStatus(status.getValue());
	}
	void setTargetEntityID(UniqueIdentifier const targetEntityID) noexcept
	{
		AvtpduControl::setStreamID(targetEntityID.getValue());
	}
	void setControllerEntityID(UniqueIdentifier const controllerEntityID) noexcept
	{
		_controllerEntityID = controllerEntityID;
	}
	void setSequenceID(AecpSequenceID const sequenceID) noexcept
	{
		_sequenceID = sequenceID;
	}
	void setAecpSpecificDataLength(size_t const commandSpecificDataLength)
	{
		auto controlDataLength = static_cast<std::uint16_t>(Aecpdu::HeaderLength + commandSpecificDataLength);
		// Check Aecp do not exceed maximum allowed length
#if defined(ALLOW_BIG_AEM_PAYLOADS)
		if (controlDataLength > Aecpdu::MaximumBigPayloadLength)
#else // !ALLOW_BIG_AEM_PAYLOADS
		if (controlDataLength > Aecpdu::MaximumLength)
#endif // ALLOW_BIG_AEM_PAYLOADS
		{
			throw std::invalid_argument("AECP payload too big");
		}
		AvtpduControl::setControlDataLength(controlDataLength);
	}

	// Getters
	AecpMessageType getMessageType() const noexcept
	{
		return AecpMessageType(AvtpduControl::getControlData());
	}
	AecpStatus getStatus() const noexcept
	{
		return AecpStatus(AvtpduControl::getStatus());
	}
	UniqueIdentifier getTargetEntityID() const noexcept
	{
		return UniqueIdentifier{ AvtpduControl::getStreamID() };
	}
	UniqueIdentifier getControllerEntityID() const noexcept
	{
		return _controllerEntityID;
	}
	AecpSequenceID getSequenceID() const noexcept
	{
		return _sequenceID;
	}

	/** Serialization method */
	virtual LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION serialize(SerializationBuffer& buffer) const = 0;

	/** Deserialization method */
	virtual LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION deserialize(DeserializationBuffer& buffer) = 0;

	/** Copy method */
	virtual LA_AVDECC_API UniquePointer LA_AVDECC_CALL_CONVENTION copy() const = 0;

	// Defaulted compiler auto-generated methods
	Aecpdu(Aecpdu&&) = default;
	Aecpdu(Aecpdu const&) = default;
	Aecpdu& operator=(Aecpdu const&) = default;
	Aecpdu& operator=(Aecpdu&&) = default;

protected:
	/** Constructor */
	LA_AVDECC_API Aecpdu() noexcept;

	/** Destructor */
	virtual ~Aecpdu() noexcept override = default;

	// Aecpdu header data
	UniqueIdentifier _controllerEntityID{};
	AecpSequenceID _sequenceID{ 0 };

private:
	/** Destroy method for COM-like interface */
	virtual LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION destroy() noexcept = 0;

	// Hide renamed AvtpduControl data
	using AvtpduControl::setControlData;
	using AvtpduControl::setControlDataLength;
	using AvtpduControl::setStreamID;
	using AvtpduControl::getControlData;
	using AvtpduControl::getControlDataLength;
	using AvtpduControl::getStreamID;
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
