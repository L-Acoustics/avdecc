/*
* Copyright (C) 2016-2021, L-Acoustics and its contributors

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
* @file streamFormat.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/internals/streamFormatInfo.hpp"

#include <cstdint>
#include <type_traits>

namespace la
{
namespace avdecc
{
namespace entity
{
namespace model
{
template<std::uint8_t FirstBit, std::uint8_t LastBit, typename T>
constexpr T contiguousBitMask()
{
	static_assert(FirstBit <= LastBit, "FirstBit must be lower than LastBit");
	static_assert(LastBit < (sizeof(T) * 8), "LastBist must be inferior to the maximum bits T can hold");
	constexpr auto nBits = LastBit - FirstBit + 1;

	return std::integral_constant<T, (~(~std::uint64_t(0) << nBits)) << FirstBit>::value;
}

/** Specialization required to handle the case of all bits sets for an std::uint64_t value */
template<>
constexpr std::uint64_t contiguousBitMask<0, 63, std::uint64_t>()
{
	return std::integral_constant<std::uint64_t, 0xffffffffffffffff>::value;
}

template<std::uint8_t FirstBit, std::uint8_t LastBit, typename T>
constexpr T clearMask()
{
	static_assert(FirstBit <= LastBit, "FirstBit must be lower than LastBit");
	static_assert(LastBit < (sizeof(T) * 8), "LastBist must be inferior to the maximum bits T can hold");

	return std::integral_constant<T, ~(contiguousBitMask<FirstBit, LastBit, T>())>::value;
}

#pragma message("TODO: Make this template works on all endianess")
template<std::uint8_t FirstBit, std::uint8_t LastBit, typename T = std::uint8_t>
T getField(StreamFormat::value_type const& format) noexcept
{
	constexpr auto sfBitsCount = sizeof(StreamFormat::value_type) * 8;
	static_assert(LastBit < sfBitsCount, "LastBit must be inferior to the maximum bits StreamFormat can hold");
	static_assert(FirstBit <= LastBit, "FirstBit must be lower than LastBit");
	static_assert((LastBit - FirstBit) <= (sizeof(T) * 8), "Count of bits must be inferior or equal to the size of return value type");

	constexpr auto shiftCountLast = sfBitsCount - LastBit - 1;
	T const v = static_cast<T>(format >> shiftCountLast);
	return v & contiguousBitMask<0, LastBit - FirstBit, T>();
}

template<std::uint8_t FirstBit, std::uint8_t LastBit, typename T = std::uint8_t>
void replaceField(StreamFormat::value_type& format, T const newValue) noexcept
{
	constexpr auto sfBitsCount = sizeof(StreamFormat::value_type) * 8;
	static_assert(LastBit < (sfBitsCount), "LastBit must be inferior to the maximum bits StreamFormat can hold");
	static_assert(FirstBit <= LastBit, "FirstBit must be lower than LastBit");
	static_assert((LastBit - FirstBit) <= (sizeof(T) * 8), "Count of bits must be inferior or equal to the size of T");

	constexpr auto shiftCountFirst = sfBitsCount - FirstBit - 1;
	constexpr auto shiftCountLast = sfBitsCount - LastBit - 1;
	// Clear previous bits
	format &= clearMask<shiftCountLast, shiftCountFirst, StreamFormat::value_type>();
	// Set new bits
	auto const v = static_cast<StreamFormat::value_type>(newValue) << shiftCountLast;
	format |= v;
}

/** Base stream format */
template<class SuperClass = StreamFormatInfo>
class StreamFormatInfoBase : public SuperClass
{
public:
	StreamFormatInfoBase(StreamFormat const& streamFormat, StreamFormatInfo::Type const type) noexcept
		: _streamFormat(streamFormat)
		, _type(type)
	{
	}

	virtual StreamFormat getStreamFormat() const noexcept override final
	{
		return StreamFormat{ _streamFormat };
	}

