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
* @file entityModelTypes.hpp
* @author Christophe Calmejane
* @brief Avdecc entity model types.
*/

#pragma once

#include "la/avdecc/utils.hpp"

#include "uniqueIdentifier.hpp"
#include "exports.hpp"

#include <cstdint>
#include <string>
#include <array>
#include <vector>
#include <iostream>
#include <cstring> // std::memcpy

namespace la
{
namespace avdecc
{
namespace entity
{
namespace model
{
using ConfigurationIndex = std::uint16_t;
using DescriptorIndex = std::uint16_t;
using AudioUnitIndex = DescriptorIndex;
using StreamIndex = DescriptorIndex;
using JackIndex = DescriptorIndex;
using AvbInterfaceIndex = DescriptorIndex;
using ClockSourceIndex = DescriptorIndex;
using MemoryObjectIndex = DescriptorIndex;
using LocaleIndex = DescriptorIndex;
using StringsIndex = DescriptorIndex;
using StreamPortIndex = DescriptorIndex;
using ExternalPortIndex = DescriptorIndex;
using InternalPortIndex = DescriptorIndex;
using ClusterIndex = DescriptorIndex;
using MapIndex = DescriptorIndex;
using ControlIndex = DescriptorIndex;
using SignalSelectorIndex = DescriptorIndex;
using MixerIndex = DescriptorIndex;
using MatrixIndex = DescriptorIndex;
using SignalSplitterIndex = DescriptorIndex;
using SignalCombinerIndex = DescriptorIndex;
using SignalDemultiplexerIndex = DescriptorIndex;
using SignalMultiplexerIndex = DescriptorIndex;
using SignalTranscoderIndex = DescriptorIndex;
using ClockDomainIndex = DescriptorIndex;
using ControlBlockIndex = DescriptorIndex;
using DescriptorCounterValidFlag = std::uint32_t; /** Counters valid flag - Clause 7.4.42 */
using DescriptorCounter = std::uint32_t; /** Counter - Clause 7.4.42 */
using OperationID = std::uint16_t; /** OperationID for OPERATIONS returned by an entity to a controller - Clause 7.4.53 */

/** Descriptor Type - Clause 7.2 */
enum class DescriptorType : std::uint16_t
{
	Entity = 0x0000,
	Configuration = 0x0001,
	AudioUnit = 0x0002,
	VideoUnit = 0x0003,
	SensorUnit = 0x0004,
	StreamInput = 0x0005,
	StreamOutput = 0x0006,
	JackInput = 0x0007,
	JackOutput = 0x0008,
	AvbInterface = 0x0009,
	ClockSource = 0x000a,
	MemoryObject = 0x000b,
	Locale = 0x000c,
	Strings = 0x000d,
	StreamPortInput = 0x000e,
	StreamPortOutput = 0x000f,
	ExternalPortInput = 0x0010,
	ExternalPortOutput = 0x0011,
	InternalPortInput = 0x0012,
	InternalPortOutput = 0x0013,
	AudioCluster = 0x0014,
	VideoCluster = 0x0015,
	SensorCluster = 0x0016,
	AudioMap = 0x0017,
	VideoMap = 0x0018,
	SensorMap = 0x0019,
	Control = 0x001a,
	SignalSelector = 0x001b,
	Mixer = 0x001c,
	Matrix = 0x001d,
	MatrixSignal = 0x001e,
	SignalSplitter = 0x001f,
	SignalCombiner = 0x0020,
	SignalDemultiplexer = 0x0021,
	SignalMultiplexer = 0x0022,
	SignalTranscoder = 0x0023,
	ClockDomain = 0x0024,
	ControlBlock = 0x0025,
	/* 0026 to fffe reserved for future use */
	Invalid = 0xffff
};
constexpr bool operator!(DescriptorType const lhs)
{
	return lhs == DescriptorType::Invalid;
}

constexpr bool operator==(DescriptorType const lhs, DescriptorType const rhs)
{
	return static_cast<std::underlying_type_t<DescriptorType>>(lhs) == static_cast<std::underlying_type_t<DescriptorType>>(rhs);
}

constexpr bool operator==(DescriptorType const lhs, std::underlying_type_t<DescriptorType> const rhs)
{
	return static_cast<std::underlying_type_t<DescriptorType>>(lhs) == rhs;
}

LA_AVDECC_API std::string LA_AVDECC_CALL_CONVENTION descriptorTypeToString(DescriptorType const descriptorType) noexcept;

/** Jack Type - Clause 7.2.7.2 */
enum class JackType : std::uint16_t
{
	Speaker = 0x0000,
	Headphone = 0x0001,
	AnalogMicrophone = 0x0002,
	Spdif = 0x0003,
	Adat = 0x0004,
	Tdif = 0x0005,
	Madi = 0x0006,
	UnbalancedAnalog = 0x0007,
	BalancedAnalog = 0x0008,
	Digital = 0x0009,
	Midi = 0x000a,
	AesEbu = 0x000b,
	CompositeVideo = 0x000c,
	SVhsVideo = 0x000d,
	ComponentVideo = 0x000e,
	Dvi = 0x000f,
	Hdmi = 0x0010,
	Udi = 0x0011,
	DisplayPort = 0x0012,
	Antenna = 0x0013,
	AnalogTuner = 0x0014,
	Ethernet = 0x0015,
	Wifi = 0x0016,
	Usb = 0x0017,
	Pci = 0x0018,
	PciE = 0x0019,
	Scsi = 0x001a,
	Ata = 0x001b,
	Imager = 0x001c,
	Ir = 0x001d,
	Thunderbolt = 0x001e,
	Sata = 0x001f,
	SmpteLtc = 0x0020,
	DigitalMicrophone = 0x0021,
	AudioMediaClock = 0x0022,
	VideoMediaClock = 0x0023,
	GnssClock = 0x0024,
	Pps = 0x0025,
	/* 0026 to fffe reserved for future use */
	Expansion = 0xffff
};
constexpr bool operator==(JackType const lhs, JackType const rhs)
{
	return static_cast<std::underlying_type_t<JackType>>(lhs) == static_cast<std::underlying_type_t<JackType>>(rhs);
}

constexpr bool operator==(JackType const lhs, std::underlying_type_t<JackType> const rhs)
{
	return static_cast<std::underlying_type_t<JackType>>(lhs) == rhs;
}

LA_AVDECC_API std::string LA_AVDECC_CALL_CONVENTION jackTypeToString(JackType const jackType) noexcept;

/** ClockSource Type - Clause 7.2.9.2 */
enum class ClockSourceType : std::uint16_t
{
	Internal = 0x0000,
	External = 0x0001,
	InputStream = 0x0002,
	/* 0003 to fffe reserved for future use */
	Expansion = 0xffff
};
constexpr bool operator==(ClockSourceType const lhs, ClockSourceType const rhs)
{
	return static_cast<std::underlying_type_t<ClockSourceType>>(lhs) == static_cast<std::underlying_type_t<ClockSourceType>>(rhs);
}

constexpr bool operator==(ClockSourceType const lhs, std::underlying_type_t<ClockSourceType> const rhs)
{
	return static_cast<std::underlying_type_t<ClockSourceType>>(lhs) == rhs;
}

LA_AVDECC_API std::string LA_AVDECC_CALL_CONVENTION clockSourceTypeToString(ClockSourceType const clockSourceType) noexcept;

/** MemoryObject Type - Clause 7.2.10.1 */
enum class MemoryObjectType : std::uint16_t
{
	FirmwareImage = 0x0000,
	VendorSpecific = 0x0001,
	CrashDump = 0x0002,
	LogObject = 0x0003,
	AutostartSettings = 0x0004,
	SnapshotSettings = 0x0005,
	SvgManufacturer = 0x0006,
	SvgEntity = 0x0007,
	SvgGeneric = 0x0008,
	PngManufacturer = 0x0009,
	PngEntity = 0x000a,
	PngGeneric = 0x000b,
	DaeManufacturer = 0x000c,
	DaeEntity = 0x000d,
	DaeGeneric = 0x000e,
	/* 000f to ffff reserved for future use */
};
constexpr bool operator==(MemoryObjectType const lhs, MemoryObjectType const rhs)
{
	return static_cast<std::underlying_type_t<MemoryObjectType>>(lhs) == static_cast<std::underlying_type_t<MemoryObjectType>>(rhs);
}

constexpr bool operator==(MemoryObjectType const lhs, std::underlying_type_t<MemoryObjectType> const rhs)
{
	return static_cast<std::underlying_type_t<MemoryObjectType>>(lhs) == rhs;
}

LA_AVDECC_API std::string LA_AVDECC_CALL_CONVENTION memoryObjectTypeToString(MemoryObjectType const memoryObjectType) noexcept;

/** MemoryObject Operation Type - Clause 7.2.10.2 */
enum class MemoryObjectOperationType : std::uint16_t
{
	Store = 0x0000,
	StoreAndReboot = 0x0001,
	Read = 0x0002,
	Erase = 0x0003,
	Upload = 0x0004,
	/* 0005 to ffff reserved for future use */
};
constexpr bool operator==(MemoryObjectOperationType const lhs, MemoryObjectOperationType const rhs)
{
	return static_cast<std::underlying_type_t<MemoryObjectOperationType>>(lhs) == static_cast<std::underlying_type_t<MemoryObjectOperationType>>(rhs);
}

constexpr bool operator==(MemoryObjectOperationType const lhs, std::underlying_type_t<MemoryObjectOperationType> const rhs)
{
	return static_cast<std::underlying_type_t<MemoryObjectOperationType>>(lhs) == rhs;
}

LA_AVDECC_API std::string LA_AVDECC_CALL_CONVENTION memoryObjectOperationTypeToString(MemoryObjectOperationType const memoryObjectOperationType) noexcept;

/** AudioCluster Format - Clause 7.2.16.1 */
enum class AudioClusterFormat : std::uint8_t
{
	Iec60958 = 0x00,
	Mbla = 0x40,
	Midi = 0x80,
	Smpte = 0x88,
};
constexpr bool operator==(AudioClusterFormat const lhs, AudioClusterFormat const rhs)
{
	return static_cast<std::underlying_type_t<AudioClusterFormat>>(lhs) == static_cast<std::underlying_type_t<AudioClusterFormat>>(rhs);
}

constexpr bool operator==(AudioClusterFormat const lhs, std::underlying_type_t<AudioClusterFormat> const rhs)
{
	return static_cast<std::underlying_type_t<AudioClusterFormat>>(lhs) == rhs;
}

LA_AVDECC_API std::string LA_AVDECC_CALL_CONVENTION audioClusterFormatToString(AudioClusterFormat const audioClusterFormat) noexcept;

/** Audio Mapping - Clause 7.2.19.1 */
struct AudioMapping
{
	StreamIndex streamIndex{ StreamIndex(0u) };
	std::uint16_t streamChannel{ 0u };
	ClusterIndex clusterOffset{ ClusterIndex(0u) };
	std::uint16_t clusterChannel{ 0u };

