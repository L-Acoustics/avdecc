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
* @file avdeccControlledEntityStaticModel.hpp
* @author Christophe Calmejane
* @brief Static part of the avdecc entity model for a #la::avdecc::controller::ControlledEntity.
* @note This is the part of the AEM that can be preloaded using the EntityModelID.
*/

#pragma once

#include <la/avdecc/avdecc.hpp>
#include "avdeccControlledEntityCommonModel.hpp"
#include <cstdint>

namespace la
{
namespace avdecc
{
namespace controller
{
namespace model
{

struct AudioUnitNodeStaticModel
{
	entity::model::LocalizedStringReference localizedDescription{ entity::model::getNullLocalizedStringReference() };
	entity::model::ClockDomainIndex clockDomainIndex{ 0u };
	std::uint16_t numberOfStreamInputPorts{ 0u };
	entity::model::StreamPortIndex baseStreamInputPort{ entity::model::StreamPortIndex(0u) };
	std::uint16_t numberOfStreamOutputPorts{ 0u };
	entity::model::StreamPortIndex baseStreamOutputPort{ entity::model::StreamPortIndex(0u) };
	std::uint16_t numberOfExternalInputPorts{ 0u };
	entity::model::ExternalPortIndex baseExternalInputPort{ entity::model::ExternalPortIndex(0u) };
	std::uint16_t numberOfExternalOutputPorts{ 0u };
	entity::model::ExternalPortIndex baseExternalOutputPort{ entity::model::ExternalPortIndex(0u) };
	std::uint16_t numberOfInternalInputPorts{ 0u };
	entity::model::InternalPortIndex baseInternalInputPort{ entity::model::InternalPortIndex(0u) };
	std::uint16_t numberOfInternalOutputPorts{ 0u };
	entity::model::InternalPortIndex baseInternalOutputPort{ entity::model::InternalPortIndex(0u) };
	std::uint16_t numberOfControls{ 0u };
	entity::model::ControlIndex baseControl{ entity::model::ControlIndex(0u) };
	std::uint16_t numberOfSignalSelectors{ 0u };
	entity::model::SignalSelectorIndex baseSignalSelector{ entity::model::SignalSelectorIndex(0u) };
	std::uint16_t numberOfMixers{ 0u };
	entity::model::MixerIndex baseMixer{ entity::model::MixerIndex(0u) };
	std::uint16_t numberOfMatrices{ 0u };
	entity::model::MatrixIndex baseMatrix{ entity::model::MatrixIndex(0u) };
	std::uint16_t numberOfSplitters{ 0u };
	entity::model::SignalSplitterIndex baseSplitter{ entity::model::SignalSplitterIndex(0u) };
	std::uint16_t numberOfCombiners{ 0u };
	entity::model::SignalCombinerIndex baseCombiner{ entity::model::SignalCombinerIndex(0u) };
	std::uint16_t numberOfDemultiplexers{ 0u };
	entity::model::SignalDemultiplexerIndex baseDemultiplexer{ entity::model::SignalDemultiplexerIndex(0u) };
	std::uint16_t numberOfMultiplexers{ 0u };
	entity::model::SignalMultiplexerIndex baseMultiplexer{ entity::model::SignalMultiplexerIndex(0u) };
	std::uint16_t numberOfTranscoders{ 0u };
	entity::model::SignalTranscoderIndex baseTranscoder{ entity::model::SignalTranscoderIndex(0u) };
	std::uint16_t numberOfControlBlocks{ 0u };
	entity::model::ControlBlockIndex baseControlBlock{ entity::model::ControlBlockIndex(0u) };
	SamplingRates samplingRates{};
};

struct StreamNodeStaticModel
{
	entity::model::LocalizedStringReference localizedDescription{ entity::model::getNullLocalizedStringReference() };
	entity::model::ClockDomainIndex clockDomainIndex{ entity::model::ClockDomainIndex(0u) };
	entity::StreamFlags streamFlags{ entity::StreamFlags::None };
	UniqueIdentifier backupTalkerEntityID_0{ getNullIdentifier() };
	std::uint16_t backupTalkerUniqueID_0{ 0u };
	UniqueIdentifier backupTalkerEntityID_1{ getNullIdentifier() };
	std::uint16_t backupTalkerUniqueID_1{ 0u };
	UniqueIdentifier backupTalkerEntityID_2{ getNullIdentifier() };
	std::uint16_t backupTalkerUniqueID_2{ 0u };
	UniqueIdentifier backedupTalkerEntityID{ getNullIdentifier() };
	std::uint16_t backedupTalkerUnique{ 0u };
	entity::model::AvbInterfaceIndex avbInterfaceIndex{ entity::model::AvbInterfaceIndex(0u) };
	std::uint32_t bufferLength{ 0u };
	StreamFormats formats{};
	RedundantStreams redundantStreams{};
	bool isRedundant{ false }; // True is stream is part of a redundant stream association
};

struct AvbInterfaceNodeStaticModel
{
	entity::model::LocalizedStringReference localizedDescription{ entity::model::getNullLocalizedStringReference() };
	networkInterface::MacAddress macAddress{};
	entity::AvbInterfaceFlags interfaceFlags{ entity::AvbInterfaceFlags::None };
	UniqueIdentifier clockIdentify{ 0u };
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
	entity::model::LocalizedStringReference localizedDescription{ entity::model::getNullLocalizedStringReference() };
	entity::model::ClockSourceType clockSourceType{ entity::model::ClockSourceType::Internal };
	entity::model::DescriptorType clockSourceLocationType{ entity::model::DescriptorType::Invalid };
	entity::model::DescriptorIndex clockSourceLocationIndex{ entity::model::DescriptorIndex(0u) };
};

struct LocaleNodeStaticModel
{
	entity::model::AvdeccFixedString localeID{};
	std::uint16_t numberOfStringDescriptors{ 0u };
	entity::model::StringsIndex baseStringDescriptorIndex{ entity::model::StringsIndex(0u) };
};

struct StringsNodeStaticModel
{
	AvdeccFixedStrings strings{};
};

struct StreamPortNodeStaticModel
{
	entity::model::ClockDomainIndex clockDomainIndex{ entity::model::ClockDomainIndex(0u) };
	entity::PortFlags portFlags{ entity::PortFlags::None };
	std::uint16_t numberOfControls{ 0u };
	entity::model::ControlIndex baseControl{ entity::model::ControlIndex(0u) };
	std::uint16_t numberOfClusters{ 0u };
	entity::model::ClusterIndex baseCluster{ entity::model::ClusterIndex(0u) };
	std::uint16_t numberOfMaps{ 0u };
	entity::model::MapIndex baseMap{ entity::model::MapIndex(0u) };
	bool hasDynamicAudioMap{ false };
};

struct AudioClusterNodeStaticModel
{
	entity::model::LocalizedStringReference localizedDescription{ entity::model::getNullLocalizedStringReference() };
	entity::model::DescriptorType signalType{ entity::model::DescriptorType::Invalid };
	entity::model::DescriptorIndex signalIndex{ entity::model::DescriptorIndex(0u) };
	std::uint16_t signalOutput{ 0u };
	std::uint32_t pathLatency{ 0u };
	std::uint32_t blockLatency{ 0u };
	std::uint16_t channelCount{ 0u };
	entity::model::AudioClusterFormat format{ entity::model::AudioClusterFormat::Iec60958 };
};

struct AudioMapNodeStaticModel
{
	entity::model::AudioMappings mappings{};
};

struct ClockDomainNodeStaticModel
{
	entity::model::LocalizedStringReference localizedDescription{ entity::model::getNullLocalizedStringReference() };
	ClockSources clockSources{};
};

struct ConfigurationNodeStaticModel
{
	entity::model::LocalizedStringReference localizedDescription{ entity::model::getNullLocalizedStringReference() };
	DescriptorCounts descriptorCounts{};
};

struct EntityNodeStaticModel
{
	entity::model::LocalizedStringReference vendorNameString{ entity::model::getNullLocalizedStringReference() };
	entity::model::LocalizedStringReference modelNameString{ entity::model::getNullLocalizedStringReference() };
};

} // namespace model
} // namespace controller
} // namespace avdecc
} // namespace la