	virtual StreamFormat getAdaptedStreamFormat(std::uint16_t const channelsCount) const noexcept override
	{
		AVDECC_ASSERT(_upToChannelsCount == false, "getAdaptedStreamFormat must be specialized for StreamFormat supported upToChannelsCount");
		if (channelsCount != _channelsCount)
			return StreamFormat::getNullStreamFormat();
		return getStreamFormat();
	}

	virtual StreamFormatInfo::Type getType() const noexcept override final
	{
		return _type;
	}

	virtual std::uint16_t getChannelsCount() const noexcept override final
	{
		return _channelsCount;
	}

	virtual bool isUpToChannelsCount() const noexcept override final
	{
		return _upToChannelsCount;
	}

	virtual StreamFormatInfo::SamplingRate getSamplingRate() const noexcept override final
	{
		return _samplingRate;
	}

	virtual StreamFormatInfo::SampleFormat getSampleFormat() const noexcept override final
	{
		return _sampleFormat;
	}

	virtual bool useSynchronousClock() const noexcept override
	{
		return _useSynchronousClock;
	}

	virtual std::uint16_t getSampleSize() const noexcept override
	{
		switch (_sampleFormat)
		{
			case StreamFormatInfo::SampleFormat::Int8:
				return 8;
			case StreamFormatInfo::SampleFormat::Int16:
				return 16;
			case StreamFormatInfo::SampleFormat::Int24:
				return 24;
			case StreamFormatInfo::SampleFormat::Int32:
				return 32;
			case StreamFormatInfo::SampleFormat::Int64:
				return 64;
			case StreamFormatInfo::SampleFormat::FixedPoint32:
				return 32;
			case StreamFormatInfo::SampleFormat::FloatingPoint32:
				return 32;
			case StreamFormatInfo::SampleFormat::Unknown:
				return 0u;
			default:
				AVDECC_ASSERT(false, "Unhandled SampleFormat");
				return 0u;
		}
	}

	virtual std::uint16_t getSampleBitDepth() const noexcept override
	{
		return _sampleDepth;
	}

	virtual void destroy() noexcept override
	{
		delete this;
	}

protected:
	StreamFormat::value_type _streamFormat{ 0 };
	StreamFormatInfo::Type _type{ StreamFormatInfo::Type::Unsupported };
	std::uint16_t _channelsCount{ 0u };
	bool _upToChannelsCount{ false };
	StreamFormatInfo::SamplingRate _samplingRate{ StreamFormatInfo::SamplingRate::Unknown };
	StreamFormatInfo::SampleFormat _sampleFormat{ StreamFormatInfo::SampleFormat::Unknown };
	bool _useSynchronousClock{ false };
	std::uint16_t _sampleDepth{ 0u };
};

/** None stream format */
class StreamFormatInfoAAF_None final : public StreamFormatInfoBase<>
{
public:
	StreamFormatInfoAAF_None(StreamFormat const& streamFormat)
		: StreamFormatInfoBase(streamFormat, Type::None)
	{
	}
};

/** IEC 61883 stream format */
class StreamFormatInfoIEC_61883 : public StreamFormatInfoBase<>
{
public:
	enum class Fmt
	{
		IEC_61883_4_FMT = 0x20,
		IEC_61883_6_FMT = 0x10,
		IEC_61883_7_FMT = 0x21,
		IEC_61883_8_FMT = 0x01,
		Unknown,
	};
	StreamFormatInfoIEC_61883(StreamFormat const& streamFormat, Type const type, Fmt const fmt) noexcept
		: StreamFormatInfoBase(streamFormat, type)
		, _fmt(fmt)
	{
	}

protected:
	Fmt _fmt{ Fmt::Unknown };
};

