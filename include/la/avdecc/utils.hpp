/*
* Copyright (C) 2016-2022, L-Acoustics and its contributors

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
* @file utils.hpp
* @author Christophe Calmejane
* @brief Useful templates and global methods.
*/

#pragma once

#include "internals/exports.hpp"
#include "internals/uniqueIdentifier.hpp"

#include <type_traits>
#include <tuple>
#include <iterator>
#include <functional>
#include <cstdarg>
#include <iomanip> // setprecision / setfill
#include <ios> // uppercase
#include <string> // string
#include <cstring> // strncmp
#include <sstream> // stringstream
#include <limits> // numeric_limits
#include <stdexcept> // out_of_range / invalid_argument / logic_error
#include <set>
#include <vector>
#include <mutex>

namespace la
{
namespace avdecc
{
namespace utils
{
enum class ThreadPriority
{
	Idle = 0,
	Lowest = 1,
	BelowNormal = 3,
	Normal = 5,
	AboveNormal = 7,
	Highest = 9,
	TimeCritical = 10,
};

LA_AVDECC_API bool LA_AVDECC_CALL_CONVENTION setCurrentThreadName(std::string const& name);
LA_AVDECC_API bool LA_AVDECC_CALL_CONVENTION setCurrentThreadPriority(ThreadPriority const prio);
LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION enableAssert() noexcept;
LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION disableAssert() noexcept;
LA_AVDECC_API bool LA_AVDECC_CALL_CONVENTION isAssertEnabled() noexcept;
LA_AVDECC_API void LA_AVDECC_CALL_CONVENTION displayAssertDialog(char const* const file, unsigned const line, char const* const message, va_list arg) noexcept;

template<typename Cond>
bool avdeccAssert(char const* const file, unsigned const line, Cond const condition, char const* const message, ...) noexcept
{
	bool const result = !!condition;
	if (isAssertEnabled())
	{
		if (!result)
		{
			va_list argptr;
			va_start(argptr, message);
			displayAssertDialog(file, line, message, argptr);
			va_end(argptr);
		}
	}
	return result;
}

template<typename Cond>
bool avdeccAssert(char const* const file, unsigned const line, Cond const condition, std::string const& message) noexcept
{
	return avdeccAssert(file, line, condition, message.c_str());
}

template<typename Cond>
bool avdeccAssertRelease(Cond const condition) noexcept
{
	bool const result = !!condition;
	return result;
}

} // namespace utils
} // namespace avdecc
} // namespace la

#if defined(DEBUG) || defined(COMPILE_AVDECC_ASSERT)
#	define AVDECC_ASSERT(cond, msg, ...) (void)la::avdecc::utils::avdeccAssert(__FILE__, __LINE__, cond, msg, ##__VA_ARGS__)
#	define AVDECC_ASSERT_WITH_RET(cond, msg, ...) la::avdecc::utils::avdeccAssert(__FILE__, __LINE__, cond, msg, ##__VA_ARGS__)
#else // !DEBUG && !COMPILE_AVDECC_ASSERT
#	define AVDECC_ASSERT(cond, msg, ...)
#	define AVDECC_ASSERT_WITH_RET(cond, msg, ...) la::avdecc::utils::avdeccAssertRelease(cond)
#endif // DEBUG || COMPILE_AVDECC_ASSERT

