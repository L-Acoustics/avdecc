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
* @file protocolAemAecpdu.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "protocolAecpdu.hpp"
#include <utility>
#include <array>

namespace la
{
namespace avdecc
{
namespace protocol
{

/** AEM Aecpdu message */
class AemAecpdu final : public Aecpdu
{
public:
	static constexpr size_t HeaderLength = 2; /* Unsolicited + CommandType */
	static constexpr size_t MaximumPayloadLength = Aecpdu::MaximumLength - Aecpdu::HeaderLength - HeaderLength;
#if defined(ALLOW_BIG_AEM_PAYLOADS)
	static constexpr size_t MaximumBigPayloadLength = Aecpdu::MaximumBigPayloadLength - Aecpdu::HeaderLength - HeaderLength;
#endif // !ALLOW_BIG_AEM_PAYLOADS
	static LA_AVDECC_API la::avdecc::networkInterface::MacAddress Identify_Mac_Address;
	using Payload = std::pair<void const*, size_t>;

	/**
	* @brief Factory method to create a new AemAecpdu.
	* @details Creates a new AemAecpdu as a unique pointer.
	* @return A new AemAecpdu as a Aecpdu::UniquePointer.
	*/
	static UniquePointer create()
	{
		auto deleter = [](Aecpdu* self)
		{
			static_cast<AemAecpdu*>(self)->destroy();
		};
		return UniquePointer(createRawAemAecpdu(), deleter);
	}

	/** Constructor for heap AemAecpdu */
	LA_AVDECC_API AemAecpdu() noexcept;

	/** Destructor (for some reason we have to define it in the cpp file or clang complains about missing vtable, using = default or inline not working) */
	virtual LA_AVDECC_API ~AemAecpdu() noexcept override;

	// Setters
	void setUnsolicited(bool const unsolicited) noexcept
	{
		_unsolicited = unsolicited;
	}
	void setCommandType(AemCommandType const commandType) noexcept
	{
		_commandType = commandType;
	}
	void setCommandSpecificData(void const* const commandSpecificData, size_t const commandSpecificDataLength)
	{
		// Check Aecp do not exceed maximum allowed length
#if defined(ALLOW_BIG_AEM_PAYLOADS)
		if (commandSpecificDataLength > MaximumBigPayloadLength)
#else // !ALLOW_BIG_AEM_PAYLOADS
		if (commandSpecificDataLength > MaximumPayloadLength)
#endif // ALLOW_BIG_AEM_PAYLOADS
		{
			throw std::invalid_argument("AEM payload too big");
		}

		_commandSpecificDataLength = commandSpecificDataLength;
		if (_commandSpecificDataLength > 0)
		{
			AVDECC_ASSERT(commandSpecificData != nullptr, "commandSpecificData is nullptr");
			memcpy(_commandSpecificData.data(), commandSpecificData, _commandSpecificDataLength);
		}
		// Don't forget to update parent's specific data length field
		setAecpSpecificDataLength(AemAecpdu::HeaderLength + commandSpecificDataLength);
	}

	// Getters
	bool getUnsolicited() const noexcept
	{
		return _unsolicited;
	}
	AemCommandType getCommandType() const noexcept
	{
		return _commandType;
	}
	Payload getPayload() const noexcept
	{
		return std::make_pair(_commandSpecificData.data(), _commandSpecificDataLength);
	}

	/** Serialization method */
	virtual LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION serialize(SerializationBuffer& buffer) const override;

	/** Deserialization method */
	virtual LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION deserialize(DeserializationBuffer& buffer) override;

	/** Copy method */
	virtual LA_AVDECC_API UniquePointer LA_AVDECC_CALL_CONVENTION copy() const override;

	// Defaulted compiler auto-generated methods
	AemAecpdu(AemAecpdu&&) = default;
	AemAecpdu(AemAecpdu const&) = default;
	AemAecpdu& operator=(AemAecpdu const&) = default;
	AemAecpdu& operator=(AemAecpdu&&) = default;

private:
	/** Entry point */
	static LA_AVDECC_API AemAecpdu* LA_AVDECC_CALL_CONVENTION createRawAemAecpdu();

	/** Destroy method for COM-like interface */
	virtual LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION destroy() noexcept override;

	// Aem header data
	bool _unsolicited{ false };
	AemCommandType _commandType{ AemCommandType::InvalidCommandType };
#if defined(ALLOW_BIG_AEM_PAYLOADS)
	std::array<std::uint8_t, MaximumBigPayloadLength> _commandSpecificData{};
#else // !ALLOW_BIG_AEM_PAYLOADS
	std::array<std::uint8_t, MaximumPayloadLength> _commandSpecificData{};
#endif // ALLOW_BIG_AEM_PAYLOADS
	size_t _commandSpecificDataLength{ 0 };
};

} // namespace protocol
} // namespace avdecc
} // namespace la
