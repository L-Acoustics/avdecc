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
* @file entityModelTreeStatic.hpp
* @author Christophe Calmejane
* @brief Static part of the avdecc entity model tree.
* @note This is the part of the AEM that can be preloaded using the EntityModelID.
*/

#pragma once

#include "entityModelTreeCommon.hpp"

#include <cstdint>

namespace la
{
namespace avdecc
{
namespace entity
{
namespace model
{
struct AudioUnitNodeStaticModel
{
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
	SamplingRates samplingRates{};
};

struct StreamNodeStaticModel
{
	LocalizedStringReference localizedDescription{};
	ClockDomainIndex clockDomainIndex{ ClockDomainIndex(0u) };
	entity::StreamFlags streamFlags{};
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
	StreamFormats formats{};
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
	RedundantStreams redundantStreams{};
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
};

struct JackNodeStaticModel
{
	LocalizedStringReference localizedDescription{};
	entity::JackFlags jackFlags{};
	JackType jackType{ JackType::Speaker };
	std::uint16_t numberOfControls{ 0u };
	ControlIndex baseControl{ ControlIndex(0u) };
};

struct AvbInterfaceNodeStaticModel
{
	LocalizedStringReference localizedDescription{};
	networkInterface::MacAddress macAddress{};
	entity::AvbInterfaceFlags interfaceFlags{};
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

struct ClockSourceNodeStaticModel
{
	LocalizedStringReference localizedDescription{};
	ClockSourceType clockSourceType{ ClockSourceType::Internal };
	DescriptorType clockSourceLocationType{ DescriptorType::Invalid };
	DescriptorIndex clockSourceLocationIndex{ DescriptorIndex(0u) };
};

struct MemoryObjectNodeStaticModel
{
	LocalizedStringReference localizedDescription{};
	MemoryObjectType memoryObjectType{ MemoryObjectType::FirmwareImage };
	DescriptorType targetDescriptorType{ DescriptorType::Invalid };
	DescriptorIndex targetDescriptorIndex{ DescriptorIndex(0u) };
	std::uint64_t startAddress{ 0u };
	std::uint64_t maximumLength{ 0u };
};

struct LocaleNodeStaticModel
{
	AvdeccFixedString localeID{};
	std::uint16_t numberOfStringDescriptors{ 0u };
	StringsIndex baseStringDescriptorIndex{ StringsIndex(0u) };
};

struct StringsNodeStaticModel
{
	AvdeccFixedStrings strings{};
};

struct StreamPortNodeStaticModel
{
	ClockDomainIndex clockDomainIndex{ ClockDomainIndex(0u) };
	entity::PortFlags portFlags{};
	std::uint16_t numberOfControls{ 0u };
	ControlIndex baseControl{ ControlIndex(0u) };
	std::uint16_t numberOfClusters{ 0u };
	ClusterIndex baseCluster{ ClusterIndex(0u) };
	std::uint16_t numberOfMaps{ 0u };
	MapIndex baseMap{ MapIndex(0u) };
	bool hasDynamicAudioMap{ false };
};

struct AudioClusterNodeStaticModel
{
	LocalizedStringReference localizedDescription{};
	DescriptorType signalType{ DescriptorType::Invalid };
	DescriptorIndex signalIndex{ DescriptorIndex(0u) };
	std::uint16_t signalOutput{ 0u };
	std::uint32_t pathLatency{ 0u };
	std::uint32_t blockLatency{ 0u };
	std::uint16_t channelCount{ 0u };
	AudioClusterFormat format{ AudioClusterFormat::Iec60958 };
};

struct AudioMapNodeStaticModel
{
	AudioMappings mappings{};
};

struct ControlNodeStaticModel
{
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
	ControlValues values{};
};

struct ClockDomainNodeStaticModel
{
	LocalizedStringReference localizedDescription{};
	ClockSources clockSources{};
};

struct TimingNodeStaticModel
{
	LocalizedStringReference localizedDescription{};
	TimingAlgorithm algorithm{ TimingAlgorithm::Single };
	PtpInstances ptpInstances{};
};

struct PtpInstanceNodeStaticModel
{
	LocalizedStringReference localizedDescription{};
	UniqueIdentifier clockIdentity{};
	PtpInstanceFlags flags{};
	std::uint16_t numberOfControls{ 0u };
	ControlIndex baseControl{ ControlIndex(0u) };
	std::uint16_t numberOfPtpPorts{ 0u };
	PtpPortIndex basePtpPort{ PtpPortIndex(0u) };
};

struct PtpPortNodeStaticModel
{
	LocalizedStringReference localizedDescription{};
	std::uint16_t portNumber{ 0u };
	PtpPortType portType{ PtpPortType::P2PLinkLayer };
	PtpPortFlags flags{};
	AvbInterfaceIndex avbInterfaceIndex{ AvbInterfaceIndex(0u) };
	networkInterface::MacAddress profileIdentifier{ 0u };
};

struct ConfigurationNodeStaticModel
{
	LocalizedStringReference localizedDescription{};
	DescriptorCounts descriptorCounts{};
};

struct EntityNodeStaticModel
{
	LocalizedStringReference vendorNameString{};
	LocalizedStringReference modelNameString{};
};

} // namespace model
} // namespace entity
} // namespace avdecc
} // namespace la
