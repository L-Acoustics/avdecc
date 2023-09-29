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
* @file avdeccControlledEntityModel.hpp
* @author Christophe Calmejane
* @brief Avdecc entity model for a #la::avdecc::controller::ControlledEntity.
* @note Nodes are only valid if the entity supports AEM.
*/

#pragma once

#include <la/avdecc/avdecc.hpp>
#include <la/avdecc/internals/exception.hpp>
#include <la/avdecc/internals/entityModelTree.hpp>

#include "exports.hpp"

#include <any>
#include <string>
#include <vector>
#include <optional>
#include <map>
#include <functional>
#include <set>
#include <deque>

namespace la
{
namespace avdecc
{
namespace controller
{
class ControlledEntity;

namespace model
{
using VirtualIndex = std::uint32_t; // We don't use the same type than DescriptorIndex (std::uint16_t). We want to be able to overload based on the type ('using' is not strongly typing our alias)

enum class AcquireState
{
	Undefined, /**< State undefined */
	NotSupported, /**< Acquire is not supported by this Entity */
	NotAcquired, /**< Entity is not acquired (at all) */
	AcquireInProgress, /**< Currently trying to acquire the Entity (not acquired by us, but *possibly* by another controller) */
	Acquired, /**< Entity is acquired by us */
	AcquiredByOther, /**< Entity is acquired by another controller */
	ReleaseInProgress, /**< Currently trying to release the entity (still *possibly* acquired by us) */
};

enum class LockState
{
	Undefined, /**< State undefined */
	NotSupported, /**< Lock is not supported by this Entity */
	NotLocked, /**< Entity is not locked (at all) */
	LockInProgress, /**< Currently trying to lock the Entity (not locked by us, but *possibly* by another controller) */
	Locked, /**< Entity is locked by us */
	LockedByOther, /**< Entity is locked by another controller */
	UnlockInProgress, /**< Currently trying to unlock the entity (still *possibly locked by us) */
};

struct MediaClockChainNode
{
	enum class Type
	{
		Undefined = 0, /**< Undefined media clock origin (Entity offline) */

		// Type of an active media clock origin
		Internal = 1, /**< Internal media clock */
		External = 2, /**< External media clock */
		StreamInput = 3, /**< Stream media clock */
	};

	enum class Status
	{
		Active = 0, /**< Media clock is active */

		Recursive = 1, /**< Recursive media clock (Type::StreamInput only) */
		StreamNotConnected = 2, /**< Stream not connected (Type::StreamInput only) */
		EntityOffline = 3, /**< Entity offline */

		// Unexpected errors
		UnsupportedClockSource = 97, /**< Unsupported clock source */
		AemError = 98, /**< AEM error */
		InternalError = 99, /**< Internal error */
	};

	Type type{ Type::Undefined }; // Type of this media clock chain node
	Status status{ Status::Active }; // Status of this media clock chain node
	UniqueIdentifier entityID{}; // EID of the entity of this media clock chain node
	entity::model::ClockDomainIndex clockDomainIndex{ entity::model::getInvalidDescriptorIndex() }; // ClockDomain index used by this node (may not be defined on error Status)
	entity::model::ClockSourceIndex clockSourceIndex{ entity::model::getInvalidDescriptorIndex() }; // ClockSource index used by this node (may not be defined on error Status)
	std::optional<entity::model::StreamIndex> streamInputIndex{}; // StreamInput index this entity is getting it's clock from (Type::StreamInput only). This is a copy of the ClockSource node's clockSourceLocationIndex
	std::optional<entity::model::StreamIndex> streamOutputIndex{}; // StreamOutput index this entity is sourcing it's clock to (Only if this node has a parent of Type::StreamInput)
};
using MediaClockChain = std::deque<MediaClockChainNode>;

struct Node
{
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Entity };
};

struct EntityModelNode : public Node
{
	entity::model::DescriptorIndex descriptorIndex{ 0u };

	// Constructor
	EntityModelNode(entity::model::DescriptorType const descriptorType, entity::model::DescriptorIndex const descriptorIndex) noexcept
		: Node{ descriptorType }
		, descriptorIndex{ descriptorIndex }
	{
	}
};

struct VirtualNode : public Node
{
	VirtualIndex virtualIndex{ 0u };

