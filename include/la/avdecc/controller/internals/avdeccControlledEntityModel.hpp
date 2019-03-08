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
* @file avdeccControlledEntityModel.hpp
* @author Christophe Calmejane
* @brief Avdecc entity model for a #la::avdecc::controller::ControlledEntity.
* @note Nodes are only valid if the entity supports AEM.
*/

#pragma once

#include <la/avdecc/avdecc.hpp>
#include <la/avdecc/internals/exception.hpp>
#include "avdeccControlledEntityStaticModel.hpp"
#include "avdeccControlledEntityDynamicModel.hpp"
#if defined(ENABLE_AVDECC_CUSTOM_ANY)
#	include <la/avdecc/internals/any.hpp>
#else // !ENABLE_AVDECC_CUSTOM_ANY
#	include <any>
#endif // ENABLE_AVDECC_CUSTOM_ANY
#include "exports.hpp"
#include <string>
#include <vector>
#include <map>
#include <set>

namespace la
{
namespace avdecc
{
namespace controller
{
class ControlledEntity;

namespace model
{
using VirtualIndex = std::uint32_t;

enum class AcquireState
{
	Undefined,
	NotSupported,
	NotAcquired,
	TryAcquire,
	Acquired,
	AcquiredByOther,
};

enum class LockState
{
	Undefined,
	NotSupported,
	NotLocked,
	TryLock,
	Locked,
	LockedByOther,
};

struct Node
{
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Entity };
};

struct EntityModelNode : public Node
{
	entity::model::DescriptorIndex descriptorIndex{ 0u };
};

struct VirtualNode : public Node
{
	VirtualIndex virtualIndex{ 0u };
};

struct AudioMapNode : public EntityModelNode
{
	// AEM Static info
	AudioMapNodeStaticModel const* staticModel{ nullptr };

	// AEM Dynamic info
	//AudioMapNodeDynamicModel* dynamicModel{ nullptr };
};

struct AudioClusterNode : public EntityModelNode
{
	// AEM Static info
	AudioClusterNodeStaticModel const* staticModel{ nullptr };

	// AEM Dynamic info
	AudioClusterNodeDynamicModel* dynamicModel{ nullptr };
};

struct StreamPortNode : public EntityModelNode
{
	// Children
	std::map<entity::model::ClusterIndex, AudioClusterNode> audioClusters{};
	std::map<entity::model::MapIndex, AudioMapNode> audioMaps{};

	// AEM Static info
	StreamPortNodeStaticModel const* staticModel{ nullptr };

	// AEM Dynamic info
	StreamPortNodeDynamicModel* dynamicModel{ nullptr };
};

struct AudioUnitNode : public EntityModelNode
{
	// Children
	std::map<entity::model::StreamPortIndex, StreamPortNode> streamPortInputs{};
	std::map<entity::model::StreamPortIndex, StreamPortNode> streamPortOutputs{};
	// ExternalPortInput
	// ExternalPortOutput
	// InternalPortInput
	// InternalPortOutput

	// AEM Static info
	AudioUnitNodeStaticModel const* staticModel{ nullptr };

	// AEM Dynamic info
	AudioUnitNodeDynamicModel* dynamicModel{ nullptr };
};

struct StreamNode : public EntityModelNode
{
	// AEM Static info
	StreamNodeStaticModel const* staticModel{ nullptr };
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
	bool isRedundant{ false }; // True if stream is part of a valid redundant stream association
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
};

struct StreamInputNode : public StreamNode
{
	// AEM Dynamic info
	StreamInputNodeDynamicModel* dynamicModel{ nullptr };
};

struct StreamOutputNode : public StreamNode
{
	// AEM Dynamic info
	StreamOutputNodeDynamicModel* dynamicModel{ nullptr };
};

#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
struct RedundantStreamNode : public VirtualNode
{
	// Children
	std::map<entity::model::StreamIndex, StreamNode const*> redundantStreams{}; // Either StreamInputNode or StreamOutputNode, based on Node::descriptorType

	// Quick access to the primary stream (which is also contained in this->redundantStreams)
	StreamNode const* primaryStream{ nullptr }; // Either StreamInputNode or StreamOutputNode, based on Node::descriptorType
};
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

struct AvbInterfaceNode : public EntityModelNode
{
	// AEM Static info
	AvbInterfaceNodeStaticModel const* staticModel{ nullptr };

	// AEM Dynamic info
	AvbInterfaceNodeDynamicModel* dynamicModel{ nullptr };
};

struct ClockSourceNode : public EntityModelNode
{
	// AEM Static info
	ClockSourceNodeStaticModel const* staticModel{ nullptr };

	// AEM Dynamic info
	ClockSourceNodeDynamicModel* dynamicModel{ nullptr };
};

struct MemoryObjectNode : public EntityModelNode
{
	// AEM Static info
	MemoryObjectNodeStaticModel const* staticModel{ nullptr };

	// AEM Dynamic info
	MemoryObjectNodeDynamicModel* dynamicModel{ nullptr };
};

struct StringsNode : public EntityModelNode
{
	// AEM Static info
	StringsNodeStaticModel const* staticModel{ nullptr };

