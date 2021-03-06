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
			return "SNAPSHOT";
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

std::string LA_AVDECC_CALL_CONVENTION controlValueUnitToString(ControlValueUnit::Unit const controlValueUnit) noexcept
{
	switch (controlValueUnit)
	{
		case ControlValueUnit::Unit::Unitless:
			return "UNITLESS";
		case ControlValueUnit::Unit::Count:
			return "COUNT";
		case ControlValueUnit::Unit::Percent:
			return "PERCENT";
		case ControlValueUnit::Unit::FStop:
			return "FSTOP";
		case ControlValueUnit::Unit::Seconds:
			return "SECONDS";
		case ControlValueUnit::Unit::Minutes:
			return "MINUTES";
		case ControlValueUnit::Unit::Hours:
			return "HOURS";
		case ControlValueUnit::Unit::Days:
			return "DAYS";
		case ControlValueUnit::Unit::Months:
			return "MONTHS";
		case ControlValueUnit::Unit::Years:
			return "YEARS";
		case ControlValueUnit::Unit::Samples:
			return "SAMPLES";
		case ControlValueUnit::Unit::Frames:
			return "FRAMES";
		case ControlValueUnit::Unit::Hertz:
			return "HERTZ";
		case ControlValueUnit::Unit::Semitones:
			return "SEMITONES";
		case ControlValueUnit::Unit::Cents:
			return "CENTS";
		case ControlValueUnit::Unit::Octaves:
			return "OCTAVES";
		case ControlValueUnit::Unit::Fps:
			return "FPS";
		case ControlValueUnit::Unit::Metres:
			return "METRES";
		case ControlValueUnit::Unit::Kelvin:
			return "KELVIN";
		case ControlValueUnit::Unit::Grams:
			return "GRAMS";
		case ControlValueUnit::Unit::Volts:
			return "VOLTS";
		case ControlValueUnit::Unit::Dbv:
			return "DBV";
		case ControlValueUnit::Unit::Dbu:
			return "DBU";
		case ControlValueUnit::Unit::Amps:
			return "AMPS";
		case ControlValueUnit::Unit::Watts:
			return "WATTS";
		case ControlValueUnit::Unit::Dbm:
			return "DBM";
		case ControlValueUnit::Unit::Dbw:
			return "DBW";
		case ControlValueUnit::Unit::Pascals:
			return "PASCALS";
		case ControlValueUnit::Unit::Bits:
			return "BITS";
		case ControlValueUnit::Unit::Bytes:
			return "BYTES";
		case ControlValueUnit::Unit::KibiBytes:
			return "KIBIBYTES";
		case ControlValueUnit::Unit::MebiBytes:
			return "MEBIBYTES";
		case ControlValueUnit::Unit::GibiBytes:
			return "GIBIBYTES";
		case ControlValueUnit::Unit::TebiBytes:
			return "TEBIBYTES";
		case ControlValueUnit::Unit::BitsPerSec:
			return "BITS_PER_SEC";
		case ControlValueUnit::Unit::BytesPerSec:
			return "BYTES_PER_SEC";
		case ControlValueUnit::Unit::KibiBytesPerSec:
			return "KIBIBYTES_PER_SEC";
		case ControlValueUnit::Unit::MebiBytesPerSec:
			return "MEBIBYTES_PER_SEC";
		case ControlValueUnit::Unit::GibiBytesPerSec:
			return "GIBIBYTES_PER_SEC";
		case ControlValueUnit::Unit::TebiBytesPerSec:
			return "TEBIBYTES_PER_SEC";
		case ControlValueUnit::Unit::Candelas:
			return "CANDELAS";
		case ControlValueUnit::Unit::Joules:
			return "JOULES";
		case ControlValueUnit::Unit::Radians:
			return "RADIANS";
		case ControlValueUnit::Unit::Newtons:
			return "NEWTONS";
		case ControlValueUnit::Unit::Ohms:
			return "OHMS";
		case ControlValueUnit::Unit::MetresPerSec:
			return "METRES_PER_SEC";
		case ControlValueUnit::Unit::RadiansPerSec:
			return "RADIANS_PER_SEC";
		case ControlValueUnit::Unit::MetresPerSecSquared:
			return "METRES_PER_SEC_SQUARED";
		case ControlValueUnit::Unit::RadiansPerSecSquared:
			return "RADIANS_PER_SEC_SQUARED";
		case ControlValueUnit::Unit::Teslas:
			return "TESLAS";
		case ControlValueUnit::Unit::Webers:
			return "WEBERS";
		case ControlValueUnit::Unit::AmpsPerMetre:
			return "AMPS_PER_METRE";
		case ControlValueUnit::Unit::MetresSquared:
			return "METRES_SQUARED";
		case ControlValueUnit::Unit::MetresCubed:
			return "METRES_CUBED";
		case ControlValueUnit::Unit::Litres:
			return "LITRES";
		case ControlValueUnit::Unit::Db:
			return "DB";
		case ControlValueUnit::Unit::DbPeak:
			return "DB_PEAK";
		case ControlValueUnit::Unit::DbRms:
			return "DB_RMS";
		case ControlValueUnit::Unit::Dbfs:
			return "DBFS";
		case ControlValueUnit::Unit::DbfsPeak:
			return "DBFS_PEAK";
		case ControlValueUnit::Unit::DbfsRms:
			return "DBFS_RMS";
		case ControlValueUnit::Unit::Dbtp:
			return "DBTP";
		case ControlValueUnit::Unit::DbSplA:
			return "DB_SPL_A";
		case ControlValueUnit::Unit::DbZ:
			return "DB_Z";
		case ControlValueUnit::Unit::DbSplC:
			return "DB_SPL_C";
		case ControlValueUnit::Unit::DbSpl:
			return "DB_SPL";
		case ControlValueUnit::Unit::Lu:
			return "LU";
		case ControlValueUnit::Unit::Lufs:
			return "LUFS";
		case ControlValueUnit::Unit::DbA:
			return "DB_A";
		default:
			AVDECC_ASSERT(false, "Not handled!");
			return "UNKNOWN";
	}
}

