/*
* Copyright (C) 2016-2024, L-Acoustics and its contributors

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
* @file streamFormatInfo.cpp
* @author Christophe Calmejane
*/

#include <la/avdecc/avdecc.hpp>
#include <la/avdecc/utils.hpp>
#include <la/avdecc/internals/streamFormatInfo.hpp>

#include <iostream>
#include <string>
#include <cstdint>

inline std::string typeToString(la::avdecc::entity::model::StreamFormatInfo::Type const type) noexcept
{
	switch (type)
	{
		case la::avdecc::entity::model::StreamFormatInfo::Type::None:
			return "None";
		case la::avdecc::entity::model::StreamFormatInfo::Type::IEC_61883_6:
			return "IEC";
		case la::avdecc::entity::model::StreamFormatInfo::Type::AAF:
			return "AAF";
		case la::avdecc::entity::model::StreamFormatInfo::Type::ClockReference:
			return "CRF";
		case la::avdecc::entity::model::StreamFormatInfo::Type::Unsupported:
			return "Unsupported";
		default:
			return "Unhandled";
	}
}
inline std::string samplingRateToString(la::avdecc::entity::model::SamplingRate const rate) noexcept
{
	auto const freq = static_cast<std::uint32_t>(rate.getNominalSampleRate());
	if (freq != 0)
	{
		if (freq < 1000)
		{
			return std::to_string(freq) + " Hz";
		}
		else
		{
			// Round to nearest integer but keep one decimal part
			auto const freqRounded = freq / 1000;
			auto const freqDecimal = (freq % 1000) / 100;
			if (freqDecimal == 0)
			{
				return std::to_string(freqRounded) + " kHz";
			}
			else
			{
				return std::to_string(freqRounded) + "." + std::to_string(freqDecimal) + " kHz";
			}
		}
	}
	return "Unknown";
}
inline std::string sampleFormatToString(la::avdecc::entity::model::StreamFormatInfo::SampleFormat const format) noexcept
{
	switch (format)
	{
		case la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int8:
			return "INT8";
		case la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int16:
			return "INT16";
		case la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int24:
			return "INT24";
		case la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int32:
			return "INT32";
		case la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Int64:
			return "INT64";
		case la::avdecc::entity::model::StreamFormatInfo::SampleFormat::FixedPoint32:
			return "FIXED32";
		case la::avdecc::entity::model::StreamFormatInfo::SampleFormat::FloatingPoint32:
			return "FLOAT32";
		case la::avdecc::entity::model::StreamFormatInfo::SampleFormat::Unknown:
			return "Unknown";
		default:
			return "Unhandled";
	}
}

using StreamFormatValue = la::avdecc::entity::model::StreamFormat::value_type;

int doJob(StreamFormatValue const value)
{
	auto const sf = la::avdecc::entity::model::StreamFormat{ value };
	auto const sfi = la::avdecc::entity::model::StreamFormatInfo::create(sf);

	std::cout << "StreamFormat " << la::avdecc::utils::toHexString(value) << " information:\n";
	std::cout << " - Type: " << typeToString(sfi->getType()) << "\n";
	if (sfi->isUpToChannelsCount())
	{
		std::cout << " - Max Channels: " << std::to_string(sfi->getChannelsCount()) << "\n";
	}
	else
	{
		std::cout << " - Channels: " << std::to_string(sfi->getChannelsCount()) << "\n";
	}
	std::cout << " - Sampling Rate: " << samplingRateToString(sfi->getSamplingRate()) << "\n";
	std::cout << " - Sample Format: " << sampleFormatToString(sfi->getSampleFormat()) << "\n";
	std::cout << " - Sample Size: " << std::to_string(sfi->getSampleSize()) << "\n";
	std::cout << " - Sample Depth: " << std::to_string(sfi->getSampleBitDepth()) << "\n";
	std::cout << " - Synchronous Clock: " << std::string{ sfi->useSynchronousClock() ? "True" : "False" } << "\n";

	return 0;
}

int main(int argc, char* argv[])
{
	// Check avdecc library interface version (only required when using the shared version of the library, but the code is here as an example)
	if (!la::avdecc::isCompatibleWithInterfaceVersion(la::avdecc::InterfaceVersion))
	{
		std::cout << "Avdecc shared library interface version invalid:\nCompiled with interface " << std::to_string(la::avdecc::InterfaceVersion) << " (v" + la::avdecc::getVersion() << "), but running interface " << std::to_string(la::avdecc::getInterfaceVersion()) << "\n";
		return -1;
	}

	if (argc != 2)
	{
		std::cout << "Usage:\nStreamFormatInfo <stream format value>\n";
		return -1;
	}

	auto const value = la::avdecc::utils::convertFromString<StreamFormatValue>(argv[1]);
	return doJob(value);
}