	static constexpr size_t size()
	{
		return sizeof(streamIndex) + sizeof(streamChannel) + sizeof(clusterOffset) + sizeof(clusterChannel);
	}
	constexpr friend bool operator==(AudioMapping const& lhs, AudioMapping const& rhs) noexcept
	{
		return (lhs.streamIndex == rhs.streamIndex) && (lhs.streamChannel == rhs.streamChannel) && (lhs.clusterOffset == rhs.clusterOffset) && (lhs.clusterChannel == rhs.clusterChannel);
	}
	constexpr friend bool operator!=(AudioMapping const& lhs, AudioMapping const& rhs) noexcept
	{
		return !operator==(lhs, rhs);
	}
};

using AudioMappings = std::vector<AudioMapping>;

/** MSRP Mapping - Clause 7.4.40.2.1 */
struct MsrpMapping
{
	std::uint8_t trafficClass{ 0x00 };
	std::uint8_t priority{ 0xff };
	std::uint16_t vlanID{ 0u };

	static constexpr size_t size()
	{
		return sizeof(trafficClass) + sizeof(priority) + sizeof(vlanID);
	}
	constexpr friend bool operator==(MsrpMapping const& lhs, MsrpMapping const& rhs) noexcept
	{
		return (lhs.trafficClass == rhs.trafficClass) && (lhs.priority == rhs.priority) && (lhs.vlanID == rhs.vlanID);
	}
	constexpr friend bool operator!=(MsrpMapping const& lhs, MsrpMapping const& rhs) noexcept
	{
		return !operator==(lhs, rhs);
	}
};

using MsrpMappings = std::vector<MsrpMapping>;

/** GET_AS_PATH Dynamic Information - Clause 7.4.41.2 */
using PathSequence = std::vector<UniqueIdentifier>;

/** GET_COUNTERS - Clause 7.4.42.2 */
using DescriptorCounters = std::array<DescriptorCounter, 32>;

/** UTF-8 String */
class AvdeccFixedString final
{
public:
	static constexpr size_t MaxLength = 64;
	using value_type = char;

