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
* @file streamFormat_tests.cpp
* @author Christophe Calmejane
*/

// Public API
#include <la/avdecc/internals/streamFormatInfo.hpp>

#include <gtest/gtest.h>

TEST(StreamFormatInfo, NotAVTPFormat)
{
	auto format = la::avdecc::entity::model::StreamFormatInfo::create(la::avdecc::entity::model::StreamFormat{ 0x8000000000000000 });
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::Type::Unsupported, format->getType());
}

TEST(StreamFormatInfo, IIDC_61883_6_Mono_48kHz_24bits_Async)
{
	auto const fmt{ la::avdecc::entity::model::StreamFormat{ 0x00A0020140000100 } };
	EXPECT_EQ(fmt, la::avdecc::entity::model::StreamFormatInfo::buildFormat_IEC_61883_6(1, false, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int24, false));
	auto const format = la::avdecc::entity::model::StreamFormatInfo::create(fmt);
	EXPECT_EQ(fmt, format->getStreamFormat());
	EXPECT_EQ(fmt, format->getAdaptedStreamFormat(1));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), format->getAdaptedStreamFormat(0));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), format->getAdaptedStreamFormat(2));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::Type::IEC_61883_6, format->getType());
	EXPECT_EQ(1, format->getChannelsCount());
	EXPECT_FALSE(format->isUpToChannelsCount());
	EXPECT_EQ(0, format->getSamplingRate().getPull());
	EXPECT_EQ(48000, format->getSamplingRate().getBaseFrequency());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int24, format->getSampleFormat());
	EXPECT_FALSE(format->useSynchronousClock());
	EXPECT_EQ(24, format->getSampleSize());
	EXPECT_EQ(24, format->getSampleBitDepth());
}

TEST(StreamFormatInfo, IIDC_61883_6_Mono_48kHz_24bits_Sync)
{
	auto const fmt{ la::avdecc::entity::model::StreamFormat{ 0x00A0020150000100 } };
	EXPECT_EQ(fmt, la::avdecc::entity::model::StreamFormatInfo::buildFormat_IEC_61883_6(1, false, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int24, true));
	auto const format = la::avdecc::entity::model::StreamFormatInfo::create(fmt);
	EXPECT_EQ(fmt, format->getStreamFormat());
	EXPECT_EQ(fmt, format->getAdaptedStreamFormat(1));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), format->getAdaptedStreamFormat(0));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), format->getAdaptedStreamFormat(2));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::Type::IEC_61883_6, format->getType());
	EXPECT_EQ(1, format->getChannelsCount());
	EXPECT_FALSE(format->isUpToChannelsCount());
	EXPECT_EQ(0, format->getSamplingRate().getPull());
	EXPECT_EQ(48000, format->getSamplingRate().getBaseFrequency());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int24, format->getSampleFormat());
	EXPECT_TRUE(format->useSynchronousClock());
	EXPECT_EQ(24, format->getSampleSize());
	EXPECT_EQ(24, format->getSampleBitDepth());
}

TEST(StreamFormatInfo, IIDC_61883_6_Octo_48kHz_24bits_Async)
{
	auto const fmt{ la::avdecc::entity::model::StreamFormat{ 0x00A0020840000800 } };
	EXPECT_EQ(fmt, la::avdecc::entity::model::StreamFormatInfo::buildFormat_IEC_61883_6(8, false, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int24, false));
	auto const format = la::avdecc::entity::model::StreamFormatInfo::create(fmt);
	EXPECT_EQ(fmt, format->getStreamFormat());
	EXPECT_EQ(fmt, format->getAdaptedStreamFormat(8));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), format->getAdaptedStreamFormat(0));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), format->getAdaptedStreamFormat(4));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::Type::IEC_61883_6, format->getType());
	EXPECT_EQ(8, format->getChannelsCount());
	EXPECT_FALSE(format->isUpToChannelsCount());
	EXPECT_EQ(0, format->getSamplingRate().getPull());
	EXPECT_EQ(48000, format->getSamplingRate().getBaseFrequency());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int24, format->getSampleFormat());
	EXPECT_FALSE(format->useSynchronousClock());
	EXPECT_EQ(24, format->getSampleSize());
	EXPECT_EQ(24, format->getSampleBitDepth());
}

