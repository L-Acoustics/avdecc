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
* @file entityModel.hpp
* @author Christophe Calmejane
* @brief Avdecc entity model.
*/

#pragma once

#include "la/avdecc/utils.hpp"

#include "entityEnums.hpp"
#include "uniqueIdentifier.hpp"
#include "protocolDefines.hpp"
#include "entityModelTypes.hpp"

#include <la/networkInterfaceHelper/networkInterfaceHelper.hpp>

#include <cstdint>
#include <array>
#include <unordered_map>
#include <set>
#include <tuple>
#include <vector>
#include <optional>

namespace la
{
namespace avdecc
{
namespace entity
{
namespace model
{
/** ENTITY Descriptor - IEEE1722.1-2013 Clause 7.2.1 */
struct EntityDescriptor
{
	UniqueIdentifier entityID{};
	UniqueIdentifier entityModelID{};
	EntityCapabilities entityCapabilities{};
	std::uint16_t talkerStreamSources{ 0u };
	TalkerCapabilities talkerCapabilities{};
	std::uint16_t listenerStreamSinks{ 0u };
	ListenerCapabilities listenerCapabilities{};
	ControllerCapabilities controllerCapabilities{};
	std::uint32_t availableIndex{ 0u };
	UniqueIdentifier associationID{};
	AvdeccFixedString entityName{};
	LocalizedStringReference vendorNameString{};
	LocalizedStringReference modelNameString{};
	AvdeccFixedString firmwareVersion{};
	AvdeccFixedString groupName{};
	AvdeccFixedString serialNumber{};
	std::uint16_t configurationsCount{ 0u };
	std::uint16_t currentConfiguration{ 0u };
};

/** CONFIGURATION Descriptor - IEEE1722.1-2013 Clause 7.2.2 */
struct ConfigurationDescriptor
{
	AvdeccFixedString objectName{};
	LocalizedStringReference localizedDescription{};
	std::unordered_map<DescriptorType, std::uint16_t, utils::EnumClassHash> descriptorCounts{};
};

/** AUDIO_UNIT Descriptor - IEEE1722.1-2013 Clause 7.2.3 */
struct AudioUnitDescriptor
{
	AvdeccFixedString objectName{};
	LocalizedStringReference localizedDescription{};
	ClockDomainIndex clockDomainIndex{ 0u };
	std::uint16_t numberOfStreamInputPorts{ 0u };
	StreamPortIndex baseStreamInputPort{ StreamPortIndex(0u) };
	std::uint16_t numberOfStreamOutputPorts{ 0u };
	StreamPortIndex baseStreamOutputPort{ StreamPortIndex(0u) };
	std::uint16_t numberOfExternalInputPorts{ 0u };
	ExternalPortIndex baseExternalInputPort{ ExternalPortIndex(0u) };
	std::uint16_t numberOfExternalOutputPorts{ 0u };
	ExternalPortIndex baseExternalOutputPort{ ExternalPortIndex(0u) };
	std::uint16_t numberOfInternalInputPorts{ 0u };
	InternalPortIndex baseInternalInputPort{ InternalPortIndex(0u) };
	std::uint16_t numberOfInternalOutputPorts{ 0u };
	InternalPortIndex baseInternalOutputPort{ InternalPortIndex(0u) };
	std::uint16_t numberOfControls{ 0u };
	ControlIndex baseControl{ ControlIndex(0u) };
	std::uint16_t numberOfSignalSelectors{ 0u };
	SignalSelectorIndex baseSignalSelector{ SignalSelectorIndex(0u) };
	std::uint16_t numberOfMixers{ 0u };
	MixerIndex baseMixer{ MixerIndex(0u) };
	std::uint16_t numberOfMatrices{ 0u };
	MatrixIndex baseMatrix{ MatrixIndex(0u) };
	std::uint16_t numberOfSplitters{ 0u };
	SignalSplitterIndex baseSplitter{ SignalSplitterIndex(0u) };
	std::uint16_t numberOfCombiners{ 0u };
	SignalCombinerIndex baseCombiner{ SignalCombinerIndex(0u) };
	std::uint16_t numberOfDemultiplexers{ 0u };
	SignalDemultiplexerIndex baseDemultiplexer{ SignalDemultiplexerIndex(0u) };
	std::uint16_t numberOfMultiplexers{ 0u };
	SignalMultiplexerIndex baseMultiplexer{ SignalMultiplexerIndex(0u) };
	std::uint16_t numberOfTranscoders{ 0u };
	SignalTranscoderIndex baseTranscoder{ SignalTranscoderIndex(0u) };
	std::uint16_t numberOfControlBlocks{ 0u };
	ControlBlockIndex baseControlBlock{ ControlBlockIndex(0u) };
	SamplingRate currentSamplingRate{};
	std::set<SamplingRate> samplingRates{};
};

/** VIDEO_UNIT Descriptor - IEEE1722.1-2013 Clause 7.2.4 */

/** SENSOR_UNIT Descriptor - IEEE1722.1-2013 Clause 7.2.5 */

/** STREAM_INPUT and STREAM_OUTPUT Descriptor - IEEE1722.1-2013 Clause 7.2.6 */
struct StreamDescriptor
{
	AvdeccFixedString objectName{};
	LocalizedStringReference localizedDescription{};
	ClockDomainIndex clockDomainIndex{ ClockDomainIndex(0u) };
	StreamFlags streamFlags{};
	StreamFormat currentFormat{};
	UniqueIdentifier backupTalkerEntityID_0{};
	std::uint16_t backupTalkerUniqueID_0{ 0u };
	UniqueIdentifier backupTalkerEntityID_1{};
	std::uint16_t backupTalkerUniqueID_1{ 0u };
	UniqueIdentifier backupTalkerEntityID_2{};
	std::uint16_t backupTalkerUniqueID_2{ 0u };
	UniqueIdentifier backedupTalkerEntityID{};
	std::uint16_t backedupTalkerUnique{ 0u };
	AvbInterfaceIndex avbInterfaceIndex{ AvbInterfaceIndex(0u) };
	std::uint32_t bufferLength{ 0u };
	std::set<StreamFormat> formats{};
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
	std::set<StreamIndex> redundantStreams{};
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
};

/** JACK_INPUT and JACK_OUTPUT Descriptor - IEEE1722.1-2013 Clause 7.2.7 */
struct JackDescriptor
{
	AvdeccFixedString objectName{};
	LocalizedStringReference localizedDescription{};
	JackFlags jackFlags{};
	JackType jackType{ JackType::Speaker };
	std::uint16_t numberOfControls{ 0u };
	ControlIndex baseControl{ ControlIndex(0u) };
};

/** AVB_INTERFACE Descriptor - IEEE1722.1-2013 Clause 7.2.8 */
struct AvbInterfaceDescriptor
{
	AvdeccFixedString objectName{};
	LocalizedStringReference localizedDescription{};
	networkInterface::MacAddress macAddress{};
	AvbInterfaceFlags interfaceFlags{};
	UniqueIdentifier clockIdentity{};
	std::uint8_t priority1{ 0xff };
	std::uint8_t clockClass{ 0xff };
	std::uint16_t offsetScaledLogVariance{ 0x0000 };
	std::uint8_t clockAccuracy{ 0xff };
	std::uint8_t priority2{ 0xff };
	std::uint8_t domainNumber{ 0u };
	std::uint8_t logSyncInterval{ 0u };
	std::uint8_t logAnnounceInterval{ 0u };
	std::uint8_t logPDelayInterval{ 0u };
	std::uint16_t portNumber{ 0x0000 };
};

/** CLOCK_SOURCE Descriptor - IEEE1722.1-2013 Clause 7.2.9 */
struct ClockSourceDescriptor
{
	AvdeccFixedString objectName{};
	LocalizedStringReference localizedDescription{};
	ClockSourceFlags clockSourceFlags{};
	ClockSourceType clockSourceType{ ClockSourceType::Internal };
	UniqueIdentifier clockSourceIdentifier{};
	DescriptorType clockSourceLocationType{ DescriptorType::Invalid };
	DescriptorIndex clockSourceLocationIndex{ DescriptorIndex(0u) };
};

/** MEMORY_OBJECT Descriptor - IEEE1722.1-2013 Clause 7.2.10 */
struct MemoryObjectDescriptor
{
	AvdeccFixedString objectName{};
	LocalizedStringReference localizedDescription{};
	MemoryObjectType memoryObjectType{ MemoryObjectType::FirmwareImage };
	DescriptorType targetDescriptorType{ DescriptorType::Invalid };
	DescriptorIndex targetDescriptorIndex{ DescriptorIndex(0u) };
	std::uint64_t startAddress{ 0u };
	std::uint64_t maximumLength{ 0u };
	std::uint64_t length{ 0u };
};

/** LOCALE Descriptor - IEEE1722.1-2013 Clause 7.2.11 */
struct LocaleDescriptor
{
	AvdeccFixedString localeID{};
	std::uint16_t numberOfStringDescriptors{ 0u };
	StringsIndex baseStringDescriptorIndex{ StringsIndex(0u) };
};

/** STRINGS Descriptor - IEEE1722.1-2013 Clause 7.2.12 */
struct StringsDescriptor
{
	std::array<AvdeccFixedString, 7> strings{};
};

/** STREAM_PORT Descriptor - IEEE1722.1-2013 Clause 7.2.13 */
struct StreamPortDescriptor
{
	ClockDomainIndex clockDomainIndex{ ClockDomainIndex(0u) };
	PortFlags portFlags{};
	std::uint16_t numberOfControls{ 0u };
	ControlIndex baseControl{ ControlIndex(0u) };
	std::uint16_t numberOfClusters{ 0u };
	ClusterIndex baseCluster{ ClusterIndex(0u) };
	std::uint16_t numberOfMaps{ 0u };
	MapIndex baseMap{ MapIndex(0u) };
};

/** EXTERNAL_PORT Descriptor - IEEE1722.1-2013 Clause 7.2.14 */
struct ExternalPortDescriptor
{
	ClockDomainIndex clockDomainIndex{ ClockDomainIndex(0u) };
	PortFlags portFlags{};
	std::uint16_t numberOfControls{ 0u };
	ControlIndex baseControl{ ControlIndex(0u) };
	DescriptorType signalType{ DescriptorType::Invalid };
	DescriptorIndex signalIndex{ DescriptorIndex(0u) };
	std::uint16_t signalOutput{ 0u };
	std::uint32_t blockLatency{ 0u };
	JackIndex jackIndex{ JackIndex(0u) };
};

/** INTERNAL_PORT Descriptor - IEEE1722.1-2013 Clause 7.2.15 */
struct InternalPortDescriptor
{
	ClockDomainIndex clockDomainIndex{ ClockDomainIndex(0u) };
	PortFlags portFlags{};
	std::uint16_t numberOfControls{ 0u };
	ControlIndex baseControl{ ControlIndex(0u) };
	DescriptorType signalType{ DescriptorType::Invalid };
	DescriptorIndex signalIndex{ DescriptorIndex(0u) };
	std::uint16_t signalOutput{ 0u };
	std::uint32_t blockLatency{ 0u };
	InternalPortIndex internalIndex{ InternalPortIndex(0u) };
};

/** AUDIO_CLUSTER Descriptor - IEEE1722.1-2013 Clause 7.2.16 */
struct AudioClusterDescriptor
{
	AvdeccFixedString objectName{};
	LocalizedStringReference localizedDescription{};
	DescriptorType signalType{ DescriptorType::Invalid };
	DescriptorIndex signalIndex{ DescriptorIndex(0u) };
	std::uint16_t signalOutput{ 0u };
	std::uint32_t pathLatency{ 0u };
	std::uint32_t blockLatency{ 0u };
	std::uint16_t channelCount{ 0u };
	AudioClusterFormat format{ AudioClusterFormat::Iec60958 };
};

/** VIDEO_CLUSTER Descriptor - IEEE1722.1-2013 Clause 7.2.17 */

/** SENSOR_CLUSTER Descriptor - IEEE1722.1-2013 Clause 7.2.18 */

/** AUDIO_MAP Descriptor - IEEE1722.1-2013 Clause 7.2.19 */
struct AudioMapDescriptor
{
	AudioMappings mappings{};
};

/** VIDEO_MAP Descriptor - IEEE1722.1-2013 Clause 7.2.20 */

/** SENSOR_MAP Descriptor - IEEE1722.1-2013 Clause 7.2.21 */

/** CONTROL Descriptor - IEEE1722.1-2013 Clause 7.2.22 */
struct ControlDescriptor
{
	AvdeccFixedString objectName{};
	LocalizedStringReference localizedDescription{};
	std::uint32_t blockLatency{ 0u };
	std::uint32_t controlLatency{ 0u };
	std::uint16_t controlDomain{ 0u };
	UniqueIdentifier controlType{};
	std::uint32_t resetTime{ 0u };
	DescriptorType signalType{ DescriptorType::Invalid };
	DescriptorIndex signalIndex{ DescriptorIndex(0u) };
	std::uint16_t signalOutput{ 0u };
	ControlValueType controlValueType{};
	std::uint16_t numberOfValues{ 0u };
	ControlValues valuesStatic{};
	ControlValues valuesDynamic{};
};

/** SIGNAL_SELECTOR Descriptor - IEEE1722.1-2013 Clause 7.2.23 */

/** MIXER Descriptor - IEEE1722.1-2013 Clause 7.2.24 */

/** MATRIX Descriptor - IEEE1722.1-2013 Clause 7.2.25 */

/** MATRIX_SIGNAL Descriptor - IEEE1722.1-2013 Clause 7.2.26 */

/** SIGNAL_SPLITTER Descriptor - IEEE1722.1-2013 Clause 7.2.27 */

/** SIGNAL_COMBINER Descriptor - IEEE1722.1-2013 Clause 7.2.28 */

/** SIGNAL_DEMULTIPLEXER Descriptor - IEEE1722.1-2013 Clause 7.2.29 */

/** SIGNAL_MULTIPLEXER Descriptor - IEEE1722.1-2013 Clause 7.2.30 */

/** SIGNAL_TRANSCODER Descriptor - IEEE1722.1-2013 Clause 7.2.31 */

/** CLOCK_DOMAIN Descriptor - IEEE1722.1-2013 Clause 7.2.32 */
struct ClockDomainDescriptor
{
	AvdeccFixedString objectName{};
	LocalizedStringReference localizedDescription{};
	ClockSourceIndex clockSourceIndex{ ClockSourceIndex(0u) };
	std::vector<ClockSourceIndex> clockSources{};
};

/** CONTROL_BLOCK Descriptor - IEEE1722.1-2013 Clause 7.2.33 */

/** TIMING Descriptor - IEEE1722.1-2021 Clause 7.2.34 */
struct TimingDescriptor
{
	AvdeccFixedString objectName{};
	LocalizedStringReference localizedDescription{};
	TimingAlgorithm algorithm{ TimingAlgorithm::Single };
	std::vector<PtpInstanceIndex> ptpInstances{};
};

/** PTP_INSTANCE Descriptor - IEEE1722.1-2021 Clause 7.2.35 */
struct PtpInstanceDescriptor
{
	AvdeccFixedString objectName{};
	LocalizedStringReference localizedDescription{};
	UniqueIdentifier clockIdentity{};
	PtpInstanceFlags flags{};
	std::uint16_t numberOfControls{ 0u };
	ControlIndex baseControl{ ControlIndex(0u) };
	std::uint16_t numberOfPtpPorts{ 0u };
	PtpPortIndex basePtpPort{ PtpPortIndex(0u) };
};

/** PTP_PORT Descriptor - IEEE1722.1-2021 Clause 7.2.36 */
struct PtpPortDescriptor
{
	AvdeccFixedString objectName{};
	LocalizedStringReference localizedDescription{};
	std::uint16_t portNumber{ 0u };
	PtpPortType portType{ PtpPortType::P2PLinkLayer };
	PtpPortFlags flags{};
	AvbInterfaceIndex avbInterfaceIndex{ AvbInterfaceIndex(0u) };
	networkInterface::MacAddress profileIdentifier{};
};

/** GET_STREAM_INFO and SET_STREAM_INFO Dynamic Information - IEEE1722.1-2013 Clause 7.4.16.2 */
struct StreamInfo
{
	StreamInfoFlags streamInfoFlags{};
	StreamFormat streamFormat{};
	UniqueIdentifier streamID{ 0u };
	std::uint32_t msrpAccumulatedLatency{ 0u };
	networkInterface::MacAddress streamDestMac{};
	MsrpFailureCode msrpFailureCode{ MsrpFailureCode::NoFailure };
	BridgeIdentifier msrpFailureBridgeID{ 0u };
	std::uint16_t streamVlanID{ 0u };
	// Milan additions
	std::optional<StreamInfoFlagsEx> streamInfoFlagsEx{ std::nullopt };
	std::optional<ProbingStatus> probingStatus{ std::nullopt };
	std::optional<protocol::AcmpStatus> acmpStatus{ std::nullopt };
};

constexpr bool operator==(StreamInfo const& lhs, StreamInfo const& rhs) noexcept
{
	return (lhs.streamInfoFlags == rhs.streamInfoFlags) && (lhs.streamFormat == rhs.streamFormat) && (lhs.streamID == rhs.streamID) && (lhs.msrpAccumulatedLatency == rhs.msrpAccumulatedLatency) && (lhs.streamDestMac == rhs.streamDestMac) && (lhs.msrpFailureCode == rhs.msrpFailureCode) && (lhs.msrpFailureBridgeID == rhs.msrpFailureBridgeID) && (lhs.streamVlanID == rhs.streamVlanID) && (lhs.streamInfoFlagsEx == rhs.streamInfoFlagsEx) && (lhs.probingStatus == rhs.probingStatus) && (lhs.acmpStatus == rhs.acmpStatus);
}

constexpr bool operator!=(StreamInfo const& lhs, StreamInfo const& rhs) noexcept
{
	return !(lhs == rhs);
}

/** GET_AVB_INFO Dynamic Information - IEEE1722.1-2013 Clause 7.4.40.2 */
struct AvbInfo
{
	UniqueIdentifier gptpGrandmasterID{};
	std::uint32_t propagationDelay{ 0u };
	std::uint8_t gptpDomainNumber{ 0u };
	AvbInfoFlags flags{};
	entity::model::MsrpMappings mappings{};
};

constexpr bool operator==(AvbInfo const& lhs, AvbInfo const& rhs) noexcept
{
	return (lhs.gptpGrandmasterID == rhs.gptpGrandmasterID) && (lhs.propagationDelay == rhs.propagationDelay) && (lhs.gptpDomainNumber == rhs.gptpDomainNumber) && (lhs.flags == rhs.flags) && (lhs.mappings == rhs.mappings);
}

constexpr bool operator!=(AvbInfo const& lhs, AvbInfo const& rhs) noexcept
{
	return !(lhs == rhs);
}

/** GET_AS_PATH Dynamic Information - IEEE1722.1-2013 Clause 7.4.41.2 */
struct AsPath
{
	entity::model::PathSequence sequence{};
};

inline bool operator==(AsPath const& lhs, AsPath const& rhs) noexcept
{
	return lhs.sequence == rhs.sequence;
}

inline bool operator!=(AsPath const& lhs, AsPath const& rhs) noexcept
{
	return !(lhs == rhs);
}

/** GET_MILAN_INFO - Milan-2019 Clause 7.4.1 */
struct MilanInfo
{
	std::uint32_t protocolVersion{ 0u };
	MilanInfoFeaturesFlags featuresFlags{};
	std::uint32_t certificationVersion{ 0u };
};

inline bool operator==(MilanInfo const& lhs, MilanInfo const& rhs) noexcept
{
	return (lhs.protocolVersion == rhs.protocolVersion) && (lhs.featuresFlags == rhs.featuresFlags) && (lhs.certificationVersion == rhs.certificationVersion);
}

inline bool operator!=(MilanInfo const& lhs, MilanInfo const& rhs) noexcept
{
	return !(lhs == rhs);
}


/**
* @brief Make a UniqueIdentifier from vendorID, deviceID and modelID.
* @details Helper method to construct a UniqueIdentifier from vendorID, deviceID and modelID to be used as EntityModelID.
* @param[in] vendorID OUI-24 of the vendor (8 MSBs should be 0, ignored regardless).
* @param[in] deviceID ID of the device (vendor specific).
* @param[in] modelID ID of the model (vendor specific).
* @return Valid UniqueIdentifier that can be used as EntityModelID in ADP messages and EntityDescriptor.
* @note This method is provided as a helper. Packing an EntityModelID that way is NOT mandatory (except for the vendorID).
* @warning This method is intended to be used for an OUI-24, not an OUI-36.
*/
inline UniqueIdentifier makeEntityModelID(std::uint32_t const vendorID, std::uint8_t const deviceID, std::uint32_t const modelID) noexcept
{
	return UniqueIdentifier{ (static_cast<UniqueIdentifier::value_type>(vendorID) << 40) + (static_cast<UniqueIdentifier::value_type>(deviceID) << 32) + static_cast<UniqueIdentifier::value_type>(modelID) };
}

/**
* @brief Split a UniqueIdentifier representing an EntityModelID into vendorID, deviceID and modelID.
* @details Helper method to split a UniqueIdentifier representing an EntityModelID into vendorID, deviceID and modelID.
* @param[in] entityModelID The UniqueIdentifier representing an EntityModelID.
* @return Tuple of vendorID (OUI-24), deviceID and modelID.
* @note This method is provided as a helper. Packing an EntityModelID that way is NOT mandatory (except for the vendorID).
* @warning This method is intended to be used for an OUI-24, not an OUI-36.
*/
inline std::tuple<std::uint32_t, std::uint8_t, std::uint32_t> splitEntityModelID(UniqueIdentifier const entityModelID) noexcept
{
	auto const value = entityModelID.getValue();
	return std::make_tuple(static_cast<std::uint32_t>((value >> 40) & 0x0000000000FFFFFF), static_cast<std::uint8_t>((value >> 32) & 0x00000000000000FF), static_cast<std::uint32_t>(value & 0x00000000FFFFFFFF));
}

} // namespace model
} // namespace entity
} // namespace avdecc
} // namespace la