namespace la
{
namespace avdecc
{
namespace utils
{
/**
* @brief Constexpr to compute a pow(x,y) at compile-time.
* @details Computes the pow of 2 types at compile-time.<BR>
*          Example: pow(2,8) will compute 256
* @param[in] base The base of the power-of to compute.
* @param[in] exponent The exponent of the power-of to compute.
* @return The computed value.
* @note To ensure the value is always computed at compile-time, use it with std::integral_constant<BR>
*       Example to get pow(2, 15) as a std::uint32_t at compile-time:<BR>
*       std::integral_constant<std::uint32_t, pow(2,15)>::value
*/

template<class T>
inline constexpr T pow(T const base, std::uint8_t const exponent)
{
	// Compute pow using exponentiation by squaring
	return (exponent == 0) ? 1 : (exponent % 2 == 0) ? pow(base, exponent / 2) * pow(base, exponent / 2) : base * pow(base, (exponent - 1) / 2) * pow(base, (exponent - 1) / 2);
}

/** Useful template to be used with streams, it prevents a char (or uint8_t) to be printed as a char instead of the numeric value */
template<typename T>
constexpr auto forceNumeric(T const t) noexcept
{
	static_assert(std::is_arithmetic<T>::value, "forceNumeric requires an arithmetic value");

	// Promote a built-in type to at least (unsigned)int
	return +t;
}

inline std::vector<std::string> tokenizeString(std::string const& inputString, char const separator, bool const emptyIsToken) noexcept
{
	auto const* const str = inputString.c_str();
	auto const pathLen = inputString.length();

	auto tokensArray = std::vector<std::string>{};
	auto startPos = size_t{ 0u };
	auto currentPos = size_t{ 0u };
	while (currentPos < pathLen)
	{
		// Check if we found our char
		while (str[currentPos] == separator && currentPos < pathLen)
		{
			auto const foundPos = currentPos;
			if (!emptyIsToken)
			{
				// And trim consecutive chars
				while (str[currentPos] == separator)
				{
					currentPos++;
				}
			}
			else
			{
				currentPos++;
			}
			auto const subStr = inputString.substr(startPos, foundPos - startPos);
			if (!subStr.empty() || emptyIsToken)
			{
				tokensArray.push_back(subStr);
			}
			startPos = currentPos;
		}
		currentPos++;
	}

	// Add what remains as a token (except if empty and !emptyIsToken)
	auto const subStr = inputString.substr(startPos, pathLen - startPos);
	if (!subStr.empty() || emptyIsToken)
	{
		tokensArray.push_back(subStr);
	}

	return tokensArray;
}

/** Useful template to convert the string representation of any integer to its underlying type. */
template<typename T>
T convertFromString(char const* const str)
{
	static_assert(sizeof(T) > 1, "convertFromString<T> must not be char type");

	if (std::strncmp(str, "0b", 2) == 0)
	{
		char* endptr = nullptr;
		auto c = std::strtoll(str + 2, &endptr, 2);
		if (endptr != nullptr && *endptr) // Conversion failed
		{
			throw std::invalid_argument(str);
		}
		return static_cast<T>(c);
	}
	std::stringstream ss;
	if (std::strncmp(str, "0x", 2) == 0 || std::strncmp(str, "0X", 2) == 0)
	{
		ss << std::hex;
	}

	ss << str;
	T out;
	ss >> out;
	if (ss.fail())
		throw std::invalid_argument(str);
	return out;
}

/** Useful template to convert any integer value to it's hex representation. Can be filled with zeros (ex: int16(0x123) = 0x0123) and printed in uppercase. */
template<typename T, size_t FillWidth = sizeof(T) * 2>
inline std::string toHexString(T const v, bool const zeroFilled = false, bool const upper = false) noexcept
{
	static_assert(std::numeric_limits<T>::is_integer, "toHexString requires an integer value");

	try
	{
		std::stringstream stream;
		stream << "0x";
		if (zeroFilled)
			stream << std::setfill('0') << std::setw(FillWidth);
		if (upper)
			stream << std::uppercase;
		stream << std::hex << forceNumeric(v);
		return stream.str();
	}
	catch (...)
	{
		return "[Invalid Conversion]";
	}
}

/** UniqueIdentifier overload */
template<>
inline std::string toHexString<UniqueIdentifier, sizeof(UniqueIdentifier::value_type) * 2>(UniqueIdentifier const v, bool const zeroFilled, bool const upper) noexcept
{
	return toHexString(v.getValue(), zeroFilled, upper);
}

/**
* @brief Returns the value of an enum as its underlying type.
* @param[in] e The enum to get the value of.
* @return The value of the enum as its underlying type.
*/
template<typename EnumType, typename = std::enable_if_t<std::is_enum<EnumType>::value>>
constexpr auto to_integral(EnumType e) noexcept
{
	return static_cast<std::underlying_type_t<EnumType>>(e);
}

/**
* @brief Hash function for hash-type std-containers.
* @details Hash function for hash-type std-containers when using an enum as key.
*/
struct EnumClassHash
{
	template<typename T>
	std::size_t operator()(T t) const
	{
		return static_cast<std::size_t>(t);
	}
};

/**
* @brief Traits to easily handle enums.
* @details Available traits for enum:
*  - is_bitfield: Enables operators and methods to manipulate an enum that represents a bitfield.
*/
template<typename EnumType, typename = std::enable_if_t<std::is_enum<EnumType>::value>>
struct enum_traits
{
};

/**
* @brief Traits to easily handle std::function.
* @details Available traits for std::function:
*  - size_type: The number of function parameters.
*  - result_type: The function result type.
*  - args_as_tuple: All parameter types packed in a tuple.
*  - function_type: The complete function type: std::function<Ret(Args...)>
*  - arg_type<0..(size_type-1)>: The individual type for each parameter.
* @tparam Ret The std::function return type.
* @tparam Args The std::function parameter types.
*/
template<typename Ret, typename... Args>
struct function_traits;

template<typename Ret, typename... Args>
struct function_traits<std::function<Ret(Args...)>>
{
	static size_t const size_type = sizeof...(Args);
	using result_type = Ret;
	using args_as_tuple = std::tuple<Args...>;
	using function_type = std::function<Ret(Args...)>;

	template<size_t N>
	using arg_type = typename std::tuple_element<N, std::tuple<Args...>>::type;
};

/**
* @brief Traits to easily handle closure types.
* @details Available traits for closure types like lambda, std::function, std::bind, etc:
*  - arg_count: The number of closure parameters.
*  - result_type: The closure result type.
*  - args_as_tuple: All parameter types packed in a tuple.
*  - closure_type: The complete closure type (eg. std::function<Ret(Args...)>)
*  - is_const: Whether the closure is const or not.
*  - arg_type<0..(arg_count-1)>: The individual type for each parameter. Might require to use typename and template to retrieve the type (eg typename closure_traits<Closure>::template arg_type<0>).
* @tparam Closure The closure type.
*/
template<typename Closure>
struct closure_traits : closure_traits<decltype(&Closure::operator())>
{
};

// std::function specialization
template<typename Ret, typename... Args>
struct closure_traits<std::function<Ret(Args...)>>
{
	static size_t const arg_count = sizeof...(Args);
	using result_type = Ret;
	using args_as_tuple = std::tuple<Args...>;
	using closure_type = std::function<Ret(Args...)>;
	using is_const = std::false_type;

	template<size_t N>
	using arg_type = typename std::tuple_element<N, std::tuple<Args...>>::type;
};

// Class const member specialization
template<typename Class, typename Ret, typename... Args>
struct closure_traits<Ret (Class::*)(Args...) const>
{
	static size_t const arg_count = sizeof...(Args);
	using result_type = Ret;
	using args_as_tuple = std::tuple<Args...>;
	using closure_type = Ret (Class::*)(Args...) const;
	using is_const = std::true_type;

	template<size_t N>
	using arg_type = typename std::tuple_element<N, std::tuple<Args...>>::type;
};

// Class member specialization
template<typename Class, typename Ret, typename... Args>
struct closure_traits<Ret (Class::*)(Args...)>
{
	static size_t const arg_count = sizeof...(Args);
	using result_type = Ret;
	using args_as_tuple = std::tuple<Args...>;
	using closure_type = Ret (Class::*)(Args...);
	using is_const = std::false_type;

	template<size_t N>
	using arg_type = typename std::tuple_element<N, std::tuple<Args...>>::type;
};

// Class const noexcept member specialization
template<typename Class, typename Ret, typename... Args>
struct closure_traits<Ret (Class::*)(Args...) const noexcept>
{
	static size_t const arg_count = sizeof...(Args);
	using result_type = Ret;
	using args_as_tuple = std::tuple<Args...>;
	using closure_type = Ret (Class::*)(Args...) const noexcept;
	using is_const = std::true_type;

	template<size_t N>
	using arg_type = typename std::tuple_element<N, std::tuple<Args...>>::type;
};

// Class noexcept member specialization
template<typename Class, typename Ret, typename... Args>
struct closure_traits<Ret (Class::*)(Args...) noexcept>
{
	static size_t const arg_count = sizeof...(Args);
	using result_type = Ret;
	using args_as_tuple = std::tuple<Args...>;
	using closure_type = Ret (Class::*)(Args...) noexcept;
	using is_const = std::false_type;

