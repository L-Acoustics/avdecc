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
* @file avdeccControlledEntityModelTree.hpp
* @author Christophe Calmejane
* @brief Avdecc entity model tree as represented by the protocol
*/

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

struct AudioUnitDescriptor
{
	// AEM Static info
	AudioUnitNodeStaticModel staticModel;

	// AEM Dynamic info
	AudioUnitNodeDynamicModel dynamicModel;
};

struct StreamInputDescriptor
{
	// AEM Static info
	StreamNodeStaticModel staticModel;

	// AEM Dynamic info
	StreamInputNodeDynamicModel dynamicModel;
};

struct StreamOutputDescriptor
{
	// AEM Static info
	StreamNodeStaticModel staticModel;

	// AEM Dynamic info
	StreamOutputNodeDynamicModel dynamicModel;
};

struct AvbInterfaceDescriptor
{
	// AEM Static info
	AvbInterfaceNodeStaticModel staticModel;

	// AEM Dynamic info
	AvbInterfaceNodeDynamicModel dynamicModel;
};

struct ClockSourceDescriptor
{
	// AEM Static info
	ClockSourceNodeStaticModel staticModel;

	// AEM Dynamic info
	ClockSourceNodeDynamicModel dynamicModel;
};

struct LocaleDescriptor
{
	// AEM Static info
	LocaleNodeStaticModel staticModel;

	// AEM Dynamic info
	//LocaleNodeDynamicModel dynamicModel;
};

struct StringsDescriptor
{
	// AEM Static info
	StringsNodeStaticModel staticModel;

	// AEM Dynamic info
	//StringsNodeDynamicModel dynamicModel;
};

struct StreamPortDescriptor
{
	// AEM Static info
	StreamPortNodeStaticModel staticModel;

	// AEM Dynamic info
	StreamPortNodeDynamicModel dynamicModel;
};

struct AudioClusterDescriptor
{
	// AEM Static info
	AudioClusterNodeStaticModel staticModel;

	// AEM Dynamic info
	AudioClusterNodeDynamicModel dynamicModel;
};

struct AudioMapDescriptor
{
	// AEM Static info
	AudioMapNodeStaticModel staticModel;

	// AEM Dynamic info
	//AudioMapNodeDynamicModel dynamicModel;
};

struct ClockDomainDescriptor
{
	// AEM Static info
	ClockDomainNodeStaticModel staticModel;

	// AEM Dynamic info
	ClockDomainNodeDynamicModel dynamicModel;
};

struct MemoryObjectDescriptor
{
	// AEM Static info
	MemoryObjectNodeStaticModel staticModel;

	// AEM Dynamic info
	MemoryObjectNodeDynamicModel dynamicModel;
};

struct ConfigurationDescriptor
{
	// Children
	std::map<entity::model::AudioUnitIndex, AudioUnitDescriptor> audioUnitDescriptors{};
	std::map<entity::model::StreamIndex, StreamInputDescriptor> streamInputDescriptors{};
	std::map<entity::model::StreamIndex, StreamOutputDescriptor> streamOutputDescriptors{};
	//std::map<entity::model::JackIndex, JackDescriptor> jackInputDescriptors{};
	//std::map<entity::model::JackIndex, JackDescriptor> jackOutputDescriptors{};
	std::map<entity::model::AvbInterfaceIndex, AvbInterfaceDescriptor> avbInterfaceDescriptors{};
	std::map<entity::model::ClockSourceIndex, ClockSourceDescriptor> clockSourceDescriptors{};
	std::map<entity::model::MemoryObjectIndex, MemoryObjectDescriptor> memoryObjectDescriptors{};
	std::map<entity::model::LocaleIndex, LocaleDescriptor> localeDescriptors{};
	std::map<entity::model::StringsIndex, StringsDescriptor> stringsDescriptors{};
	std::map<entity::model::StreamPortIndex, StreamPortDescriptor> streamPortInputDescriptors{};
	std::map<entity::model::StreamPortIndex, StreamPortDescriptor> streamPortOutputDescriptors{};
	//std::map<entity::model::ExternalPortIndex, ExternalPortDescriptor> externalPortInputDescriptors{};
	//std::map<entity::model::ExternalPortIndex, ExternalPortDescriptor> externalPortOutputDescriptors{};
	//std::map<entity::model::InternalPortIndex, InternalPortDescriptor> internalPortInputDescriptors{};
	//std::map<entity::model::InternalPortIndex, InternalPortDescriptor> internalPortOutputDescriptors{};
	std::map<entity::model::ClusterIndex, AudioClusterDescriptor> audioClusterDescriptors{};
	std::map<entity::model::MapIndex, AudioMapDescriptor> audioMapDescriptors{};
	std::map<entity::model::ClockDomainIndex, ClockDomainDescriptor> clockDomainDescriptors{};

	// AEM Static info
	ConfigurationNodeStaticModel staticModel;

	// AEM Dynamic info
	ConfigurationNodeDynamicModel dynamicModel;
};

struct EntityDescriptor
{
	// Children
	std::map<entity::model::ConfigurationIndex, ConfigurationDescriptor> configurationDescriptors{};

	// AEM Static info
	EntityNodeStaticModel staticModel;

	// AEM Dynamic info
	EntityNodeDynamicModel dynamicModel;
};

} // namespace model
} // namespace controller
} // namespace avdecc
} // namespace la