	/** Default constructor */
	AvdeccFixedString() noexcept {}

	/** Constructor from a std::string */
	AvdeccFixedString(std::string const& str) noexcept
	{
		assign(str);
	}

	/** Constructor from a raw buffer */
	AvdeccFixedString(void const* const ptr, size_t const size) noexcept
	{
		assign(ptr, size);
	}

	/** Default destructor */
	~AvdeccFixedString() noexcept = default;

	/** Assign from a std::string */
	void assign(std::string const& str) noexcept
	{
		assign(str.c_str(), str.size());
	}

	/** Assign from a raw buffer */
	void assign(void const* const ptr, size_t const size) noexcept
	{
		auto const copySize = size <= MaxLength ? size : MaxLength; // Cannot use std::min here because MaxLength doesn't have a reference (it's a static constexpr)

		// Copy std::string to internal std::array
		auto* dstPtr = _buffer.data();
		std::memcpy(dstPtr, ptr, copySize);

		// Fill remaining bytes with \0
		for (auto i = copySize; i < MaxLength; ++i)
		{
			dstPtr[i] = '\0';
		}
	}

	/** Returns the raw buffer */
	value_type* data() noexcept
	{
		return _buffer.data();
	}

	/** Returns the raw buffer (const) (might not be NULL terminated) */
	value_type const* data() const noexcept
	{
		return _buffer.data();
	}