/** IEC 61883-6 stream format */
class StreamFormatInfoIEC_61883_6 : public StreamFormatInfoIEC_61883
{
public:
	StreamFormatInfoIEC_61883_6(StreamFormat const& streamFormat, std::uint8_t const fdf_sfc, std::uint8_t const dbs, bool const b, bool const nb, bool const ut, bool const sc)
		: StreamFormatInfoIEC_61883(streamFormat, Type::IEC_61883_6, Fmt::IEC_61883_6_FMT)
		, _b(b)
		, _nb(nb)
	{
		_channelsCount = dbs;
		_upToChannelsCount = ut;
		switch (fdf_sfc)
		{
			case 0:
				_samplingRate = SamplingRate::kHz_32;
				break;
			case 1:
				_samplingRate = SamplingRate::kHz_44_1;
				break;
			case 2:
				_samplingRate = SamplingRate::kHz_48;
				break;
			case 3:
				_samplingRate = SamplingRate::kHz_88_2;
				break;
			case 4:
				_samplingRate = SamplingRate::kHz_96;
				break;
			case 5:
				_samplingRate = SamplingRate::kHz_176_4;
				break;
			case 6:
				_samplingRate = SamplingRate::kHz_192;
				break;
			default:
				throw std::invalid_argument("Unsupported IEC 61883-6 fdf_sfc value");
		}
		_useSynchronousClock = sc;
	}

protected:
	bool _b{ false };
	bool _nb{ false };
};

/** IEC 61883-6-AM824 stream format */
class StreamFormatInfoIEC_61883_6_AM824 final : public StreamFormatInfoIEC_61883_6
{
public:
	StreamFormatInfoIEC_61883_6_AM824(StreamFormat const& streamFormat, std::uint8_t const fdf_sfc, std::uint8_t const dbs, bool const b, bool const nb, bool const ut, bool const sc, std::uint8_t const label_iec_60958_cnt, std::uint8_t const label_mbla_cnt, std::uint8_t const label_midi_cnt, std::uint8_t const label_smptecnt)
		: StreamFormatInfoIEC_61883_6(streamFormat, fdf_sfc, dbs, b, nb, ut, sc)
		, _label_iec_60958_cnt(label_iec_60958_cnt)
		, _label_mbla_cnt(label_mbla_cnt)
		, _label_midi_cnt(label_midi_cnt)
		, _label_smptecnt(label_smptecnt)
	{
		_sampleFormat = SampleFormat::Int24;
		_sampleDepth = 24;

		// Prevent compilation warning (unused private field)
		(void)_label_iec_60958_cnt;
		(void)_label_mbla_cnt;
		(void)_label_midi_cnt;
		(void)_label_smptecnt;
	}

	virtual StreamFormat getAdaptedStreamFormat(std::uint16_t const channelsCount) const noexcept override
	{
		if (_upToChannelsCount)
		{
			if (channelsCount > _channelsCount)
				return StreamFormat::getNullStreamFormat();

			auto fmt = _streamFormat;
			replaceField<34, 34>(fmt, std::uint8_t(0)); // ut field
			replaceField<24, 31>(fmt, channelsCount); // dbs field
			replaceField<48, 55>(fmt, channelsCount); // mbld_cnt field
			return StreamFormat{ fmt };
		}

		if (channelsCount != _channelsCount)
			return StreamFormat::getNullStreamFormat();
		return getStreamFormat();
	}

private:
	std::uint8_t _label_iec_60958_cnt{ 0 };
	std::uint8_t _label_mbla_cnt{ 0 };
	std::uint8_t _label_midi_cnt{ 0 };
	std::uint8_t _label_smptecnt{ 0 };
};

/** AAF stream format */
class StreamFormatInfoAAF : public StreamFormatInfoBase<>
{
public:
	StreamFormatInfoAAF(StreamFormat const& streamFormat, bool const ut, std::uint8_t const nsr)
		: StreamFormatInfoBase(streamFormat, Type::AAF)
	{
		_upToChannelsCount = ut;
		switch (nsr)
		{
			case 0:
				_samplingRate = SamplingRate::UserDefined;
				break;
			case 1:
				_samplingRate = SamplingRate::kHz_8;
				break;
			case 2:
				_samplingRate = SamplingRate::kHz_16;
				break;
			case 3:
				_samplingRate = SamplingRate::kHz_32;
				break;
			case 4:
				_samplingRate = SamplingRate::kHz_44_1;
				break;
			case 5:
				_samplingRate = SamplingRate::kHz_48;
				break;
			case 6:
				_samplingRate = SamplingRate::kHz_88_2;
				break;
			case 7:
				_samplingRate = SamplingRate::kHz_96;
				break;
			case 8:
				_samplingRate = SamplingRate::kHz_176_4;
				break;
			case 9:
				_samplingRate = SamplingRate::kHz_192;
				break;
			case 10:
				_samplingRate = SamplingRate::kHz_24;
				break;
			default:
				throw std::invalid_argument("Unsupported AAF nsr value");
		}
		_useSynchronousClock = true;
	}
};

