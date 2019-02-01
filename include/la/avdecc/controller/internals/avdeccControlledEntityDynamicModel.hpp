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
* @file avdeccControlledEntityDynamicModel.hpp
* @author Christophe Calmejane
* @brief Dynamic part of the avdecc entity model for a #la::avdecc::controller::ControlledEntity.
* @note This is the part of the AEM that can be changed dynamically, or that might be different from an Entity to another one with the same EntityModelID
*/

#pragma once

#include <la/avdecc/avdecc.hpp>
#include "avdeccControlledEntityCommonModel.hpp"
#include <unordered_map>
#include <cstdint>
#include <map>

namespace la
{
namespace avdecc
{
namespace controller
{
namespace model
{
using AvbInterfaceCounters = std::map<entity::AvbInterfaceCounterValidFlag, entity::model::DescriptorCounter>;
using ClockDomainCounters = std::map<entity::ClockDomainCounterValidFlag, entity::model::DescriptorCounter>;
using StreamInputCounters = std::map<entity::StreamInputCounterValidFlag, entity::model::DescriptorCounter>;
using StreamOutputCounters = std::map<entity::StreamOutputCounterValidFlag, entity::model::DescriptorCounter>;

struct AudioUnitNodeDynamicModel
{
	entity::model::AvdeccFixedString objectName{};
	entity::model::SamplingRate currentSamplingRate{ entity::model::getNullSamplingRate() };
};

struct StreamNodeDynamicModel
{
	entity::model::AvdeccFixedString objectName{};
	entity::model::StreamInfo streamInfo{};
};

struct StreamInputNodeDynamicModel : public StreamNodeDynamicModel
{
	model::StreamConnectionState connectionState{};
	StreamInputCounters counters{};
};

struct StreamOutputNodeDynamicModel : public StreamNodeDynamicModel
{
	model::StreamConnections connections{};
	StreamOutputCounters counters{};
};

struct AvbInterfaceNodeDynamicModel
{
	entity::model::AvdeccFixedString objectName{};
	entity::model::AvbInfo avbInfo{};
	entity::model::AsPath asPath{};
	AvbInterfaceCounters counters{};
};

struct ClockSourceNodeDynamicModel
{
	entity::model::AvdeccFixedString objectName{};
	entity::ClockSourceFlags clockSourceFlags{ entity::ClockSourceFlags::None };
	UniqueIdentifier clockSourceIdentifier{};
};

struct MemoryObjectNodeDynamicModel
{
	entity::model::AvdeccFixedString objectName{};
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
	entity::model::AudioMappings dynamicAudioMap{};
};

struct AudioClusterNodeDynamicModel
{
	entity::model::AvdeccFixedString objectName{};
};

//struct AudioMapNodeDynamicModel
//{
//};

struct ClockDomainNodeDynamicModel
{
	entity::model::AvdeccFixedString objectName{};
	entity::model::ClockSourceIndex clockSourceIndex{ entity::model::ClockSourceIndex(0u) };
	ClockDomainCounters counters{};
};

struct ConfigurationNodeDynamicModel
{
	entity::model::AvdeccFixedString objectName{};
	entity::model::StringsIndex selectedLocaleBaseIndex{ entity::model::StringsIndex{ 0u } }; /** Base StringIndex for the selected locale */
	std::unordered_map<entity::model::StringsIndex, entity::model::AvdeccFixedString> localizedStrings{}; /** Aggregated copy of all loaded localized strings */
	bool isActiveConfiguration{ false };
};

struct EntityNodeDynamicModel
{
	entity::model::AvdeccFixedString entityName{};
	entity::model::AvdeccFixedString groupName{};
	entity::model::AvdeccFixedString firmwareVersion{};
	entity::model::AvdeccFixedString serialNumber{};
	std::uint16_t currentConfiguration{ 0u };
};

} // namespace model
} // namespace controller
} // namespace avdecc
} // namespace la