	/** Returns the (fixed) size of the buffer */
	size_t size() const noexcept
	{
		return MaxLength;
	}

	/** Returns true if the buffer contains only '\0' */
	bool empty() const noexcept
	{
		return _buffer[0] == '\0';
	}

	/** operator== */
	bool operator==(AvdeccFixedString const& afs) const noexcept
	{
		return _buffer == afs._buffer;
	}

	/** operator!= */
	bool operator!=(AvdeccFixedString const& afs) const noexcept
	{
		return !operator==(afs);
	}

	/** operator== overload for a std::string */
	bool operator==(std::string const& str) const noexcept
	{
		return operator std::string() == str;
	}

	/** operator!= overload for a std::string */
	bool operator!=(std::string const& str) const noexcept
	{
		return !operator==(str);
	}

	/** Returns this AvdeccFixedString as a std::string */
	std::string str() const noexcept
	{
		return operator std::string();
	}

	/** Returns this AvdeccFixedString as a std::string */
	operator std::string() const noexcept
	{
		std::string str{};

		// If all bytes in an AvdeccFixedString are used, the buffer is not NULL-terminated. We cannot use strlen or directly copy the buffer into an std::string or we might overflow
		for (auto const c : _buffer)
		{
			if (c == 0)
				break;
			str.push_back(c);
		}

		return str;
	}

	// Defaulted compiler auto-generated methods
	AvdeccFixedString(AvdeccFixedString&&) = default;
	AvdeccFixedString(AvdeccFixedString const&) = default;
	AvdeccFixedString& operator=(AvdeccFixedString const&) = default;
	AvdeccFixedString& operator=(AvdeccFixedString&&) = default;

private:
	std::array<value_type, MaxLength> _buffer{};
};

/** Sampling Rate - Clause 7.3.1 */
class SamplingRate final
{
public:
	using value_type = std::uint32_t;

