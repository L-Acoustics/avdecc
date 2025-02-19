/*
* Copyright (C) 2016-2025, L-Acoustics and its contributors

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
* @file protocolAaAecpdu.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "protocolAecpdu.hpp"

#include "entityAddressAccessTypes.hpp"

#include <utility>
#include <array>

namespace la
{
namespace avdecc
{
namespace protocol
{
/** AA Aecpdu message */
class AaAecpdu final : public Aecpdu
{
public:
	static constexpr size_t HeaderLength = 2; /* TlvCount */
	static constexpr size_t TlvHeaderLength = 10; /* Mode + Length + Address */

	/**
	* @brief Factory method to create a new AaAecpdu.
	* @details Creates a new AaAecpdu as a unique pointer.
	* @param[in] isResponse True if the AA message is a response, false if it's a command.
	* @return A new AaAecpdu as a Aecpdu::UniquePointer.
	*/
	static UniquePointer create(bool const isResponse) noexcept
	{
		auto deleter = [](Aecpdu* self)
		{
			static_cast<AaAecpdu*>(self)->destroy();
		};
		return UniquePointer(createRawAaAecpdu(isResponse), deleter);
	}

	/** Constructor for heap AaAecpdu */
	LA_AVDECC_API AaAecpdu(bool const isResponse) noexcept;

	/** Destructor (for some reason we have to define it in the cpp file or clang complains about missing vtable, using = default or inline not working) */
	virtual LA_AVDECC_API ~AaAecpdu() noexcept override;

	// Setters
	template<class Tlv, typename = std::enable_if_t<std::is_same<entity::addressAccess::Tlv, std::remove_cv_t<std::remove_reference_t<Tlv>>>::value>>
	void addTlv(Tlv&& tlv) noexcept
	{
		auto const newLength = _tlvDataLength + TlvHeaderLength + tlv.size();

		_tlvDataLength = newLength;
		_tlvData.push_back(std::forward<Tlv>(tlv));

		// Don't forget to update parent's specific data length field
		setAecpSpecificDataLength(AaAecpdu::HeaderLength + newLength);
	}

	// Getters
	entity::addressAccess::Tlvs const& getTlvData() const noexcept
	{
		return _tlvData;
	}
	entity::addressAccess::Tlvs& getTlvData() noexcept
	{
		return _tlvData;
	}

	/** Serialization method */
	virtual LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION serialize(SerializationBuffer& buffer) const override;

	/** Deserialization method */
	virtual LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION deserialize(DeserializationBuffer& buffer) override;

	/** Contruct a Response message to this Command (only changing the messageType to be of Response kind). Returns nullptr if the message is not a Command or if no Response is possible for this messageType */
	virtual LA_AVDECC_API UniquePointer LA_AVDECC_CALL_CONVENTION responseCopy() const override;

	// Defaulted compiler auto-generated methods
	LA_AVDECC_API AaAecpdu(AaAecpdu&&);
	LA_AVDECC_API AaAecpdu(AaAecpdu const&);
	LA_AVDECC_API AaAecpdu& LA_AVDECC_CALL_CONVENTION operator=(AaAecpdu const&);
	LA_AVDECC_API AaAecpdu& LA_AVDECC_CALL_CONVENTION operator=(AaAecpdu&&);

private:
	/** Entry point */
	static LA_AVDECC_API AaAecpdu* LA_AVDECC_CALL_CONVENTION createRawAaAecpdu(bool const isResponse) noexcept;

	/** Destroy method for COM-like interface */
	virtual LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION destroy() noexcept override;

	// Aa header data
	entity::addressAccess::Tlvs _tlvData{};

	// Private data
	size_t _tlvDataLength{ 0u };
};

} // namespace protocol
} // namespace avdecc
} // namespace la