TEST(StreamFormatInfo, IIDC_61883_6_Octo_48kHz_24bits_Sync)
{
	auto const fmt{ la::avdecc::entity::model::StreamFormat{ 0x00A0020850000800 } };
	EXPECT_EQ(fmt, la::avdecc::entity::model::StreamFormatInfo::buildFormat_IEC_61883_6(8, false, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int24, true));
	auto const format = la::avdecc::entity::model::StreamFormatInfo::create(fmt);
	EXPECT_EQ(fmt, format->getStreamFormat());
	EXPECT_EQ(fmt, format->getAdaptedStreamFormat(8));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), format->getAdaptedStreamFormat(0));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), format->getAdaptedStreamFormat(4));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::Type::IEC_61883_6, format->getType());
	EXPECT_EQ(8, format->getChannelsCount());
	EXPECT_FALSE(format->isUpToChannelsCount());
	EXPECT_EQ(0, format->getSamplingRate().getPull());
	EXPECT_EQ(48000, format->getSamplingRate().getBaseFrequency());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int24, format->getSampleFormat());
	EXPECT_TRUE(format->useSynchronousClock());
	EXPECT_EQ(24, format->getSampleSize());
	EXPECT_EQ(24, format->getSampleBitDepth());
}

TEST(StreamFormatInfo, IIDC_61883_6_upTo32_48kHz_24bits_Async)
{
	auto const fmt{ la::avdecc::entity::model::StreamFormat{ 0x00A0022060002000 } };
	EXPECT_EQ(fmt, la::avdecc::entity::model::StreamFormatInfo::buildFormat_IEC_61883_6(32, true, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int24, false));
	auto const format = la::avdecc::entity::model::StreamFormatInfo::create(fmt);
	EXPECT_EQ(fmt, format->getStreamFormat());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormat{ 0x00A0020440000400 }, format->getAdaptedStreamFormat(4));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormat{ 0x00A0020840000800 }, format->getAdaptedStreamFormat(8));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormat{ 0x00A0022040002000 }, format->getAdaptedStreamFormat(32));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), format->getAdaptedStreamFormat(33));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::Type::IEC_61883_6, format->getType());
	EXPECT_EQ(32, format->getChannelsCount());
	EXPECT_TRUE(format->isUpToChannelsCount());
	EXPECT_EQ(0, format->getSamplingRate().getPull());
	EXPECT_EQ(48000, format->getSamplingRate().getBaseFrequency());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int24, format->getSampleFormat());
	EXPECT_FALSE(format->useSynchronousClock());
	EXPECT_EQ(24, format->getSampleSize());
	EXPECT_EQ(24, format->getSampleBitDepth());
}

TEST(StreamFormatInfo, IIDC_61883_6_upTo32_48kHz_24bits_Sync)
{
	auto const fmt{ la::avdecc::entity::model::StreamFormat{ 0x00A0022070002000 } };
	EXPECT_EQ(fmt, la::avdecc::entity::model::StreamFormatInfo::buildFormat_IEC_61883_6(32, true, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int24, true));
	auto const format = la::avdecc::entity::model::StreamFormatInfo::create(fmt);
	EXPECT_EQ(fmt, format->getStreamFormat());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormat{ 0x00A0020450000400 }, format->getAdaptedStreamFormat(4));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormat{ 0x00A0020850000800 }, format->getAdaptedStreamFormat(8));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormat{ 0x00A0022050002000 }, format->getAdaptedStreamFormat(32));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), format->getAdaptedStreamFormat(33));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::Type::IEC_61883_6, format->getType());
	EXPECT_EQ(32, format->getChannelsCount());
	EXPECT_TRUE(format->isUpToChannelsCount());
	EXPECT_EQ(0, format->getSamplingRate().getPull());
	EXPECT_EQ(48000, format->getSamplingRate().getBaseFrequency());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int24, format->getSampleFormat());
	EXPECT_TRUE(format->useSynchronousClock());
	EXPECT_EQ(24, format->getSampleSize());
	EXPECT_EQ(24, format->getSampleBitDepth());
}