	/** Default constructor. */
	SamplingRate() noexcept {}

	/** Constructor to create a SamplingRate from the underlying value. */
	explicit SamplingRate(value_type const value) noexcept
		: _value(value)
	{
	}

	/** Constructor to create a SamplingRate from pull and baseFrequency values. */
	SamplingRate(std::uint8_t const pull, std::uint32_t const baseFrequency) noexcept
		: _value((pull << 29) + (baseFrequency & 0x1FFFFFFF))
	{
	}

	/** Setter to change the underlying value. */
	constexpr void setValue(value_type const value) noexcept
	{
		_value = value;
	}

	/** Getter to retrieve the underlying value. */
	constexpr value_type getValue() const noexcept
	{
		return _value;
	}

	/** Gets the Nominal Sample Rate value. */
	constexpr double getNominalSampleRate() const noexcept
	{
		auto const [pull, frequency] = getPullBaseFrequency();
		switch (pull)
		{
			case 0:
				return frequency;
			case 1:
				return frequency * 1.0 / 1.001;
			case 2:
				return frequency * 1.001;
			case 3:
				return frequency * 24.0 / 25.0;
			case 4:
				return frequency * 25.0 / 24.0;
			default: // 5 to 7 reserved for future use
				AVDECC_ASSERT(false, "Unknown pull value");
				return frequency;
		}
	}

	/** Getter to retrieve the pull and baseFrequency values from this SamplingRate. */
	constexpr std::pair<std::uint8_t, std::uint32_t> getPullBaseFrequency() const noexcept
	{
		return std::make_pair(static_cast<std::uint8_t>(_value >> 29), static_cast<std::uint32_t>(_value & 0x1FFFFFFF));
	}

	/** True if the SamplingRate contains a valid underlying value, false otherwise. */
	constexpr bool isValid() const noexcept
	{
		// Clause 7.3.1.2 says base_frequency ranges from 1 to 536'870'911, so we 0 detect invalid value.
		return (_value & 0x1FFFFFFF) != 0;
	}

	/** Underlying value operator (equivalent to getValue()). */
	constexpr operator value_type() const noexcept
	{
		return getValue();
	}

	/** Underlying value validity bool operator (equivalent to isValid()). */
	explicit constexpr operator bool() const noexcept
	{
		return isValid();
	}

	/** Equality operator. Returns true if the underlying values are equal (Any 2 invalid SamplingRate are considered equal, since they are both invalid). */
	constexpr friend bool operator==(SamplingRate const& lhs, SamplingRate const& rhs) noexcept
	{
		return (!lhs.isValid() && !rhs.isValid()) || lhs._value == rhs._value;
	}

	/** Non equality operator. */
	constexpr friend bool operator!=(SamplingRate const& lhs, SamplingRate const& rhs) noexcept
	{
		return !operator==(lhs, rhs);
	}

	/** operator< */
	constexpr friend bool operator<(SamplingRate const& lhs, SamplingRate const& rhs) noexcept
	{
		return lhs._value < rhs._value;
	}

	/** Static helper method to create a Null SamplingRate (isValid() returns false). */
	static SamplingRate getNullSamplingRate() noexcept
	{
		return SamplingRate{ NullSamplingRate };
	}

	/** Hash functor to be used for std::hash */
	struct hash
	{
		std::size_t operator()(SamplingRate const& ref) const
		{
			return std::hash<value_type>()(ref._value);
		}
	};

	// Defaulted compiler auto-generated methods
	SamplingRate(SamplingRate&&) = default;
	SamplingRate(SamplingRate const&) = default;
	SamplingRate& operator=(SamplingRate const&) = default;
	SamplingRate& operator=(SamplingRate&&) = default;

private:
	static constexpr value_type NullSamplingRate = 0u;
	value_type _value{ NullSamplingRate };
};

/** Stream Format packed value - Clause 7.3.2 */
class StreamFormat final
{
public:
	using value_type = std::uint64_t;

	/** Default constructor. */
	StreamFormat() noexcept {}

	/** Constructor to create a StreamFormat from the underlying value. */
	StreamFormat(value_type const value) noexcept
		: _value(value)
	{
	}

