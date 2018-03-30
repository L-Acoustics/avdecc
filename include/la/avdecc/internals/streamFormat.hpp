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
* @file streamFormat.hpp
* @author Christophe Calmejane
* @brief Stream format (IEEE Std 1722) helper.
*/

#pragma once

#include <cstdint>
#include <memory>
#include "entityModel.hpp"
#include "exports.hpp"

namespace la
{
namespace avdecc
{
namespace entity
{
namespace model
{

class StreamFormatInfo
{
public:
	// Types
	enum class Type
	{
		None,
		IEC_61883_6,
		AAF,
		ClockReference,
		Unsupported,
	};
	// SamplingRate
	enum class SamplingRate
	{
		kHz_8,
		kHz_16,
		kHz_24,
		kHz_32,
		kHz_44_1,
		kHz_48,
		kHz_88_2,
		kHz_96,
		kHz_176_4,
		kHz_192,
		UserDefined,
		Unknown,
	};
	// Sample format (depth and type)
	enum class SampleFormat
	{
		Int8,
		Int16,
		Int24,
		Int32,
		Int64,
		FixedPoint32,
		FloatingPoint32,
		Unknown,
	};

	using UniquePointer = std::unique_ptr<StreamFormatInfo, void(*)(StreamFormatInfo*)>;

	static LA_AVDECC_API StreamFormatInfo* LA_AVDECC_CALL_CONVENTION createRawStreamFormatInfo(StreamFormat const& streamFormat) noexcept;
	static LA_AVDECC_API bool LA_AVDECC_CALL_CONVENTION isListenerFormatCompatibleWithTalkerFormat(StreamFormat const& listenerStreamFormat, StreamFormat const& talkerStreamFormat) noexcept;

	/**
	* @brief Factory method to create a new StreamFormatInfo.
	* @details Creates a new StreamFormatInfo as a unique pointer.
	* @param[in] streamFormat The stream format to parse.
	* @return A new StreamFormatInfo as a StreamFormatInfo::UniquePointer.
	*/
	static UniquePointer create(StreamFormat const& streamFormat) noexcept
	{
		auto deleter = [](StreamFormatInfo* sampleClass)
		{
			sampleClass->destroy();
		};
		return UniquePointer(createRawStreamFormatInfo(streamFormat), deleter);
	}

	/** Returns the StreamFormat as it was passed during creation. */
	virtual StreamFormat getStreamFormat() const noexcept = 0;

	/**
	* @brief Returns the StreamFormat adapted to the specified channelsCount value.
	* @details Creates a new StreamFormatInfo as a unique pointer.
	* @param[in] channelsCount Channels count to adapt the StreamFormat to.
	* @return If isUpToChannelsCount() is false and channelCount does not match getChannelsCount(), getNullStreamFormat() is returned.
	*         If channelCount is greater than isUpToChannelsCount(), getNullStreamFormat() is returned.
	*         Otherwise, returns a valid and adapted StreamFormat (isUpToChannelsCount bit cleared).
	*/
	virtual StreamFormat getAdaptedStreamFormat(std::uint16_t const channelsCount) const noexcept = 0;

	/** Returns the stream format Type. */
	virtual Type getType() const noexcept = 0;

	/** If isUpToChannelsCount() is false, returns the channels count of the stream format. If isUpToChannelsCount() is true, returns the maximum channels count of the stream format. */
	virtual std::uint16_t getChannelsCount() const noexcept = 0;

	/** Returns whether the stream format supports adjustable channels count or not. */
	virtual bool isUpToChannelsCount() const noexcept = 0;

	/** Returns the sampling rate. */
	virtual SamplingRate getSamplingRate() const noexcept = 0;

	/** Returns the sample format. */
	virtual SampleFormat getSampleFormat() const noexcept = 0;

	/** Returns whether the stream format uses a packetization clock that is synchronous to the media clock. */
	virtual bool useSynchronousClock() const noexcept = 0;

	/** Returns the size of each sample (in bits). */
	virtual std::uint16_t getSampleSize() const noexcept = 0;

	/** Returns the depth of each sample (in bits). Only valid for integer type SampleFormat (0 otherwise). This is the number of actual valid bits in each sample (value cannot exceed the value returned by getSampleSize). */
	virtual std::uint16_t getSampleBitDepth() const noexcept = 0;

	/** Constructor */
	StreamFormatInfo() noexcept = default;

	/** Destructor */
	virtual ~StreamFormatInfo() noexcept = default;

	/** Destroy method for COM-like interface */
	virtual void destroy() noexcept = 0;

	// Defaulted compiler auto-generated methods
	StreamFormatInfo(StreamFormatInfo&&) = default;
	StreamFormatInfo(StreamFormatInfo const&) = default;
	StreamFormatInfo& operator=(StreamFormatInfo const&) = default;
	StreamFormatInfo& operator=(StreamFormatInfo&&) = default;
};

class StreamFormatInfoCRF : public StreamFormatInfo
{
public:
	// Clock Reference Type
	enum class CRFType
	{
		User,
		AudioSample,
		MachineCycle,
		Unknown,
	};

	virtual std::uint16_t getTimestampInterval() const noexcept = 0;
	virtual std::uint8_t getTimestampsPerPdu() const noexcept = 0;
	virtual CRFType getCRFType() const noexcept = 0;

	// Defaulted compiler auto-generated methods
	StreamFormatInfoCRF(StreamFormatInfoCRF&&) = default;
	StreamFormatInfoCRF(StreamFormatInfoCRF const&) = default;
	StreamFormatInfoCRF& operator=(StreamFormatInfoCRF const&) = default;
	StreamFormatInfoCRF& operator=(StreamFormatInfoCRF&&) = default;

protected:
	StreamFormatInfoCRF() noexcept = default;
};


} // namespace model
} // namespace entity
} // namespace avdecc
} // namespace la
