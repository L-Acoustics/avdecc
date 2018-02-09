/*
* Copyright (C) 2016-2017, L-Acoustics and its contributors

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
* @file avdeccEntityModel.hpp
* @author Christophe Calmejane
* @brief Avdecc entity model tree for a #la::avdecc::controller::ControlledEntity.
*/

#pragma once

#include <string>
#include <la/avdecc/avdecc.hpp>
#include <la/avdecc/internals/exception.hpp>
#if __cpp_lib_any
#include <any>
#else // !__cpp_lib_any
#include <la/avdecc/internals/any.hpp>
#endif // __cpp_lib_any
#include "exports.hpp"
#include <vector>
#include <map>
#include <utility>

namespace la
{
namespace avdecc
{
namespace controller
{
class ControlledEntity;

namespace model
{

using VirtualIndex = entity::model::DescriptorIndex;

enum class AcquireState
{
	Undefined,
	NotAcquired,
	TryAcquire,
	Acquired,
	AcquiredByOther,
};

struct Node
{
	entity::model::DescriptorType descriptorType{ entity::model::DescriptorType::Entity };
};

struct EntityModelNode : public Node
{
	entity::model::DescriptorIndex descriptorIndex{ 0u };
	AcquireState acquireState{ AcquireState::Undefined };
	// TODO: Add LockState
};

struct VirtualNode : public Node
{
	VirtualIndex virtualIndex{ 0u };
};

struct AudioMapNode : public EntityModelNode
{
	// Static info
	entity::model::AudioMapDescriptor const* audioMapDescriptor{ nullptr };
};

struct AudioClusterNode : public EntityModelNode
{
	// Static info
	entity::model::AudioClusterDescriptor const* audioClusterDescriptor{ nullptr };
};

struct StreamPortNode : public EntityModelNode
{
	// Static info
	entity::model::StreamPortDescriptor const* streamPortDescriptor{ nullptr };
	std::map<entity::model::ClusterIndex, AudioClusterNode> audioClusters{};
	std::map<entity::model::MapIndex, AudioMapNode> audioMaps{};

	// Dynamic info
	bool hasDynamicAudioMap{ false };
	entity::model::AudioMappings const* dynamicAudioMap{ nullptr };
};

struct AudioUnitNode : public EntityModelNode
{
	// Static info
	entity::model::AudioUnitDescriptor const* audioUnitDescriptor{ nullptr };
	std::map<entity::model::StreamPortIndex, StreamPortNode> streamPortInputs{};
	std::map<entity::model::StreamPortIndex, StreamPortNode> streamPortOutputs{};
	// ExternalPortInput
	// ExternalPortOutput
	// InternalPortInput
	// InternalPortOutput
};

struct StreamNode : public EntityModelNode
{
	// Static info
	entity::model::StreamDescriptor const* streamDescriptor{ nullptr };
	bool isRedundant{ false }; // True is stream is part of a redundant stream association

	// Dynamic info
	entity::model::StreamConnectedState const* connectedState{ nullptr }; // Not set for a STREAM_OUTPUT
};

struct RedundantStreamNode : public VirtualNode
{
	// Static info
	std::map<entity::model::StreamIndex, StreamNode const*> redundantStreams{};
};

struct AvbInterfaceNode : public EntityModelNode
{
	// Static info
	entity::model::AvbInterfaceDescriptor const* avbInterfaceDescriptor{ nullptr };
};

struct ClockSourceNode : public EntityModelNode
{
	// Static info
	entity::model::ClockSourceDescriptor const* clockSourceDescriptor{ nullptr };
};

struct StringsNode : public EntityModelNode
{
	// Static info
	entity::model::StringsDescriptor const* stringsDescriptor{ nullptr };
};

struct LocaleNode : public EntityModelNode
{
	// Static info
	entity::model::LocaleDescriptor const* localeDescriptor{ nullptr };
	std::map<entity::model::StringsIndex, StringsNode> strings{};
};

struct ClockDomainNode : public EntityModelNode
{
	// Static info
	entity::model::ClockDomainDescriptor const* clockDomainDescriptor{ nullptr };
	std::map<entity::model::ClockSourceIndex, ClockSourceNode const*> clockSources{};
};

struct ConfigurationNode : public EntityModelNode
{
	// Static info
	entity::model::ConfigurationDescriptor const* configurationDescriptor{ nullptr };
	bool isActiveConfiguration{ false };

	// Following fields only valid if isActiveConfiguration is true
	std::map<entity::model::AudioUnitIndex, AudioUnitNode> audioUnits{};
	std::map<entity::model::StreamIndex, StreamNode> streamInputs{};
	std::map<entity::model::StreamIndex, StreamNode> streamOutputs{};
	// JackInput
	// JackOutput
	std::map<entity::model::AvbInterfaceIndex, AvbInterfaceNode> avbInterfaces{};
	std::map<entity::model::ClockSourceIndex, ClockSourceNode> clockSources{};
	// MemoryObject
	std::map<entity::model::LocaleIndex, LocaleNode> locales{};
	std::map<entity::model::ClockDomainIndex, ClockDomainNode> clockDomains{};

	std::map<VirtualIndex, RedundantStreamNode> redundantStreamInputs{}; /** Redundant input streams */
	std::map<VirtualIndex, RedundantStreamNode> redundantStreamOutputs{}; /** Redundant output streams */
};

struct EntityNode : public EntityModelNode
{
	// Static info
	entity::model::EntityDescriptor const* entityDescriptor{ nullptr };
	std::map<entity::model::ConfigurationIndex, ConfigurationNode> configurations{};
};

class EntityModelVisitor
{
public:
	virtual void visit(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::controller::model::Node const* const parent, la::avdecc::controller::model::EntityNode const& node) noexcept = 0;
	virtual void visit(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::controller::model::Node const* const parent, la::avdecc::controller::model::ConfigurationNode const& node) noexcept = 0;
	virtual void visit(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::controller::model::Node const* const parent, la::avdecc::controller::model::AudioUnitNode const& node) noexcept = 0;
	virtual void visit(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::controller::model::Node const* const parent, la::avdecc::controller::model::StreamNode const& node) noexcept = 0;
	virtual void visit(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::controller::model::Node const* const parent, la::avdecc::controller::model::AvbInterfaceNode const& node) noexcept = 0;
	virtual void visit(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::controller::model::Node const* const parent, la::avdecc::controller::model::ClockSourceNode const& node) noexcept = 0;
	virtual void visit(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::controller::model::Node const* const parent, la::avdecc::controller::model::LocaleNode const& node) noexcept = 0;
	virtual void visit(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::controller::model::Node const* const parent, la::avdecc::controller::model::StringsNode const& node) noexcept = 0;
	virtual void visit(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::controller::model::Node const* const parent, la::avdecc::controller::model::StreamPortNode const& node) noexcept = 0;
	virtual void visit(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::controller::model::Node const* const parent, la::avdecc::controller::model::AudioClusterNode const& node) noexcept = 0;
	virtual void visit(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::controller::model::Node const* const parent, la::avdecc::controller::model::AudioMapNode const& node) noexcept = 0;
	virtual void visit(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::controller::model::Node const* const parent, la::avdecc::controller::model::ClockDomainNode const& node) noexcept = 0;
	virtual void visit(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::controller::model::Node const* const parent, la::avdecc::controller::model::RedundantStreamNode const& node) noexcept = 0;

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