/** AAF stream format */
class StreamFormatInfoAAF_PCM final : public StreamFormatInfoAAF
{
public:
	StreamFormatInfoAAF_PCM(StreamFormat const& streamFormat, bool const ut, std::uint8_t const nsr, std::uint8_t const format, std::uint8_t const bit_depth, std::uint16_t const channels_per_frame, std::uint16_t const samples_per_frame)
		: StreamFormatInfoAAF(streamFormat, ut, nsr)
		, _samples_per_frame(samples_per_frame)
	{
		_channelsCount = channels_per_frame;
		switch (format)
		{
			case 0x02: // PCM INT_32BIT
				_sampleFormat = SampleFormat::Int32;
				break;
			case 0x03: // PCM INT_24BIT
				_sampleFormat = SampleFormat::Int24;
				break;
			case 0x04: // PCM INT_16BIT
				_sampleFormat = SampleFormat::Int16;
				break;
			default:
				throw std::invalid_argument("Unsupported AAF PCM format value");
		}
		_sampleDepth = bit_depth;

		// Prevent compilation warning (unused private field)
		(void)_samples_per_frame;
	}

	virtual StreamFormat getAdaptedStreamFormat(std::uint16_t const channelsCount) const noexcept override
	{
		if (_upToChannelsCount)
		{
			if (channelsCount > _channelsCount)
				return StreamFormat::getNullStreamFormat();

			auto fmt = _streamFormat;
			replaceField<11, 11>(fmt, std::uint8_t(0)); // ut field
			replaceField<32, 41>(fmt, channelsCount); // channels_per_frame field
			return StreamFormat{ fmt };
		}

		if (channelsCount != _channelsCount)
			return StreamFormat::getNullStreamFormat();
		return getStreamFormat();
	}

private:
	std::uint16_t _samples_per_frame{ 0 };
};

/** CRF stream format */
class StreamFormatInfoCRFImpl final : public StreamFormatInfoBase<StreamFormatInfoCRF>
{
public:
	StreamFormatInfoCRFImpl(StreamFormat const& streamFormat, std::uint8_t const crf_type, std::uint16_t const timestamp_interval, std::uint8_t const timestamps_per_pdu, std::uint8_t const pull, std::uint32_t const base_frequency)
		: StreamFormatInfoBase(streamFormat, Type::ClockReference)
		, _timestamp_interval(timestamp_interval)
		, _timestamps_per_pdu(timestamps_per_pdu)
		, _pull(pull)
	{
		_channelsCount = 0u;
		_upToChannelsCount = false;
		switch (base_frequency)
		{
			case 500:
				_samplingRate = SamplingRate::Hz_500;
				break;
			case 32000:
				_samplingRate = SamplingRate::kHz_32;
				break;
			case 44100:
				_samplingRate = SamplingRate::kHz_44_1;
				break;
			case 48000:
				_samplingRate = SamplingRate::kHz_48;
				break;
			case 88200:
				_samplingRate = SamplingRate::kHz_88_2;
				break;
			case 96000:
				_samplingRate = SamplingRate::kHz_96;
				break;
			case 176400:
				_samplingRate = SamplingRate::kHz_176_4;
				break;
			case 192000:
				_samplingRate = SamplingRate::kHz_192;
				break;
			default:
				throw std::invalid_argument("Unsupported CRF base_frequency value");
		}
		_sampleFormat = SampleFormat::Int64;
		_sampleDepth = 64;
		_useSynchronousClock = true;
		switch (crf_type)
		{
			case 0:
				_crfType = CRFType::User;
				break;
			case 1:
				_crfType = CRFType::AudioSample;
				break;
			case 4:
				_crfType = CRFType::MachineCycle;
				break;
			default:
				throw std::invalid_argument("Unsupported CRF crf_type value");
		}

		// Prevent compilation warning (unused private field)
		(void)_pull;
	}

