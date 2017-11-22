/*
* Copyright (C) 2016-2017, L-Acoustics and its contributors

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

#include <gtest/gtest.h>
#include "la/avdecc/internals/streamFormat.hpp"

TEST(StreamFormat, NotAVTPFormat)
{
	auto format = la::avdecc::entity::model::StreamFormatInfo::create(0x8000000000000000);
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::Type::Unsupported, format->getType());
}

TEST(StreamFormat, IIDC_61883_6_Mono_48kHz_24bits_Async)
{
	auto const fmt = 0x00A0020140000100;
	auto const format = la::avdecc::entity::model::StreamFormatInfo::create(fmt);
	EXPECT_EQ(fmt, format->getStreamFormat());
	EXPECT_EQ(fmt, format->getAdaptedStreamFormat(1));
	EXPECT_EQ(la::avdecc::entity::model::getNullStreamFormat(), format->getAdaptedStreamFormat(0));
	EXPECT_EQ(la::avdecc::entity::model::getNullStreamFormat(), format->getAdaptedStreamFormat(2));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::Type::IEC_61883_6, format->getType());
	EXPECT_EQ(1, format->getChannelsCount());
	EXPECT_FALSE(format->isUpToChannelsCount());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SamplingRate::kHz_48, format->getSamplingRate());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int24, format->getSampleFormat());
	EXPECT_FALSE(format->useSynchronousClock());
}

TEST(StreamFormat, IIDC_61883_6_Mono_48kHz_24bits_Sync)
{
	auto const fmt = 0x00A0020150000100;
	auto const format = la::avdecc::entity::model::StreamFormatInfo::create(fmt);
	EXPECT_EQ(fmt, format->getStreamFormat());
	EXPECT_EQ(fmt, format->getAdaptedStreamFormat(1));
	EXPECT_EQ(la::avdecc::entity::model::getNullStreamFormat(), format->getAdaptedStreamFormat(0));
	EXPECT_EQ(la::avdecc::entity::model::getNullStreamFormat(), format->getAdaptedStreamFormat(2));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::Type::IEC_61883_6, format->getType());
	EXPECT_EQ(1, format->getChannelsCount());
	EXPECT_FALSE(format->isUpToChannelsCount());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SamplingRate::kHz_48, format->getSamplingRate());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int24, format->getSampleFormat());
	EXPECT_TRUE(format->useSynchronousClock());
}

TEST(StreamFormat, IIDC_61883_6_Octo_48kHz_24bits_Async)
{
	auto const fmt = 0x00A0020840000800;
	auto const format = la::avdecc::entity::model::StreamFormatInfo::create(fmt);
	EXPECT_EQ(fmt, format->getStreamFormat());
	EXPECT_EQ(fmt, format->getAdaptedStreamFormat(8));
	EXPECT_EQ(la::avdecc::entity::model::getNullStreamFormat(), format->getAdaptedStreamFormat(0));
	EXPECT_EQ(la::avdecc::entity::model::getNullStreamFormat(), format->getAdaptedStreamFormat(4));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::Type::IEC_61883_6, format->getType());
	EXPECT_EQ(8, format->getChannelsCount());
	EXPECT_FALSE(format->isUpToChannelsCount());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SamplingRate::kHz_48, format->getSamplingRate());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int24, format->getSampleFormat());
	EXPECT_FALSE(format->useSynchronousClock());
}

TEST(StreamFormat, IIDC_61883_6_Octo_48kHz_24bits_Sync)
{
	auto const fmt = 0x00A0020850000800;
	auto const format = la::avdecc::entity::model::StreamFormatInfo::create(fmt);
	EXPECT_EQ(fmt, format->getStreamFormat());
	EXPECT_EQ(fmt, format->getAdaptedStreamFormat(8));
	EXPECT_EQ(la::avdecc::entity::model::getNullStreamFormat(), format->getAdaptedStreamFormat(0));
	EXPECT_EQ(la::avdecc::entity::model::getNullStreamFormat(), format->getAdaptedStreamFormat(4));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::Type::IEC_61883_6, format->getType());
	EXPECT_EQ(8, format->getChannelsCount());
	EXPECT_FALSE(format->isUpToChannelsCount());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SamplingRate::kHz_48, format->getSamplingRate());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int24, format->getSampleFormat());
	EXPECT_TRUE(format->useSynchronousClock());
}

TEST(StreamFormat, IIDC_61883_6_upTo32_48kHz_24bits_Async)
{
	auto const fmt = 0x00A0022060002000;
	auto const format = la::avdecc::entity::model::StreamFormatInfo::create(fmt);
	EXPECT_EQ(fmt, format->getStreamFormat());
	EXPECT_EQ(0x00A0020440000400, format->getAdaptedStreamFormat(4));
	EXPECT_EQ(0x00A0020840000800, format->getAdaptedStreamFormat(8));
	EXPECT_EQ(0x00A0022040002000, format->getAdaptedStreamFormat(32));
	EXPECT_EQ(la::avdecc::entity::model::getNullStreamFormat(), format->getAdaptedStreamFormat(33));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::Type::IEC_61883_6, format->getType());
	EXPECT_EQ(32, format->getChannelsCount());
	EXPECT_TRUE(format->isUpToChannelsCount());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SamplingRate::kHz_48, format->getSamplingRate());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int24, format->getSampleFormat());
	EXPECT_FALSE(format->useSynchronousClock());
}

TEST(StreamFormat, IIDC_61883_6_upTo32_48kHz_24bits_Sync)
{
	auto const fmt = 0x00A0022070002000;
	auto const format = la::avdecc::entity::model::StreamFormatInfo::create(fmt);
	EXPECT_EQ(fmt, format->getStreamFormat());
	EXPECT_EQ(0x00A0020450000400, format->getAdaptedStreamFormat(4));
	EXPECT_EQ(0x00A0020850000800, format->getAdaptedStreamFormat(8));
	EXPECT_EQ(0x00A0022050002000, format->getAdaptedStreamFormat(32));
	EXPECT_EQ(la::avdecc::entity::model::getNullStreamFormat(), format->getAdaptedStreamFormat(33));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::Type::IEC_61883_6, format->getType());
	EXPECT_EQ(32, format->getChannelsCount());
	EXPECT_TRUE(format->isUpToChannelsCount());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SamplingRate::kHz_48, format->getSamplingRate());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int24, format->getSampleFormat());
	EXPECT_TRUE(format->useSynchronousClock());
}

TEST(StreamFormat, AAF_Stereo_48kHz_6spf_16bits)
{
	auto const fmt = 0x0205041000806000;
	auto const format = la::avdecc::entity::model::StreamFormatInfo::create(fmt);
	EXPECT_EQ(fmt, format->getStreamFormat());
	EXPECT_EQ(fmt, format->getAdaptedStreamFormat(2));
	EXPECT_EQ(la::avdecc::entity::model::getNullStreamFormat(), format->getAdaptedStreamFormat(0));
	EXPECT_EQ(la::avdecc::entity::model::getNullStreamFormat(), format->getAdaptedStreamFormat(1));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::Type::AAF, format->getType());
	EXPECT_EQ(2, format->getChannelsCount());
	EXPECT_FALSE(format->isUpToChannelsCount());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SamplingRate::kHz_48, format->getSamplingRate());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int16, format->getSampleFormat());
	EXPECT_TRUE(format->useSynchronousClock());
}

TEST(StreamFormat, AAF_Octo_48kHz_64spf_16bits)
{
	auto const fmt = 0x0205041002040000;
	auto const format = la::avdecc::entity::model::StreamFormatInfo::create(fmt);
	EXPECT_EQ(fmt, format->getStreamFormat());
	EXPECT_EQ(fmt, format->getAdaptedStreamFormat(8));
	EXPECT_EQ(la::avdecc::entity::model::getNullStreamFormat(), format->getAdaptedStreamFormat(0));
	EXPECT_EQ(la::avdecc::entity::model::getNullStreamFormat(), format->getAdaptedStreamFormat(4));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::Type::AAF, format->getType());
	EXPECT_EQ(8, format->getChannelsCount());
	EXPECT_FALSE(format->isUpToChannelsCount());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SamplingRate::kHz_48, format->getSamplingRate());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int16, format->getSampleFormat());
	EXPECT_TRUE(format->useSynchronousClock());
}

TEST(StreamFormat, AAF_upTo32_48kHz_64spf_16bits)
{
	auto const fmt = 0x0215041008040000;
	auto const format = la::avdecc::entity::model::StreamFormatInfo::create(fmt);
	EXPECT_EQ(fmt, format->getStreamFormat());
	EXPECT_EQ(0x0205041001040000, format->getAdaptedStreamFormat(4));
	EXPECT_EQ(0x0205041002040000, format->getAdaptedStreamFormat(8));
	EXPECT_EQ(0x0205041008040000, format->getAdaptedStreamFormat(32));
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::Type::AAF, format->getType());
	EXPECT_EQ(32, format->getChannelsCount());
	EXPECT_TRUE(format->isUpToChannelsCount());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SamplingRate::kHz_48, format->getSamplingRate());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int16, format->getSampleFormat());
	EXPECT_TRUE(format->useSynchronousClock());
}

TEST(StreamFormat, CR_48_6intvl_1ts)
{
	auto const fmt = 0x041006010000bb80;
	auto const format = la::avdecc::entity::model::StreamFormatInfo::create(fmt);
	EXPECT_EQ(fmt, format->getStreamFormat());
	ASSERT_EQ(la::avdecc::entity::model::StreamFormatInfo::Type::ClockReference, format->getType());
	auto const crfFormat = static_cast<la::avdecc::entity::model::StreamFormatInfoCRF const*>(format.get());
	EXPECT_EQ(1, format->getChannelsCount());
	EXPECT_FALSE(format->isUpToChannelsCount());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SamplingRate::kHz_48, format->getSamplingRate());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int64, format->getSampleFormat());
	EXPECT_TRUE(format->useSynchronousClock());
	EXPECT_EQ(6u, crfFormat->getTimestampInterval());
	EXPECT_EQ(1u, crfFormat->getTimestampsPerPdu());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfoCRF::CRFType::AudioSample, crfFormat->getCRFType());
}

TEST(StreamFormat, CR_96_12intvl_1ts)
{
	auto const fmt = 0x04100c0100017700;
	auto const format = la::avdecc::entity::model::StreamFormatInfo::create(fmt);
	EXPECT_EQ(fmt, format->getStreamFormat());
	ASSERT_EQ(la::avdecc::entity::model::StreamFormatInfo::Type::ClockReference, format->getType());
	auto const crfFormat = static_cast<la::avdecc::entity::model::StreamFormatInfoCRF const*>(format.get());
	EXPECT_EQ(1, format->getChannelsCount());
	EXPECT_FALSE(format->isUpToChannelsCount());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SamplingRate::kHz_96, format->getSamplingRate());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int64, format->getSampleFormat());
	EXPECT_TRUE(format->useSynchronousClock());
	EXPECT_EQ(12u, crfFormat->getTimestampInterval());
	EXPECT_EQ(1u, crfFormat->getTimestampsPerPdu());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfoCRF::CRFType::AudioSample, crfFormat->getCRFType());
}

TEST(StreamFormat, CR_96_320intvl_6ts)
{
	auto const fmt = 0x0411400600017700;
	auto const format = la::avdecc::entity::model::StreamFormatInfo::create(fmt);
	EXPECT_EQ(fmt, format->getStreamFormat());
	ASSERT_EQ(la::avdecc::entity::model::StreamFormatInfo::Type::ClockReference, format->getType());
	auto const crfFormat = static_cast<la::avdecc::entity::model::StreamFormatInfoCRF const*>(format.get());
	EXPECT_EQ(1, format->getChannelsCount());
	EXPECT_FALSE(format->isUpToChannelsCount());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SamplingRate::kHz_96, format->getSamplingRate());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int64, format->getSampleFormat());
	EXPECT_TRUE(format->useSynchronousClock());
	EXPECT_EQ(320u, crfFormat->getTimestampInterval());
	EXPECT_EQ(6u, crfFormat->getTimestampsPerPdu());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfoCRF::CRFType::AudioSample, crfFormat->getCRFType());
}

TEST(StreamFormat, CR_96_768intvl_5ts)
{
	auto const fmt = 0x0413000500017700;
	auto const format = la::avdecc::entity::model::StreamFormatInfo::create(fmt);
	EXPECT_EQ(fmt, format->getStreamFormat());
	ASSERT_EQ(la::avdecc::entity::model::StreamFormatInfo::Type::ClockReference, format->getType());
	auto const crfFormat = static_cast<la::avdecc::entity::model::StreamFormatInfoCRF const*>(format.get());
	EXPECT_EQ(1, format->getChannelsCount());
	EXPECT_FALSE(format->isUpToChannelsCount());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SamplingRate::kHz_96, format->getSamplingRate());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int64, format->getSampleFormat());
	EXPECT_TRUE(format->useSynchronousClock());
	EXPECT_EQ(768u, crfFormat->getTimestampInterval());
	EXPECT_EQ(5u, crfFormat->getTimestampsPerPdu());
	EXPECT_EQ(la::avdecc::entity::model::StreamFormatInfoCRF::CRFType::AudioSample, crfFormat->getCRFType());
}
