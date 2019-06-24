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
* @file exception.hpp
* @author Christophe Calmejane
* @brief Avdecc exceptions.
*/

#pragma once

#include "exports.hpp"

#include <string.h>
#include <string>
#include <cstring>
#include <stdlib.h>

namespace la
{
namespace avdecc
{
/**
* @brief Base exception type class for the library.
* @details Base exception type class to be used by all library APIs that throw something.
* @note We cannot use stl objects (neither std::string nor std::exception) here because of the required LA_AVDECC_API tag on the class
*       (otherwise VisualStudio complains about dll-linkage errors).
*       The tag is required mainly for macOS dylib system, since leaving a dylib will slice the class to the base common class of both dylib sides
*       (std::exception in that case if we derivate our class from that).
*/
class LA_AVDECC_API Exception
{
public:
	Exception(std::string const& text) noexcept
		: Exception(text.c_str())
	{
	}

	Exception(char const* const text) noexcept
	{
		copyText(text);
	}

	Exception(Exception const& other)
	{
		copyText(other._text);
	}

	Exception(Exception&& other)
	{
		moveText(std::move(other));
	}

	virtual ~Exception() noexcept
	{
		freeText();
	}

	Exception& operator=(Exception const& other)
	{
		if (this == &other)
		{
			return *this;
		}

		copyText(other._text);
		return *this;
	}

	Exception& operator=(Exception&& other)
	{
		if (this == &other)
		{
			return *this;
		}

		moveText(std::move(other));
		return *this;
	}

	char const* what() const noexcept
	{
		return _text == nullptr ? "Empty exception error message or not enough memory to allocate it" : _text;
	}

private:
	void copyText(char const* const text) noexcept
	{
		// First free previous text
		freeText();

		if (text != nullptr)
		{
			// Allocate a buffer to hold the new text
			auto const len = std::strlen(text) + 1;
			_text = (char*)malloc(len);

			// Copy the new text
			if (_text != nullptr)
				std::memcpy(_text, text, len);
		}
	}

	void moveText(Exception&& other) noexcept
	{
		freeText();
		_text = other._text;
		other._text = nullptr;
	}

	void freeText() noexcept
	{
		if (_text != nullptr)
			free(_text);
		_text = nullptr;
	}

	char* _text{ nullptr };
};

} // namespace avdecc
} // namespace la