	virtual std::uint16_t getTimestampInterval() const noexcept override final
	{
		return _timestamp_interval;
	}

	virtual std::uint8_t getTimestampsPerPdu() const noexcept override final
	{
		return _timestamps_per_pdu;
	}

	virtual CRFType getCRFType() const noexcept override final
	{
		return _crfType;
	}

private:
	CRFType _crfType{ CRFType::Unknown }; // 0=CRF_USER, 1=CRF_AUDIO_SAMPLE, 4=CRF_MACHINE_CYCLE
	std::uint16_t _timestamp_interval{ 0 };
	std::uint8_t _timestamps_per_pdu{ 0 };
	std::uint8_t _pull{ 0 }; // 0=*1.0, 1=*1/1.001, 2=*1.001, 3=*24/25, 4=*25/24, 5=*1/8
};

/** StreamFormat unpacker */
StreamFormatInfo* LA_AVDECC_CALL_CONVENTION StreamFormatInfo::createRawStreamFormatInfo(StreamFormat const& streamFormat) noexcept
{
	// No stream format
	if (!streamFormat)
		return new StreamFormatInfoAAF_None(streamFormat);

	try
	{
		auto const v = getField<0, 0>(streamFormat);
		if (v == 1) // 'v' field must be set to zero for an AVTP defined time-sensitive stream
			throw std::invalid_argument("Unsupported non AVTP time-sensitive stream");

		auto const subtype = getField<1, 7>(streamFormat);
		switch (subtype)
		{
			case 0x00: // 61883 or IIDC
			{
				auto const sf = getField<8, 8>(streamFormat);
				if (sf == 0) // IIDC
				{
					//auto const iidc_format = getField<std::uint16_t>(streamFormat, 40, 55);
					//auto const iidc_rate = getField(streamFormat, 56, 63);
					throw std::invalid_argument("Unsupported IIDC format");
				}
				else // IEC 61883
				{
					auto const fmt = getField<9, 14>(streamFormat);
					switch (fmt)
					{
						case 0x10: // IEC 61883-6
						{
							auto const fdf_evt = getField<16, 20>(streamFormat);
							auto const fdf_sfc = getField<21, 23>(streamFormat);
							auto const dbs = getField<24, 31>(streamFormat);
							auto const b = getField<32, 32>(streamFormat);
							auto const nb = getField<33, 33>(streamFormat);
							auto const ut = getField<34, 34>(streamFormat);
							auto const sc = getField<35, 35>(streamFormat);
							switch (fdf_evt)
							{
								case 0x00: // IEC 61883-6 AM824
								{
									auto const label_iec_60958_cnt = getField<40, 47>(streamFormat);
									auto const label_mbla_cnt = getField<48, 55>(streamFormat);
									auto const label_midi_cnt = getField<56, 59>(streamFormat);
									auto const label_smptecnt = getField<60, 63>(streamFormat);
									// The sum of the 4 fields must be equal to dbs
									AVDECC_ASSERT((label_iec_60958_cnt + label_mbla_cnt + label_midi_cnt + label_smptecnt) == dbs, "The sum of the 4 fields must be equal to dbs");
									AVDECC_ASSERT(label_mbla_cnt == dbs, "We assume all bits are in mbla, but it might not be true");
									return new StreamFormatInfoIEC_61883_6_AM824(streamFormat, fdf_sfc, dbs, b != 0, nb != 0, ut != 0, sc != 0, label_iec_60958_cnt, label_mbla_cnt, label_midi_cnt, label_smptecnt);
								}
								default:
									throw std::invalid_argument("Unsupported IEC 61883-6 fdf_evt value");
							}
							break;
						}
						default:
							throw std::invalid_argument("Unsupported IEC 61883 fmt value");
					}
				}
				break;
			}
			case 0x02: // AAF (AVTP Audio Format)
			{
				auto const ut = getField<11, 11>(streamFormat);
				auto const nsr = getField<12, 15>(streamFormat);
				auto const format = getField<16, 23>(streamFormat);
				switch (format)
				{
					case 0x02: // PCM INT_32BIT
					case 0x03: // PCM INT_24BIT
					case 0x04: // PCM INT_16BIT
					{
						auto const bit_depth = getField<24, 31>(streamFormat);
						auto const channels_per_frame = getField<32, 41, std::uint16_t>(streamFormat);
						auto const samples_per_frame = getField<42, 51, std::uint16_t>(streamFormat);
						return new StreamFormatInfoAAF_PCM(streamFormat, ut != 0, nsr, format, bit_depth, channels_per_frame, samples_per_frame);
					}
					default:
						throw std::invalid_argument("Unsupported AAF format value");
				}
				break;
			}
			case 0x04: // Clock Reference Format
			{
				auto const crf_type = getField<8, 11>(streamFormat);
				auto const timestamp_interval = getField<12, 23, std::uint16_t>(streamFormat);
				auto const timestamps_per_pdu = getField<24, 31>(streamFormat);
				auto const pull = getField<32, 34>(streamFormat);
				auto const base_frequency = getField<35, 63, std::uint32_t>(streamFormat);
				return new StreamFormatInfoCRFImpl(streamFormat, crf_type, timestamp_interval, timestamps_per_pdu, pull, base_frequency);
				break;
			}
			default: // Unknown or unsupported
				throw std::invalid_argument("Unsupported subtype value");
		}
	}
	catch (...)
	{
		// TODO: Catch and log the invalid_argument exception. Maybe re-throw, instead of having this method noexcept
	}
	return new StreamFormatInfoBase<>(streamFormat, Type::Unsupported); // Unsupported
}

