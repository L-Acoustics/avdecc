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
* @file entityModelTypes.hpp
* @author Christophe Calmejane
* @brief Avdecc entity model types.
*/

#pragma once

#include "la/avdecc/utils.hpp"

#include "uniqueIdentifier.hpp"
#include "exports.hpp"

#include <any>
#include <cstdint>
#include <string>
#include <array>
#include <vector>
#include <iostream>
#include <optional>
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
using TimingIndex = DescriptorIndex;
using PtpInstanceIndex = DescriptorIndex;
using PtpPortIndex = DescriptorIndex;
using DescriptorCounterValidFlag = std::uint32_t; /** Counters valid flag - IEEE1722.1-2013 Clause 7.4.42 */
using DescriptorCounter = std::uint32_t; /** Counter - IEEE1722.1-2013 Clause 7.4.42 */
using OperationID = std::uint16_t; /** OperationID for OPERATIONS returned by an entity to a controller - IEEE1722.1-2013 Clause 7.4.53 */
using BridgeIdentifier = std::uint64_t;

constexpr DescriptorIndex getInvalidDescriptorIndex() noexcept
{
	return DescriptorIndex(0xFFFF);
}

/** Descriptor Type - IEEE1722.1-2013 Clause 7.2 */
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
	Timing = 0x0026,
	PtpInstance = 0x0027,
	PtpPort = 0x0028,
	LAST_VALID_DESCRIPTOR = 0x0028,
	/* 0029 to fffe reserved for future use */
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

/** Jack Type - IEEE1722.1-2013 Clause 7.2.7.2 */
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

/** ClockSource Type - IEEE1722.1-2013 Clause 7.2.9.2 */
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

/** MemoryObject Type - IEEE1722.1-2013 Clause 7.2.10.1 */
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

/** MemoryObject Operation Type - IEEE1722.1-2013 Clause 7.2.10.2 */
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

/** AudioCluster Format - IEEE1722.1-2013 Clause 7.2.16.1 */
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

/** Audio Mapping - IEEE1722.1-2013 Clause 7.2.19.1 */
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

/** Timing Algorithm - IEEE1722.1-2021 Clause 7.2.34.1 */
enum class TimingAlgorithm : std::uint16_t
{
	Single = 0x0000,
	Fallback = 0x0001,
	Combined = 0x0002,
	/* 0003 to ffff reserved for future use */
};
constexpr bool operator==(TimingAlgorithm const lhs, TimingAlgorithm const rhs)
{
	return static_cast<std::underlying_type_t<TimingAlgorithm>>(lhs) == static_cast<std::underlying_type_t<TimingAlgorithm>>(rhs);
}

constexpr bool operator==(TimingAlgorithm const lhs, std::underlying_type_t<TimingAlgorithm> const rhs)
{
	return static_cast<std::underlying_type_t<TimingAlgorithm>>(lhs) == rhs;
}

LA_AVDECC_API std::string LA_AVDECC_CALL_CONVENTION timingAlgorithmToString(TimingAlgorithm const timingAlgorithm) noexcept;

/** PTP Port Type - IEEE1722.1-2021 Clause 7.2.36.1 */
enum class PtpPortType : std::uint16_t
{
	P2PLinkLayer = 0x0000,
	P2PMulticastUdpV4 = 0x0001,
	P2PMulticastUdpV6 = 0x0002,
	TimingMeasurement = 0x0003,
	FineTimingMeasurement = 0x0004,
	E2ELinkLayer = 0x0005,
	E2EMulticastUdpV4 = 0x0006,
	E2EMulticastUdpV6 = 0x0007,
	P2PUnicastUdpV4 = 0x0008,
	P2PUnicastUdpV6 = 0x0009,
	E2EUnicastUdpV4 = 0x000a,
	E2EUnicastUdpV6 = 0x000b,
	/* 000c to ffff reserved for future use */
};
constexpr bool operator==(PtpPortType const lhs, PtpPortType const rhs)
{
	return static_cast<std::underlying_type_t<PtpPortType>>(lhs) == static_cast<std::underlying_type_t<PtpPortType>>(rhs);
}