	/** Setter to change the underlying value. */
	constexpr void setValue(value_type const value) noexcept
	{
		_value = value;
	}

	/** Getter to retrieve the underlying value. */
	constexpr value_type getValue() const noexcept
	{
		return _value;
	}

	/** True if the StreamFormat contains a valid underlying value, false otherwise. */
	constexpr bool isValid() const noexcept
	{
		return _value != NullStreamFormat;
	}

	/** Underlying value operator (equivalent to getValue()). */
	constexpr operator value_type() const noexcept
	{
		return getValue();
	}

	/** Underlying value validity bool operator (equivalent to isValid()). */
	explicit constexpr operator bool() const noexcept
	{
		return isValid();
	}

	/** Equality operator. Returns true if the underlying values are equal (Any 2 invalid StreamFormats are considered equal, since they are both invalid). */
	constexpr friend bool operator==(StreamFormat const& lhs, StreamFormat const& rhs) noexcept
	{
		return (!lhs.isValid() && !rhs.isValid()) || lhs._value == rhs._value;
	}

	/** Non equality operator. */
	constexpr friend bool operator!=(StreamFormat const& lhs, StreamFormat const& rhs) noexcept
	{
		return !operator==(lhs, rhs);
	}

	/** operator< */
	constexpr friend bool operator<(StreamFormat const& lhs, StreamFormat const& rhs) noexcept
	{
		return lhs._value < rhs._value;
	}

	/** Static helper method to create a Null StreamFormat (isValid() returns false). */
	static StreamFormat getNullStreamFormat() noexcept
	{
		return StreamFormat{ NullStreamFormat };
	}

	/** Hash functor to be used for std::hash */
	struct hash
	{
		std::size_t operator()(StreamFormat const& ref) const
		{
			return std::hash<value_type>()(ref._value);
		}
	};

	// Defaulted compiler auto-generated methods
	StreamFormat(StreamFormat&&) = default;
	StreamFormat(StreamFormat const&) = default;
	StreamFormat& operator=(StreamFormat const&) = default;
	StreamFormat& operator=(StreamFormat&&) = default;

private:
	static constexpr value_type NullStreamFormat = 0ul;
	value_type _value{ NullStreamFormat };
};


/** Localized String Reference - Clause 7.3.6 */
class LocalizedStringReference final
{
public:
	using value_type = std::uint16_t;

	/** Default constructor. */
	LocalizedStringReference() noexcept {}

	/** Constructor to create a LocalizedStringReference from the underlying value. */
	explicit LocalizedStringReference(value_type const value) noexcept
		: _value(value)
	{
	}

	/** Constructor to create a LocalizedStringReference from offset and index values. */
	LocalizedStringReference(std::uint16_t const offset, std::uint8_t const index) noexcept
		: _value((offset << 3) + (index & 0x07))
	{
	}

	/** Setter to change the underlying value. */
	constexpr void setValue(value_type const value) noexcept
	{
		_value = value;
	}

	/** Getter to retrieve the underlying value. */
	constexpr value_type getValue() const noexcept
	{
		return _value;
	}

	/** Getter to retrieve the global offset for this LocalizedStringReference. Throws std::invalid_argument if this LocalizedStringReference is invalid. */
	constexpr value_type getGlobalOffset() const
	{
		if (!isValid())
		{
			throw std::invalid_argument("Invalid LocalizedStringReference");
		}

		auto const [offset, index] = getOffsetIndex();
		return ((offset * 7u) + index) & 0xFFFF;
	}

	/** Getter to retrieve the offset and index values from this LocalizedStringReference. */
	constexpr std::pair<std::uint16_t, std::uint8_t> getOffsetIndex() const noexcept
	{
		return std::make_pair(static_cast<std::uint16_t>(_value >> 3), static_cast<std::uint8_t>(_value & 0x0007));
	}

	/** True if the LocalizedStringReference contains a valid underlying value, false otherwise. */
	constexpr bool isValid() const noexcept
	{
		// Clause 7.3.6 says any index value of 7 is invalid, we just have to check that.
		return (_value & 0x0007) != 0x07;
	}