TEST(StreamFormatInfo, AAF_Stereo_48kHz_6spf_16bits)
{
	auto const fmt{ la::avdecc::entity::model::StreamFormat{ 0x0205041000806000 } };
	EXPECT_EQ(fmt, la::avdecc::entity::model::StreamFormatInfo::buildFormat_AAF(2, false, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int16, 16, 6));
	auto const format = la::avdecc::entity::model::StreamFormatInfo::create(fmt);
	EXPECT_EQ(fmt, format->getStreamFormat());
	EXPECT_EQ(fmt, format->getAdaptedStreamFormat(2));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), format->getAdaptedStreamFormat(0));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), format->getAdaptedStreamFormat(1));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::Type::AAF, format->getType());
	EXPECT_EQ(2, format->getChannelsCount());
	EXPECT_FALSE(format->isUpToChannelsCount());
	EXPECT_EQ(0, format->getSamplingRate().getPull());
	EXPECT_EQ(48000, format->getSamplingRate().getBaseFrequency());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int16, format->getSampleFormat());
	EXPECT_TRUE(format->useSynchronousClock());
	EXPECT_EQ(16, format->getSampleSize());
	EXPECT_EQ(16, format->getSampleBitDepth());
}

TEST(StreamFormatInfo, AAF_Octo_48kHz_64spf_16bits)
{
	auto const fmt{ la::avdecc::entity::model::StreamFormat{ 0x0205041002040000 } };
	EXPECT_EQ(fmt, la::avdecc::entity::model::StreamFormatInfo::buildFormat_AAF(8, false, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int16, 16, 64));
	auto const format = la::avdecc::entity::model::StreamFormatInfo::create(fmt);
	EXPECT_EQ(fmt, format->getStreamFormat());
	EXPECT_EQ(fmt, format->getAdaptedStreamFormat(8));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), format->getAdaptedStreamFormat(0));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), format->getAdaptedStreamFormat(4));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::Type::AAF, format->getType());
	EXPECT_EQ(8, format->getChannelsCount());
	EXPECT_FALSE(format->isUpToChannelsCount());
	EXPECT_EQ(0, format->getSamplingRate().getPull());
	EXPECT_EQ(48000, format->getSamplingRate().getBaseFrequency());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int16, format->getSampleFormat());
	EXPECT_TRUE(format->useSynchronousClock());
	EXPECT_EQ(16, format->getSampleSize());
	EXPECT_EQ(16, format->getSampleBitDepth());
}

TEST(StreamFormatInfo, AAF_Hexa_96kHz_12spf_32bits_24depth)
{
	auto const fmt{ la::avdecc::entity::model::StreamFormat{ 0x020702180180C000 } };
	EXPECT_EQ(fmt, la::avdecc::entity::model::StreamFormatInfo::buildFormat_AAF(6, false, la::avdecc::entity::model::SamplingRate{ 0, 96000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int32, 24, 12));
	auto const format = la::avdecc::entity::model::StreamFormatInfo::create(fmt);
	EXPECT_EQ(fmt, format->getStreamFormat());
	EXPECT_EQ(fmt, format->getAdaptedStreamFormat(6));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), format->getAdaptedStreamFormat(0));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), format->getAdaptedStreamFormat(4));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::Type::AAF, format->getType());
	EXPECT_EQ(6, format->getChannelsCount());
	EXPECT_FALSE(format->isUpToChannelsCount());
	EXPECT_EQ(0, format->getSamplingRate().getPull());
	EXPECT_EQ(96000, format->getSamplingRate().getBaseFrequency());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int32, format->getSampleFormat());
	EXPECT_TRUE(format->useSynchronousClock());
	EXPECT_EQ(32, format->getSampleSize());
	EXPECT_EQ(24, format->getSampleBitDepth());
}

TEST(StreamFormatInfo, AAF_upTo32_48kHz_64spf_16bits)
{
	auto const fmt{ la::avdecc::entity::model::StreamFormat{ 0x0215041008040000 } };
	EXPECT_EQ(fmt, la::avdecc::entity::model::StreamFormatInfo::buildFormat_AAF(32, true, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int16, 16, 64));
	auto const format = la::avdecc::entity::model::StreamFormatInfo::create(fmt);
	EXPECT_EQ(fmt, format->getStreamFormat());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormat{ 0x0205041001040000 }, format->getAdaptedStreamFormat(4));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormat{ 0x0205041002040000 }, format->getAdaptedStreamFormat(8));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormat{ 0x0205041008040000 }, format->getAdaptedStreamFormat(32));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::Type::AAF, format->getType());
	EXPECT_EQ(32, format->getChannelsCount());
	EXPECT_TRUE(format->isUpToChannelsCount());
	EXPECT_EQ(0, format->getSamplingRate().getPull());
	EXPECT_EQ(48000, format->getSamplingRate().getBaseFrequency());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int16, format->getSampleFormat());
	EXPECT_TRUE(format->useSynchronousClock());
	EXPECT_EQ(16, format->getSampleSize());
	EXPECT_EQ(16, format->getSampleBitDepth());
}