	// AEM Dynamic info
	//StringsNodeDynamicModel* dynamicModel{ nullptr };
};

struct LocaleNode : public EntityModelNode
{
	// Children
	std::map<entity::model::StringsIndex, StringsNode> strings{};

	// AEM Static info
	LocaleNodeStaticModel const* staticModel{ nullptr };

	// AEM Dynamic info
	//LocaleNodeDynamicModel* dynamicModel{ nullptr };
};

struct ClockDomainNode : public EntityModelNode
{
	// Children
	std::map<entity::model::ClockSourceIndex, ClockSourceNode const*> clockSources{};

	// AEM Static info
	ClockDomainNodeStaticModel const* staticModel{ nullptr };

	// AEM Dynamic info
	ClockDomainNodeDynamicModel* dynamicModel{ nullptr };
};

struct ConfigurationNode : public EntityModelNode
{
	// Children (only set if this is the active configuration)
	std::map<entity::model::AudioUnitIndex, AudioUnitNode> audioUnits{};
	std::map<entity::model::StreamIndex, StreamInputNode> streamInputs{};
	std::map<entity::model::StreamIndex, StreamOutputNode> streamOutputs{};
	// JackInput
	// JackOutput
	std::map<entity::model::AvbInterfaceIndex, AvbInterfaceNode> avbInterfaces{};
	std::map<entity::model::ClockSourceIndex, ClockSourceNode> clockSources{};
	std::map<entity::model::MemoryObjectIndex, MemoryObjectNode> memoryObjects{};
	std::map<entity::model::LocaleIndex, LocaleNode> locales{};
	std::map<entity::model::ClockDomainIndex, ClockDomainNode> clockDomains{};

#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
	std::map<VirtualIndex, RedundantStreamNode> redundantStreamInputs{};
	std::map<VirtualIndex, RedundantStreamNode> redundantStreamOutputs{};
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

	// AEM Static info
	ConfigurationNodeStaticModel const* staticModel{ nullptr };

	// AEM Dynamic info
	ConfigurationNodeDynamicModel* dynamicModel{ nullptr };
};

struct EntityNode : public EntityModelNode
{
	// Children
	std::map<entity::model::ConfigurationIndex, ConfigurationNode> configurations{};

	// AEM Static info
	EntityNodeStaticModel const* staticModel{ nullptr };

	// AEM Dynamic info
	EntityNodeDynamicModel* dynamicModel{ nullptr };
};

class EntityModelVisitor
{
public:
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::EntityNode const& /*node*/) noexcept {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::EntityNode const* const /*parent*/, la::avdecc::controller::model::ConfigurationNode const& /*node*/) noexcept {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::AudioUnitNode const& /*node*/) noexcept {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::StreamInputNode const& /*node*/) noexcept {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::StreamOutputNode const& /*node*/) noexcept {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::AvbInterfaceNode const& /*node*/) noexcept {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::ClockSourceNode const& /*node*/) noexcept {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::MemoryObjectNode const& /*node*/) noexcept {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::LocaleNode const& /*node*/) noexcept {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::LocaleNode const* const /*parent*/, la::avdecc::controller::model::StringsNode const& /*node*/) noexcept {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::AudioUnitNode const* const /*parent*/, la::avdecc::controller::model::StreamPortNode const& /*node*/) noexcept {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandGrandParent*/, la::avdecc::controller::model::AudioUnitNode const* const /*grandParent*/, la::avdecc::controller::model::StreamPortNode const* const /*parent*/, la::avdecc::controller::model::AudioClusterNode const& /*node*/) noexcept {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandGrandParent*/, la::avdecc::controller::model::AudioUnitNode const* const /*grandParent*/, la::avdecc::controller::model::StreamPortNode const* const /*parent*/, la::avdecc::controller::model::AudioMapNode const& /*node*/) noexcept {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::ClockDomainNode const& /*node*/) noexcept {}
	// Virtual parenting to show ClockSourceNode which have the specified ClockDomainNode as parent
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::ClockDomainNode const* const /*parent*/, la::avdecc::controller::model::ClockSourceNode const& /*node*/) noexcept {}
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::RedundantStreamNode const& /*node*/) noexcept {}
	// Virtual parenting to show StreamInputNode which have the specified RedundantStreamNode as parent
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::RedundantStreamNode const* const /*parent*/, la::avdecc::controller::model::StreamInputNode const& /*node*/) noexcept {}
	// Virtual parenting to show StreamOutputNode which have the specified RedundantStreamNode as parent
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::RedundantStreamNode const* const /*parent*/, la::avdecc::controller::model::StreamOutputNode const& /*node*/) noexcept {}
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

	// Defaulted compiler auto-generated methods
	EntityModelVisitor() noexcept = default;
	virtual ~EntityModelVisitor() noexcept = default;
	EntityModelVisitor(EntityModelVisitor&&) = default;
	EntityModelVisitor(EntityModelVisitor const&) = default;
	EntityModelVisitor& operator=(EntityModelVisitor const&) = default;
	EntityModelVisitor& operator=(EntityModelVisitor&&) = default;
};

} // namespace model
} // namespace controller
} // namespace avdecc
} // namespace la