	// Constructor
	VirtualNode(entity::model::DescriptorType const descriptorType, VirtualIndex const virtualIndex) noexcept
		: Node{ descriptorType }
		, virtualIndex{ virtualIndex }
	{
	}
};

struct ControlNode : public EntityModelNode
{
	// AEM Static info
	entity::model::ControlNodeStaticModel staticModel{};

	// AEM Dynamic info
	entity::model::ControlNodeDynamicModel dynamicModel{};

	// Constructor
	explicit ControlNode(entity::model::DescriptorIndex const descriptorIndex) noexcept
		: EntityModelNode{ entity::model::DescriptorType::Control, descriptorIndex }
	{
	}
};

struct AudioMapNode : public EntityModelNode
{
	// AEM Static info
	entity::model::AudioMapNodeStaticModel staticModel{};

	// AEM Dynamic info
	//AudioMapNodeDynamicModel dynamicModel{  };

	// Constructor
	explicit AudioMapNode(entity::model::DescriptorIndex const descriptorIndex) noexcept
		: EntityModelNode{ entity::model::DescriptorType::AudioMap, descriptorIndex }
	{
	}
};

struct AudioClusterNode : public EntityModelNode
{
	// AEM Static info
	entity::model::AudioClusterNodeStaticModel staticModel{};

	// AEM Dynamic info
	entity::model::AudioClusterNodeDynamicModel dynamicModel{};

	// Constructor
	explicit AudioClusterNode(entity::model::DescriptorIndex const descriptorIndex) noexcept
		: EntityModelNode{ entity::model::DescriptorType::AudioCluster, descriptorIndex }
	{
	}
};

struct StreamPortNode : public EntityModelNode
{
	// Children
	std::map<entity::model::ClusterIndex, AudioClusterNode> audioClusters{};
	std::map<entity::model::MapIndex, AudioMapNode> audioMaps{};
	std::map<entity::model::ControlIndex, ControlNode> controls{};

	// AEM Static info
	entity::model::StreamPortNodeStaticModel staticModel{};

	// AEM Dynamic info
	entity::model::StreamPortNodeDynamicModel dynamicModel{};

protected:
	// Constructor
	StreamPortNode(entity::model::DescriptorType descriptorType, entity::model::DescriptorIndex const descriptorIndex) noexcept
		: EntityModelNode{ descriptorType, descriptorIndex }
	{
	}
};

struct StreamPortInputNode : public StreamPortNode
{
	// Constructor
	explicit StreamPortInputNode(entity::model::DescriptorIndex const descriptorIndex) noexcept
		: StreamPortNode{ entity::model::DescriptorType::StreamPortInput, descriptorIndex }
	{
	}
};

struct StreamPortOutputNode : public StreamPortNode
{
	// Constructor
	explicit StreamPortOutputNode(entity::model::DescriptorIndex const descriptorIndex) noexcept
		: StreamPortNode{ entity::model::DescriptorType::StreamPortOutput, descriptorIndex }
	{
	}
};

struct AudioUnitNode : public EntityModelNode
{
	// Children
	std::map<entity::model::StreamPortIndex, StreamPortInputNode> streamPortInputs{};
	std::map<entity::model::StreamPortIndex, StreamPortOutputNode> streamPortOutputs{};
	// ExternalPortInput
	// ExternalPortOutput
	// InternalPortInput
	// InternalPortOutput
	std::map<entity::model::ControlIndex, ControlNode> controls{};

	// AEM Static info
	entity::model::AudioUnitNodeStaticModel staticModel{};

	// AEM Dynamic info
	entity::model::AudioUnitNodeDynamicModel dynamicModel{};

	// Constructor
	explicit AudioUnitNode(entity::model::DescriptorIndex const descriptorIndex) noexcept
		: EntityModelNode{ entity::model::DescriptorType::AudioUnit, descriptorIndex }
	{
	}
};

struct StreamNode : public EntityModelNode
{
	// AEM Static info
	entity::model::StreamNodeStaticModel staticModel{};
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
	bool isRedundant{ false }; // True if stream is part of a valid redundant stream association
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

protected:
	// Constructor
	StreamNode(entity::model::DescriptorType descriptorType, entity::model::DescriptorIndex const descriptorIndex) noexcept
		: EntityModelNode{ descriptorType, descriptorIndex }
	{
	}
};

struct StreamInputNode : public StreamNode
{
	// AEM Dynamic info
	entity::model::StreamInputNodeDynamicModel dynamicModel{};