StreamFormat LA_AVDECC_CALL_CONVENTION StreamFormatInfo::buildFormat_IEC_61883_6(std::uint16_t const channelsCount, bool const isUpToChannelsCount, SamplingRate const samplingRate, SampleFormat const sampleFormat, bool const useSynchronousClock) noexcept
{
	StreamFormat::value_type fmt{ 0u };
	replaceField<0, 0>(fmt, static_cast<std::uint8_t>(0)); // 'v' field must be set to zero for an AVTP defined time-sensitive stream

	replaceField<1, 7>(fmt, static_cast<std::uint8_t>(0x00)); // subtype = 61883 or IIDC

	replaceField<8, 8>(fmt, static_cast<std::uint8_t>(1)); // sf = IEC 61883
	replaceField<9, 14>(fmt, static_cast<std::uint8_t>(0x10)); // fmt = IEC 61883-6

	std::uint8_t fdf_evt{ 0u };
	switch (sampleFormat)
	{
		case SampleFormat::Int24: // IEC 61883-6 AM824
			fdf_evt = 0x00;
			break;
		case SampleFormat::FixedPoint32: // IEC 61883-6 32-bit fixed point packetization
			[[fallthrough]]; // Not supported
		case SampleFormat::FloatingPoint32: // IEC 61883-6 32-bit floating point packetization
			[[fallthrough]]; // Not supported
		default:
			return StreamFormat::getNullStreamFormat();
	}
	replaceField<16, 20>(fmt, static_cast<std::uint8_t>(fdf_evt)); // fdf_evt = sampleFormat

	std::uint8_t fdf_sfc{ 0u };
	switch (samplingRate)
	{
		case SamplingRate::kHz_32:
			fdf_sfc = 0;
			break;
		case SamplingRate::kHz_44_1:
			fdf_sfc = 1;
			break;
		case SamplingRate::kHz_48:
			fdf_sfc = 2;
			break;
		case SamplingRate::kHz_88_2:
			fdf_sfc = 3;
			break;
		case SamplingRate::kHz_96:
			fdf_sfc = 4;
			break;
		case SamplingRate::kHz_176_4:
			fdf_sfc = 5;
			break;
		case SamplingRate::kHz_192:
			fdf_sfc = 6;
			break;
		default:
			return StreamFormat::getNullStreamFormat();
	}
	replaceField<21, 23>(fmt, static_cast<std::uint8_t>(fdf_sfc)); // fdf_sfc = samplingRate
	replaceField<24, 31>(fmt, static_cast<std::uint16_t>(channelsCount)); // dbs = channelsCount
	replaceField<33, 33>(fmt, static_cast<std::uint8_t>(1)); // nb = 1
	replaceField<34, 34>(fmt, static_cast<std::uint8_t>(isUpToChannelsCount)); // ut = isUpToChannelsCount
	replaceField<35, 35>(fmt, static_cast<std::uint8_t>(useSynchronousClock)); // sc = useSynchronousClock
	replaceField<48, 55>(fmt, static_cast<std::uint16_t>(channelsCount)); // label_mbla_cnt = channelsCount

	return StreamFormat{ fmt };
}