TEST(StreamFormatInfo, CR_48_6intvl_1ts)
{
	auto const fmt{ la::avdecc::entity::model::StreamFormat{ 0x041006010000bb80 } };
	auto const format = la::avdecc::entity::model::StreamFormatInfo::create(fmt);
	EXPECT_EQ(fmt, format->getStreamFormat());
	ASSERT_EQ(la::avdecc::entity::model::StreamFormatInfo::Type::ClockReference, format->getType());
	auto const crfFormat = static_cast<la::avdecc::entity::model::StreamFormatInfoCRF const*>(format.get());
	EXPECT_EQ(0, format->getChannelsCount());
	EXPECT_FALSE(format->isUpToChannelsCount());
	EXPECT_EQ(0, format->getSamplingRate().getPull());
	EXPECT_EQ(48000, format->getSamplingRate().getBaseFrequency());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int64, format->getSampleFormat());
	EXPECT_TRUE(format->useSynchronousClock());
	EXPECT_EQ(6u, crfFormat->getTimestampInterval());
	EXPECT_EQ(1u, crfFormat->getTimestampsPerPdu());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfoCRF::CRFType::AudioSample, crfFormat->getCRFType());
	EXPECT_EQ(64, format->getSampleSize());
	EXPECT_EQ(64, format->getSampleBitDepth());
}

TEST(StreamFormatInfo, CR_96_12intvl_1ts)
{
	auto const fmt{ la::avdecc::entity::model::StreamFormat{ 0x04100c0100017700 } };
	auto const format = la::avdecc::entity::model::StreamFormatInfo::create(fmt);
	EXPECT_EQ(fmt, format->getStreamFormat());
	ASSERT_EQ(la::avdecc::entity::model::StreamFormatInfo::Type::ClockReference, format->getType());
	auto const crfFormat = static_cast<la::avdecc::entity::model::StreamFormatInfoCRF const*>(format.get());
	EXPECT_EQ(0, format->getChannelsCount());
	EXPECT_FALSE(format->isUpToChannelsCount());
	EXPECT_EQ(0, format->getSamplingRate().getPull());
	EXPECT_EQ(96000, format->getSamplingRate().getBaseFrequency());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int64, format->getSampleFormat());
	EXPECT_TRUE(format->useSynchronousClock());
	EXPECT_EQ(12u, crfFormat->getTimestampInterval());
	EXPECT_EQ(1u, crfFormat->getTimestampsPerPdu());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfoCRF::CRFType::AudioSample, crfFormat->getCRFType());
	EXPECT_EQ(64, format->getSampleSize());
	EXPECT_EQ(64, format->getSampleBitDepth());
}

TEST(StreamFormatInfo, CR_96_320intvl_6ts)
{
	auto const fmt{ la::avdecc::entity::model::StreamFormat{ 0x0411400600017700 } };
	auto const format = la::avdecc::entity::model::StreamFormatInfo::create(fmt);
	EXPECT_EQ(fmt, format->getStreamFormat());
	ASSERT_EQ(la::avdecc::entity::model::StreamFormatInfo::Type::ClockReference, format->getType());
	auto const crfFormat = static_cast<la::avdecc::entity::model::StreamFormatInfoCRF const*>(format.get());
	EXPECT_EQ(0, format->getChannelsCount());
	EXPECT_FALSE(format->isUpToChannelsCount());
	EXPECT_EQ(0, format->getSamplingRate().getPull());
	EXPECT_EQ(96000, format->getSamplingRate().getBaseFrequency());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int64, format->getSampleFormat());
	EXPECT_TRUE(format->useSynchronousClock());
	EXPECT_EQ(320u, crfFormat->getTimestampInterval());
	EXPECT_EQ(6u, crfFormat->getTimestampsPerPdu());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfoCRF::CRFType::AudioSample, crfFormat->getCRFType());
	EXPECT_EQ(64, format->getSampleSize());
	EXPECT_EQ(64, format->getSampleBitDepth());
}

