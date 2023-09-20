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
* @file entityModelTreeDynamic.hpp
* @author Christophe Calmejane
* @brief Dynamic part of the avdecc entity model tree.
* @note This is the part of the AEM that can be changed dynamically, or that might be different from an Entity to another one with the same EntityModelID
*/

#pragma once

#include "entityModelTreeCommon.hpp"

#include <unordered_map>
#include <cstdint>
#include <map>
#include <optional>

namespace la
{
namespace avdecc
{
namespace entity
{
namespace model
{
using EntityCounters = std::map<entity::EntityCounterValidFlag, DescriptorCounter>;
using AvbInterfaceCounters = std::map<entity::AvbInterfaceCounterValidFlag, DescriptorCounter>;
using ClockDomainCounters = std::map<entity::ClockDomainCounterValidFlag, DescriptorCounter>;
using StreamInputCounters = std::map<entity::StreamInputCounterValidFlag, DescriptorCounter>;
using StreamOutputCounters = std::map<entity::StreamOutputCounterValidFlag, DescriptorCounter>;

struct AudioUnitNodeDynamicModel
{
	AvdeccFixedString objectName{};
	SamplingRate currentSamplingRate{};
};

struct StreamNodeDynamicModel
{
	AvdeccFixedString objectName{};
	StreamFormat streamFormat{};
	std::optional<bool> isStreamRunning{ std::nullopt };
	std::optional<StreamDynamicInfo> streamDynamicInfo{ std::nullopt };
};

struct StreamInputNodeDynamicModel : public StreamNodeDynamicModel
{
	model::StreamInputConnectionInfo connectionInfo{};
	std::optional<StreamInputCounters> counters{ std::nullopt };
};

struct StreamOutputNodeDynamicModel : public StreamNodeDynamicModel
{
	model::StreamConnections connections{};
	std::optional<StreamOutputCounters> counters{ std::nullopt };
};

struct JackNodeDynamicModel
{
	AvdeccFixedString objectName{};
};

struct AvbInterfaceNodeDynamicModel
{
	AvdeccFixedString objectName{};
	UniqueIdentifier gptpGrandmasterID{};
	std::uint8_t gptpDomainNumber{ 0u };
	std::optional<AvbInterfaceInfo> avbInterfaceInfo{ std::nullopt };
	std::optional<AsPath> asPath{ std::nullopt };
	std::optional<AvbInterfaceCounters> counters{ std::nullopt };
};

struct ClockSourceNodeDynamicModel
{
	AvdeccFixedString objectName{};
	entity::ClockSourceFlags clockSourceFlags{};
	UniqueIdentifier clockSourceIdentifier{};
};

struct MemoryObjectNodeDynamicModel
{
	AvdeccFixedString objectName{};
	std::uint64_t length{ 0u };
};

//struct LocaleNodeDynamicModel
//{
//};

//struct StringsNodeDynamicModel
//{
//};

struct StreamPortNodeDynamicModel
{
	AudioMappings dynamicAudioMap{};
};

struct AudioClusterNodeDynamicModel
{
	AvdeccFixedString objectName{};
};

//struct AudioMapNodeDynamicModel
//{
//};

struct ControlNodeDynamicModel
{
	AvdeccFixedString objectName{};
	ControlValues values{};
};

struct ClockDomainNodeDynamicModel
{
	AvdeccFixedString objectName{};
	ClockSourceIndex clockSourceIndex{ ClockSourceIndex(0u) };
	std::optional<ClockDomainCounters> counters{ std::nullopt };
};

struct TimingNodeDynamicModel
{
	AvdeccFixedString objectName{};
};

struct PtpInstanceNodeDynamicModel
{
	AvdeccFixedString objectName{};
	// TODO: Add PTP_INSTANCE dynamic info - See GET_PTP_INSTANCE_INFO (7.4.82) and other PTP_INSTANCE commands
};

struct PtpPortNodeDynamicModel
{
	AvdeccFixedString objectName{};
	// TODO: Add PTP_PORT dynamic info - See GET_PTP_PORT_INFO (7.4.95) and other PTP_PORT commands
};

struct ConfigurationNodeDynamicModel
{
	AvdeccFixedString objectName{};
	bool isActiveConfiguration{ false };

	// Internal variables
	StringsIndex selectedLocaleBaseIndex{ StringsIndex{ 0u } }; /** Base StringIndex for the selected locale */
	StringsIndex selectedLocaleCountIndexes{ StringsIndex{ 0u } }; /** Count StringIndexes for the selected locale */
	std::unordered_map<StringsIndex, AvdeccFixedString> localizedStrings{}; /** Aggregated copy of all loaded localized strings */
};

struct EntityNodeDynamicModel
{
	AvdeccFixedString entityName{};
	AvdeccFixedString groupName{};
	AvdeccFixedString firmwareVersion{};
	AvdeccFixedString serialNumber{};
	std::uint16_t currentConfiguration{ 0u };
	std::optional<EntityCounters> counters{ std::nullopt };
};

} // namespace model
} // namespace entity
} // namespace avdecc
} // namespace la