StreamFormat LA_AVDECC_CALL_CONVENTION StreamFormatInfo::buildFormat_AAF(std::uint16_t const channelsCount, bool const isUpToChannelsCount, SamplingRate const samplingRate, SampleFormat const sampleFormat, std::uint16_t const sampleBitDepth, std::uint16_t const samplesPerFrame) noexcept
{
	StreamFormat::value_type fmt{ 0u };
	replaceField<0, 0>(fmt, static_cast<std::uint8_t>(0)); // 'v' field must be set to zero for an AVTP defined time-sensitive stream

	replaceField<1, 7>(fmt, static_cast<std::uint8_t>(0x02)); // subtype = AAF (AVTP Audio Format)

	replaceField<11, 11>(fmt, static_cast<std::uint8_t>(isUpToChannelsCount)); // ut = isUpToChannelsCount

	std::uint8_t nsr{ 0u };
	switch (samplingRate)
	{
		case SamplingRate::UserDefined:
			nsr = 0;
			break;
		case SamplingRate::kHz_8:
			nsr = 1;
			break;
		case SamplingRate::kHz_16:
			nsr = 2;
			break;
		case SamplingRate::kHz_32:
			nsr = 3;
			break;
		case SamplingRate::kHz_44_1:
			nsr = 4;
			break;
		case SamplingRate::kHz_48:
			nsr = 5;
			break;
		case SamplingRate::kHz_88_2:
			nsr = 6;
			break;
		case SamplingRate::kHz_96:
			nsr = 7;
			break;
		case SamplingRate::kHz_176_4:
			nsr = 8;
			break;
		case SamplingRate::kHz_192:
			nsr = 9;
			break;
		case SamplingRate::kHz_24:
			nsr = 10;
			break;
		default:
			return StreamFormat::getNullStreamFormat();
	}
	replaceField<12, 15>(fmt, static_cast<std::uint8_t>(nsr)); // nsr = samplingRate

	std::uint8_t format{ 0u };
	std::uint16_t maxDepth{ 0u };
	switch (sampleFormat)
	{
		case SampleFormat::Int32:
			maxDepth = 32;
			format = 0x02;
			break;
		case SampleFormat::Int24:
			maxDepth = 24;
			format = 0x03;
			break;
		case SampleFormat::Int16:
			maxDepth = 16;
			format = 0x04;
			break;
		default:
			return StreamFormat::getNullStreamFormat();
	}
	if (sampleBitDepth > maxDepth)
		return StreamFormat::getNullStreamFormat();

	replaceField<16, 23>(fmt, static_cast<std::uint8_t>(format)); // format = sampleFormat
	replaceField<24, 31>(fmt, static_cast<std::uint16_t>(sampleBitDepth)); // bit_depth = sampleBitDepth
	replaceField<32, 41>(fmt, static_cast<std::uint16_t>(channelsCount)); // channels_per_frame = channelsCount
	replaceField<42, 51>(fmt, static_cast<std::uint16_t>(samplesPerFrame)); // samples_per_frame = samplesPerFrame

	return StreamFormat{ fmt };
}

