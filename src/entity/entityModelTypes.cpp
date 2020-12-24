/*
* Copyright (C) 2016-2020, L-Acoustics and its contributors

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

std::string LA_AVDECC_CALL_CONVENTION jackTypeToString(JackType const jackType) noexcept
{
	switch (jackType)
	{
		case JackType::Speaker:
			return "SPEAKER";
		case JackType::Headphone:
			return "HEADPHONE";
		case JackType::AnalogMicrophone:
			return "ANALOG_MICROPHONE";
		case JackType::Spdif:
			return "SPDIF";
		case JackType::Adat:
			return "ADAT";
		case JackType::Tdif:
			return "TDIF";
		case JackType::Madi:
			return "MADI";
		case JackType::UnbalancedAnalog:
			return "UNBALANCED_ANALOG";
		case JackType::BalancedAnalog:
			return "BALANCED_ANALOG";
		case JackType::Digital:
			return "DIGITAL";
		case JackType::Midi:
			return "MIDI";
		case JackType::AesEbu:
			return "AES_EBU";
		case JackType::CompositeVideo:
			return "COMPOSITE_VIDEO";
		case JackType::SVhsVideo:
			return "S_VHS_VIDEO";
		case JackType::ComponentVideo:
			return "COMPONENT_VIDEO";
		case JackType::Dvi:
			return "DVI";
		case JackType::Hdmi:
			return "HDMI";
		case JackType::Udi:
			return "UDI";
		case JackType::DisplayPort:
			return "DISPLAYPORT";
		case JackType::Antenna:
			return "ANTENNA";
		case JackType::AnalogTuner:
			return "ANALOG_TUNER";
		case JackType::Ethernet:
			return "ETHERNET";
		case JackType::Wifi:
			return "WIFI";
		case JackType::Usb:
			return "USB";
		case JackType::Pci:
			return "PCI";
		case JackType::PciE:
			return "PCI_E";
		case JackType::Scsi:
			return "SCSI";
		case JackType::Ata:
			return "ATA";
		case JackType::Imager:
			return "IMAGER";
		case JackType::Ir:
			return "IR";
		case JackType::Thunderbolt:
			return "THUNDERBOLT";
		case JackType::Sata:
			return "SATA";
		case JackType::SmpteLtc:
			return "SMPTE_LTC";
		case JackType::DigitalMicrophone:
			return "DIGITAL_MICROPHONE";
		case JackType::AudioMediaClock:
			return "AUDIO_MEDIA_CLOCK";
		case JackType::VideoMediaClock:
			return "VIDEO_MEDIA_CLOCK";
		case JackType::GnssClock:
			return "GNSS_CLOCK";
		case JackType::Pps:
			return "PPS";
		case JackType::Expansion:
			return "EXPANSION";
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

std::string LA_AVDECC_CALL_CONVENTION controlTypeToString(ControlType const controlType) noexcept
{
	switch (controlType)
	{
		case ControlType::Enable:
			return "ENABLE";
		case ControlType::Identify:
			return "IDENTIFY";
		case ControlType::Mute:
			return "MUTE";
		case ControlType::Invert:
			return "INVERT";
		case ControlType::Gain:
			return "GAIN";
		case ControlType::Attenuate:
			return "ATTENUATE";
		case ControlType::Delay:
			return "DELAY";
		case ControlType::SrcMode:
			return "SRC MODE";
		case ControlType::Snapshot:
			return "CONTROLTYPE";
		case ControlType::PowLineFreq:
			return "POW LINE FREQ";
		case ControlType::PowerStatus:
			return "POWER STATUS";
		case ControlType::FanStatus:
			return "FAN STATUS";
		case ControlType::Temperature:
			return "TEMPERATURE";
		case ControlType::Altitude:
			return "ALTITUDE";
		case ControlType::AbsoluteHumidity:
			return "ABSOLUTE HUMIDITY";
		case ControlType::RelativeHumidity:
			return "RELATIVE HUMIDITY";
		case ControlType::Orientation:
			return "ORIENTATION";
		case ControlType::Velocity:
			return "VELOCITY";
		case ControlType::Acceleration:
			return "ACCELERATION";
		case ControlType::FilterResponse:
			return "FILTER RESPONSE";
		case ControlType::Panpot:
			return "PANPOT";
		case ControlType::Phantom:
			return "PHANTOM";
		case ControlType::AudioScale:
			return "AUDIO SCALE";
		case ControlType::AudioMeters:
			return "AUDIO METERS";
		case ControlType::AudioSpectrum:
			return "AUDIO SPECTRUM";
		case ControlType::ScanningMode:
			return "SCANNING MODE";
		case ControlType::AutoExpMode:
			return "AUTO EXP MODE";
		case ControlType::AutoExpPrio:
			return "AUTO EXP PRIO";
		case ControlType::ExpTime:
			return "EXP TIME";
		case ControlType::Focus:
			return "FOCUS";
		case ControlType::FocusAuto:
			return "FOCUS AUTO";
		case ControlType::Iris:
			return "IRIS";
		case ControlType::Zoom:
			return "ZOOM";
		case ControlType::Privacy:
			return "PRIVACY";
		case ControlType::Backlight:
			return "BACKLIGHT";
		case ControlType::Brightness:
			return "BRIGHTNESS";
		case ControlType::Contrast:
			return "CONTRAST";
		case ControlType::Hue:
			return "HUE";
		case ControlType::Saturation:
			return "SATURATION";
		case ControlType::Sharpness:
			return "SHARPNESS";
		case ControlType::Gamma:
			return "GAMMA";
		case ControlType::WhiteBalTemp:
			return "WHITE BAL TEMP";
		case ControlType::WhiteBalTempAuto:
			return "WHITE BAL TEMP AUTO";
		case ControlType::WhiteBalComp:
			return "WHITE BAL COMP";
		case ControlType::WhiteBalCompAuto:
			return "WHITE BAL COMP AUTO";
		case ControlType::DigitalZoom:
			return "DIGITAL ZOOM";
		case ControlType::MediaPlaylist:
			return "MEDIA PLAYLIST";
		case ControlType::MediaPlaylistName:
			return "MEDIA PLAYLIST NAME";
		case ControlType::MediaDisk:
			return "MEDIA DISK";
		case ControlType::MediaDiskName:
			return "MEDIA DISK NAME";
		case ControlType::MediaTrack:
			return "MEDIA TRACK";
		case ControlType::MediaTrackName:
			return "MEDIA TRACK NAME";
		case ControlType::MediaSpeed:
			return "MEDIA SPEED";
		case ControlType::MediaSamplePosition:
			return "MEDIA SAMPLE POSITION";
		case ControlType::MediaPlaybackTransport:
			return "MEDIA PLAYBACK TRANSPORT";
		case ControlType::MediaRecordTransport:
			return "MEDIA RECORD TRANSPORT";
		case ControlType::Frequency:
			return "FREQUENCY";
		case ControlType::Modulation:
			return "MODULATION";
		case ControlType::Polarization:
			return "POLARIZATION";
		default:
			AVDECC_ASSERT(false, "Not handled!");
			return "UNKNOWN";
	}
}

} // namespace model
} // namespace entity
} // namespace avdecc
} // namespace la