TEST(StreamFormatInfo, CR_96_768intvl_5ts)
{
	auto const fmt{ la::avdecc::entity::model::StreamFormat{ 0x0413000500017700 } };
	auto const format = la::avdecc::entity::model::StreamFormatInfo::create(fmt);
	EXPECT_EQ(fmt, format->getStreamFormat());
	ASSERT_EQ(la::avdecc::entity::model::StreamFormatInfo::Type::ClockReference, format->getType());
	auto const crfFormat = static_cast<la::avdecc::entity::model::StreamFormatInfoCRF const*>(format.get());
	EXPECT_EQ(0, format->getChannelsCount());
	EXPECT_FALSE(format->isUpToChannelsCount());
	EXPECT_EQ(0, format->getSamplingRate().getPull());
	EXPECT_EQ(96000, format->getSamplingRate().getBaseFrequency());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int64, format->getSampleFormat());
	EXPECT_TRUE(format->useSynchronousClock());
	EXPECT_EQ(768u, crfFormat->getTimestampInterval());
	EXPECT_EQ(5u, crfFormat->getTimestampsPerPdu());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfoCRF::CRFType::AudioSample, crfFormat->getCRFType());
	EXPECT_EQ(64, format->getSampleSize());
	EXPECT_EQ(64, format->getSampleBitDepth());
}

TEST(StreamFormatInfo, CR_500Hz)
{
	auto const fmt{ la::avdecc::entity::model::StreamFormat{ 0x04100101000001F4 } };
	auto const format = la::avdecc::entity::model::StreamFormatInfo::create(fmt);
	EXPECT_EQ(fmt, format->getStreamFormat());
	ASSERT_EQ(la::avdecc::entity::model::StreamFormatInfo::Type::ClockReference, format->getType());
	auto const crfFormat = static_cast<la::avdecc::entity::model::StreamFormatInfoCRF const*>(format.get());
	EXPECT_EQ(0, format->getChannelsCount());
	EXPECT_FALSE(format->isUpToChannelsCount());
	EXPECT_EQ(0, format->getSamplingRate().getPull());
	EXPECT_EQ(500, format->getSamplingRate().getBaseFrequency());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int64, format->getSampleFormat());
	EXPECT_TRUE(format->useSynchronousClock());
	EXPECT_EQ(1u, crfFormat->getTimestampInterval());
	EXPECT_EQ(1u, crfFormat->getTimestampsPerPdu());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfoCRF::CRFType::AudioSample, crfFormat->getCRFType());
	EXPECT_EQ(64, format->getSampleSize());
	EXPECT_EQ(64, format->getSampleBitDepth());
}

