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
* @file jsonSerialization.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "la/avdecc/utils.hpp"

#include "exception.hpp"
#include "entityModelTree.hpp"
#include "exports.hpp"

#include <exception>
#ifdef ENABLE_AVDECC_FEATURE_JSON
#	include <nlohmann/json.hpp>
#endif // ENABLE_AVDECC_FEATURE_JSON

#include <cstdint>

namespace la
{
namespace avdecc
{
namespace jsonSerializer
{
enum class SerializationError
{
	NoError = 0,
	AccessDenied = 1, /**< File access denied. */
	UnknownEntity = 2, /**< Specified entityID unknown. */
	InvalidDescriptorIndex = 3, /**< A descriptor index of the model has an invalid numbering. */
	Incomplete = 4, /**< Serialization partially done (had errors but continued anyway). */
	NotSupported = 98, /**< Serialization feature not supported by the library. */
	InternalError = 99, /**< Internal error, please report the issue. */
};

enum class DeserializationError
{
	NoError = 0,
	AccessDenied = 1, /**< File access denied. */
	FileReadError = 2, /**< Error reading file. */
	IncompatibleDumpVersion = 3, /**< json dump version not compatible with this version of the library. */
	ParseError = 4, /**< Error during json parsing. */
	MissingKey = 5, /**< A mandatory Key is missing from the json model. */
	InvalidKey = 6, /**< Key couldn't be converted from json to field's expected data type. */
	InvalidValue = 7, /**< Value couldn't be converted from json to field's expected data type. */
	OtherError = 8, /**< Other json conversion error. */
	DuplicateEntityID = 9, /**< An Entity already exists with the same EntityID. */
	NotCompliant = 10, /**< Model is not full compliant with IEEE1722.1 and IgnoreSanityChecks flag was not set. */
	Incomplete = 11, /**< Deserialization partially done (had errors but continued anyway). */
	MissingInformation = 12, /**< Missing information in the model. */
	NotSupported = 98, /**< Deserialization feature not supported by the library. */
	InternalError = 99, /**< Internal error, please report the issue. */
};

class LA_AVDECC_API SerializationException final : public la::avdecc::Exception
{
public:
	template<class T>
	SerializationException(SerializationError const error, T&& text) noexcept
		: la::avdecc::Exception(std::forward<T>(text))
		, _error(error)
	{
	}
	SerializationError getError() const noexcept
	{
		return _error;
	}

private:
	SerializationError const _error{ SerializationError::NoError };
};

class LA_AVDECC_API DeserializationException final : public la::avdecc::Exception
{
public:
	template<class T>
	DeserializationException(DeserializationError const error, T&& text) noexcept
		: la::avdecc::Exception(std::forward<T>(text))
		, _error(error)
	{
	}
	DeserializationError getError() const noexcept
	{
		return _error;
	}

private:
	DeserializationError const _error{ DeserializationError::NoError };
};

/* Operator overloads */
constexpr bool operator!(SerializationError const error)
{
	return error == SerializationError::NoError;
}

constexpr bool operator!(DeserializationError const error)
{
	return error == DeserializationError::NoError;
}

} // namespace jsonSerializer

namespace entity
{
namespace model
{
namespace jsonSerializer
{
enum class Flag : std::uint16_t
{
	None = 0,
	ProcessADP = 1u << 0, /**< READ/WRITE ADP information */
	ProcessStaticModel = 1u << 1, /**< READ/WRITE AEM Static information */
	ProcessDynamicModel = 1u << 2, /**< READ/WRITE AEM Dynamic information */
	ProcessMilan = 1u << 3, /**< READ/WRITE Milan information */
	ProcessState = 1u << 4, /**< READ/WRITE Global Entity State */
	ProcessStatistics = 1u << 5, /**< READ/WRITE Global Entity Statistics */
	ProcessCompatibility = 1u << 6, /**< READ/WRITE Entity Compatibility */
	ProcessDiagnostics = 1u << 7, /**< READ/WRITE Global Entity Diagnostics */

	BinaryFormat = 1u << 14, /**< READ/WRITE in binary format (MessagePack) */
	IgnoreAEMSanityChecks = 1u << 15, /**< Ignore AEM Sanity Checks when READING or WRITING */
};
using Flags = utils::EnumBitfield<Flag>;

#ifdef ENABLE_AVDECC_FEATURE_JSON
// Serialization methods
LA_AVDECC_API nlohmann::json LA_AVDECC_CALL_CONVENTION createJsonObject(EntityTree const& entityTree, Flags const flags); // Throws SerializationException

// Deserialization methods
LA_AVDECC_API EntityTree LA_AVDECC_CALL_CONVENTION createEntityTree(nlohmann::json const& object, Flags const flags); // Throws DeserializationException
#endif // ENABLE_AVDECC_FEATURE_JSON

} // namespace jsonSerializer
} // namespace model
} // namespace entity
} // namespace avdecc
} // namespace la
