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
* @file entityModelTree.hpp
* @author Christophe Calmejane
* @brief Avdecc entity model tree as represented by the protocol
*/

#pragma once

#include "entityModelTreeDynamic.hpp"
#include "entityModelTreeStatic.hpp"

#include <map>
#include <set>

namespace la
{
namespace avdecc
{
namespace entity
{
namespace model
{
struct ConfigurationDynamicTree
{
	// Children
	std::map<AudioUnitIndex, AudioUnitNodeDynamicModel> audioUnitDynamicModels{};
	std::map<StreamIndex, StreamInputNodeDynamicModel> streamInputDynamicModels{};
	std::map<StreamIndex, StreamOutputNodeDynamicModel> streamOutputDynamicModels{};
	//std::map<JackIndex, JackNodeDynamicModel> jackInputDynamicModels{};
	//std::map<JackIndex, JackNodeDynamicModel> jackOutputDynamicModels{};
	std::map<AvbInterfaceIndex, AvbInterfaceNodeDynamicModel> avbInterfaceDynamicModels{};
	std::map<ClockSourceIndex, ClockSourceNodeDynamicModel> clockSourceDynamicModels{};
	std::map<MemoryObjectIndex, MemoryObjectNodeDynamicModel> memoryObjectDynamicModels{};
	std::map<StreamPortIndex, StreamPortNodeDynamicModel> streamPortInputDynamicModels{};
	std::map<StreamPortIndex, StreamPortNodeDynamicModel> streamPortOutputDynamicModels{};
	//std::map<ExternalPortIndex, ExternalPortNodeDynamicModel> externalPortInputDynamicModels{};
	//std::map<ExternalPortIndex, ExternalPortNodeDynamicModel> externalPortOutputDynamicModels{};
	//std::map<InternalPortIndex, InternalPortNodeDynamicModel> internalPortInputDynamicModels{};
	//std::map<InternalPortIndex, InternalPortNodeDynamicModel> internalPortOutputDynamicModels{};
	std::map<ClusterIndex, AudioClusterNodeDynamicModel> audioClusterDynamicModels{};
	std::map<ClockDomainIndex, ClockDomainNodeDynamicModel> clockDomainDynamicModels{};

	// AEM Dynamic info
	ConfigurationNodeDynamicModel dynamicModel;
};

struct EntityDynamicTree
{
	// Children
	std::map<ConfigurationIndex, ConfigurationDynamicTree> configurationDynamicTrees{};

	// AEM Dynamic info
	EntityNodeDynamicModel dynamicModel;
};

struct ConfigurationStaticTree
{
	// Children
	std::map<AudioUnitIndex, AudioUnitNodeStaticModel> audioUnitStaticModels{};
	std::map<StreamIndex, StreamNodeStaticModel> streamInputStaticModels{};
	std::map<StreamIndex, StreamNodeStaticModel> streamOutputStaticModels{};
	//std::map<JackIndex, JackNodeStaticModel> jackInputStaticModels{};
	//std::map<JackIndex, JackNodeStaticModel> jackOutputStaticModels{};
	std::map<AvbInterfaceIndex, AvbInterfaceNodeStaticModel> avbInterfaceStaticModels{};
	std::map<ClockSourceIndex, ClockSourceNodeStaticModel> clockSourceStaticModels{};
	std::map<MemoryObjectIndex, MemoryObjectNodeStaticModel> memoryObjectStaticModels{};
	std::map<LocaleIndex, LocaleNodeStaticModel> localeStaticModels{};
	std::map<StringsIndex, StringsNodeStaticModel> stringsStaticModels{};
	std::map<StreamPortIndex, StreamPortNodeStaticModel> streamPortInputStaticModels{};
	std::map<StreamPortIndex, StreamPortNodeStaticModel> streamPortOutputStaticModels{};
	//std::map<ExternalPortIndex, ExternalPortNodeStaticModel> externalPortInputStaticModels{};
	//std::map<ExternalPortIndex, ExternalPortNodeStaticModel> externalPortOutputStaticModels{};
	//std::map<InternalPortIndex, InternalPortNodeStaticModel> internalPortInputStaticModels{};
	//std::map<InternalPortIndex, InternalPortNodeStaticModel> internalPortOutputStaticModels{};
	std::map<ClusterIndex, AudioClusterNodeStaticModel> audioClusterStaticModels{};
	std::map<MapIndex, AudioMapNodeStaticModel> audioMapStaticModels{};
	std::map<ClockDomainIndex, ClockDomainNodeStaticModel> clockDomainStaticModels{};

	// AEM Static info
	ConfigurationNodeStaticModel staticModel;
};

struct EntityStaticTree
{
	// Children
	std::map<ConfigurationIndex, ConfigurationStaticTree> configurationStaticTrees{};

	// AEM Static info
	EntityNodeStaticModel staticModel;
};

} // namespace model
} // namespace entity
} // namespace avdecc
} // namespace la