constexpr bool operator==(PtpPortType const lhs, std::underlying_type_t<PtpPortType> const rhs)
{
	return static_cast<std::underlying_type_t<PtpPortType>>(lhs) == rhs;
}

LA_AVDECC_API std::string LA_AVDECC_CALL_CONVENTION ptpPortTypeToString(PtpPortType const ptpPortType) noexcept;

/** Control Type - IEEE1722.1-2013 Clause 7.3.4 */
using ControlType = UniqueIdentifier;
constexpr std::uint32_t StandardControlTypeVendorID = 0x90e0f0;

LA_AVDECC_API std::string LA_AVDECC_CALL_CONVENTION controlTypeToString(ControlType const& controlType) noexcept;

enum class StandardControlType : std::uint64_t
{
	Enable = 0x90e0f00000000000,
	Identify = 0x90e0f00000000001,
	Mute = 0x90e0f00000000002,
	Invert = 0x90e0f00000000003,
	Gain = 0x90e0f00000000004,
	Attenuate = 0x90e0f00000000005,
	Delay = 0x90e0f00000000006,
	SrcMode = 0x90e0f00000000007,
	Snapshot = 0x90e0f00000000008,
	PowLineFreq = 0x90e0f00000000009,
	PowerStatus = 0x90e0f0000000000a,
	FanStatus = 0x90e0f0000000000b,
	Temperature = 0x90e0f0000000000c,
	Altitude = 0x90e0f0000000000d,
	AbsoluteHumidity = 0x90e0f0000000000e,
	RelativeHumidity = 0x90e0f0000000000f,
	Orientation = 0x90e0f00000000010,
	Velocity = 0x90e0f00000000011,
	Acceleration = 0x90e0f00000000012,
	FilterResponse = 0x90e0f00000000013,
	/* 0x90e0f00000000014 to 0x90e0f0000000ffff reserved for future use */
	Panpot = 0x90e0f00000010000,
	Phantom = 0x90e0f00000010001,
	AudioScale = 0x90e0f00000010002,
	AudioMeters = 0x90e0f00000010003,
	AudioSpectrum = 0x90e0f00000010004,
	/* 0x90e0f00000010005 to 0x90e0f0000001ffff reserved for future use */
	ScanningMode = 0x90e0f00000020000,
	AutoExpMode = 0x90e0f00000020001,
	AutoExpPrio = 0x90e0f00000020002,
	ExpTime = 0x90e0f00000020003,
	Focus = 0x90e0f00000020004,
	FocusAuto = 0x90e0f00000020005,
	Iris = 0x90e0f00000020006,
	Zoom = 0x90e0f00000020007,
	Privacy = 0x90e0f00000020008,
	Backlight = 0x90e0f00000020009,
	Brightness = 0x90e0f0000002000a,
	Contrast = 0x90e0f0000002000b,
	Hue = 0x90e0f0000002000c,
	Saturation = 0x90e0f0000002000d,
	Sharpness = 0x90e0f0000002000e,
	Gamma = 0x90e0f0000002000f,
	WhiteBalTemp = 0x90e0f00000020010,
	WhiteBalTempAuto = 0x90e0f00000020011,
	WhiteBalComp = 0x90e0f00000020012,
	WhiteBalCompAuto = 0x90e0f00000020013,
	DigitalZoom = 0x90e0f00000020014,
	/* 0x90e0f00000020015 to 0x90e0f0000002ffff reserved for future use */
	MediaPlaylist = 0x90e0f00000030000,
	MediaPlaylistName = 0x90e0f00000030001,
	MediaDisk = 0x90e0f00000030002,
	MediaDiskName = 0x90e0f00000030003,
	MediaTrack = 0x90e0f00000030004,
	MediaTrackName = 0x90e0f00000030005,
	MediaSpeed = 0x90e0f00000030006,
	MediaSamplePosition = 0x90e0f00000030007,
	MediaPlaybackTransport = 0x90e0f00000030008,
	MediaRecordTransport = 0x90e0f00000030009,
	/* 0x90e0f0000003000a to 0x90e0f0000003ffff reserved for future use */
	Frequency = 0x90e0f00000040000,
	Modulation = 0x90e0f00000040001,
	Polarization = 0x90e0f00000040002,
	/* 0x90e0f00000040003 to 0x90e0f0ffffffffff reserved for future use */
};
constexpr bool operator==(StandardControlType const lhs, StandardControlType const rhs)
{
	return static_cast<std::underlying_type_t<StandardControlType>>(lhs) == static_cast<std::underlying_type_t<StandardControlType>>(rhs);
}

