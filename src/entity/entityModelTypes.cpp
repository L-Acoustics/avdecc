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
* @file entityModelTypes.cpp
* @author Christophe Calmejane
*/

#include "la/avdecc/internals/entityModelTypes.hpp"
#include "la/avdecc/utils.hpp"

namespace la
{
namespace avdecc
{
namespace entity
{
namespace model
{
std::string LA_AVDECC_CALL_CONVENTION descriptorTypeToString(DescriptorType const descriptorType) noexcept
{
	switch (descriptorType)
	{
		case DescriptorType::Entity:
			return "ENTITY";
		case DescriptorType::Configuration:
			return "CONFIGURATION";
		case DescriptorType::AudioUnit:
			return "AUDIO_UNIT";
		case DescriptorType::VideoUnit:
			return "VIDEO_UNIT";
		case DescriptorType::SensorUnit:
			return "SENSOR_UNIT";
		case DescriptorType::StreamInput:
			return "STREAM_INPUT";
		case DescriptorType::StreamOutput:
			return "STREAM_OUTPUT";
		case DescriptorType::JackInput:
			return "JACK_INPUT";
		case DescriptorType::JackOutput:
			return "JACK_OUTPUT";
		case DescriptorType::AvbInterface:
			return "AVB_INTERFACE";
		case DescriptorType::ClockSource:
			return "CLOCK_SOURCE";
		case DescriptorType::MemoryObject:
			return "MEMORY_OBJECT";
		case DescriptorType::Locale:
			return "LOCALE";
		case DescriptorType::Strings:
			return "STRINGS";
		case DescriptorType::StreamPortInput:
			return "STREAM_PORT_INPUT";
		case DescriptorType::StreamPortOutput:
			return "STREAM_PORT_OUTPUT";
		case DescriptorType::ExternalPortInput:
			return "EXTRENAL_PORT_INPUT";
		case DescriptorType::ExternalPortOutput:
			return "EXTRENAL_PORT_OUTPUT";
		case DescriptorType::InternalPortInput:
			return "INTERNAL_PORT_INPUT";
		case DescriptorType::InternalPortOutput:
			return "INTERNAL_PORT_OUTPUT";
		case DescriptorType::AudioCluster:
			return "AUDIO_CLUSTER";
		case DescriptorType::VideoCluster:
			return "VIDEO_CLUSTER";
		case DescriptorType::SensorCluster:
			return "SENSOR_CLUSTER";
		case DescriptorType::AudioMap:
			return "AUDIO_MAP";
		case DescriptorType::VideoMap:
			return "VIDEO_MAP";
		case DescriptorType::SensorMap:
			return "SENSOR_MAP";
		case DescriptorType::Control:
			return "CONTROL";
		case DescriptorType::SignalSelector:
			return "SIGNAL_SELECTOR";
		case DescriptorType::Mixer:
			return "MIXER";
		case DescriptorType::Matrix:
			return "MATRIX";
		case DescriptorType::MatrixSignal:
			return "MATRIX_SIGNAL";
		case DescriptorType::SignalSplitter:
			return "SIGNAL_SPLITTER";
		case DescriptorType::SignalCombiner:
			return "SIGNAL_COMBINER";
		case DescriptorType::SignalDemultiplexer:
			return "SIGNAL_DEMULTIPLEXER";
		case DescriptorType::SignalMultiplexer:
			return "SIGNAL_MULTIPLEXER";
		case DescriptorType::SignalTranscoder:
			return "SIGNAL_TRANSCODER";
		case DescriptorType::ClockDomain:
			return "CLOCK_DOMAIN";
		case DescriptorType::ControlBlock:
			return "CONTROL_BLOCK";
		case DescriptorType::Invalid:
			return "INVALID";
		default:
			AVDECC_ASSERT(false, "Not handled!");
			return "UNKNOWN";
	}
}

std::string LA_AVDECC_CALL_CONVENTION clockSourceTypeToString(ClockSourceType const clockSourceType) noexcept
{
	switch (clockSourceType)
	{
		case ClockSourceType::Internal:
			return "INTERNAL";
		case ClockSourceType::External:
			return "EXTERNAL";
		case ClockSourceType::InputStream:
			return "INPUT_STREAM";
		case ClockSourceType::Expansion:
			return "EXPANSION";
		default:
			AVDECC_ASSERT(false, "Not handled!");
			return "UNKNOWN";
	}
}

std::string LA_AVDECC_CALL_CONVENTION memoryObjectTypeToString(MemoryObjectType const memoryObjectType) noexcept
{
	switch (memoryObjectType)
	{
		case MemoryObjectType::FirmwareImage:
			return "FIRMWARE_IMAGE";
		case MemoryObjectType::VendorSpecific:
			return "VENDOR_SPECIFIC";
		case MemoryObjectType::CrashDump:
			return "CRASH_DUMP";
		case MemoryObjectType::LogObject:
			return "LOG_OBJECT";
		case MemoryObjectType::AutostartSettings:
			return "AUTOSTART_SETTINGS";
		case MemoryObjectType::SnapshotSettings:
			return "SNAPSHOT_SETTINGS";
		case MemoryObjectType::SvgManufacturer:
			return "SVG_MANUFACTURER";
		case MemoryObjectType::SvgEntity:
			return "SVG_ENTITY";
		case MemoryObjectType::SvgGeneric:
			return "SVG_GENERIC";
		case MemoryObjectType::PngManufacturer:
			return "PNG_MANUFACTURER";
		case MemoryObjectType::PngEntity:
			return "PNG_ENTITY";
		case MemoryObjectType::PngGeneric:
			return "PNG_GENERIC";
		case MemoryObjectType::DaeManufacturer:
			return "DAE_MANUFACTURER";
		case MemoryObjectType::DaeEntity:
			return "DAE_ENTITY";
		case MemoryObjectType::DaeGeneric:
			return "DAE_GENERIC";
		default:
			AVDECC_ASSERT(false, "Not handled!");
			return "UNKNOWN";
	}
}

std::string LA_AVDECC_CALL_CONVENTION memoryObjectOperationTypeToString(MemoryObjectOperationType const memoryObjectOperationType) noexcept
{
	switch (memoryObjectOperationType)
	{
		case MemoryObjectOperationType::Store:
			return "STORE";
		case MemoryObjectOperationType::StoreAndReboot:
			return "STORE_AND_REBOOT";
		case MemoryObjectOperationType::Read:
			return "READ";
		case MemoryObjectOperationType::Erase:
			return "ERASE";
		case MemoryObjectOperationType::Upload:
			return "UPLOAD";
		default:
			AVDECC_ASSERT(false, "Not handled!");
			return "UNKNOWN";
	}
}

std::string LA_AVDECC_CALL_CONVENTION audioClusterFormatToString(AudioClusterFormat const audioClusterFormat) noexcept
{
	switch (audioClusterFormat)
	{
		case AudioClusterFormat::Iec60958:
			return "IEC 60958";
		case AudioClusterFormat::Mbla:
			return "MBLA";
		case AudioClusterFormat::Midi:
			return "MIDI";
		case AudioClusterFormat::Smpte:
			return "SMPTE";
		default:
			AVDECC_ASSERT(false, "Not handled!");
			return "UNKNOWN";
	}
}

} // namespace model
} // namespace entity
} // namespace avdecc
} // namespace la
