/*
* Copyright (C) 2016-2019, L-Acoustics and its contributors

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

#include "exception.hpp"

#include <exception>

namespace la
{
namespace avdecc
{
namespace entitySerializer
{
enum class SerializationError
{
	NoError = 0,
	AccessDenied = 1, /**< File access denied. */
	UnknownEntity = 2, /**< Specified entityID unknown. */
	SerializationError = 3, /**< Error during json objects serialization. */
	NotSupported = 98, /**< Serialization feature not supported by the library (was not compiled). */
	InternalError = 99, /**< Internal error, please report the issue. */
};

enum class DeserializationError
{
	NoError = 0,
	AccessDenied = 1, /**< File access denied. */
	UnsupportedDumpVersion = 2, /**< json dump version not supported. */
	ParseError = 3, /**< Error during json parsing. */
	DeserializationError = 4, /**< Error during json objects deserialization. */
	NotSupported = 98, /**< Deserialization feature not supported by the library (was not compiled). */
	InternalError = 99, /**< Internal error, please report the issue. */
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

} // namespace entitySerializer
} // namespace avdecc
} // namespace la