	// Constructor
	explicit StreamInputNode(entity::model::DescriptorIndex const descriptorIndex) noexcept
		: StreamNode{ entity::model::DescriptorType::StreamInput, descriptorIndex }
	{
	}
};

struct StreamOutputNode : public StreamNode
{
	// AEM Dynamic info
	entity::model::StreamOutputNodeDynamicModel dynamicModel{};

	// Constructor
	explicit StreamOutputNode(entity::model::DescriptorIndex const descriptorIndex) noexcept
		: StreamNode{ entity::model::DescriptorType::StreamOutput, descriptorIndex }
	{
	}
};

#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
struct RedundantStreamNode : public VirtualNode
{
	// Virtual name of the redundant stream, if one could be constructed (empty otherwise)
	entity::model::AvdeccFixedString virtualName{};

	// Children
	std::set<entity::model::StreamIndex> redundantStreams{}; // Either StreamInputNode or StreamOutputNode, based on Node::descriptorType

	// Quick access to the primary stream (which is also contained in this->redundantStreams)
	entity::model::StreamIndex primaryStreamIndex{ entity::model::getInvalidDescriptorIndex() };

protected:
	// Constructor
	RedundantStreamNode(entity::model::DescriptorType descriptorType, VirtualIndex const virtualIndex) noexcept
		: VirtualNode{ descriptorType, virtualIndex }
	{
	}
};

struct RedundantStreamInputNode : public RedundantStreamNode
{
	// Constructor
	explicit RedundantStreamInputNode(model::VirtualIndex const descriptorIndex) noexcept
		: RedundantStreamNode{ entity::model::DescriptorType::StreamInput, descriptorIndex }
	{
	}
};

struct RedundantStreamOutputNode : public RedundantStreamNode
{
	// Constructor
	explicit RedundantStreamOutputNode(model::VirtualIndex const descriptorIndex) noexcept
		: RedundantStreamNode{ entity::model::DescriptorType::StreamOutput, descriptorIndex }
	{
	}
};
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

struct JackNode : public EntityModelNode
{
	// Children
	std::map<entity::model::ControlIndex, ControlNode> controls{};

	// AEM Static info
	entity::model::JackNodeStaticModel staticModel{};

	// AEM Dynamic info
	entity::model::JackNodeDynamicModel dynamicModel{};

protected:
	// Constructor
	JackNode(entity::model::DescriptorType descriptorType, entity::model::DescriptorIndex const descriptorIndex) noexcept
		: EntityModelNode{ descriptorType, descriptorIndex }
	{
	}
};

struct JackInputNode : public JackNode
{
	// Constructor
	explicit JackInputNode(entity::model::DescriptorIndex const descriptorIndex) noexcept
		: JackNode{ entity::model::DescriptorType::JackInput, descriptorIndex }
	{
	}
};

struct JackOutputNode : public JackNode
{
	// Constructor
	explicit JackOutputNode(entity::model::DescriptorIndex const descriptorIndex) noexcept
		: JackNode{ entity::model::DescriptorType::JackOutput, descriptorIndex }
	{
	}
};

struct AvbInterfaceNode : public EntityModelNode
{
	// AEM Static info
	entity::model::AvbInterfaceNodeStaticModel staticModel{};

	// AEM Dynamic info
	entity::model::AvbInterfaceNodeDynamicModel dynamicModel{};

	// Constructor
	explicit AvbInterfaceNode(entity::model::DescriptorIndex const descriptorIndex) noexcept
		: EntityModelNode{ entity::model::DescriptorType::AvbInterface, descriptorIndex }
	{
	}
};

struct ClockSourceNode : public EntityModelNode
{
	// AEM Static info
	entity::model::ClockSourceNodeStaticModel staticModel{};

	// AEM Dynamic info
	entity::model::ClockSourceNodeDynamicModel dynamicModel{};