TEST(StreamFormatInfo, isListenerFormatCompatibleWithTalkerFormat)
{
	// Up-to bit formats shall not be passed to isListenerFormatCompatibleWithTalkerFormat
	{
		auto const fmt8 = la::avdecc::entity::model::StreamFormatInfo::buildFormat_AAF(8, false, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int16, 16, 64);
		auto const fmtUpTo8 = la::avdecc::entity::model::StreamFormatInfo::buildFormat_AAF(8, true, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int16, 16, 64);
		ASSERT_NE(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmt8);
		ASSERT_NE(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmtUpTo8);
		EXPECT_FALSE(la::avdecc::entity::model::StreamFormatInfo::isListenerFormatCompatibleWithTalkerFormat(fmt8, fmtUpTo8));
		EXPECT_FALSE(la::avdecc::entity::model::StreamFormatInfo::isListenerFormatCompatibleWithTalkerFormat(fmtUpTo8, fmt8));
		EXPECT_FALSE(la::avdecc::entity::model::StreamFormatInfo::isListenerFormatCompatibleWithTalkerFormat(fmtUpTo8, fmtUpTo8));
	}

	// Difference in Type should fail
	{
		auto const fmtIEC = la::avdecc::entity::model::StreamFormatInfo::buildFormat_IEC_61883_6(8, false, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int24, true);
		auto const fmtAAF = la::avdecc::entity::model::StreamFormatInfo::buildFormat_AAF(8, false, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int24, 24, 64);
		ASSERT_NE(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmtIEC);
		ASSERT_NE(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmtAAF);
		EXPECT_FALSE(la::avdecc::entity::model::StreamFormatInfo::isListenerFormatCompatibleWithTalkerFormat(fmtIEC, fmtAAF));
		EXPECT_FALSE(la::avdecc::entity::model::StreamFormatInfo::isListenerFormatCompatibleWithTalkerFormat(fmtAAF, fmtIEC));
	}

	// Difference in Sampling Rate should fail
	{
		auto const fmtRate48 = la::avdecc::entity::model::StreamFormatInfo::buildFormat_AAF(8, false, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int32, 24, 64);
		auto const fmtRate96 = la::avdecc::entity::model::StreamFormatInfo::buildFormat_AAF(8, false, la::avdecc::entity::model::SamplingRate{ 0, 96000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int32, 32, 64);
		ASSERT_NE(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmtRate48);
		ASSERT_NE(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmtRate96);
		EXPECT_FALSE(la::avdecc::entity::model::StreamFormatInfo::isListenerFormatCompatibleWithTalkerFormat(fmtRate48, fmtRate96));
		EXPECT_FALSE(la::avdecc::entity::model::StreamFormatInfo::isListenerFormatCompatibleWithTalkerFormat(fmtRate96, fmtRate48));
	}

	// Difference in Sample Format should fail (even though the bit depth is the same)
	{
		auto const fmt24 = la::avdecc::entity::model::StreamFormatInfo::buildFormat_AAF(8, false, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int24, 24, 64);
		auto const fmt32 = la::avdecc::entity::model::StreamFormatInfo::buildFormat_AAF(8, false, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int32, 24, 64);
		ASSERT_NE(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmt24);
		ASSERT_NE(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmt32);
		EXPECT_FALSE(la::avdecc::entity::model::StreamFormatInfo::isListenerFormatCompatibleWithTalkerFormat(fmt24, fmt32));
		EXPECT_FALSE(la::avdecc::entity::model::StreamFormatInfo::isListenerFormatCompatibleWithTalkerFormat(fmt32, fmt24));
	}

	// Same (non up-to) formats should be compatible
	{
		auto const lFmt = la::avdecc::entity::model::StreamFormatInfo::buildFormat_AAF(8, false, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int16, 16, 64);
		auto const tFmt = la::avdecc::entity::model::StreamFormatInfo::buildFormat_AAF(8, false, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int16, 16, 64);
		ASSERT_NE(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), lFmt);
		ASSERT_NE(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), tFmt);
		EXPECT_TRUE(la::avdecc::entity::model::StreamFormatInfo::isListenerFormatCompatibleWithTalkerFormat(lFmt, tFmt));
	}

	// Same (non up-to) formats but with different depth should be compatible
	{
		auto const fmtDepth24 = la::avdecc::entity::model::StreamFormatInfo::buildFormat_AAF(8, false, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int32, 24, 64);
		auto const fmtDepth32 = la::avdecc::entity::model::StreamFormatInfo::buildFormat_AAF(8, false, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int32, 32, 64);
		ASSERT_NE(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmtDepth24);
		ASSERT_NE(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmtDepth32);
		EXPECT_TRUE(la::avdecc::entity::model::StreamFormatInfo::isListenerFormatCompatibleWithTalkerFormat(fmtDepth24, fmtDepth32));
		EXPECT_TRUE(la::avdecc::entity::model::StreamFormatInfo::isListenerFormatCompatibleWithTalkerFormat(fmtDepth32, fmtDepth24));
	}

	// Only Async Talker to Sync Listener should fail
	{
		auto const sync = la::avdecc::entity::model::StreamFormatInfo::buildFormat_IEC_61883_6(8, false, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int24, true);
		auto const async = la::avdecc::entity::model::StreamFormatInfo::buildFormat_IEC_61883_6(8, false, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int24, false);
		ASSERT_NE(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), sync);
		ASSERT_NE(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), async);
		EXPECT_TRUE(la::avdecc::entity::model::StreamFormatInfo::isListenerFormatCompatibleWithTalkerFormat(sync, sync));
		EXPECT_FALSE(la::avdecc::entity::model::StreamFormatInfo::isListenerFormatCompatibleWithTalkerFormat(sync, async));
		EXPECT_TRUE(la::avdecc::entity::model::StreamFormatInfo::isListenerFormatCompatibleWithTalkerFormat(async, sync));
		EXPECT_TRUE(la::avdecc::entity::model::StreamFormatInfo::isListenerFormatCompatibleWithTalkerFormat(async, async));
	}
}

