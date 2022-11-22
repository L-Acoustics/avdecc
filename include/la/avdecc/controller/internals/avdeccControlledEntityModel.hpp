/*
* Copyright (C) 2016-2022, L-Acoustics and its contributors

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
};

struct VirtualNode : public Node
{
	VirtualIndex virtualIndex{ 0u };
};

struct AudioMapNode : public EntityModelNode
{
	// AEM Static info
	entity::model::AudioMapNodeStaticModel const* staticModel{ nullptr };

	// AEM Dynamic info
	//AudioMapNodeDynamicModel* dynamicModel{ nullptr };
};

struct AudioClusterNode : public EntityModelNode
{
	// AEM Static info
	entity::model::AudioClusterNodeStaticModel const* staticModel{ nullptr };

	// AEM Dynamic info
	entity::model::AudioClusterNodeDynamicModel* dynamicModel{ nullptr };
};

struct StreamPortNode : public EntityModelNode
{
	// Children
	std::map<entity::model::ClusterIndex, AudioClusterNode> audioClusters{};
	std::map<entity::model::MapIndex, AudioMapNode> audioMaps{};

	// AEM Static info
	entity::model::StreamPortNodeStaticModel const* staticModel{ nullptr };

	// AEM Dynamic info
	entity::model::StreamPortNodeDynamicModel* dynamicModel{ nullptr };
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
	entity::model::AudioUnitNodeStaticModel const* staticModel{ nullptr };

	// AEM Dynamic info
	entity::model::AudioUnitNodeDynamicModel* dynamicModel{ nullptr };
};

struct StreamNode : public EntityModelNode
{
	// AEM Static info
	entity::model::StreamNodeStaticModel const* staticModel{ nullptr };
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
	bool isRedundant{ false }; // True if stream is part of a valid redundant stream association
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY
};

struct StreamInputNode : public StreamNode
{
	// AEM Dynamic info
	entity::model::StreamInputNodeDynamicModel* dynamicModel{ nullptr };
};

struct StreamOutputNode : public StreamNode
{
	// AEM Dynamic info
	entity::model::StreamOutputNodeDynamicModel* dynamicModel{ nullptr };
};

#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
struct RedundantStreamNode : public VirtualNode
{
	// Virtual name of the redundant stream, if one could be constructed (empty otherwise)
	entity::model::AvdeccFixedString virtualName{};

	// Children
	std::map<entity::model::StreamIndex, StreamNode const*> redundantStreams{}; // Either StreamInputNode or StreamOutputNode, based on Node::descriptorType

	// Quick access to the primary stream (which is also contained in this->redundantStreams)
	StreamNode const* primaryStream{ nullptr }; // Either StreamInputNode or StreamOutputNode, based on Node::descriptorType
};
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

struct AvbInterfaceNode : public EntityModelNode
{
	// AEM Static info
	entity::model::AvbInterfaceNodeStaticModel const* staticModel{ nullptr };

	// AEM Dynamic info
	entity::model::AvbInterfaceNodeDynamicModel* dynamicModel{ nullptr };
};

struct ClockSourceNode : public EntityModelNode
{
	// AEM Static info
	entity::model::ClockSourceNodeStaticModel const* staticModel{ nullptr };

	// AEM Dynamic info
	entity::model::ClockSourceNodeDynamicModel* dynamicModel{ nullptr };
};

struct MemoryObjectNode : public EntityModelNode
{
	// AEM Static info
	entity::model::MemoryObjectNodeStaticModel const* staticModel{ nullptr };

	// AEM Dynamic info
	entity::model::MemoryObjectNodeDynamicModel* dynamicModel{ nullptr };
};

struct StringsNode : public EntityModelNode
{
	// AEM Static info
	entity::model::StringsNodeStaticModel const* staticModel{ nullptr };

	// AEM Dynamic info
	//StringsNodeDynamicModel* dynamicModel{ nullptr };
};

struct LocaleNode : public EntityModelNode
{
	// Children
	std::map<entity::model::StringsIndex, StringsNode> strings{};

	// AEM Static info
	entity::model::LocaleNodeStaticModel const* staticModel{ nullptr };

	// AEM Dynamic info
	//LocaleNodeDynamicModel* dynamicModel{ nullptr };
};

struct ControlNode : public EntityModelNode
{
	// AEM Static info
	entity::model::ControlNodeStaticModel const* staticModel{ nullptr };

	// AEM Dynamic info
	entity::model::ControlNodeDynamicModel* dynamicModel{ nullptr };
};

struct ClockDomainNode : public EntityModelNode
{
	// Children
	std::map<entity::model::ClockSourceIndex, ClockSourceNode const*> clockSources{};

	// AEM Static info
	entity::model::ClockDomainNodeStaticModel const* staticModel{ nullptr };

	// AEM Dynamic info
	entity::model::ClockDomainNodeDynamicModel* dynamicModel{ nullptr };

	MediaClockChain mediaClockChain{}; // Complete chain of MediaClock for this domain
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
	std::map<entity::model::ControlIndex, ControlNode> controls{};
	std::map<entity::model::ClockDomainIndex, ClockDomainNode> clockDomains{};

#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
	std::map<VirtualIndex, RedundantStreamNode> redundantStreamInputs{};
	std::map<VirtualIndex, RedundantStreamNode> redundantStreamOutputs{};
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

	// AEM Static info
	entity::model::ConfigurationNodeStaticModel const* staticModel{ nullptr };

	// AEM Dynamic info
	entity::model::ConfigurationNodeDynamicModel* dynamicModel{ nullptr };
};

struct EntityNode : public EntityModelNode
{
	// Children
	std::map<entity::model::ConfigurationIndex, ConfigurationNode> configurations{};

	// AEM Static info
	entity::model::EntityNodeStaticModel const* staticModel{ nullptr };

	// AEM Dynamic info
	entity::model::EntityNodeDynamicModel* dynamicModel{ nullptr };
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
	virtual void visit(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::controller::model::ConfigurationNode const* const /*parent*/, la::avdecc::controller::model::ControlNode const& /*node*/) noexcept {}
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