	// Constructor
	explicit ClockSourceNode(entity::model::DescriptorIndex const descriptorIndex) noexcept
		: EntityModelNode{ entity::model::DescriptorType::ClockSource, descriptorIndex }
	{
	}
};

struct MemoryObjectNode : public EntityModelNode
{
	// AEM Static info
	entity::model::MemoryObjectNodeStaticModel staticModel{};

	// AEM Dynamic info
	entity::model::MemoryObjectNodeDynamicModel dynamicModel{};

	// Constructor
	explicit MemoryObjectNode(entity::model::DescriptorIndex const descriptorIndex) noexcept
		: EntityModelNode{ entity::model::DescriptorType::MemoryObject, descriptorIndex }
	{
	}
};

struct StringsNode : public EntityModelNode
{
	// AEM Static info
	entity::model::StringsNodeStaticModel staticModel{};

	// AEM Dynamic info
	//StringsNodeDynamicModel dynamicModel{  };

	// Constructor
	explicit StringsNode(entity::model::DescriptorIndex const descriptorIndex) noexcept
		: EntityModelNode{ entity::model::DescriptorType::Strings, descriptorIndex }
	{
	}
};

struct LocaleNode : public EntityModelNode
{
	// Children
	std::map<entity::model::StringsIndex, StringsNode> strings{};

	// AEM Static info
	entity::model::LocaleNodeStaticModel staticModel{};

	// AEM Dynamic info
	//LocaleNodeDynamicModel dynamicModel{ nullptr };

	// Constructor
	explicit LocaleNode(entity::model::DescriptorIndex const descriptorIndex) noexcept
		: EntityModelNode{ entity::model::DescriptorType::Locale, descriptorIndex }
	{
	}
};

struct ClockDomainNode : public EntityModelNode
{
	// AEM Static info
	entity::model::ClockDomainNodeStaticModel staticModel{};

	// AEM Dynamic info
	entity::model::ClockDomainNodeDynamicModel dynamicModel{};

	MediaClockChain mediaClockChain{}; // Complete chain of MediaClock for this domain

	// Constructor
	explicit ClockDomainNode(entity::model::DescriptorIndex const descriptorIndex) noexcept
		: EntityModelNode{ entity::model::DescriptorType::ClockDomain, descriptorIndex }
	{
	}
};

struct TimingNode : public EntityModelNode
{
	// AEM Static info
	entity::model::TimingNodeStaticModel staticModel{};

	// AEM Dynamic info
	entity::model::TimingNodeDynamicModel dynamicModel{};

	// Constructor
	explicit TimingNode(entity::model::DescriptorIndex const descriptorIndex) noexcept
		: EntityModelNode{ entity::model::DescriptorType::Timing, descriptorIndex }
	{
	}
};

struct PtpPortNode : public EntityModelNode
{
	// AEM Static info
	entity::model::PtpPortNodeStaticModel staticModel{};

	// AEM Dynamic info
	entity::model::PtpPortNodeDynamicModel dynamicModel{};

	// Constructor
	explicit PtpPortNode(entity::model::DescriptorIndex const descriptorIndex) noexcept
		: EntityModelNode{ entity::model::DescriptorType::PtpPort, descriptorIndex }
	{
	}
};

struct PtpInstanceNode : public EntityModelNode
{
	// Children
	std::map<entity::model::ControlIndex, ControlNode> controls{};
	std::map<entity::model::PtpPortIndex, PtpPortNode> ptpPorts{};

	// AEM Static info
	entity::model::PtpInstanceNodeStaticModel staticModel{};

	// AEM Dynamic info
	entity::model::PtpInstanceNodeDynamicModel dynamicModel{};