	template<size_t N>
	using arg_type = typename std::tuple_element<N, std::tuple<Args...>>::type;
};

// Function pointer specialization
template<typename Ret, typename... Args>
struct closure_traits<Ret (*)(Args...)>
{
	static size_t const arg_count = sizeof...(Args);
	using result_type = Ret;
	using args_as_tuple = std::tuple<Args...>;
	using closure_type = Ret (*)(Args...);
	using is_const = std::true_type;

	template<size_t N>
	using arg_type = typename std::tuple_element<N, std::tuple<Args...>>::type;
};

// Function const pointer specialization
template<typename Ret, typename... Args>
struct closure_traits<Ret (*const)(Args...)>
{
	static size_t const arg_count = sizeof...(Args);
	using result_type = Ret;
	using args_as_tuple = std::tuple<Args...>;
	using closure_type = Ret (*const)(Args...);
	using is_const = std::true_type;

	template<size_t N>
	using arg_type = typename std::tuple_element<N, std::tuple<Args...>>::type;
};

#if defined(_WIN32) && !defined(_WIN64)
// __stdcall function pointer specialization
template<typename Ret, typename... Args>
struct closure_traits<Ret(__stdcall*)(Args...)>
{
	static size_t const arg_count = sizeof...(Args);
	using result_type = Ret;
	using args_as_tuple = std::tuple<Args...>;
	using closure_type = Ret(__stdcall*)(Args...);
	using is_const = std::true_type;

	template<size_t N>
	using arg_type = typename std::tuple_element<N, std::tuple<Args...>>::type;
};
// __stdcall function const pointer specialization
template<typename Ret, typename... Args>
struct closure_traits<Ret(__stdcall* const)(Args...)>
{
	static size_t const arg_count = sizeof...(Args);
	using result_type = Ret;
	using args_as_tuple = std::tuple<Args...>;
	using closure_type = Ret(__stdcall* const)(Args...);
	using is_const = std::true_type;

	template<size_t N>
	using arg_type = typename std::tuple_element<N, std::tuple<Args...>>::type;
};
#endif // _WIN32 && !_WIN64

/** Class to easily manipulate an enum that represents a bitfield (strongly typed alternative to traits). */
template<typename EnumType, typename = std::enable_if_t<std::is_enum<EnumType>::value>>
class EnumBitfield final
{
public:
	using value_type = EnumType;
	using underlying_value_type = std::underlying_type_t<value_type>;
	static constexpr size_t value_size = sizeof(underlying_value_type) * 8;

	/** Iterator allowing quick enumeration of all the bits that are set in the bitfield */
	class iterator final
	{
		using self_name = iterator;
		static constexpr auto EndBit = value_size;

	public:
		using value_type = EnumType;
		using underlying_value_type = std::underlying_type_t<value_type>;
		using difference_type = size_t;
		using iterator_category = std::forward_iterator_tag;
		using reference = value_type&;
		using const_reference = value_type const&;
		using pointer = value_type*;
		using const_pointer = value_type const*;

		constexpr iterator(underlying_value_type const value, std::uint8_t const currentBitPosition) noexcept
			: _value(value)
			, _currentBitPosition(currentBitPosition)
		{
			findNextBitSet();
		}
		// Pre-increment operator
		constexpr self_name& operator++() noexcept
		{
			++_currentBitPosition;
			findNextBitSet();
			return *this;
		}
		// Post-increment operator
		constexpr self_name operator++(int) noexcept
		{
			auto tmp(*this);
			operator++();
			return tmp;
		}
		// Addition operator
		constexpr self_name operator+(size_t const count) const noexcept
		{
			auto tmp(*this);
			tmp.operator+=(count);
			return tmp;
		}
		// Addition assignment operator
		constexpr self_name& operator+=(size_t const count) noexcept
		{
			for (auto c = 0u; c < count; ++c)
			{
				if (_currentBitPosition == EndBit)
				{
					break;
				}
				++_currentBitPosition;
				findNextBitSet();
			}
			return *this;
		}
		constexpr value_type operator*() const noexcept
		{
			return static_cast<value_type>(_currentValue);
		}
		constexpr bool operator==(self_name const& other) const noexcept
		{
			return _currentBitPosition == other._currentBitPosition;
		}
		constexpr bool operator!=(self_name const& other) const noexcept
		{
			return !operator==(other);
		}

	private:
		constexpr void updateCurrentValue() noexcept
		{
			// Make a mask for current bit
			auto mask = pow(underlying_value_type(2), _currentBitPosition);

			// Extract the current bit
			_currentValue = _value & mask;
		}
		constexpr void findNextBitSet() noexcept
		{
			while (_currentBitPosition < EndBit)
			{
				updateCurrentValue();
				if (_currentValue != static_cast<underlying_value_type>(0))
				{
					break;
				}
				++_currentBitPosition;
			}
		}

		underlying_value_type const _value{ static_cast<underlying_value_type>(0) }; /** All possible Values this iterator can point to */
		std::uint8_t _currentBitPosition{ 0u }; /** Bit position the iterator is currently pointing to */
		underlying_value_type _currentValue{ static_cast<underlying_value_type>(0) }; /** Value the iterator is currently pointing to */
	};

	/** Construct a bitfield using individual bits passed as variadic parameters. If passed value is not valid (not exactly one bit set), this leads to undefined behavior. */
	template<typename... Values>
	constexpr explicit EnumBitfield(value_type const value, Values const... values) noexcept
		: _value(to_integral(value))
	{
		checkInvalidValue(value);
		(checkInvalidValue(values), ...);
		((_value |= to_integral(values)), ...);
	}

	/** Assigns the entire underlying bitfield with the passed value */
	void assign(underlying_value_type const value) noexcept
	{
		_value = value;
	}

	/** Returns true if the specified flag is set in the bitfield */
	constexpr bool test(value_type const flag) const noexcept
	{
		return (_value & to_integral(flag)) != static_cast<underlying_value_type>(0);
	}