constexpr bool operator==(StandardControlType const lhs, std::underlying_type_t<StandardControlType> const rhs)
{
	return static_cast<std::underlying_type_t<StandardControlType>>(lhs) == rhs;
}

LA_AVDECC_API std::string LA_AVDECC_CALL_CONVENTION standardControlTypeToString(StandardControlType const controlType) noexcept;

/** MSRP Mapping - IEEE1722.1-2013 Clause 7.4.40.2.1 */
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

/** GET_AS_PATH Dynamic Information - IEEE1722.1-2013 Clause 7.4.41.2 */
using PathSequence = std::vector<UniqueIdentifier>;

/** GET_COUNTERS - IEEE1722.1-2013 Clause 7.4.42.2 */
using DescriptorCounters = std::array<DescriptorCounter, 32>;

/** UTF-8 String */
class AvdeccFixedString final
{
public:
	static constexpr size_t MaxLength = 64;
	using value_type = char;

	/** Default constructor */
	constexpr AvdeccFixedString() noexcept {}

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

	/** Direct access operator */
	value_type& operator[](size_t const pos)
	{
		if (pos >= MaxLength)
		{
			throw std::out_of_range("AvdeccFixedString::operator[]");
		}

		return _buffer[pos];
	}

	/** Direct access const operator */
	value_type const& operator[](size_t const pos) const
	{
		if (pos >= MaxLength)
		{
			throw std::out_of_range("AvdeccFixedString::operator[]");
		}

		return _buffer[pos];
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

/** Sampling Rate - IEEE1722.1-2013 Clause 7.3.1 */
class SamplingRate final
{
public:
	using value_type = std::uint32_t;

	/** Default constructor. */
	constexpr SamplingRate() noexcept {}

	/** Constructor to create a SamplingRate from the underlying value. */
	explicit constexpr SamplingRate(value_type const value) noexcept
		: _value(value)
	{
	}

	/** Constructor to create a SamplingRate from pull and baseFrequency values. */
	constexpr SamplingRate(std::uint8_t const pull, std::uint32_t const baseFrequency) noexcept
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

	/** Getter to retrieve the pull value from this SamplingRate. */
	constexpr std::uint8_t getPull() const noexcept
	{
		return static_cast<std::uint8_t>(_value >> 29);
	}

	/** Getter to retrieve the baseFrequency value from this SamplingRate. */
	constexpr std::uint32_t getBaseFrequency() const noexcept
	{
		return static_cast<std::uint32_t>(_value & 0x1FFFFFFF);
	}

	/** True if the SamplingRate contains a valid underlying value, false otherwise. */
	constexpr bool isValid() const noexcept
	{
		// IEEE1722.1-2013 Clause 7.3.1.2 says base_frequency ranges from 1 to 536'870'911, so we can use 0 to detect invalid value.
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

/** Stream Format packed value - IEEE1722.1-2013 Clause 7.3.2 */
class StreamFormat final
{
public:
	using value_type = std::uint64_t;

	/** Default constructor. */
	constexpr StreamFormat() noexcept {}

	/** Constructor to create a StreamFormat from the underlying value. */
	explicit constexpr StreamFormat(value_type const value) noexcept
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

/** Localized String Reference - IEEE1722.1-2013 Clause 7.3.6 */
class LocalizedStringReference final
{
public:
	using value_type = std::uint16_t;

	/** Default constructor. */
	constexpr LocalizedStringReference() noexcept {}

	/** Constructor to create a LocalizedStringReference from the underlying value. */
	explicit constexpr LocalizedStringReference(value_type const value) noexcept
		: _value(value)
	{
	}

	/** Constructor to create a LocalizedStringReference from offset and index values. */
	constexpr LocalizedStringReference(std::uint16_t const offset, std::uint8_t const index) noexcept
	{
		setOffsetIndex(offset, index);
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

	/** Setter to change the offset and index values for this LocalizedStringReference. */
	constexpr void setOffsetIndex(std::uint16_t const offset, std::uint8_t const index) noexcept
	{
		_value = (offset << 3) + (index & 0x07);
	}

	/** Getter to retrieve the offset and index values from this LocalizedStringReference. */
	constexpr std::pair<std::uint16_t, std::uint8_t> getOffsetIndex() const noexcept
	{
		return std::make_pair(static_cast<std::uint16_t>(_value >> 3), static_cast<std::uint8_t>(_value & 0x0007));
	}

	/** True if the LocalizedStringReference contains a valid underlying value, false otherwise. */
	constexpr bool isValid() const noexcept
	{
		// IEEE1722.1-2013 Clause 7.3.6 says any index value of 7 is invalid, we just have to check that.
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

/** Control Value Unit - IEEE1722.1-2013 Clause 7.3.3 */
class ControlValueUnit final
{
public:
	using value_type = std::uint16_t;

	enum class Unit : std::uint8_t
	{
		// Unitless Quantities
		Unitless = 0x00,
		Count = 0x01,
		Percent = 0x02,
		FStop = 0x03,
		// 0x04 to 0x07 reserved

		// Time Quantities
		Seconds = 0x08,
		Minutes = 0x09,
		Hours = 0x0a,
		Days = 0x0b,
		Months = 0x0c,
		Years = 0x0d,
		Samples = 0x0e,
		Frames = 0x0f,

		// Frequency Quantities
		Hertz = 0x10,
		Semitones = 0x11,
		Cents = 0x12,
		Octaves = 0x13,
		Fps = 0x14,
		// 0x15 to 0x17 reserved

		// Distance Quantities
		Metres = 0x18,
		// 0x19 to 0x1f reserved

		// Temperature Quantities
		Kelvin = 0x20,
		// 0x21 to 0x27 reserved

		// Mass Quantities
		Grams = 0x28,
		// 0x29 to 0x2f reserved

		// Voltage Quantities
		Volts = 0x30,
		Dbv = 0x31,
		Dbu = 0x32,
		// 0x33 to 0x37 reserved

		// Current Quantities
		Amps = 0x38,
		// 0x39 to 0x3f reserved

		// Power Quantities
		Watts = 0x40,
		Dbm = 0x41,
		Dbw = 0x42,
		// 0x43 to 0x47 reserved

		// Pressure Quantities
		Pascals = 0x48,
		// 0x49 to 0x4f reserved

		// Memory Quantities
		Bits = 0x50,
		Bytes = 0x51,
		KibiBytes = 0x52,
		MebiBytes = 0x53,
		GibiBytes = 0x54,
		TebiBytes = 0x55,
		// 0x56 to 0x57 reserved

		// Bandwidth Quantities
		BitsPerSec = 0x58,
		BytesPerSec = 0x59,
		KibiBytesPerSec = 0x5a,
		MebiBytesPerSec = 0x5b,
		GibiBytesPerSec = 0x5c,
		TebiBytesPerSec = 0x5d,
		// 0x5e to 0x5f reserved

		// Luminosity Quantities
		Candelas = 0x60,
		// 0x61 to 0x67 reserved

		// Energy Quantities
		Joules = 0x68,
		// 0x69 to 0x6f reserved

		// Angle Quantities
		Radians = 0x70,
		// 0x71 to 0x77 reserved

		// Force Quantities
		Newtons = 0x78,
		// 0x79 to 0x7f reserved

		// Resistance Quantities
		Ohms = 0x80,
		// 0x81 to 0x87 reserved

		// Velocity Quantities
		MetresPerSec = 0x88,
		RadiansPerSec = 0x89,
		// 0x8a to 0x8f reserved

		// Acceleration Quantities
		MetresPerSecSquared = 0x90,
		RadiansPerSecSquared = 0x91,
		// 0x92 to 0x97 reserved

		// Magnetic Flux and Fields Quantities
		Teslas = 0x98,
		Webers = 0x99,
		AmpsPerMetre = 0x9a,
		// 0x9b to 0x9f reserved

		// Area Quantities
		MetresSquared = 0xa0,
		// 0xa1 to 0xa7 reserved

		// Volume Quantities
		MetresCubed = 0xa8,
		Litres = 0xa9,
		// 0xaa to 0xaf reserved

		// Level and Loudness Quantities
		Db = 0xb0,
		DbPeak = 0xb1,
		DbRms = 0xb2,
		Dbfs = 0xb3,
		DbfsPeak = 0xb4,
		DbfsRms = 0xb5,
		Dbtp = 0xb6,
		DbSplA = 0xb7,
		DbZ = 0xb8,
		DbSplC = 0xb9,
		DbSpl = 0xba,
		Lu = 0xbb,
		Lufs = 0xbc,
		DbA = 0xbd,
		// 0xbe to 0xbf reserved
	};

	/** Default constructor. */
	constexpr ControlValueUnit() noexcept {}

	/** Constructor to create a ControlValueUnit from the underlying value. */
	explicit constexpr ControlValueUnit(value_type const value) noexcept
		: _value(value)
	{
	}

	/** Constructor to create a ControlValueUnit from multiplier and unit values. */
	constexpr ControlValueUnit(std::int8_t const multiplier, Unit const unit) noexcept
	{
		setMultiplierUnit(multiplier, unit);
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

	/** Getter to retrieve the multiplier from this ControlValueUnit. */
	constexpr std::int8_t getMultiplier() const noexcept
	{
		return static_cast<std::int8_t>(_value >> 8);
	}

	/** Getter to retrieve unit from this ControlValueUnit. */
	constexpr Unit getUnit() const noexcept
	{
		return static_cast<Unit>(_value & 0x0000FFFF);
	}

	/** Setter to change the multiplier and unit values for this ControlValueUnit. */
	constexpr void setMultiplierUnit(std::int8_t const multiplier, Unit const unit) noexcept
	{
		_value = (static_cast<std::uint8_t>(multiplier) << 8) + utils::to_integral(unit);
	}

	/** Getter to retrieve the multiplier and unit values from this ControlValueUnit. */
	constexpr std::pair<std::int8_t, Unit> getMultiplierUnit() const noexcept
	{
		return std::make_pair(getMultiplier(), getUnit());
	}

	/** Underlying value operator (equivalent to getValue()). */
	constexpr operator value_type() const noexcept
	{
		return getValue();
	}

	/** Equality operator. Returns true if the underlying values are equal. */
	constexpr friend bool operator==(ControlValueUnit const& lhs, ControlValueUnit const& rhs) noexcept
	{
		return lhs._value == rhs._value;
	}

	/** Non equality operator. */
	constexpr friend bool operator!=(ControlValueUnit const& lhs, ControlValueUnit const& rhs) noexcept
	{
		return !operator==(lhs, rhs);
	}

	/** operator< */
	constexpr friend bool operator<(ControlValueUnit const& lhs, ControlValueUnit const& rhs) noexcept
	{
		return lhs._value < rhs._value;
	}

	/** Hash functor to be used for std::hash */
	struct hash
	{
		std::size_t operator()(ControlValueUnit const& ref) const
		{
			return std::hash<value_type>()(ref._value);
		}
	};

	// Defaulted compiler auto-generated methods
	ControlValueUnit(ControlValueUnit&&) = default;
	ControlValueUnit(ControlValueUnit const&) = default;
	ControlValueUnit& operator=(ControlValueUnit const&) = default;
	ControlValueUnit& operator=(ControlValueUnit&&) = default;

private:
	static constexpr value_type NullControlValueUnit = 0u;
	value_type _value{ NullControlValueUnit };
};

LA_AVDECC_API std::string LA_AVDECC_CALL_CONVENTION controlValueUnitToString(ControlValueUnit::Unit const controlValueUnit) noexcept;

/** Control Value Type - IEEE1722.1-2013 Clause 7.3.5 */
class ControlValueType final
{
public:
	using value_type = std::uint16_t;

	enum class Type : std::uint16_t
	{
		ControlLinearInt8 = 0x0000,
		ControlLinearUInt8 = 0x0001,
		ControlLinearInt16 = 0x0002,
		ControlLinearUInt16 = 0x0003,
		ControlLinearInt32 = 0x0004,
		ControlLinearUInt32 = 0x0005,
		ControlLinearInt64 = 0x0006,
		ControlLinearUInt64 = 0x0007,
		ControlLinearFloat = 0x0008,
		ControlLinearDouble = 0x0009,
		ControlSelectorInt8 = 0x000a,
		ControlSelectorUInt8 = 0x000b,
		ControlSelectorInt16 = 0x000c,
		ControlSelectorUInt16 = 0x000d,
		ControlSelectorInt32 = 0x000e,
		ControlSelectorUInt32 = 0x000f,
		ControlSelectorInt64 = 0x0010,
		ControlSelectorUInt64 = 0x0011,
		ControlSelectorFloat = 0x0012,
		ControlSelectorDouble = 0x0013,
		ControlSelectorString = 0x0014,
		ControlArrayInt8 = 0x0015,
		ControlArrayUInt8 = 0x0016,
		ControlArrayInt16 = 0x0017,
		ControlArrayUInt16 = 0x0018,
		ControlArrayInt32 = 0x0019,
		ControlArrayUInt32 = 0x001a,
		ControlArrayInt64 = 0x001b,
		ControlArrayUInt64 = 0x001c,
		ControlArrayFloat = 0x001d,
		ControlArrayDouble = 0x001e,
		ControlUtf8 = 0x001f,
		ControlBodePlot = 0x0020,
		ControlSmpteTime = 0x0021,
		ControlSampleRate = 0x0022,
		ControlGptpTime = 0x0023,
		// 0x0024 to 0x3ffd reserved for future use
		ControlVendor = 0x3ffe,
		Expansion = 0x3fff,
	};

	/** Default constructor. */
	constexpr ControlValueType() noexcept {}

	/** Constructor to create a ControlValueType from the underlying value. */
	explicit constexpr ControlValueType(value_type const value) noexcept
		: _value(value)
	{
	}

	/** Constructor to create a ControlValueType from isReadOnly, isUnknown and type values. */
	constexpr ControlValueType(bool const isReadOnly, bool const isUnknown, Type const type) noexcept
	{
		setReadOnlyUnknownType(isReadOnly, isUnknown, type);
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

	/** Getter to retrieve the readOnly bit from this ControlValueType. */
	constexpr bool isReadOnly() const noexcept
	{
		return ((_value >> 15) & 0x1) == 1;
	}

	/** Getter to retrieve the isUnknown bit from this ControlValueType. */
	constexpr bool isUnknown() const noexcept
	{
		return ((_value >> 14) & 0x1) == 1;
	}

	/** Getter to retrieve Type from this ControlValueType. */
	constexpr Type getType() const noexcept
	{
		return static_cast<Type>(_value & 0x3FFF);
	}

	constexpr void setReadOnlyUnknownType(bool const isReadOnly, bool const isUnknown, Type const type) noexcept
	{
		_value = ((isReadOnly & 0x1) << 15) + ((isUnknown & 0x1) << 14) + (utils::to_integral(type) & 0x3FFF);
	}

	/** Underlying value operator (equivalent to getValue()). */
	constexpr operator value_type() const noexcept
	{
		return getValue();
	}

	/** Equality operator. Returns true if the underlying values are equal. */
	constexpr friend bool operator==(ControlValueType const& lhs, ControlValueType const& rhs) noexcept
	{
		return lhs._value == rhs._value;
	}

	/** Non equality operator. */
	constexpr friend bool operator!=(ControlValueType const& lhs, ControlValueType const& rhs) noexcept
	{
		return !operator==(lhs, rhs);
	}

	/** operator< */
	constexpr friend bool operator<(ControlValueType const& lhs, ControlValueType const& rhs) noexcept
	{
		return lhs._value < rhs._value;
	}

	/** Hash functor to be used for std::hash */
	struct hash
	{
		std::size_t operator()(ControlValueType const& ref) const
		{
			return std::hash<value_type>()(ref._value);
		}
	};

	// Defaulted compiler auto-generated methods
	ControlValueType(ControlValueType&&) = default;
	ControlValueType(ControlValueType const&) = default;
	ControlValueType& operator=(ControlValueType const&) = default;
	ControlValueType& operator=(ControlValueType&&) = default;

private:
	static constexpr value_type NullControlValueType = 1u << 14;
	value_type _value{ NullControlValueType };
};

LA_AVDECC_API std::string LA_AVDECC_CALL_CONVENTION controlValueTypeToString(ControlValueType::Type const controlValueType) noexcept;

/** Control Values - IEEE1722.1-2013 Clause 7.3.5 */
class ControlValues final
{
public:
	/** Traits to handle ValueDetails behavior. */
	template<typename ValueDetailsType>
	struct control_value_details_traits
	{
		static constexpr bool is_value_details = false;
		static constexpr bool is_dynamic = false;
		static constexpr std::optional<bool> static_dynamic_counts_identical = std::nullopt;
		static constexpr ControlValueType::Type control_value_type = ControlValueType::Type::Expansion; // Not the best default value but none is provided by the standard
	};

	constexpr ControlValues() noexcept {}

	template<class ValueDetailsType, typename Traits = control_value_details_traits<std::decay_t<ValueDetailsType>>, typename = std::enable_if_t<!std::is_same_v<std::decay_t<ValueDetailsType>, ControlValues>>>
	explicit ControlValues(ValueDetailsType const& values) noexcept
		: _isValid{ true }
		, _type{ Traits::control_value_type }
		, _areDynamic{ Traits::is_dynamic }
		, _countMustBeIdentical{ Traits::static_dynamic_counts_identical ? *Traits::static_dynamic_counts_identical : false } // Check for optional presence delayed to body so we have a nicer message than with an enable_if template parameter
		, _countValues{ values.countValues() }
		, _values{ values }
	{
		static_assert(Traits::is_value_details, "ControlValues::ControlValues, control_value_details_traits::is_value_details trait not defined for requested ValueDetailsType. Did you include entityModelControlValuesTraits.hpp?");
		static_assert(Traits::static_dynamic_counts_identical.has_value(), "ControlValues::ControlValues, control_value_details_traits::static_dynamic_counts_identical trait not defined for requested ValueDetailsType.");
	}

	template<class ValueDetailsType, typename Traits = control_value_details_traits<std::decay_t<ValueDetailsType>>, typename = std::enable_if_t<!std::is_same_v<std::decay_t<ValueDetailsType>, ControlValues>>>
	explicit ControlValues(ValueDetailsType&& values) noexcept
		: _isValid{ true }
		, _type{ Traits::control_value_type }
		, _areDynamic{ Traits::is_dynamic }
		, _countMustBeIdentical{ Traits::static_dynamic_counts_identical ? *Traits::static_dynamic_counts_identical : false } // Check for optional presence delayed to body so we have a nicer message than with an enable_if template parameter
		, _countValues{ values.countValues() } // Careful with order here, we are moving 'values'
		, _values{ std::forward<ValueDetailsType>(values) }
	{
		static_assert(Traits::is_value_details, "ControlValues::ControlValues, control_value_details_traits::is_value_details trait not defined for requested ValueDetailsType. Did you include entityModelControlValuesTraits.hpp?");
		static_assert(Traits::static_dynamic_counts_identical.has_value(), "ControlValues::ControlValues, control_value_details_traits::static_dynamic_counts_identical trait not defined for requested ValueDetailsType.");
	}

	constexpr ControlValueType::Type getType() const noexcept
	{
		return _type;
	}

	/** True if the values are Dynamic, false if they are Static */
	constexpr bool areDynamicValues() const noexcept
	{
		return _areDynamic;
	}

	/** True if the count of Static Values must be identical to the count of Dynamic Values (depends on ControlValueType) */
	constexpr bool countMustBeIdentical() const noexcept
	{
		return _countMustBeIdentical;
	}

	/** Number of values (either static or dynamic, depending on the template type) in the ControlValues. */
	constexpr std::uint16_t size() const noexcept
	{
		return _countValues;
	}

	constexpr bool empty() const noexcept
	{
		return _countValues == 0;
	}

	/** True if the ControlValues contains valid values, false otherwise. */
	constexpr bool isValid() const noexcept
	{
		return _isValid;
	}

	/** Validity bool operator (equivalent to isValid()). */
	explicit constexpr operator bool() const noexcept
	{
		return isValid();
	}

	template<class ValueDetailsType, typename Traits = control_value_details_traits<std::decay_t<ValueDetailsType>>>
	std::decay_t<ValueDetailsType> getValues() const
	{
		static_assert(Traits::is_value_details, "ControlValues::getValues, control_value_details_traits::is_value_details trait not defined for requested ValueDetailsType. Did you include entityModelControlValuesTraits.hpp?");
		if (!isValid())
		{
			throw std::invalid_argument("ControlValues::getValues, no valid values to get");
		}
		if (_type != Traits::control_value_type)
		{
			throw std::invalid_argument("ControlValues::getValues, incorrect ControlValueType::Type");
		}
		if (_areDynamic != Traits::is_dynamic)
		{
			throw std::invalid_argument("ControlValues::getValues, static/dynamic mismatch");
		}
		return std::any_cast<std::decay_t<ValueDetailsType>>(_values);
	}

	// Comparison operators
	template<class ValueDetailsType, typename Traits = control_value_details_traits<std::decay_t<ValueDetailsType>>>
	inline bool isEqualTo(ControlValues const& other) const
	{
		static_assert(Traits::is_value_details, "ControlValues::isEqualTo, control_value_details_traits::is_value_details trait not defined for requested ValueDetailsType. Did you include entityModelControlValuesTraits.hpp?");
		// Both must have the same valid state
		if (_isValid != other._isValid)
		{
			return false;
		}
		// If both are invalid, they are equal
		if (!_isValid)
		{
			return true;
		}
		// If both are valid, they must have all the same parameters
		if (_type != other._type || _areDynamic != other._areDynamic || _countMustBeIdentical != other._countMustBeIdentical || _countValues != other._countValues)
		{
			return false;
		}
		// Now compare the actual values
		return getValues<ValueDetailsType>() == other.getValues<ValueDetailsType>();
	}

	// Defaulted compiler auto-generated methods
	ControlValues(ControlValues const&) = default;
	ControlValues(ControlValues&&) = default;
	ControlValues& operator=(ControlValues const&) = default;
	ControlValues& operator=(ControlValues&&) = default;

private:
	bool _isValid{ false };
	ControlValueType::Type _type{};
	bool _areDynamic{ false };
	bool _countMustBeIdentical{ false };
	std::uint16_t _countValues{ 0u };
	std::any _values{};
};

/** Stream Identification (EntityID/StreamIndex couple) */
struct StreamIdentification
{
	UniqueIdentifier entityID{};
	entity::model::StreamIndex streamIndex{ entity::model::getInvalidDescriptorIndex() };
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

/** Probing Status - Milan-2019 Clause 6.8.6 */
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

/** MSRP Failure Code - 802.1Q-2018 Table 35-6 */
enum class MsrpFailureCode : std::uint8_t
{
	NoFailure = 0,
	InsufficientBandwidth = 1,
	InsufficientResources = 2,
	InsufficientTrafficClassBandwidth = 3,
	StreamIDInUse = 4,
	StreamDestinationAddressInUse = 5,
	StreamPreemptedByHigherRank = 6,
	LatencyHasChanged = 7,
	EgressPortNotAVBCapable = 8,
	UseDifferentDestinationAddress = 9,
	OutOfMSRPResources = 10,
	OutOfMMRPResources = 11,
	CannotStoreDestinationAddress = 12,
	PriorityIsNotAnSRClass = 13,
	MaxFrameSizeTooLarge = 14,
	MaxFanInPortsLimitReached = 15,
	FirstValueChangedForStreamID = 16,
	VlanBlockedOnEgress = 17,
	VlanTaggingDisabledOnEgress = 18,
	SrClassPriorityMismatch = 19,
};
constexpr bool operator==(MsrpFailureCode const lhs, MsrpFailureCode const rhs)
{
	return static_cast<std::underlying_type_t<MsrpFailureCode>>(lhs) == static_cast<std::underlying_type_t<MsrpFailureCode>>(rhs);
}

constexpr bool operator==(MsrpFailureCode const lhs, std::underlying_type_t<MsrpFailureCode> const rhs)
{
	return static_cast<std::underlying_type_t<MsrpFailureCode>>(lhs) == rhs;
}

LA_AVDECC_API std::string LA_AVDECC_CALL_CONVENTION msrpFailureCodeToString(MsrpFailureCode const msrpFailureCode) noexcept;

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
