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
* @file protocolAdpdu.hpp
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
/** Adpdu message */
class Adpdu final : public AvtpduControl
{
public:
	static constexpr size_t Length = 56; /* ADPDU size - IEEE1722.1-2013 Clause 6.2.1.7 */
	using UniquePointer = std::unique_ptr<Adpdu, void (*)(Adpdu*)>;
	static LA_AVDECC_API networkInterface::MacAddress const Multicast_Mac_Address; /* Annex B */

	/**
	* @brief Factory method to create a new Adpdu.
	* @details Creates a new Adpdu as a unique pointer.
	* @return A new Adpdu as a Adpdu::UniquePointer.
	*/
	static UniquePointer create() noexcept
	{
		auto deleter = [](Adpdu* self)
		{
			self->destroy();
		};
		return UniquePointer(createRawAdpdu(), deleter);
	}

	/** Constructor for heap Adpdu */
	LA_AVDECC_API Adpdu() noexcept;

	/** Destructor (for some reason we have to define it in the cpp file or clang complains about missing vtable, using = default or inline not working) */
	virtual LA_AVDECC_API ~Adpdu() noexcept override;

	// Setters
	void LA_AVDECC_CALL_CONVENTION setMessageType(AdpMessageType const messageType) noexcept
	{
		AvtpduControl::setControlData(messageType.getValue());
	}
	void LA_AVDECC_CALL_CONVENTION setValidTime(std::uint8_t const validTime) noexcept
	{
		AvtpduControl::setStatus(validTime);
	}
	void LA_AVDECC_CALL_CONVENTION setEntityID(UniqueIdentifier const entityID) noexcept
	{
		AvtpduControl::setStreamID(entityID.getValue());
	}
	void LA_AVDECC_CALL_CONVENTION setEntityModelID(UniqueIdentifier const entityModelID) noexcept
	{
		_entityModelID = entityModelID;
	}
	void LA_AVDECC_CALL_CONVENTION setEntityCapabilities(entity::EntityCapabilities const entityCapabilities) noexcept
	{
		_entityCapabilities = entityCapabilities;
	}
	void LA_AVDECC_CALL_CONVENTION setTalkerStreamSources(std::uint16_t const talkerStreamSources) noexcept
	{
		_talkerStreamSources = talkerStreamSources;
	}
	void LA_AVDECC_CALL_CONVENTION setTalkerCapabilities(entity::TalkerCapabilities const talkerCapabilities) noexcept
	{
		_talkerCapabilities = talkerCapabilities;
	}
	void LA_AVDECC_CALL_CONVENTION setListenerStreamSinks(std::uint16_t const listenerStreamSinks) noexcept
	{
		_listenerStreamSinks = listenerStreamSinks;
	}
	void LA_AVDECC_CALL_CONVENTION setListenerCapabilities(entity::ListenerCapabilities const listenerCapabilities) noexcept
	{
		_listenerCapabilities = listenerCapabilities;
	}
	void LA_AVDECC_CALL_CONVENTION setControllerCapabilities(entity::ControllerCapabilities const controllerCapabilities) noexcept
	{
		_controllerCapabilities = controllerCapabilities;
	}
	void LA_AVDECC_CALL_CONVENTION setAvailableIndex(std::uint32_t const availableIndex) noexcept
	{
		_availableIndex = availableIndex;
	}
	void LA_AVDECC_CALL_CONVENTION setGptpGrandmasterID(UniqueIdentifier const gptpGrandmasterID) noexcept
	{
		_gptpGrandmasterID = gptpGrandmasterID;
	}
	void LA_AVDECC_CALL_CONVENTION setGptpDomainNumber(std::uint8_t const gptpDomainNumber) noexcept
	{
		_gptpDomainNumber = gptpDomainNumber;
	}
	void LA_AVDECC_CALL_CONVENTION setIdentifyControlIndex(entity::model::ControlIndex const identifyControlIndex) noexcept
	{
		_identifyControlIndex = identifyControlIndex;
	}
	void LA_AVDECC_CALL_CONVENTION setInterfaceIndex(entity::model::AvbInterfaceIndex const interfaceIndex) noexcept
	{
		_interfaceIndex = interfaceIndex;
	}
	void LA_AVDECC_CALL_CONVENTION setAssociationID(UniqueIdentifier const associationID) noexcept
	{
		_associationID = associationID;
	}