	/** Sets the specified flag. If passed value is not valid (not exactly one bit set), this leads to undefined behavior. */
	EnumBitfield& set(value_type const flag) noexcept
	{
		checkInvalidValue(flag);
		_value |= to_integral(flag);
		return *this;
	}

	/** Clears the specified flag. If passed value is not valid (not exactly one bit set), this leads to undefined behavior. */
	EnumBitfield& reset(value_type const flag) noexcept
	{
		checkInvalidValue(flag);
		_value &= ~to_integral(flag);
		return *this;
	}

	/** Clears all the flags. */
	void clear() noexcept
	{
		_value = {};
	}

	/** Returns true if no bit is set */
	constexpr bool empty() const noexcept
	{
		return _value == static_cast<underlying_value_type>(0);
	}

	/** Returns the size number of bits the bitfield can hold */
	constexpr size_t size() const noexcept
	{
		return value_size;
	}

	/** Returns the number of bits that are set */
	constexpr size_t count() const noexcept
	{
		return countBits(_value);
	}

	/** Returns the underlying value of the bitfield */
	constexpr underlying_value_type value() const noexcept
	{
		return _value;
	}

	/** Comparison operator (equality) */
	constexpr bool operator==(EnumBitfield const other) const noexcept
	{
		return other._value == _value;
	}

	/** Comparison operator (difference) */
	constexpr bool operator!=(EnumBitfield const other) const noexcept
	{
		return !operator==(other);
	}

	/** OR EQUAL operator (sets the bits that are present in this or the other EnumBitfield, clears all other bits) */
	EnumBitfield& operator|=(EnumBitfield const other) noexcept
	{
		_value |= other._value;
		return *this;
	}

	/** AND EQUAL operator (sets the bits that are present in this and the other EnumBitfield, clears all other bits) */
	EnumBitfield& operator&=(EnumBitfield const other) noexcept
	{
		_value &= other._value;
		return *this;
	}

	/** OR operator (sets the bits that are present in either lhs or rhs, clears all other bits) */
	friend constexpr EnumBitfield operator|(EnumBitfield const lhs, EnumBitfield const rhs) noexcept
	{
		auto result = EnumBitfield{};
		result._value = lhs._value | rhs._value;
		return result;
	}

	/** AND operator (sets the bits that are present in both lhs and rhs, clears all other bits) */
	friend constexpr EnumBitfield operator&(EnumBitfield const lhs, EnumBitfield const rhs) noexcept
	{
		auto result = EnumBitfield{};
		result._value = lhs._value & rhs._value;
		return result;
	}

	/** Returns the value at the specified bit set position (only counting bits that are set). Specified position must be inclusively comprised btw 0 and (count() - 1) or an out_of_range exception will be thrown. */
	constexpr value_type at(size_t const setPosition) const
	{
		if (setPosition >= count())
		{
			throw std::out_of_range("EnumBitfield::at() out of range");
		}
		return *(begin() + setPosition);
	}

	/** Returns the bit set position for the specified value (only counting bits that are set). Specified value must be set or an out_of_range exception will be thrown. */
	constexpr size_t getBitSetPosition(value_type const value) const
	{
		checkInvalidValue(value);
		if (!test(value))
		{
			throw std::out_of_range("EnumBitfield::getBitSetPosition() out of range");
		}
		return getBitPosition(to_integral(value), _value);
	}

	/** Returns the bit position for the specified value. Specified value must has exactly one bit set or an out_of_range exception will be thrown. */
	static constexpr size_t getPosition(value_type const value)
	{
		checkInvalidValue(value);
		return getBitPosition(to_integral(value));
	}

	/** Returns the begin iterator */
	constexpr iterator begin() noexcept
	{
		return iterator(_value, 0);
	}

	/** Returns the begin const iterator */
	constexpr iterator const begin() const noexcept
	{
		return iterator(_value, 0);
	}

	/** Returns the end iterator */
	constexpr iterator end() noexcept
	{
		return iterator(_value, value_size);
	}

	/** Returns the end const iterator */
	constexpr iterator const end() const noexcept
	{
		return iterator(_value, value_size);
	}

	struct Hash
	{
		std::size_t operator()(EnumBitfield t) const
		{
			return std::hash<underlying_value_type>()(t._value);
		}
	};

	// Defaulted compiler auto-generated methods
	EnumBitfield() noexcept = default;
	EnumBitfield(EnumBitfield&&) noexcept = default;
	EnumBitfield(EnumBitfield const&) noexcept = default;
	EnumBitfield& operator=(EnumBitfield const&) noexcept = default;
	EnumBitfield& operator=(EnumBitfield&&) noexcept = default;

private:
	static constexpr void checkInvalidValue([[maybe_unused]] value_type const value)
	{
		if (countBits(to_integral(value)) != 1)
		{
			throw std::logic_error("Invalid value: not exactly one 1 bit set");
		}
	}
	static constexpr size_t countBits(underlying_value_type const value) noexcept
	{
		return (value == 0u) ? 0u : 1u + countBits(value & (value - 1u));
	}
	static constexpr size_t getBitPosition(underlying_value_type const value, underlying_value_type const setBitValue = static_cast<underlying_value_type>(-1)) noexcept
	{
		return (value == 1u) ? 0u : (setBitValue & 0x1) + getBitPosition(value >> 1, setBitValue >> 1);
	}

	underlying_value_type _value{};
};

} // namespace utils
} // namespace avdecc
} // namespace la

/* The following operator overloads are declared in the global namespace, so they can easily be accessed (as long as the traits are enabled for the desired enum) */

/**
* @brief operator& for a bitfield enum.
* @details The is_bitfield trait must be defined to true.
*/
template<typename EnumType>
constexpr std::enable_if_t<la::avdecc::utils::enum_traits<EnumType>::is_bitfield, EnumType> operator&(EnumType const lhs, EnumType const rhs)
{
	return static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(lhs) & static_cast<std::underlying_type_t<EnumType>>(rhs));
}

/**
* @brief operator| for a bitfield enum.
* @details The is_bitfield trait must be defined to true.
*/
template<typename EnumType>
constexpr std::enable_if_t<la::avdecc::utils::enum_traits<EnumType>::is_bitfield, EnumType> operator|(EnumType const lhs, EnumType const rhs)
{
	return static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(lhs) | static_cast<std::underlying_type_t<EnumType>>(rhs));
}