	// Constructor
	explicit PtpInstanceNode(entity::model::DescriptorIndex const descriptorIndex) noexcept
		: EntityModelNode{ entity::model::DescriptorType::PtpInstance, descriptorIndex }
	{
	}
};

struct ConfigurationNode : public EntityModelNode
{
	// Children (only set if this is the active configuration)
	std::map<entity::model::AudioUnitIndex, AudioUnitNode> audioUnits{};
	std::map<entity::model::StreamIndex, StreamInputNode> streamInputs{};
	std::map<entity::model::StreamIndex, StreamOutputNode> streamOutputs{};
	std::map<entity::model::JackIndex, JackInputNode> jackInputs{};
	std::map<entity::model::JackIndex, JackOutputNode> jackOutputs{};
	std::map<entity::model::AvbInterfaceIndex, AvbInterfaceNode> avbInterfaces{};
	std::map<entity::model::ClockSourceIndex, ClockSourceNode> clockSources{};
	std::map<entity::model::MemoryObjectIndex, MemoryObjectNode> memoryObjects{};
	std::map<entity::model::LocaleIndex, LocaleNode> locales{};
	std::map<entity::model::ControlIndex, ControlNode> controls{};
	std::map<entity::model::ClockDomainIndex, ClockDomainNode> clockDomains{};
	std::map<entity::model::TimingIndex, TimingNode> timings{};
	std::map<entity::model::PtpInstanceIndex, PtpInstanceNode> ptpInstances{};

#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
	std::map<VirtualIndex, RedundantStreamInputNode> redundantStreamInputs{};
	std::map<VirtualIndex, RedundantStreamOutputNode> redundantStreamOutputs{};
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

	// AEM Static info
	entity::model::ConfigurationNodeStaticModel staticModel{};

	// AEM Dynamic info
	entity::model::ConfigurationNodeDynamicModel dynamicModel{};

	// Constructor
	explicit ConfigurationNode(entity::model::DescriptorIndex const descriptorIndex) noexcept
		: EntityModelNode{ entity::model::DescriptorType::Configuration, descriptorIndex }
	{
	}
};

struct EntityNode : public EntityModelNode
{
	// Children
	std::map<entity::model::ConfigurationIndex, ConfigurationNode> configurations{};

	// AEM Static info
	entity::model::EntityNodeStaticModel staticModel{};

	// AEM Dynamic info
	entity::model::EntityNodeDynamicModel dynamicModel{};

