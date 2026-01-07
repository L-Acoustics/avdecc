/*
* Copyright (C) 2016-2026, L-Acoustics and its contributors

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
* @file endian.hpp
* @author Christophe Calmejane
* @brief Arch dependent defines based on endianness.
*/

#pragma once

/* GNU libc provides a header defining __BYTE_ORDER, or _BYTE_ORDER.
* And some OSs provide some for of endian header also.
*/
#if defined(__APPLE__)
#	include <machine/endian.h>
#elif defined(__GNUC__)
#	include <endian.h>
#endif

#if defined(__BYTE_ORDER)
#	if defined(__BIG_ENDIAN) && (__BYTE_ORDER == __BIG_ENDIAN)
#		define AVDECC_BIG_ENDIAN
#	endif
#	if defined(__LITTLE_ENDIAN) && (__BYTE_ORDER == __LITTLE_ENDIAN)
#		define AVDECC_LITTLE_ENDIAN
#	endif
#endif
#if !defined(__BYTE_ORDER) && defined(_BYTE_ORDER)
#	if defined(_BIG_ENDIAN) && (_BYTE_ORDER == _BIG_ENDIAN)
#		define AVDECC_BIG_ENDIAN
#	endif
#	if defined(_LITTLE_ENDIAN) && (_BYTE_ORDER == _LITTLE_ENDIAN)
#		define AVDECC_LITTLE_ENDIAN
#	endif
#endif


/* Built-in byte-swpped big-endian macros.
*/
#if (defined(__BIG_ENDIAN__) && !defined(__LITTLE_ENDIAN__)) || (defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)) || defined(__ARMEB__) || defined(__THUMBEB__) || defined(__AARCH64EB__) || defined(_MIPSEB) || defined(__MIPSEB) || defined(__MIPSEB__)
#	define AVDECC_BIG_ENDIAN
#endif

/* Built-in byte-swpped little-endian macros.
*/
#if (defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)) || (defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)) || defined(__ARMEL__) || defined(__THUMBEL__) || defined(__AARCH64EL__) || defined(_MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__)
#	define AVDECC_LITTLE_ENDIAN
#endif

/* Some architectures are strictly one endianess (as opposed
* the current common bi-endianess).
*/
#if defined(i386) || defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__) || defined(__i386) || defined(_M_IX86) || defined(_X86_) || defined(__THW_INTEL__) || defined(__I86__) || defined(__INTEL__) || defined(__x86_64) || defined(__x86_64__) || defined(__amd64__) || defined(__amd64) || defined(_M_X64)
#	define AVDECC_LITTLE_ENDIAN
#endif

#include <climits>
#include <cstdint>

namespace la
{
namespace avdecc
{
enum class Endianness
{
	Unknown,
	LittleEndian,
	BigEndian,
	NetworkEndian = BigEndian,
#if defined(AVDECC_LITTLE_ENDIAN)
	HostEndian = LittleEndian,
	InvertHostEndian = BigEndian,
#elif defined(AVDECC_BIG_ENDIAN)
	HostEndian = BigEndian,
	InvertHostEndian = LittleEndian,
#else
#	error "Unknown host endianness"
#endif
};

namespace detail
{
template<typename T>
T swapBytes(T const& u)
{
	static_assert(CHAR_BIT == 8, "CHAR_BIT != 8");

	union
	{
		T u{};
		unsigned char u8[sizeof(T)];
	} source, dest;

	source.u = u;

	for (size_t k = 0; k < sizeof(T); k++)
		dest.u8[k] = source.u8[sizeof(T) - k - 1];

	return dest.u;
}

template<Endianness from, Endianness to, typename T>
struct endianSwap
{
	inline T operator()(T const& u)
	{
		return swapBytes<T>(u);
	}
};

// Specializations when attempting to swap to the same endianess
template<typename T>
struct endianSwap<Endianness::LittleEndian, Endianness::LittleEndian, T>
{
	inline T operator()(T const& value)
	{
		return value;
	}
};
template<typename T>
struct endianSwap<Endianness::BigEndian, Endianness::BigEndian, T>
{
	inline T operator()(T const& value)
	{
		return value;
	}
};

} // namespace detail

template<Endianness from, Endianness to, typename T>
inline T endianSwap(T const& u)
{
	static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8, "Unsupported value size");
	//static_assert(std::is_arithmetic<T>::value, "Only supporting arithmetic types");

	return detail::endianSwap<from, to, T>()(u);
}

} // namespace avdecc
} // namespace la

#define AVDECC_PACK_TYPE(x, y) la::avdecc::endianSwap<la::avdecc::Endianness::HostEndian, la::avdecc::Endianness::NetworkEndian, y>(x)
#define AVDECC_PACK_WORD(x) la::avdecc::endianSwap<la::avdecc::Endianness::HostEndian, la::avdecc::Endianness::NetworkEndian, std::uint16_t>(x)
#define AVDECC_PACK_DWORD(x) la::avdecc::endianSwap<la::avdecc::Endianness::HostEndian, la::avdecc::Endianness::NetworkEndian, std::uint32_t>(x)
#define AVDECC_PACK_QWORD(x) la::avdecc::endianSwap<la::avdecc::Endianness::HostEndian, la::avdecc::Endianness::NetworkEndian, std::uint64_t>(x)

#define AVDECC_UNPACK_TYPE(x, y) la::avdecc::endianSwap<la::avdecc::Endianness::NetworkEndian, la::avdecc::Endianness::HostEndian, y>(x)
#define AVDECC_UNPACK_WORD(x) la::avdecc::endianSwap<la::avdecc::Endianness::NetworkEndian, la::avdecc::Endianness::HostEndian, std::uint16_t>(x)
#define AVDECC_UNPACK_DWORD(x) la::avdecc::endianSwap<la::avdecc::Endianness::NetworkEndian, la::avdecc::Endianness::HostEndian, std::uint32_t>(x)
#define AVDECC_UNPACK_QWORD(x) la::avdecc::endianSwap<la::avdecc::Endianness::NetworkEndian, la::avdecc::Endianness::HostEndian, std::uint64_t>(x)