/**
* @brief operator|= for a bitfield enum.
* @details The is_bitfield trait must be defined to true.
*/
template<typename EnumType>
constexpr std::enable_if_t<la::avdecc::utils::enum_traits<EnumType>::is_bitfield, EnumType>& operator|=(EnumType& lhs, EnumType const rhs)
{
	lhs = lhs | rhs;
	return lhs;
}

/**
* @brief operator&= for a bitfield enum.
* @details The is_bitfield trait must be defined to true.
*/
template<typename EnumType>
constexpr std::enable_if_t<la::avdecc::utils::enum_traits<EnumType>::is_bitfield, EnumType>& operator&=(EnumType& lhs, EnumType const rhs)
{
	lhs = lhs & rhs;
	return lhs;
}

/**
* @brief operator~ for a bitfield enum.
* @details The is_bitfield trait must be defined to true.
*/
template<typename EnumType>
constexpr std::enable_if_t<la::avdecc::utils::enum_traits<EnumType>::is_bitfield, EnumType> operator~(EnumType e)
{
	return static_cast<EnumType>(~static_cast<std::underlying_type_t<EnumType>>(e));
}

namespace la
{
namespace avdecc
{
namespace utils
{
/**
* @brief Test method for a bitfield enum to check if the specified flag is set.
* @details The is_bitfield trait must be defined to true.
* @param[in] value Value to be tested for the presence of the specified flag.
* @param[in] flag Flag to test the presence of in specified value.
* @return Returns true if the specified flag is set in the specified value.
*/
template<typename EnumType>
constexpr std::enable_if_t<la::avdecc::utils::enum_traits<EnumType>::is_bitfield, bool> hasFlag(EnumType const value, EnumType const flag)
{
	return ::operator&(value, flag) != static_cast<EnumType>(0);
}

/**
* @brief Test method for a bitfield enum to check if any flag is set.
* @details The is_bitfield trait must be defined to true.
* @param[in] value Value to be tested for the presence of any flag.
* @return Returns true if any flag is set in the specified value.
*/
template<typename EnumType>
constexpr std::enable_if_t<la::avdecc::utils::enum_traits<EnumType>::is_bitfield, bool> hasAnyFlag(EnumType const value)
{
	return value != static_cast<EnumType>(0);
}

/**
* @brief Adds a flag to a bitfield enum. Multiple flags can be added at once if combined using operator|.
* @details The is_bitfield trait must be defined to true.
* @param[in] value Value to which the specified flag(s) is(are) to be added.
* @param[in] flag Flag(s) to be added to the specified value.
* @note Effectively equivalent to value |= flag
* @return Returns a copy of the modified value.
*/
template<typename EnumType>
constexpr std::enable_if_t<la::avdecc::utils::enum_traits<EnumType>::is_bitfield, EnumType> addFlag(EnumType& value, EnumType const flag)
{
	value |= flag;

	return value;
}

/**
* @brief Clears a flag from a bitfield enum. Multiple flags can be cleared at once if combined using operator|.
* @details The is_bitfield trait must be defined to true.
* @param[in] value Value to which the specified flag(s) is(are) to be removed.
* @param[in] flag Flag(s) to be cleared from the specified value.
* @note Effectively equivalent to value &= ~flag
* @return Returns a copy of the modified value.
*/
template<typename EnumType>
constexpr std::enable_if_t<la::avdecc::utils::enum_traits<EnumType>::is_bitfield, EnumType> clearFlag(EnumType& value, EnumType const flag)
{
	value &= ~flag;

	return value;
}

/** Return type of a closure. */
template<typename CallableType>
using CallableReturnType = typename closure_traits<std::remove_reference_t<CallableType>>::result_type;

/**
* @brief Function to safely call a handler (in the form of a std::function), forwarding all parameters to it.
* @param[in] handler The callable object to be invoked.
* @param[in] params The parameters to pass to the handler.
* @return The result of the handler, if any.
* @details Calls the specified handler in the current thread, protecting the caller from any thrown exception in the handler itself.
*/
template<typename CallableType, typename... Ts>
CallableReturnType<CallableType> invokeProtectedHandler(CallableType&& handler, Ts&&... params) noexcept
{
	if (handler)
	{
		try
		{
			if constexpr (std::is_same_v<CallableReturnType<CallableType>, void>)
			{
				handler(std::forward<Ts>(params)...);
			}
			else
			{
				return handler(std::forward<Ts>(params)...);
			}
		}
		catch (std::exception const& e)
		{
			/* Forcing the assert to fail, but with code so we don't get a warning on gcc */
			AVDECC_ASSERT(handler == nullptr, (std::string("invokeProtectedHandler caught an exception in handler: ") + e.what()).c_str());
			(void)e;
		}
		catch (...)
		{
		}
	}

	if constexpr (!std::is_same_v<CallableReturnType<CallableType>, void>)
	{
		return CallableReturnType<CallableType>{};
	}
}

/**
* @brief Function to safely call a class method, forwarding all parameters to it.
* @return The result of the method, if any.
* @details Calls the specified class method, protecting the caller from any thrown exception in the handler itself.
*/
template<typename Method, class Object, typename... Parameters>
CallableReturnType<Method> invokeProtectedMethod(Method&& method, Object* const object, Parameters&&... params) noexcept
{
	if (method != nullptr && object != nullptr)
	{
		try
		{
			if constexpr (std::is_same_v<CallableReturnType<Method>, void>)
			{
				(object->*method)(std::forward<Parameters>(params)...);
			}
			else
			{
				return (object->*method)(std::forward<Parameters>(params)...);
			}
		}
		catch (std::exception const& e)
		{
			/* Forcing the assert to fail, but with code so we don't get a warning on gcc */
			AVDECC_ASSERT(method == nullptr, (std::string("invokeProtectedMethod caught an exception in method: ") + e.what()).c_str());
			(void)e;
		}
		catch (...)
		{
		}
	}

	if constexpr (!std::is_same_v<CallableReturnType<Method>, void>)
	{
		return CallableReturnType<Method>{};
	}
}

/** Useful template to create strongly typed defines that can be extended using inheritance */
template<class Derived, typename DataType, typename = std::enable_if_t<std::is_arithmetic<DataType>::value || std::is_enum<DataType>::value>>
class TypedDefine
{
public:
	using derived_type = Derived;
	using value_type = DataType;