std::string LA_AVDECC_CALL_CONVENTION controlValueTypeToString(ControlValueType::Type const controlValueType) noexcept
{
	switch (controlValueType)
	{
		case ControlValueType::Type::ControlLinearInt8:
			return "CONTROL_LINEAR_INT8";
		case ControlValueType::Type::ControlLinearUInt8:
			return "CONTROL_LINEAR_UINT8";
		case ControlValueType::Type::ControlLinearInt16:
			return "CONTROL_LINEAR_INT16";
		case ControlValueType::Type::ControlLinearUInt16:
			return "CONTROL_LINEAR_UINT16";
		case ControlValueType::Type::ControlLinearInt32:
			return "CONTROL_LINEAR_INT32";
		case ControlValueType::Type::ControlLinearUInt32:
			return "CONTROL_LINEAR_UINT32";
		case ControlValueType::Type::ControlLinearInt64:
			return "CONTROL_LINEAR_INT64";
		case ControlValueType::Type::ControlLinearUInt64:
			return "CONTROL_LINEAR_UINT64";
		case ControlValueType::Type::ControlLinearFloat:
			return "CONTROL_LINEAR_FLOAT";
		case ControlValueType::Type::ControlLinearDouble:
			return "CONTROL_LINEAR_DOUBLE";
		case ControlValueType::Type::ControlSelectorInt8:
			return "CONTROL_SELECTOR_INT8";
		case ControlValueType::Type::ControlSelectorUInt8:
			return "CONTROL_SELECTOR_UINT8";
		case ControlValueType::Type::ControlSelectorInt16:
			return "CONTROL_SELECTOR_INT16";
		case ControlValueType::Type::ControlSelectorUInt16:
			return "CONTROL_SELECTOR_UINT16";
		case ControlValueType::Type::ControlSelectorInt32:
			return "CONTROL_SELECTOR_INT32";
		case ControlValueType::Type::ControlSelectorUInt32:
			return "CONTROL_SELECTOR_UINT32";
		case ControlValueType::Type::ControlSelectorInt64:
			return "CONTROL_SELECTOR_INT64";
		case ControlValueType::Type::ControlSelectorUInt64:
			return "CONTROL_SELECTOR_UINT64";
		case ControlValueType::Type::ControlSelectorFloat:
			return "CONTROL_SELECTOR_FLOAT";
		case ControlValueType::Type::ControlSelectorDouble:
			return "CONTROL_SELECTOR_DOUBLE";
		case ControlValueType::Type::ControlSelectorString:
			return "CONTROL_SELECTOR_STRING";
		case ControlValueType::Type::ControlArrayInt8:
			return "CONTROL_ARRAY_INT8";
		case ControlValueType::Type::ControlArrayUInt8:
			return "CONTROL_ARRAY_UINT8";
		case ControlValueType::Type::ControlArrayInt16:
			return "CONTROL_ARRAY_INT16";
		case ControlValueType::Type::ControlArrayUInt16:
			return "CONTROL_ARRAY_UINT16";
		case ControlValueType::Type::ControlArrayInt32:
			return "CONTROL_ARRAY_INT32";
		case ControlValueType::Type::ControlArrayUInt32:
			return "CONTROL_ARRAY_UINT32";
		case ControlValueType::Type::ControlArrayInt64:
			return "CONTROL_ARRAY_INT64";
		case ControlValueType::Type::ControlArrayUInt64:
			return "CONTROL_ARRAY_UINT64";
		case ControlValueType::Type::ControlArrayFloat:
			return "CONTROL_ARRAY_FLOAT";
		case ControlValueType::Type::ControlArrayDouble:
			return "CONTROL_ARRAY_DOUBLE";
		case ControlValueType::Type::ControlUtf8:
			return "CONTROL_UTF8";
		case ControlValueType::Type::ControlBodePlot:
			return "CONTROL_BODE_PLOT";
		case ControlValueType::Type::ControlSmpteTime:
			return "CONTROL_SMPTE_TIME";
		case ControlValueType::Type::ControlSampleRate:
			return "CONTROL_SAMPLE_RATE";
		case ControlValueType::Type::ControlGptpTime:
			return "CONTROL_GPTP_TIME";
		case ControlValueType::Type::ControlVendor:
			return "CONTROL_VENDOR";
		case ControlValueType::Type::Expansion:
			return "EXPANSION";
		default:
			AVDECC_ASSERT(false, "Not handled!");
			return "UNKNOWN";
	}
}

} // namespace model
} // namespace entity
} // namespace avdecc
} // namespace la
