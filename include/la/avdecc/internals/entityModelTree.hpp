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
struct AudioUnitNodeModels
{
	AudioUnitNodeStaticModel staticModel{};
	AudioUnitNodeDynamicModel dynamicModel{};
};

struct StreamInputNodeModels
{
	StreamNodeStaticModel staticModel{};
	StreamInputNodeDynamicModel dynamicModel{};
};

struct StreamOutputNodeModels
{
	StreamNodeStaticModel staticModel{};
	StreamOutputNodeDynamicModel dynamicModel{};
};

struct JackNodeModels
{
	JackNodeStaticModel staticModel{};
	JackNodeDynamicModel dynamicModel{};
};

struct AvbInterfaceNodeModels
{
	AvbInterfaceNodeStaticModel staticModel{};
	AvbInterfaceNodeDynamicModel dynamicModel{};
};

struct ClockSourceNodeModels
{
	ClockSourceNodeStaticModel staticModel{};
	ClockSourceNodeDynamicModel dynamicModel{};
};

struct MemoryObjectNodeModels
{
	MemoryObjectNodeStaticModel staticModel{};
	MemoryObjectNodeDynamicModel dynamicModel{};
};

struct LocaleNodeModels
{
	LocaleNodeStaticModel staticModel{};
};

struct StringsNodeModels
{
	StringsNodeStaticModel staticModel{};
};

struct StreamPortNodeModels
{
	StreamPortNodeStaticModel staticModel{};
	StreamPortNodeDynamicModel dynamicModel{};
};

struct AudioClusterNodeModels
{
	AudioClusterNodeStaticModel staticModel{};
	AudioClusterNodeDynamicModel dynamicModel{};
};

struct AudioMapNodeModels
{
	AudioMapNodeStaticModel staticModel{};
};

struct ControlNodeModels
{
	ControlNodeStaticModel staticModel{};
	ControlNodeDynamicModel dynamicModel{};
};

struct ClockDomainNodeModels
{
	ClockDomainNodeStaticModel staticModel{};
	ClockDomainNodeDynamicModel dynamicModel{};
};

struct ConfigurationTree
{
	// Children
	std::map<AudioUnitIndex, AudioUnitNodeModels> audioUnitModels{};
	std::map<StreamIndex, StreamInputNodeModels> streamInputModels{};
	std::map<StreamIndex, StreamOutputNodeModels> streamOutputModels{};
	std::map<JackIndex, JackNodeModels> jackInputModels{};
	std::map<JackIndex, JackNodeModels> jackOutputModels{};
	std::map<AvbInterfaceIndex, AvbInterfaceNodeModels> avbInterfaceModels{};
	std::map<ClockSourceIndex, ClockSourceNodeModels> clockSourceModels{};
	std::map<MemoryObjectIndex, MemoryObjectNodeModels> memoryObjectModels{};
	std::map<LocaleIndex, LocaleNodeModels> localeModels{};
	std::map<StringsIndex, StringsNodeModels> stringsModels{};
	std::map<StreamPortIndex, StreamPortNodeModels> streamPortInputModels{};
	std::map<StreamPortIndex, StreamPortNodeModels> streamPortOutputModels{};
	//std::map<ExternalPortIndex, ExternalPortNodeModels> externalPortInputModels{};
	//std::map<ExternalPortIndex, ExternalPortNodeModels> externalPortOutputModels{};
	//std::map<InternalPortIndex, InternalPortNodeModels> internalPortInputModels{};
	//std::map<InternalPortIndex, InternalPortNodeModels> internalPortOutputModels{};
	std::map<ClusterIndex, AudioClusterNodeModels> audioClusterModels{};
	std::map<MapIndex, AudioMapNodeModels> audioMapModels{};
	std::map<ControlIndex, ControlNodeModels> controlModels{};
	std::map<ClockDomainIndex, ClockDomainNodeModels> clockDomainModels{};

	// AEM Static info
	ConfigurationNodeStaticModel staticModel;

	// AEM Dynamic info
	ConfigurationNodeDynamicModel dynamicModel;
};

struct EntityTree
{
	using ConfigurationTrees = std::map<ConfigurationIndex, ConfigurationTree>;

	// Children
	ConfigurationTrees configurationTrees{};

	// AEM Static info
	EntityNodeStaticModel staticModel;

	// AEM Dynamic info
	EntityNodeDynamicModel dynamicModel;
};

} // namespace model
} // namespace entity
} // namespace avdecc
} // namespace la