	explicit TypedDefine(value_type const value) noexcept
		: _value(value)
	{
	}

	value_type getValue() const noexcept
	{
		return _value;
	}

	void setValue(value_type const value) noexcept
	{
		_value = value;
	}

	explicit operator value_type() const noexcept
	{
		return _value;
	}

	bool operator==(TypedDefine const& v) const noexcept
	{
		return v.getValue() == _value;
	}

	bool operator!=(TypedDefine const& v) const noexcept
	{
		return !operator==(v);
	}

	friend auto operator&(TypedDefine const& lhs, TypedDefine const& rhs)
	{
		return static_cast<derived_type>(lhs._value & rhs._value);
	}

	friend auto operator|(TypedDefine const& lhs, TypedDefine const& rhs)
	{
		return static_cast<derived_type>(lhs._value | rhs._value);
	}

	struct Hash
	{
		std::size_t operator()(TypedDefine t) const
		{
			return static_cast<std::size_t>(t._value);
		}
	};

	// Defaulted compiler auto-generated methods
	TypedDefine(TypedDefine&&) = default;
	TypedDefine(TypedDefine const&) = default;
	TypedDefine& operator=(TypedDefine const&) = default;
	TypedDefine& operator=(TypedDefine&&) = default;

private:
	value_type _value{};
};

} // namespace utils
} // namespace avdecc
} // namespace la

namespace la
{
namespace avdecc
{
namespace utils
{
/** EmptyLock implementing the BasicLockable concept. Can be used as template parameter for Observer and Subject classes in this file. */
class EmptyLock
{
public:
	void lock() const noexcept {}
	void unlock() const noexcept {}
	// Defaulted compiler auto-generated methods
	EmptyLock() noexcept = default;
	~EmptyLock() noexcept = default;
	EmptyLock(EmptyLock&&) noexcept = default;
	EmptyLock(EmptyLock const&) noexcept = default;
	EmptyLock& operator=(EmptyLock const&) noexcept = default;
	EmptyLock& operator=(EmptyLock&&) noexcept = default;
};

// Forward declare Subject template class
template<class Derived, class Mut>
class Subject;
// Forward declare ObserverGuard template class
template<class ObserverType>
class ObserverGuard;

/** Dummy struct required to postpone the Observer template resolution when it's actually needed. This is required because of the forward declaration of the Subject template class, in order to access it's mutex_type typedef. */
template<typename Subject>
struct GetMutexType
{
	using type = typename Subject::mutex_type;
};

/**
* @brief An observer base class.
* @details Observer base class using RAII concept to automatically remove itself from Subjects upon destruction.
* @note This class is thread-safe and the mutex type can be specified as template parameter.<BR>
*       A no lock version is possible using EmptyLock instead of a real mutex type.
* @warning Not catching std::system_error from mutex, which will cause std::terminate() to be called if a critical system error occurs
*/
template<class Observable>
class Observer
{
public:
	using mutex_type = typename GetMutexType<Observable>::type;

	virtual ~Observer() noexcept
	{
		// Lock Subjects
		try
		{
			std::lock_guard<decltype(_mutex)> const lg(_mutex);
			AVDECC_ASSERT(_subjects.empty(), "All subjects must be unregistered before Observer is destroyed. Either manually call subject->unregisterObserver or add an ObserverGuard member at the end of your Observer derivated class.");
			for (auto subject : _subjects)
			{
				try
				{
					subject->removeObserver(this);
				}
				catch (std::invalid_argument const&)
				{
					// Ignore error
				}
			}
		}
		catch (...)
		{
		}
	}

	/**
	* @brief Gets the count of currently registered Subjects.
	* @return The count of currently registered Subjects.
	*/
	size_t countSubjects() const noexcept
	{
		// Lock Subjects
		std::lock_guard<decltype(_mutex)> const lg(_mutex);
		return _subjects.size();
	}

	// Defaulted compiler auto-generated methods
	Observer() = default;
	Observer(Observer&&) = default;
	Observer(Observer const&) = default;
	Observer& operator=(Observer const&) = default;
	Observer& operator=(Observer&&) = default;

private:
	friend class Subject<Observable, mutex_type>;
	template<class ObserverType>
	friend class ObserverGuard;

	void removeAllSubjects() noexcept
	{
		// Lock Subjects
		std::lock_guard<decltype(_mutex)> const lg(_mutex);
		for (auto subject : _subjects)
		{
			try
			{
				subject->removeObserver(this);
			}
			catch (std::invalid_argument const&)
			{
				// Ignore error
			}
		}
		_subjects.clear();
	}

	void registerSubject(Observable const* const subject) const
	{
		// Lock Subjects
		std::lock_guard<decltype(_mutex)> const lg(_mutex);
		// Search if observer already registered
		if (_subjects.find(subject) != _subjects.end())
			throw std::invalid_argument("Subject already registered");
		// Add Subject
		_subjects.insert(subject);
	}

	void unregisterSubject(Observable const* const subject) const
	{
		// Lock Subjects
		std::lock_guard<decltype(_mutex)> const lg(_mutex);
		// Search if Subject is registered
		auto const it = _subjects.find(subject);
		if (it == _subjects.end())
			throw std::invalid_argument("Subject not registered");
		// Remove Subject
		_subjects.erase(it);
	}

	// Private variables
	mutable mutex_type _mutex{};
	mutable std::set<Observable const*> _subjects{};
};

/**
* @brief An Observer guard class to allow safe RAII usage of an Observer class.
* @details This class is intended to be used in conjunction with an Observer class to allow safe RAII usage of the Observer.<BR>
*          Because a class has to inherit from an Observer to override notification handlers, it means by the time the Observer destructor is called
*          the derivated class portion has already been destroyed. If the subject is notifying observers at the same time from another thread, the derivated
*          vtable is no longer valid and will cause a crash. This class can be used for full RAII by simply declaring a member of this type in the derivated class,
*          at the end of the data members list.
* @warning Not catching std::system_error from mutex, which will cause std::terminate() to be called if a critical system error occurs
*/
template<class ObserverType>
class ObserverGuard final
{
public:
	ObserverGuard(ObserverType& observer) noexcept
		: _observer(observer)
	{
	}

