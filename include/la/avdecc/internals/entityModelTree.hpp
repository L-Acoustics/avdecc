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

struct StringsNodeModels
{
	StringsNodeStaticModel staticModel{};
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

struct TimingNodeModels
{
	TimingNodeStaticModel staticModel{};
	TimingNodeDynamicModel dynamicModel{};
};

struct PtpPortNodeModels
{
	PtpPortNodeStaticModel staticModel{};
	PtpPortNodeDynamicModel dynamicModel{};
};

struct PtpInstanceTree
{
	// Children
	std::map<ControlIndex, ControlNodeModels> controlModels{};
	std::map<PtpPortIndex, PtpPortNodeModels> ptpPortModels{};

	// AEM Static info
	PtpInstanceNodeStaticModel staticModel{};

	// AEM Dynamic info
	PtpInstanceNodeDynamicModel dynamicModel{};
};

struct JackTree
{
	// Children
	std::map<ControlIndex, ControlNodeModels> controlModels{};

	// AEM Static info
	JackNodeStaticModel staticModel{};

	// AEM Dynamic info
	JackNodeDynamicModel dynamicModel{};
};

struct LocaleTree
{
	// Children
	std::map<StringsIndex, StringsNodeModels> stringsModels{};

	// AEM Static info
	LocaleNodeStaticModel staticModel{};
};

struct StreamPortTree
{
	// Children
	std::map<ClusterIndex, AudioClusterNodeModels> audioClusterModels{};
	std::map<MapIndex, AudioMapNodeModels> audioMapModels{};
	std::map<ControlIndex, ControlNodeModels> controlModels{};

	// AEM Static info
	StreamPortNodeStaticModel staticModel{};

	// AEM Dynamic info
	StreamPortNodeDynamicModel dynamicModel{};
};

struct AudioUnitTree
{
	using StreamPortTrees = std::map<StreamPortIndex, StreamPortTree>;
	//using ExternalPortInputTrees;
	//using ExternalPortOutputTrees;
	//using InternalPortInputTrees;
	//using InternalPortOutputTrees;

	// Children
	StreamPortTrees streamPortInputTrees{};
	StreamPortTrees streamPortOutputTrees{};
	//ExternalPortInputTrees ExternalPortInputTrees{};
	//ExternalPortOutputTrees ExternalPortOutputTrees{};
	//InternalPortInputTrees InternalPortInputTrees{};
	//InternalPortOutputTrees InternalPortOutputTrees{};
	std::map<ControlIndex, ControlNodeModels> controlModels{};

	// AEM Static info
	AudioUnitNodeStaticModel staticModel{};

	// AEM Dynamic info
	AudioUnitNodeDynamicModel dynamicModel{};
};

struct ConfigurationTree
{
	using AudioUnitTrees = std::map<AudioUnitIndex, AudioUnitTree>;
	using LocaleTrees = std::map<LocaleIndex, LocaleTree>;
	using JackTrees = std::map<JackIndex, JackTree>;
	using PtpInstanceTrees = std::map<PtpInstanceIndex, PtpInstanceTree>;

	// Children
	AudioUnitTrees audioUnitTrees{};
	std::map<StreamIndex, StreamInputNodeModels> streamInputModels{};
	std::map<StreamIndex, StreamOutputNodeModels> streamOutputModels{};
	JackTrees jackInputTrees{};
	JackTrees jackOutputTrees{};
	std::map<AvbInterfaceIndex, AvbInterfaceNodeModels> avbInterfaceModels{};
	std::map<ClockSourceIndex, ClockSourceNodeModels> clockSourceModels{};
	std::map<MemoryObjectIndex, MemoryObjectNodeModels> memoryObjectModels{};
	LocaleTrees localeTrees{};
	std::map<ControlIndex, ControlNodeModels> controlModels{};
	std::map<ClockDomainIndex, ClockDomainNodeModels> clockDomainModels{};
	std::map<TimingIndex, TimingNodeModels> timingModels{};
	PtpInstanceTrees ptpInstanceTrees{};

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
