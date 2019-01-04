/*
* Copyright (C) 2016-2019, L-Acoustics and its contributors

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
* @file avdeccControlledEntityModelTree.hpp
* @author Christophe Calmejane
* @brief Avdecc entity model tree as represented by the protocol
*/

#pragma once

#include "la/avdecc/controller/internals/avdeccControlledEntityStaticModel.hpp"
#include "la/avdecc/controller/internals/avdeccControlledEntityDynamicModel.hpp"
#include <map>
#include <set>

namespace la
{
namespace avdecc
{
namespace controller
{
namespace model
{
struct ConfigurationDynamicTree
{
	// Children
	std::map<entity::model::AudioUnitIndex, AudioUnitNodeDynamicModel> audioUnitDynamicModels{};
	std::map<entity::model::StreamIndex, StreamInputNodeDynamicModel> streamInputDynamicModels{};
	std::map<entity::model::StreamIndex, StreamOutputNodeDynamicModel> streamOutputDynamicModels{};
	//std::map<entity::model::JackIndex, JackNodeDynamicModel> jackInputDynamicModels{};
	//std::map<entity::model::JackIndex, JackNodeDynamicModel> jackOutputDynamicModels{};
	std::map<entity::model::AvbInterfaceIndex, AvbInterfaceNodeDynamicModel> avbInterfaceDynamicModels{};
	std::map<entity::model::ClockSourceIndex, ClockSourceNodeDynamicModel> clockSourceDynamicModels{};
	std::map<entity::model::MemoryObjectIndex, MemoryObjectNodeDynamicModel> memoryObjectDynamicModels{};
	std::map<entity::model::StreamPortIndex, StreamPortNodeDynamicModel> streamPortInputDynamicModels{};
	std::map<entity::model::StreamPortIndex, StreamPortNodeDynamicModel> streamPortOutputDynamicModels{};
	//std::map<entity::model::ExternalPortIndex, ExternalPortNodeDynamicModel> externalPortInputDynamicModels{};
	//std::map<entity::model::ExternalPortIndex, ExternalPortNodeDynamicModel> externalPortOutputDynamicModels{};
	//std::map<entity::model::InternalPortIndex, InternalPortNodeDynamicModel> internalPortInputDynamicModels{};
	//std::map<entity::model::InternalPortIndex, InternalPortNodeDynamicModel> internalPortOutputDynamicModels{};
	std::map<entity::model::ClusterIndex, AudioClusterNodeDynamicModel> audioClusterDynamicModels{};
	std::map<entity::model::ClockDomainIndex, ClockDomainNodeDynamicModel> clockDomainDynamicModels{};

	// AEM Dynamic info
	ConfigurationNodeDynamicModel dynamicModel;
};

struct EntityDynamicTree
{
	// Children
	std::map<entity::model::ConfigurationIndex, ConfigurationDynamicTree> configurationDynamicTrees{};

	// AEM Dynamic info
	EntityNodeDynamicModel dynamicModel;
};

struct ConfigurationStaticTree
{
	// Children
	std::map<entity::model::AudioUnitIndex, AudioUnitNodeStaticModel> audioUnitStaticModels{};
	std::map<entity::model::StreamIndex, StreamNodeStaticModel> streamInputStaticModels{};
	std::map<entity::model::StreamIndex, StreamNodeStaticModel> streamOutputStaticModels{};
	//std::map<entity::model::JackIndex, JackNodeStaticModel> jackInputStaticModels{};
	//std::map<entity::model::JackIndex, JackNodeStaticModel> jackOutputStaticModels{};
	std::map<entity::model::AvbInterfaceIndex, AvbInterfaceNodeStaticModel> avbInterfaceStaticModels{};
	std::map<entity::model::ClockSourceIndex, ClockSourceNodeStaticModel> clockSourceStaticModels{};
	std::map<entity::model::MemoryObjectIndex, MemoryObjectNodeStaticModel> memoryObjectStaticModels{};
	std::map<entity::model::LocaleIndex, LocaleNodeStaticModel> localeStaticModels{};
	std::map<entity::model::StringsIndex, StringsNodeStaticModel> stringsStaticModels{};
	std::map<entity::model::StreamPortIndex, StreamPortNodeStaticModel> streamPortInputStaticModels{};
	std::map<entity::model::StreamPortIndex, StreamPortNodeStaticModel> streamPortOutputStaticModels{};
	//std::map<entity::model::ExternalPortIndex, ExternalPortNodeStaticModel> externalPortInputStaticModels{};
	//std::map<entity::model::ExternalPortIndex, ExternalPortNodeStaticModel> externalPortOutputStaticModels{};
	//std::map<entity::model::InternalPortIndex, InternalPortNodeStaticModel> internalPortInputStaticModels{};
	//std::map<entity::model::InternalPortIndex, InternalPortNodeStaticModel> internalPortOutputStaticModels{};
	std::map<entity::model::ClusterIndex, AudioClusterNodeStaticModel> audioClusterStaticModels{};
	std::map<entity::model::MapIndex, AudioMapNodeStaticModel> audioMapStaticModels{};
	std::map<entity::model::ClockDomainIndex, ClockDomainNodeStaticModel> clockDomainStaticModels{};

	// AEM Static info
	ConfigurationNodeStaticModel staticModel;
};

struct EntityStaticTree
{
	// Children
	std::map<entity::model::ConfigurationIndex, ConfigurationStaticTree> configurationStaticTrees{};

	// AEM Static info
	EntityNodeStaticModel staticModel;
};

} // namespace model
} // namespace controller
} // namespace avdecc
} // namespace la