	~ObserverGuard() noexcept
	{
		_observer.removeAllSubjects();
	}

	// Defaulted compiler auto-generated methods
	ObserverGuard(ObserverGuard&&) noexcept = default;
	ObserverGuard(ObserverGuard const&) noexcept = default;
	ObserverGuard& operator=(ObserverGuard const&) noexcept = default;
	ObserverGuard& operator=(ObserverGuard&&) noexcept = default;

private:
	ObserverType& _observer{ nullptr };
};

/**
* @brief A Subject base class.
* @details Subject base class using RAII concept to automatically remove itself from observers upon destruction.<BR>
*          The convenience method #notifyObservers or #notifyObserversMethod shall be used to notify all observers in a thread-safe way.
* @note This class is thread-safe and the mutex type can be specified as template parameter.<BR>
*       A no lock version is possible using EmptyLock instead of a real mutex type.<BR>
*       Subject and Observer classes use CRTP pattern.
* @warning If the derived class wants to allow observers to be destroyed inside an event handler, it shall use a
*          recursive mutex kind as template parameter.<BR>
*          Not catching std::system_error from mutex, which will cause std::terminate() to be called if a critical system error occurs
*/
template<class Derived, class Mut>
class Subject
{
public:
	using mutex_type = Mut;
	using observer_type = Observer<Derived>;

	void registerObserver(observer_type* const observer) const
	{
		if (observer == nullptr)
			throw std::invalid_argument("Observer cannot be nullptr");

		auto isFirst = false;
		{
			// Lock observers
			std::lock_guard<decltype(_mutex)> const lg(_mutex);
			// Search if observer already registered
			if (_observers.find(observer) != _observers.end())
				throw std::invalid_argument("Observer already registered");
			// Register to the observer
			try
			{
				observer->registerSubject(self());
			}
			catch (std::invalid_argument const&)
			{
				// Already registered
			}
			// Add observer
			isFirst = _observers.empty();
			_observers.insert(observer);
		}

		if (isFirst)
		{
			// Inform the subject that the first observer has registered
			try
			{
				const_cast<Subject*>(this)->onFirstObserverRegistered();
			}
			catch (...)
			{
				// Ignore exceptions in handler
			}
		}
		// Inform the subject that a new observer has registered
		try
		{
			const_cast<Subject*>(this)->onObserverRegistered(observer);
		}
		catch (...)
		{
			// Ignore exceptions in handler
		}
	}

	/**
	* @brief Unregisters an observer.
	* @details Unregisters an observer from the subject.
	* @param[in] observer The observer to remove from the list.
	* @note If the observer has never been registered, or has already been
	*       unregistered, this method will throw an std::invalid_argument.
	*       If you try to unregister an observer during a notification without
	*       using a recursive mutex kind, this method will throw a std::system_error
	*       and the observer will not be removed (strong exception guarantee).
	*/
	void unregisterObserver(observer_type* const observer) const
	{
		if (observer == nullptr)
			throw std::invalid_argument("Observer cannot be nullptr");

		// Unregister from the observer
		try
		{
			observer->unregisterSubject(self());
		}
		catch (std::invalid_argument const&)
		{
			// Already unregistered, but we continue so the removeObserver method will throw to the user
		}
		catch (...) // Catch mutex errors (or any other)
		{
			// Rethrow the last exception without trying to remove the observer
			throw;
		}
		// Remove observer
		try
		{
			removeObserver(observer);
		}
		catch (std::invalid_argument const&)
		{
			// Already unregistered, rethrow to the user
			throw;
		}
		catch (...) // Catch mutex errors (or any other)
		{
			// Restore the state by re-registering to the observer, then rethraw
			observer->registerSubject(self());
			throw;
		}
	}

	/** BasicLockable concept 'lock' method for the whole Subject */
	void lock() noexcept
	{
		_mutex.lock();
	}

	/** BasicLockable concept 'unlock' method for the whole Subject */
	void unlock() noexcept
	{
		_mutex.unlock();
	}

	/**
	* @brief Gets the count of currently registered observers.
	* @return The count of currently registered observers.
	*/
	size_t countObservers() const noexcept
	{
		// Lock observers
		std::lock_guard<decltype(_mutex)> const lg(_mutex);
		return _observers.size();
	}

	/**
	* @brief Checks if specified observer is currently registered
	* @param[in] observer Observer to check for
	* @return Returns true if the specified observer is currently registered, false otherwise.
	*/
	bool isObserverRegistered(observer_type* const observer) const noexcept
	{
		// Lock observers
		std::lock_guard<decltype(_mutex)> const lg(_mutex);
		// Search if observer is registered
		return _observers.find(observer) != _observers.end();
	}

	virtual ~Subject() noexcept
	{
		removeAllObservers();
	}

	// Defaulted compiler auto-generated methods
	Subject() = default;
	Subject(Subject&&) = default;
	Subject(Subject const&) = default;
	Subject& operator=(Subject const&) = default;
	Subject& operator=(Subject&&) = default;

protected:
#ifdef DEBUG
	mutex_type& getMutex() noexcept
	{
		return _mutex;
	}

	mutex_type const& getMutex() const noexcept
	{
		return _mutex;
	}
#endif // DEBUG

	/**
	* @brief Convenience method to notify all observers.
	* @details Convenience method to notify all observers in a thread-safe way.
	* @param[in] evt An std::function to be called for each registered observer.
	* @note The internal class lock will be taken during the whole call, meaning
	*       you cannot call another method of this class in the provided event handler
	*       except if the class template parameter is a recursive mutex kind (or EmptyLock).
	*/
	template<class DerivedObserver>
	void notifyObservers(std::function<void(DerivedObserver* obs)> const& evt) const noexcept
	{
		// Lock observers
		std::lock_guard<decltype(_mutex)> const lg(_mutex);
		_iteratingNotify = true;
		// Call each observer
		for (auto it = _observers.begin(); it != _observers.end(); ++it)
		{
			// Do not call an observer in the to be removed list
			if (_toBeRemoved.find(*it) != _toBeRemoved.end())
				continue;
			// Using try-catch to protect ourself from errors in the handler
			try
			{
				evt(static_cast<DerivedObserver*>(*it));
			}
			catch (...)
			{
			}
		}
		_iteratingNotify = false;
		// Time to remove observers scheduled for removal inside the notification loop
		for (auto* obs : _toBeRemoved)
		{
			try
			{
				removeObserver(obs);
			}
			catch (...)
			{
				// Don't care about already unregistered observer
			}
		}
		_toBeRemoved.clear();
	}