	// Constructor
	EntityNode() noexcept
		: EntityModelNode{ entity::model::DescriptorType::Entity, entity::model::DescriptorIndex{ 0u } }
	{
	}
};

/** Visitor base class for the Entity Model of a ControlledEntity. */
class EntityModelVisitor
{
public:
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::EntityNode const& /*node*/) noexcept = 0;
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::EntityNode const* const /*parent*/, la::avdecc::controller::model::ConfigurationNode const& /*node*/) noexcept = 0;
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::AudioUnitNode const& /*node*/) noexcept = 0;
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::StreamInputNode const& /*node*/) noexcept = 0;
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::StreamOutputNode const& /*node*/) noexcept = 0;
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::JackInputNode const& /*node*/) noexcept = 0;
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::JackOutputNode const& /*node*/) noexcept = 0;
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::JackNode const* const /*parent*/, la::avdecc::controller::model::ControlNode const& /*node*/) noexcept = 0;
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::AvbInterfaceNode const& /*node*/) noexcept = 0;
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::ClockSourceNode const& /*node*/) noexcept = 0;
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::MemoryObjectNode const& /*node*/) noexcept = 0;
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::LocaleNode const& /*node*/) noexcept = 0;
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::LocaleNode const* const /*parent*/, la::avdecc::controller::model::StringsNode const& /*node*/) noexcept = 0;
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::AudioUnitNode const* const /*parent*/, la::avdecc::controller::model::StreamPortInputNode const& /*node*/) noexcept = 0;
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::AudioUnitNode const* const /*parent*/, la::avdecc::controller::model::StreamPortOutputNode const& /*node*/) noexcept = 0;
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandGrandParent*/, la::avdecc::controller::model::AudioUnitNode const* const /*grandParent*/, la::avdecc::controller::model::StreamPortNode const* const /*parent*/, la::avdecc::controller::model::AudioClusterNode const& /*node*/) noexcept = 0;
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandGrandParent*/, la::avdecc::controller::model::AudioUnitNode const* const /*grandParent*/, la::avdecc::controller::model::StreamPortNode const* const /*parent*/, la::avdecc::controller::model::AudioMapNode const& /*node*/) noexcept = 0;
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandGrandParent*/, la::avdecc::controller::model::AudioUnitNode const* const /*grandParent*/, la::avdecc::controller::model::StreamPortNode const* const /*parent*/, la::avdecc::controller::model::ControlNode const& /*node*/) noexcept = 0;
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::AudioUnitNode const* const /*parent*/, la::avdecc::controller::model::ControlNode const& /*node*/) noexcept = 0;
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::ControlNode const& /*node*/) noexcept = 0;
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::ClockDomainNode const& /*node*/) noexcept = 0;
	// Virtual parenting to show ClockSourceNode which have the specified ClockDomainNode as parent
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::ClockDomainNode const* const /*parent*/, la::avdecc::controller::model::ClockSourceNode const& /*node*/) noexcept = 0;
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::TimingNode const& /*node*/) noexcept = 0;
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::PtpInstanceNode const& /*node*/) noexcept = 0;
	// Virtual parenting to show PtpInstanceNode which have the specified TimingNode as parent
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::TimingNode const* const /*parent*/, la::avdecc::controller::model::PtpInstanceNode const& /*node*/) noexcept = 0;
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::PtpInstanceNode const* const /*parent*/, la::avdecc::controller::model::ControlNode const& /*node*/) noexcept = 0;
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::PtpInstanceNode const* const /*parent*/, la::avdecc::controller::model::PtpPortNode const& /*node*/) noexcept = 0;
	// Virtual parenting to show ControlNode which have the specified TimingNode as grandParent and PtpInstanceNode as parent
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandGrandParent*/, la::avdecc::controller::model::TimingNode const* const /*grandParent*/, la::avdecc::controller::model::PtpInstanceNode const* const /*parent*/, la::avdecc::controller::model::ControlNode const& /*node*/) noexcept = 0;
	// Virtual parenting to show PtpPortNode which have the specified TimingNode as grandParent and PtpInstanceNode as parent
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandGrandParent*/, la::avdecc::controller::model::TimingNode const* const /*grandParent*/, la::avdecc::controller::model::PtpInstanceNode const* const /*parent*/, la::avdecc::controller::model::PtpPortNode const& /*node*/) noexcept = 0;
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::RedundantStreamInputNode const& /*node*/) noexcept = 0;
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::RedundantStreamOutputNode const& /*node*/) noexcept = 0;
	// Virtual parenting to show StreamInputNode which have the specified RedundantStreamNode as parent
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::RedundantStreamNode const* const /*parent*/, la::avdecc::controller::model::StreamInputNode const& /*node*/) noexcept = 0;
	// Virtual parenting to show StreamOutputNode which have the specified RedundantStreamNode as parent
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::RedundantStreamNode const* const /*parent*/, la::avdecc::controller::model::StreamOutputNode const& /*node*/) noexcept = 0;
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

	// Defaulted compiler auto-generated methods
	EntityModelVisitor() noexcept = default;
	virtual ~EntityModelVisitor() noexcept = default;
	EntityModelVisitor(EntityModelVisitor&&) = default;
	EntityModelVisitor(EntityModelVisitor const&) = default;
	EntityModelVisitor& operator=(EntityModelVisitor const&) = default;
	EntityModelVisitor& operator=(EntityModelVisitor&&) = default;
};

