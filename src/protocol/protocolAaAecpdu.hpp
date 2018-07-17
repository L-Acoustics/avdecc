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
* @file protocolAaAecpdu.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "protocolAecpdu.hpp"
#include "la/avdecc/internals/entityAddressAccessTypes.hpp"
#include <utility>
#include <array>

namespace la
{
namespace avdecc
{
namespace protocol
{

/** AA Aecpdu heaader */
class AaAecpdu final : public Aecpdu
{
public:
	static constexpr size_t HeaderLength = 2; /* TlvCount */
	static constexpr size_t MaximumTlvDataLength = Aecpdu::MaximumLength - Aecpdu::HeaderLength - HeaderLength; /* Maximum tlv_data field length */
	static constexpr size_t TlvHeaderLength = 10; /* Mode + Length + Address */
	static constexpr size_t MaximumSingleTlvMemoryDataLength = Aecpdu::MaximumLength - Aecpdu::HeaderLength - HeaderLength - TlvHeaderLength; /* Maximum individual TLV memory_data length */

	/**
	* @brief Factory method to create a new AaAecpdu.
	* @details Creates a new AaAecpdu as a unique pointer.
	* @return A new AaAecpdu as a Aecpdu::UniquePointer.
	*/
	static UniquePointer create()
	{
		auto deleter = [](Aecpdu* self)
		{
			static_cast<AaAecpdu*>(self)->destroy();
		};
		return UniquePointer(createRawAaAecpdu(), deleter);
	}

	// Setters
	template<class Tlv, typename = std::enable_if_t<std::is_same<entity::addressAccess::Tlv, std::remove_cv_t<std::remove_reference_t<Tlv>>>::value>>
	void addTlv(Tlv&& tlv)
	{
		auto const newLength = _tlvDataLength + TlvHeaderLength + tlv.size();

		// Check Aecp do not exceed maximum allowed length
		if (newLength > MaximumTlvDataLength)
		{
			throw std::invalid_argument("Not enough room in packet for this TLV");
		}

		_tlvDataLength = newLength;
		_tlvData.push_back(std::move(tlv));

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
	virtual void serialize(SerializationBuffer& buffer) const override;

	/** Deserialization method */
	virtual void deserialize(DeserializationBuffer& buffer) override;

	/** Copy method */
	virtual UniquePointer copy() const override;
	
	// Defaulted compiler auto-generated methods
	AaAecpdu(AaAecpdu&&) = default;
	AaAecpdu(AaAecpdu const&) = default;
	AaAecpdu& operator=(AaAecpdu const&) = default;
	AaAecpdu& operator=(AaAecpdu&&) = default;

private:
	/** Constructor */
	AaAecpdu() noexcept;

	/** Destructor */
	virtual ~AaAecpdu() noexcept override = default;

	/** Entry point */
	static AaAecpdu* createRawAaAecpdu();

	/** Destroy method for COM-like interface */
	virtual void destroy() noexcept override;

	// Aa header data
	entity::addressAccess::Tlvs _tlvData{};

	// Private data
	size_t _tlvDataLength{ 0u };
};

} // namespace protocol
} // namespace avdecc
} // namespace la