	/**
	* @brief Convenience method to notify all observers.
	* @details Convenience method to notify all observers in a thread-safe way.
	* @param[in] method The Observer method to be called. The first parameter of the method should always be self().
	* @param[in] params Variadic parameters to be forwarded to the method for each observer.
	* @note The internal class lock will be taken during the whole call, meaning
	*       you cannot call another method of this class in the provided event handler
	*       except if the class template parameter is a recursive mutex kind (or EmptyLock).
	*/
	template<class DerivedObserver, typename Method, typename... Parameters>
	void notifyObserversMethod(Method&& method, Parameters&&... params) const noexcept
	{
		if (method != nullptr)
		{
			// Lock observers
			std::lock_guard<decltype(_mutex)> const lg(_mutex);
			_iteratingNotify = true;
			// Call each observer
			for (auto it = _observers.begin(); it != _observers.end(); ++it)
			{
				// Do not call an observer in the to be removed list
				if (_toBeRemoved.find(*it) != _toBeRemoved.end())
					continue;
				// Using try-catch to protect ourself from errors in the handler
				try
				{
					(static_cast<DerivedObserver*>(*it)->*method)(std::forward<Parameters>(params)...);
				}
				catch (...)
				{
				}
			}
			_iteratingNotify = false;
			// Time to remove observers scheduled for removal inside the notification loop
			for (auto* obs : _toBeRemoved)
			{
				try
				{
					removeObserver(obs);
				}
				catch (...)
				{
					// Don't care about already unregistered observer
				}
			}
			_toBeRemoved.clear();
		}
	}

	/** Remove all observers from the subject. */
	void removeAllObservers() noexcept
	{
		// Unregister from all the observers
		{
			// Lock observers
			std::lock_guard<decltype(_mutex)> const lg(_mutex);
			for (auto const obs : _observers)
			{
				try
				{
					obs->unregisterSubject(self());
				}
				catch (std::invalid_argument const&)
				{
					// Ignore error
				}
			}
			_observers.clear();
		}
		// Inform the subject that the last observer has unregistered
		try
		{
			const_cast<Subject*>(this)->onLastObserverUnregistered();
		}
		catch (...)
		{
			// Ignore exceptions in handler
		}
	}

	/** Allow the Subject to be informed when the first observer has registered. */
	virtual void onFirstObserverRegistered() noexcept {}
	/** Allow the Subject to be informed when a new observer has registered. */
	virtual void onObserverRegistered(observer_type* const /*observer*/) noexcept {}
	/** Allow the Subject to be informed when an observer has unregistered. */
	virtual void onObserverUnregistered(observer_type* const /*observer*/) noexcept {}
	/** Allow the Subject to be informed when the last observer has unregistered. */
	virtual void onLastObserverUnregistered() noexcept {}

	/** Convenience method to return this as the real Derived class type */
	Derived* self() noexcept
	{
		return static_cast<Derived*>(this);
	}

	/** Convenience method to return this as the real const Derived class type */
	Derived const* self() const noexcept
	{
		return static_cast<Derived const*>(this);
	}

private:
	friend class Observer<Derived>;

	void removeObserver(observer_type* const observer) const
	{
		auto isLast = false;
		{
			// Lock observers
			std::lock_guard<decltype(_mutex)> const lg(_mutex);
			// Search if observer is registered
			auto const it = _observers.find(observer);
			if (it == _observers.end())
				throw std::invalid_argument("Observer not registered");
			// Check if we are currently iterating notification
			if (_iteratingNotify)
				_toBeRemoved.insert(observer); // Schedule destruction for later
			else
				_observers.erase(it); // Remove observer immediately
			isLast = _observers.empty();
		}
		// Inform the subject that an observer has unregistered
		try
		{
			const_cast<Subject*>(this)->onObserverUnregistered(observer);
		}
		catch (...)
		{
			// Ignore exceptions in handler
		}
		if (isLast)
		{
			// Inform the subject that the last observer has unregistered
			try
			{
				const_cast<Subject*>(this)->onLastObserverUnregistered();
			}
			catch (...)
			{
				// Ignore exceptions in handler
			}
		}
	}

	// Private variables
	mutable Mut _mutex{};
	mutable std::set<observer_type*> _observers{};
	mutable bool _iteratingNotify{ false }; // Are we currently notifying observers
	mutable std::set<observer_type*> _toBeRemoved{};
};


/**
* @brief A Subject derived template class with tag dispatching.
* @details Subject derived template class publicly exposing notifyObservers and notifyObserversMethod.<BR>
*          Use this template along with a using directive to declare a tag dispatched Subject, so it can be used
*          as a class member (with direct access to notify methods).
*          Ex: using MySubject = TypedSubject<struct MySubjectTag, std::mutex>;
*/
template<typename Tag, class Mut>
class TypedSubject : public Subject<TypedSubject<Tag, Mut>, Mut>
{
public:
	using Subject<TypedSubject<Tag, Mut>, Mut>::notifyObservers;
	using Subject<TypedSubject<Tag, Mut>, Mut>::notifyObserversMethod;
	using Subject<TypedSubject<Tag, Mut>, Mut>::removeAllObservers;
};

#define DECLARE_AVDECC_OBSERVER_GUARD_NAME(selfClassType, variableName) \
	friend class la::avdecc::utils::ObserverGuard<selfClassType>; \
	la::avdecc::utils::ObserverGuard<selfClassType> variableName \
	{ \
		*this \
	}

#define DECLARE_AVDECC_OBSERVER_GUARD(selfClassType) \
	friend class la::avdecc::utils::ObserverGuard<selfClassType>; \
	la::avdecc::utils::ObserverGuard<selfClassType> _observer_guard_ \
	{ \
		*this \
	}

} // namespace utils
} // namespace avdecc
} // namespace la