/** Defaulted version of the visitor base class. */
class DefaultedEntityModelVisitor : public EntityModelVisitor
{
public:
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::EntityNode const& /*node*/) noexcept override {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::EntityNode const* const /*parent*/, la::avdecc::controller::model::ConfigurationNode const& /*node*/) noexcept override {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::AudioUnitNode const& /*node*/) noexcept override {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::StreamInputNode const& /*node*/) noexcept override {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::StreamOutputNode const& /*node*/) noexcept override {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::JackInputNode const& /*node*/) noexcept override {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::JackOutputNode const& /*node*/) noexcept override {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::JackNode const* const /*parent*/, la::avdecc::controller::model::ControlNode const& /*node*/) noexcept override {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::AvbInterfaceNode const& /*node*/) noexcept override {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::ClockSourceNode const& /*node*/) noexcept override {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::MemoryObjectNode const& /*node*/) noexcept override {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::LocaleNode const& /*node*/) noexcept override {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::LocaleNode const* const /*parent*/, la::avdecc::controller::model::StringsNode const& /*node*/) noexcept override {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::AudioUnitNode const* const /*parent*/, la::avdecc::controller::model::StreamPortInputNode const& /*node*/) noexcept override {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::AudioUnitNode const* const /*parent*/, la::avdecc::controller::model::StreamPortOutputNode const& /*node*/) noexcept override {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandGrandParent*/, la::avdecc::controller::model::AudioUnitNode const* const /*grandParent*/, la::avdecc::controller::model::StreamPortNode const* const /*parent*/, la::avdecc::controller::model::AudioClusterNode const& /*node*/) noexcept override {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandGrandParent*/, la::avdecc::controller::model::AudioUnitNode const* const /*grandParent*/, la::avdecc::controller::model::StreamPortNode const* const /*parent*/, la::avdecc::controller::model::AudioMapNode const& /*node*/) noexcept override {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandGrandParent*/, la::avdecc::controller::model::AudioUnitNode const* const /*grandParent*/, la::avdecc::controller::model::StreamPortNode const* const /*parent*/, la::avdecc::controller::model::ControlNode const& /*node*/) noexcept override {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::AudioUnitNode const* const /*parent*/, la::avdecc::controller::model::ControlNode const& /*node*/) noexcept override {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::ControlNode const& /*node*/) noexcept override {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::ClockDomainNode const& /*node*/) noexcept override {}
	// Virtual parenting to show ClockSourceNode which have the specified ClockDomainNode as parent
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::ClockDomainNode const* const /*parent*/, la::avdecc::controller::model::ClockSourceNode const& /*node*/) noexcept override {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::TimingNode const& /*node*/) noexcept override {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::PtpInstanceNode const& /*node*/) noexcept override {}
	// Virtual parenting to show PtpInstanceNode which have the specified TimingNode as parent
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::TimingNode const* const /*parent*/, la::avdecc::controller::model::PtpInstanceNode const& /*node*/) noexcept override {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::PtpInstanceNode const* const /*parent*/, la::avdecc::controller::model::ControlNode const& /*node*/) noexcept override {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::PtpInstanceNode const* const /*parent*/, la::avdecc::controller::model::PtpPortNode const& /*node*/) noexcept override {}
	// Virtual parenting to show ControlNode which have the specified TimingNode as grandParent and PtpInstanceNode as parent
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandGrandParent*/, la::avdecc::controller::model::TimingNode const* const /*grandParent*/, la::avdecc::controller::model::PtpInstanceNode const* const /*parent*/, la::avdecc::controller::model::ControlNode const& /*node*/) noexcept override {}
	// Virtual parenting to show PtpPortNode which have the specified TimingNode as grandParent and PtpInstanceNode as parent
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandGrandParent*/, la::avdecc::controller::model::TimingNode const* const /*grandParent*/, la::avdecc::controller::model::PtpInstanceNode const* const /*parent*/, la::avdecc::controller::model::PtpPortNode const& /*node*/) noexcept override {}
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::RedundantStreamInputNode const& /*node*/) noexcept override {}
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::RedundantStreamOutputNode const& /*node*/) noexcept override {}
	// Virtual parenting to show StreamInputNode which have the specified RedundantStreamNode as parent
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::RedundantStreamNode const* const /*parent*/, la::avdecc::controller::model::StreamInputNode const& /*node*/) noexcept override {}
	// Virtual parenting to show StreamOutputNode which have the specified RedundantStreamNode as parent
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*grandParent*/, la::avdecc::controller::model::RedundantStreamNode const* const /*parent*/, la::avdecc::controller::model::StreamOutputNode const& /*node*/) noexcept override {}
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

	// Defaulted compiler auto-generated methods
	DefaultedEntityModelVisitor() noexcept = default;
	virtual ~DefaultedEntityModelVisitor() noexcept override = default;
	DefaultedEntityModelVisitor(DefaultedEntityModelVisitor&&) = default;
	DefaultedEntityModelVisitor(DefaultedEntityModelVisitor const&) = default;
	DefaultedEntityModelVisitor& operator=(DefaultedEntityModelVisitor const&) = default;
	DefaultedEntityModelVisitor& operator=(DefaultedEntityModelVisitor&&) = default;
};

} // namespace model
} // namespace controller
} // namespace avdecc
} // namespace la