TEST(StreamFormatInfo, getAdaptedStreamFormats)
{
	// Difference in Type should fail
	{
		auto const fmtIEC = la::avdecc::entity::model::StreamFormatInfo::buildFormat_IEC_61883_6(8, false, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int24, true);
		auto const fmtAAF = la::avdecc::entity::model::StreamFormatInfo::buildFormat_AAF(8, false, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int24, 24, 64);
		ASSERT_NE(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmtIEC);
		ASSERT_NE(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmtAAF);
		{
			auto const fmts = la::avdecc::entity::model::StreamFormatInfo::getAdaptedCompatibleFormats(fmtIEC, fmtAAF);
			EXPECT_EQ(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmts.first);
			EXPECT_EQ(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmts.second);
		}
		{
			auto const fmts = la::avdecc::entity::model::StreamFormatInfo::getAdaptedCompatibleFormats(fmtAAF, fmtIEC);
			EXPECT_EQ(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmts.first);
			EXPECT_EQ(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmts.second);
		}
	}

	// Difference in Sampling Rate should fail
	{
		auto const fmtRate48 = la::avdecc::entity::model::StreamFormatInfo::buildFormat_AAF(8, false, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int32, 24, 64);
		auto const fmtRate96 = la::avdecc::entity::model::StreamFormatInfo::buildFormat_AAF(8, false, la::avdecc::entity::model::SamplingRate{ 0, 96000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int32, 32, 64);
		ASSERT_NE(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmtRate48);
		ASSERT_NE(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmtRate96);
		{
			auto const fmts = la::avdecc::entity::model::StreamFormatInfo::getAdaptedCompatibleFormats(fmtRate48, fmtRate96);
			EXPECT_EQ(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmts.first);
			EXPECT_EQ(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmts.second);
		}
		{
			auto const fmts = la::avdecc::entity::model::StreamFormatInfo::getAdaptedCompatibleFormats(fmtRate96, fmtRate48);
			EXPECT_EQ(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmts.first);
			EXPECT_EQ(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmts.second);
		}
	}

	// Difference in Sample Format should fail (even though the bit depth is the same)
	{
		auto const fmt24 = la::avdecc::entity::model::StreamFormatInfo::buildFormat_AAF(8, false, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int24, 24, 64);
		auto const fmt32 = la::avdecc::entity::model::StreamFormatInfo::buildFormat_AAF(8, false, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int32, 24, 64);
		ASSERT_NE(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmt24);
		ASSERT_NE(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmt32);
		{
			auto const fmts = la::avdecc::entity::model::StreamFormatInfo::getAdaptedCompatibleFormats(fmt24, fmt32);
			EXPECT_EQ(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmts.first);
			EXPECT_EQ(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmts.second);
		}
		{
			auto const fmts = la::avdecc::entity::model::StreamFormatInfo::getAdaptedCompatibleFormats(fmt32, fmt24);
			EXPECT_EQ(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmts.first);
			EXPECT_EQ(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmts.second);
		}
	}

	// Same (non up-to) formats should be compatible
	{
		auto const lFmt = la::avdecc::entity::model::StreamFormatInfo::buildFormat_AAF(8, false, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int16, 16, 64);
		auto const tFmt = la::avdecc::entity::model::StreamFormatInfo::buildFormat_AAF(8, false, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int16, 16, 64);
		ASSERT_NE(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), lFmt);
		ASSERT_NE(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), tFmt);
		auto const fmts = la::avdecc::entity::model::StreamFormatInfo::getAdaptedCompatibleFormats(lFmt, tFmt);
		EXPECT_EQ(fmts.first, fmts.second);
		EXPECT_EQ(lFmt, fmts.first);
	}

	// Same (non up-to) formats but with different depth should be compatible
	{
		auto const fmtDepth24 = la::avdecc::entity::model::StreamFormatInfo::buildFormat_AAF(8, false, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int32, 24, 64);
		auto const fmtDepth32 = la::avdecc::entity::model::StreamFormatInfo::buildFormat_AAF(8, false, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int32, 32, 64);
		ASSERT_NE(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmtDepth24);
		ASSERT_NE(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmtDepth32);
		{
			auto const fmts = la::avdecc::entity::model::StreamFormatInfo::getAdaptedCompatibleFormats(fmtDepth24, fmtDepth32);
			EXPECT_EQ(fmtDepth24, fmts.first);
			EXPECT_EQ(fmtDepth32, fmts.second);
		}
		{
			auto const fmts = la::avdecc::entity::model::StreamFormatInfo::getAdaptedCompatibleFormats(fmtDepth32, fmtDepth24);
			EXPECT_EQ(fmtDepth32, fmts.first);
			EXPECT_EQ(fmtDepth24, fmts.second);
		}
	}

	// Only Async Talker to Sync Listener should fail
	{
		auto const sync = la::avdecc::entity::model::StreamFormatInfo::buildFormat_IEC_61883_6(8, false, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int24, true);
		auto const async = la::avdecc::entity::model::StreamFormatInfo::buildFormat_IEC_61883_6(8, false, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int24, false);
		ASSERT_NE(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), sync);
		ASSERT_NE(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), async);
		{
			auto const fmts = la::avdecc::entity::model::StreamFormatInfo::getAdaptedCompatibleFormats(sync, sync);
			EXPECT_EQ(sync, fmts.first);
			EXPECT_EQ(sync, fmts.second);
		}
		{
			auto const fmts = la::avdecc::entity::model::StreamFormatInfo::getAdaptedCompatibleFormats(sync, async);
			EXPECT_EQ(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmts.first);
			EXPECT_EQ(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmts.second);
		}
		{
			auto const fmts = la::avdecc::entity::model::StreamFormatInfo::getAdaptedCompatibleFormats(async, sync);
			EXPECT_EQ(async, fmts.first);
			EXPECT_EQ(sync, fmts.second);
		}
		{
			auto const fmts = la::avdecc::entity::model::StreamFormatInfo::getAdaptedCompatibleFormats(async, async);
			EXPECT_EQ(async, fmts.first);
			EXPECT_EQ(async, fmts.second);
		}
	}

	// Same formats (with both up-to bit) should be compatible and lowest channels count should be used
	{
		auto const fmtUpTo16 = la::avdecc::entity::model::StreamFormatInfo::buildFormat_AAF(16, true, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int32, 32, 64);
		auto const fmtUpTo24 = la::avdecc::entity::model::StreamFormatInfo::buildFormat_AAF(24, true, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int32, 32, 64);
		ASSERT_NE(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmtUpTo16);
		ASSERT_NE(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmtUpTo24);
		{
			auto const fmts = la::avdecc::entity::model::StreamFormatInfo::getAdaptedCompatibleFormats(fmtUpTo16, fmtUpTo24);
			EXPECT_EQ(fmts.first, fmts.second);
			EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::buildFormat_AAF(16, false, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int32, 32, 64), fmts.first);
		}
		{
			auto const fmts = la::avdecc::entity::model::StreamFormatInfo::getAdaptedCompatibleFormats(fmtUpTo24, fmtUpTo16);
			EXPECT_EQ(fmts.first, fmts.second);
			EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::buildFormat_AAF(16, false, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int32, 32, 64), fmts.first);
		}
	}

	// Same formats (one with up-to bit) should be compatible if non up-to is included in the up-to one, and lowest channels count should be used
	{
		auto const fmtUpTo12 = la::avdecc::entity::model::StreamFormatInfo::buildFormat_AAF(12, true, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int32, 32, 64);
		auto const fmt8 = la::avdecc::entity::model::StreamFormatInfo::buildFormat_AAF(8, false, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int32, 32, 64);
		ASSERT_NE(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmtUpTo12);
		ASSERT_NE(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmt8);
		{
			auto const fmts = la::avdecc::entity::model::StreamFormatInfo::getAdaptedCompatibleFormats(fmtUpTo12, fmt8);
			EXPECT_EQ(fmts.first, fmts.second);
			EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::buildFormat_AAF(8, false, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int32, 32, 64), fmts.first);
		}
		{
			auto const fmts = la::avdecc::entity::model::StreamFormatInfo::getAdaptedCompatibleFormats(fmt8, fmtUpTo12);
			EXPECT_EQ(fmts.first, fmts.second);
			EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::buildFormat_AAF(8, false, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int32, 32, 64), fmts.first);
		}
	}

	// Same formats (one with up-to bit) should not be compatible if non up-to is not included in the up-to one
	{
		auto const fmtUpTo12 = la::avdecc::entity::model::StreamFormatInfo::buildFormat_AAF(12, true, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int32, 32, 64);
		auto const fmt16 = la::avdecc::entity::model::StreamFormatInfo::buildFormat_AAF(16, false, la::avdecc::entity::model::SamplingRate{ 0, 48000 }, la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int32, 32, 64);
		ASSERT_NE(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmtUpTo12);
		ASSERT_NE(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmt16);
		{
			auto const fmts = la::avdecc::entity::model::StreamFormatInfo::getAdaptedCompatibleFormats(fmtUpTo12, fmt16);
			EXPECT_EQ(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmts.first);
			EXPECT_EQ(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmts.second);
		}
		{
			auto const fmts = la::avdecc::entity::model::StreamFormatInfo::getAdaptedCompatibleFormats(fmt16, fmtUpTo12);
			EXPECT_EQ(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmts.first);
			EXPECT_EQ(la::avdecc::entity::model::StreamFormat::getNullStreamFormat(), fmts.second);
		}
	}
}