	/** Underlying value operator (equivalent to getValue()). */
	constexpr operator value_type() const noexcept
	{
		return getValue();
	}

	/** Underlying value validity bool operator (equivalent to isValid()). */
	explicit constexpr operator bool() const noexcept
	{
		return isValid();
	}

	/** Equality operator. Returns true if the underlying values are equal (Any 2 invalid LocalizedStringReferences are considered equal, since they are both invalid). */
	constexpr friend bool operator==(LocalizedStringReference const& lhs, LocalizedStringReference const& rhs) noexcept
	{
		return (!lhs.isValid() && !rhs.isValid()) || lhs._value == rhs._value;
	}

	/** Non equality operator. */
	constexpr friend bool operator!=(LocalizedStringReference const& lhs, LocalizedStringReference const& rhs) noexcept
	{
		return !operator==(lhs, rhs);
	}

	/** operator< */
	constexpr friend bool operator<(LocalizedStringReference const& lhs, LocalizedStringReference const& rhs) noexcept
	{
		return lhs._value < rhs._value;
	}

	/** Static helper method to create a Null LocalizedStringReference (isValid() returns false). */
	static LocalizedStringReference getNullLocalizedStringReference() noexcept
	{
		return LocalizedStringReference{ NullLocalizedStringReference };
	}

	/** Hash functor to be used for std::hash */
	struct hash
	{
		std::size_t operator()(LocalizedStringReference const& ref) const
		{
			return std::hash<value_type>()(ref._value);
		}
	};

	// Defaulted compiler auto-generated methods
	LocalizedStringReference(LocalizedStringReference&&) = default;
	LocalizedStringReference(LocalizedStringReference const&) = default;
	LocalizedStringReference& operator=(LocalizedStringReference const&) = default;
	LocalizedStringReference& operator=(LocalizedStringReference&&) = default;

private:
	static constexpr value_type NullLocalizedStringReference = 0xFFFF;
	value_type _value{ NullLocalizedStringReference };
};

/** Stream Identification (EntityID/StreamIndex couple) */
struct StreamIdentification
{
	UniqueIdentifier entityID{};
	entity::model::StreamIndex streamIndex{ entity::model::StreamIndex(0u) };
};

constexpr bool operator==(StreamIdentification const& lhs, StreamIdentification const& rhs) noexcept
{
	return (lhs.entityID == rhs.entityID) && (lhs.streamIndex == rhs.streamIndex);
}

constexpr bool operator!=(StreamIdentification const& lhs, StreamIdentification const& rhs) noexcept
{
	return !(lhs == rhs);
}

constexpr bool operator<(StreamIdentification const& lhs, StreamIdentification const& rhs) noexcept
{
	return (lhs.entityID.getValue() < rhs.entityID.getValue()) || (lhs.entityID == rhs.entityID && lhs.streamIndex < rhs.streamIndex);
}

/** Probing Status - Milan Clause 6.8.6 */
enum class ProbingStatus : std::uint8_t
{
	Disabled = 0x00, /** The sink is not probing because it is not bound. */
	Passive = 0x01, /** The sink is probing passively. It waits until the bound talker has been discovered. */
	Active = 0x02, /** The sink is probing actively. It is querying the stream parameters to the talker. */
	Completed = 0x03, /** The sink is not probing because it is settled. */
	/* 04 to 07 reserved for future use */
};
constexpr bool operator==(ProbingStatus const lhs, ProbingStatus const rhs)
{
	return static_cast<std::underlying_type_t<ProbingStatus>>(lhs) == static_cast<std::underlying_type_t<ProbingStatus>>(rhs);
}

constexpr bool operator==(ProbingStatus const lhs, std::underlying_type_t<ProbingStatus> const rhs)
{
	return static_cast<std::underlying_type_t<ProbingStatus>>(lhs) == rhs;
}

} // namespace model
} // namespace entity
} // namespace avdecc
} // namespace la

/** ostream overload for an AvdeccFixedString */
inline std::ostream& operator<<(std::ostream& os, la::avdecc::entity::model::AvdeccFixedString const& rhs)
{
	os << rhs.str();
	return os;
}