bool LA_AVDECC_CALL_CONVENTION StreamFormatInfo::isListenerFormatCompatibleWithTalkerFormat(StreamFormat const& listenerStreamFormat, StreamFormat const& talkerStreamFormat) noexcept
{
	auto lFormatInfo = StreamFormatInfo::create(listenerStreamFormat);
	auto tFormatInfo = StreamFormatInfo::create(talkerStreamFormat);

	if (lFormatInfo->getType() == tFormatInfo->getType() // Same type
			&& lFormatInfo->getChannelsCount() == tFormatInfo->getChannelsCount() // Same channels count
			&& !lFormatInfo->isUpToChannelsCount() // Not an up-to channels count format (has to be an Adapted one)
			&& !tFormatInfo->isUpToChannelsCount() // Not an up-to channels count format (has to be an Adapted one)
			&& lFormatInfo->getSamplingRate() == tFormatInfo->getSamplingRate() // Same sampling rate
			&& lFormatInfo->getSampleFormat() == tFormatInfo->getSampleFormat() // Same sample format
		// Ignore SampleBitDepth, because it only affects quality, not compatibility
	)
	{
		// Check clock sync compatibility (All accepted except if Talker is Async and Listener is Sync)
		if (tFormatInfo->useSynchronousClock() || !lFormatInfo->useSynchronousClock())
		{
			return true;
		}
	}

	return false;
}

std::pair<StreamFormat, StreamFormat> LA_AVDECC_CALL_CONVENTION StreamFormatInfo::getAdaptedCompatibleFormats(StreamFormat const& listenerStreamFormat, StreamFormat const& talkerStreamFormat) noexcept
{
	auto lFormatInfo = StreamFormatInfo::create(listenerStreamFormat);
	auto tFormatInfo = StreamFormatInfo::create(talkerStreamFormat);

	// First perform basic checks
	if (lFormatInfo->getType() == tFormatInfo->getType() // Same type
			&& lFormatInfo->getSamplingRate() == tFormatInfo->getSamplingRate() // Same sampling rate
			&& lFormatInfo->getSampleFormat() == tFormatInfo->getSampleFormat() // Same sample format
			// Ignore SampleBitDepth, because it only affects quality, not compatibility
			&& (tFormatInfo->useSynchronousClock() || !lFormatInfo->useSynchronousClock()) // All accepted except if Talker is Async and Listener is Sync
	)
	{
		auto lChanCount = lFormatInfo->getChannelsCount();
		auto tChanCount = tFormatInfo->getChannelsCount();

		// If listener is an up-to format, get the min between 'max listener up-to' and 'talker count' (which might be an up-to as well)
		if (lFormatInfo->isUpToChannelsCount())
		{
			lChanCount = std::min(lChanCount, tChanCount);
		}
		// Same for talker
		if (tFormatInfo->isUpToChannelsCount())
		{
			tChanCount = std::min(tChanCount, lChanCount);
		}

		// Now we can compare the channel count
		if (lChanCount == tChanCount)
		{
			// Ok, return adapted formats for both talker and listener
			auto const lAdapted = lFormatInfo->getAdaptedStreamFormat(lChanCount);
			auto const tAdapted = tFormatInfo->getAdaptedStreamFormat(tChanCount);
			if (AVDECC_ASSERT_WITH_RET(lAdapted && tAdapted, "Failed to get AdaptedFormat for either Listener or Talker"))
			{
				return std::make_pair(lAdapted, tAdapted);
			}
		}
	}

	return std::make_pair(StreamFormat::getNullStreamFormat(), StreamFormat::getNullStreamFormat());
}

} // namespace model
} // namespace entity
} // namespace avdecc
} // namespace la
