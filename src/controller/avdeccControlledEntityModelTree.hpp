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
* @file avdeccControlledEntityModelTree.hpp
* @author Christophe Calmejane
* @brief Avdecc entity model tree as represented by the protocol
*/

#pragma once

#include <la/avdecc/internals/entityModelTreeDynamic.hpp>
#include <la/avdecc/internals/entityModelTreeStatic.hpp>

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
	std::map<entity::model::AudioUnitIndex, entity::model::AudioUnitNodeDynamicModel> audioUnitDynamicModels{};
	std::map<entity::model::StreamIndex, entity::model::StreamInputNodeDynamicModel> streamInputDynamicModels{};
	std::map<entity::model::StreamIndex, entity::model::StreamOutputNodeDynamicModel> streamOutputDynamicModels{};
	//std::map<entity::model::JackIndex, entity::model::JackNodeDynamicModel> jackInputDynamicModels{};
	//std::map<entity::model::JackIndex, entity::model::JackNodeDynamicModel> jackOutputDynamicModels{};
	std::map<entity::model::AvbInterfaceIndex, entity::model::AvbInterfaceNodeDynamicModel> avbInterfaceDynamicModels{};
	std::map<entity::model::ClockSourceIndex, entity::model::ClockSourceNodeDynamicModel> clockSourceDynamicModels{};
	std::map<entity::model::MemoryObjectIndex, entity::model::MemoryObjectNodeDynamicModel> memoryObjectDynamicModels{};
	std::map<entity::model::StreamPortIndex, entity::model::StreamPortNodeDynamicModel> streamPortInputDynamicModels{};
	std::map<entity::model::StreamPortIndex, entity::model::StreamPortNodeDynamicModel> streamPortOutputDynamicModels{};
	//std::map<entity::model::ExternalPortIndex, entity::model::ExternalPortNodeDynamicModel> externalPortInputDynamicModels{};
	//std::map<entity::model::ExternalPortIndex, entity::model::ExternalPortNodeDynamicModel> externalPortOutputDynamicModels{};
	//std::map<entity::model::InternalPortIndex, entity::model::InternalPortNodeDynamicModel> internalPortInputDynamicModels{};
	//std::map<entity::model::InternalPortIndex, entity::model::InternalPortNodeDynamicModel> internalPortOutputDynamicModels{};
	std::map<entity::model::ClusterIndex, entity::model::AudioClusterNodeDynamicModel> audioClusterDynamicModels{};
	std::map<entity::model::ClockDomainIndex, entity::model::ClockDomainNodeDynamicModel> clockDomainDynamicModels{};

	// AEM Dynamic info
	entity::model::ConfigurationNodeDynamicModel dynamicModel;
};

struct EntityDynamicTree
{
	// Children
	std::map<entity::model::ConfigurationIndex, ConfigurationDynamicTree> configurationDynamicTrees{};

	// AEM Dynamic info
	entity::model::EntityNodeDynamicModel dynamicModel;
};

struct ConfigurationStaticTree
{
	// Children
	std::map<entity::model::AudioUnitIndex, entity::model::AudioUnitNodeStaticModel> audioUnitStaticModels{};
	std::map<entity::model::StreamIndex, entity::model::StreamNodeStaticModel> streamInputStaticModels{};
	std::map<entity::model::StreamIndex, entity::model::StreamNodeStaticModel> streamOutputStaticModels{};
	//std::map<entity::model::JackIndex, entity::model::JackNodeStaticModel> jackInputStaticModels{};
	//std::map<entity::model::JackIndex, entity::model::JackNodeStaticModel> jackOutputStaticModels{};
	std::map<entity::model::AvbInterfaceIndex, entity::model::AvbInterfaceNodeStaticModel> avbInterfaceStaticModels{};
	std::map<entity::model::ClockSourceIndex, entity::model::ClockSourceNodeStaticModel> clockSourceStaticModels{};
	std::map<entity::model::MemoryObjectIndex, entity::model::MemoryObjectNodeStaticModel> memoryObjectStaticModels{};
	std::map<entity::model::LocaleIndex, entity::model::LocaleNodeStaticModel> localeStaticModels{};
	std::map<entity::model::StringsIndex, entity::model::StringsNodeStaticModel> stringsStaticModels{};
	std::map<entity::model::StreamPortIndex, entity::model::StreamPortNodeStaticModel> streamPortInputStaticModels{};
	std::map<entity::model::StreamPortIndex, entity::model::StreamPortNodeStaticModel> streamPortOutputStaticModels{};
	//std::map<entity::model::ExternalPortIndex, entity::model::ExternalPortNodeStaticModel> externalPortInputStaticModels{};
	//std::map<entity::model::ExternalPortIndex, entity::model::ExternalPortNodeStaticModel> externalPortOutputStaticModels{};
	//std::map<entity::model::InternalPortIndex, entity::model::InternalPortNodeStaticModel> internalPortInputStaticModels{};
	//std::map<entity::model::InternalPortIndex, entity::model::InternalPortNodeStaticModel> internalPortOutputStaticModels{};
	std::map<entity::model::ClusterIndex, entity::model::AudioClusterNodeStaticModel> audioClusterStaticModels{};
	std::map<entity::model::MapIndex, entity::model::AudioMapNodeStaticModel> audioMapStaticModels{};
	std::map<entity::model::ClockDomainIndex, entity::model::ClockDomainNodeStaticModel> clockDomainStaticModels{};

	// AEM Static info
	entity::model::ConfigurationNodeStaticModel staticModel;
};

struct EntityStaticTree
{
	// Children
	std::map<entity::model::ConfigurationIndex, ConfigurationStaticTree> configurationStaticTrees{};

	// AEM Static info
	entity::model::EntityNodeStaticModel staticModel;
};

} // namespace model
} // namespace controller
} // namespace avdecc
} // namespace la