	// Getters
	AdpMessageType LA_AVDECC_CALL_CONVENTION getMessageType() const noexcept
	{
		return AdpMessageType(AvtpduControl::getControlData());
	}
	std::uint8_t LA_AVDECC_CALL_CONVENTION getValidTime() const noexcept
	{
		return AvtpduControl::getStatus();
	}
	UniqueIdentifier LA_AVDECC_CALL_CONVENTION getEntityID() const noexcept
	{
		return UniqueIdentifier{ AvtpduControl::getStreamID() };
	}
	UniqueIdentifier LA_AVDECC_CALL_CONVENTION getEntityModelID() const noexcept
	{
		return _entityModelID;
	}
	entity::EntityCapabilities LA_AVDECC_CALL_CONVENTION getEntityCapabilities() const noexcept
	{
		return _entityCapabilities;
	}
	std::uint16_t LA_AVDECC_CALL_CONVENTION getTalkerStreamSources() const noexcept
	{
		return _talkerStreamSources;
	}
	entity::TalkerCapabilities LA_AVDECC_CALL_CONVENTION getTalkerCapabilities() const noexcept
	{
		return _talkerCapabilities;
	}
	std::uint16_t LA_AVDECC_CALL_CONVENTION getListenerStreamSinks() const noexcept
	{
		return _listenerStreamSinks;
	}
	entity::ListenerCapabilities LA_AVDECC_CALL_CONVENTION getListenerCapabilities() const noexcept
	{
		return _listenerCapabilities;
	}
	entity::ControllerCapabilities LA_AVDECC_CALL_CONVENTION getControllerCapabilities() const noexcept
	{
		return _controllerCapabilities;
	}
	std::uint32_t LA_AVDECC_CALL_CONVENTION getAvailableIndex() const noexcept
	{
		return _availableIndex;
	}
	UniqueIdentifier LA_AVDECC_CALL_CONVENTION getGptpGrandmasterID() const noexcept
	{
		return _gptpGrandmasterID;
	}
	std::uint8_t LA_AVDECC_CALL_CONVENTION getGptpDomainNumber() const noexcept
	{
		return _gptpDomainNumber;
	}
	entity::model::ControlIndex LA_AVDECC_CALL_CONVENTION getIdentifyControlIndex() const noexcept
	{
		return _identifyControlIndex;
	}
	entity::model::AvbInterfaceIndex LA_AVDECC_CALL_CONVENTION getInterfaceIndex() const noexcept
	{
		return _interfaceIndex;
	}
	UniqueIdentifier LA_AVDECC_CALL_CONVENTION getAssociationID() const noexcept
	{
		return _associationID;
	}

	/** Serialization method */
	LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION serialize(SerializationBuffer& buffer) const;

	/** Deserialization method */
	LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION deserialize(DeserializationBuffer& buffer);

	/** Copy method */
	LA_AVDECC_API UniquePointer LA_AVDECC_CALL_CONVENTION copy() const;

	// Defaulted compiler auto-generated methods
	LA_AVDECC_API Adpdu(Adpdu&&);
	LA_AVDECC_API Adpdu(Adpdu const&);
	LA_AVDECC_API Adpdu& LA_AVDECC_CALL_CONVENTION operator=(Adpdu const&);
	LA_AVDECC_API Adpdu& LA_AVDECC_CALL_CONVENTION operator=(Adpdu&&);

private:
	/** Entry point */
	static LA_AVDECC_API Adpdu* LA_AVDECC_CALL_CONVENTION createRawAdpdu() noexcept;

	/** Destroy method for COM-like interface */
	LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION destroy() noexcept;

	// Adpdu header data
	UniqueIdentifier _entityModelID{};
	entity::EntityCapabilities _entityCapabilities{};
	std::uint16_t _talkerStreamSources{};
	entity::TalkerCapabilities _talkerCapabilities{};
	std::uint16_t _listenerStreamSinks{};
	entity::ListenerCapabilities _listenerCapabilities{};
	entity::ControllerCapabilities _controllerCapabilities{};
	std::uint32_t _availableIndex{};
	UniqueIdentifier _gptpGrandmasterID{};
	std::uint8_t _gptpDomainNumber{ 0u };
	// Reserved 24bits
	entity::model::ControlIndex _identifyControlIndex{ 0u };
	entity::model::AvbInterfaceIndex _interfaceIndex{ 0u };
	UniqueIdentifier _associationID{};
	// Reserved 32bits

private:
	// Hide renamed AvtpduControl data
	using AvtpduControl::setControlData;
	using AvtpduControl::setControlDataLength;
	using AvtpduControl::setStatus;
	using AvtpduControl::setStreamID;
	using AvtpduControl::getControlData;
	using AvtpduControl::getControlDataLength;
	using AvtpduControl::getStatus;
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
