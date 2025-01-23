/*
* Copyright (C) 2016-2024, L-Acoustics and its contributors

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
* @file virtualEntityModelVisitor.cpp
* @author Christophe Calmejane
*/

#include "virtualEntityModelVisitor.hpp"

#include <la/avdecc/internals/endian.hpp>
#include <la/avdecc/utils.hpp>

#include <cstdint>
#include <type_traits>
#include <cstring> // memcpy
#include <array>
#include <sstream>
#include <iomanip>
#include <ios>

namespace la
{
namespace avdecc
{
namespace controller
{
VirtualEntityModelVisitor::VirtualEntityModelVisitor(ControlledEntityImpl* const controlledEntity, model::VirtualEntityBuilder* const builder) noexcept
	: _controlledEntity{ controlledEntity }
	, _builder{ builder }
{
	// Build the global state variables
	// AcquireState
	{
		auto state = model::AcquireState{};
		auto owningController = UniqueIdentifier{};

		// Call the builder
		utils::invokeProtectedMethod<void (model::VirtualEntityBuilder::*)(model::AcquireState&, UniqueIdentifier&)>(&model::VirtualEntityBuilder::build, _builder, state, owningController);
	}
	// LockState
	{
		auto lockState = model::LockState{};
		auto lockingController = UniqueIdentifier{};

		// Call the builder
		utils::invokeProtectedMethod<void (model::VirtualEntityBuilder::*)(model::LockState&, UniqueIdentifier&)>(&model::VirtualEntityBuilder::build, _builder, lockState, lockingController);
	}
	// UnsolicitedNotifications
	{
		auto isSupported = false;
		auto isSubscribed = false;

		// Call the builder
		utils::invokeProtectedMethod<void (model::VirtualEntityBuilder::*)(bool&, bool&)>(&model::VirtualEntityBuilder::build, _builder, isSupported, isSubscribed);
	}
	// Statistics
	{
		auto aecpRetryCounter = std::uint64_t{};
		auto aecpTimeoutCounter = std::uint64_t{};
		auto aecpUnexpectedResponseCounter = std::uint64_t{};
		auto aecpResponseAverageTime = std::chrono::milliseconds{};
		auto aemAecpUnsolicitedCounter = std::uint64_t{};
		auto aemAecpUnsolicitedLossCounter = std::uint64_t{};
		auto enumerationTime = std::chrono::milliseconds{};

		// Call the builder
		utils::invokeProtectedMethod<void (model::VirtualEntityBuilder::*)(std::uint64_t&, std::uint64_t&, std::uint64_t&, std::chrono::milliseconds&, std::uint64_t&, std::uint64_t&, std::chrono::milliseconds&)>(&model::VirtualEntityBuilder::build, _builder, aecpRetryCounter, aecpTimeoutCounter, aecpUnexpectedResponseCounter, aecpResponseAverageTime, aemAecpUnsolicitedCounter, aemAecpUnsolicitedLossCounter, enumerationTime);
	}
	// MilanInfo
	{
		auto info = entity::model::MilanInfo{};

		// Call the builder
		utils::invokeProtectedMethod<void (model::VirtualEntityBuilder::*)(entity::model::MilanInfo&)>(&model::VirtualEntityBuilder::build, _builder, info);
	}
}

bool VirtualEntityModelVisitor::isError() const noexcept
{
	return _isError;
}

std::string VirtualEntityModelVisitor::getErrorMessage() const noexcept
{
	return _errorMessage;
}

// Private methods

// la::avdecc::controller::model::EntityModelVisitor overrides
void VirtualEntityModelVisitor::visit(ControlledEntity const* const entity, model::EntityNode const& /*node*/) noexcept
{
	auto* const n = _controlledEntity->getModelAccessStrategy().getEntityNode(TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	if (!n)
	{
		_isError = true;
		_errorMessage = "Failed to get EntityNode";
		return;
	}
	// Call the builder
	utils::invokeProtectedMethod<void (model::VirtualEntityBuilder::*)(ControlledEntity const*, entity::model::EntityNodeStaticModel const&, entity::model::EntityNodeDynamicModel&)>(&model::VirtualEntityBuilder::build, _builder, entity, n->staticModel, n->dynamicModel);
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const entity, model::EntityNode const* const /*parent*/, model::ConfigurationNode const& node) noexcept
{
	auto* const n = _controlledEntity->getModelAccessStrategy().getConfigurationNode(node.descriptorIndex, TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	if (!n)
	{
		_isError = true;
		_errorMessage = "Failed to get ConfigurationNode";
		return;
	}
	// Call the builder
	utils::invokeProtectedMethod<void (model::VirtualEntityBuilder::*)(ControlledEntity const*, entity::model::ConfigurationIndex, entity::model::ConfigurationNodeStaticModel const&, entity::model::ConfigurationNodeDynamicModel&)>(&model::VirtualEntityBuilder::build, _builder, entity, node.descriptorIndex, n->staticModel, n->dynamicModel);
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const entity, model::ConfigurationNode const* const parent, model::AudioUnitNode const& node) noexcept
{
	auto* const n = _controlledEntity->getModelAccessStrategy().getAudioUnitNode(parent->descriptorIndex, node.descriptorIndex, TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	if (!n)
	{
		_isError = true;
		_errorMessage = "Failed to get AudioUnitNode";
		return;
	}
	// Call the builder
	utils::invokeProtectedMethod<void (model::VirtualEntityBuilder::*)(ControlledEntity const*, entity::model::AudioUnitIndex, entity::model::AudioUnitNodeStaticModel const&, entity::model::AudioUnitNodeDynamicModel&)>(&model::VirtualEntityBuilder::build, _builder, entity, node.descriptorIndex, n->staticModel, n->dynamicModel);
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const entity, model::ConfigurationNode const* const parent, model::StreamInputNode const& node) noexcept
{
	auto* const n = _controlledEntity->getModelAccessStrategy().getStreamInputNode(parent->descriptorIndex, node.descriptorIndex, TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	if (!n)
	{
		_isError = true;
		_errorMessage = "Failed to get StreamInputNode";
		return;
	}
	// Call the builder
	utils::invokeProtectedMethod<void (model::VirtualEntityBuilder::*)(ControlledEntity const*, entity::model::StreamIndex, entity::model::StreamNodeStaticModel const&, entity::model::StreamInputNodeDynamicModel&)>(&model::VirtualEntityBuilder::build, _builder, entity, node.descriptorIndex, n->staticModel, n->dynamicModel);
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const entity, model::ConfigurationNode const* const parent, model::StreamOutputNode const& node) noexcept
{
	auto* const n = _controlledEntity->getModelAccessStrategy().getStreamOutputNode(parent->descriptorIndex, node.descriptorIndex, TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	if (!n)
	{
		_isError = true;
		_errorMessage = "Failed to get StreamOutputNode";
		return;
	}
	// Call the builder
	utils::invokeProtectedMethod<void (model::VirtualEntityBuilder::*)(ControlledEntity const*, entity::model::StreamIndex, entity::model::StreamNodeStaticModel const&, entity::model::StreamOutputNodeDynamicModel&)>(&model::VirtualEntityBuilder::build, _builder, entity, node.descriptorIndex, n->staticModel, n->dynamicModel);
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const entity, model::ConfigurationNode const* const parent, model::JackInputNode const& node) noexcept
{
	auto* const n = _controlledEntity->getModelAccessStrategy().getJackInputNode(parent->descriptorIndex, node.descriptorIndex, TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	if (!n)
	{
		_isError = true;
		_errorMessage = "Failed to get JackInputNode";
		return;
	}
	// Call the builder
	utils::invokeProtectedMethod<void (model::VirtualEntityBuilder::*)(ControlledEntity const*, entity::model::JackIndex, entity::model::JackNodeStaticModel const&, entity::model::JackNodeDynamicModel&)>(&model::VirtualEntityBuilder::build, _builder, entity, node.descriptorIndex, n->staticModel, n->dynamicModel);
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const entity, model::ConfigurationNode const* const parent, model::JackOutputNode const& node) noexcept
{
	auto* const n = _controlledEntity->getModelAccessStrategy().getJackOutputNode(parent->descriptorIndex, node.descriptorIndex, TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	if (!n)
	{
		_isError = true;
		_errorMessage = "Failed to get JackOutputNode";
		return;
	}
	// Call the builder
	utils::invokeProtectedMethod<void (model::VirtualEntityBuilder::*)(ControlledEntity const*, entity::model::JackIndex, entity::model::JackNodeStaticModel const&, entity::model::JackNodeDynamicModel&)>(&model::VirtualEntityBuilder::build, _builder, entity, node.descriptorIndex, n->staticModel, n->dynamicModel);
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const entity, model::ConfigurationNode const* const grandParent, model::JackNode const* const parent, model::ControlNode const& node) noexcept
{
	auto* const n = _controlledEntity->getModelAccessStrategy().getControlNode(grandParent->descriptorIndex, node.descriptorIndex, TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	if (!n)
	{
		_isError = true;
		_errorMessage = "Failed to get ControlNode";
		return;
	}
	// Call the builder
	utils::invokeProtectedMethod<void (model::VirtualEntityBuilder::*)(ControlledEntity const*, entity::model::ControlIndex, entity::model::DescriptorType, entity::model::ControlNodeStaticModel const&, entity::model::ControlNodeDynamicModel&)>(&model::VirtualEntityBuilder::build, _builder, entity, node.descriptorIndex, parent->descriptorType, n->staticModel, n->dynamicModel);
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const entity, model::ConfigurationNode const* const parent, model::AvbInterfaceNode const& node) noexcept
{
	auto* const n = _controlledEntity->getModelAccessStrategy().getAvbInterfaceNode(parent->descriptorIndex, node.descriptorIndex, TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	if (!n)
	{
		_isError = true;
		_errorMessage = "Failed to get AvbInterfaceNode";
		return;
	}
	// Call the builder
	utils::invokeProtectedMethod<void (model::VirtualEntityBuilder::*)(ControlledEntity const*, entity::model::AvbInterfaceIndex, entity::model::AvbInterfaceNodeStaticModel const&, entity::model::AvbInterfaceNodeDynamicModel&)>(&model::VirtualEntityBuilder::build, _builder, entity, node.descriptorIndex, n->staticModel, n->dynamicModel);
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const entity, model::ConfigurationNode const* const parent, model::ClockSourceNode const& node) noexcept
{
	auto* const n = _controlledEntity->getModelAccessStrategy().getClockSourceNode(parent->descriptorIndex, node.descriptorIndex, TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	if (!n)
	{
		_isError = true;
		_errorMessage = "Failed to get ClockSourceNode";
		return;
	}
	// Call the builder
	utils::invokeProtectedMethod<void (model::VirtualEntityBuilder::*)(ControlledEntity const*, entity::model::ClockSourceIndex, entity::model::ClockSourceNodeStaticModel const&, entity::model::ClockSourceNodeDynamicModel&)>(&model::VirtualEntityBuilder::build, _builder, entity, node.descriptorIndex, n->staticModel, n->dynamicModel);
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const entity, model::ConfigurationNode const* const parent, model::MemoryObjectNode const& node) noexcept
{
	auto* const n = _controlledEntity->getModelAccessStrategy().getMemoryObjectNode(parent->descriptorIndex, node.descriptorIndex, TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	if (!n)
	{
		_isError = true;
		_errorMessage = "Failed to get MemoryObjectNode";
		return;
	}
	// Call the builder
	utils::invokeProtectedMethod<void (model::VirtualEntityBuilder::*)(ControlledEntity const*, entity::model::MemoryObjectIndex, entity::model::MemoryObjectNodeStaticModel const&, entity::model::MemoryObjectNodeDynamicModel&)>(&model::VirtualEntityBuilder::build, _builder, entity, node.descriptorIndex, n->staticModel, n->dynamicModel);
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*parent*/, model::LocaleNode const& /*node*/) noexcept
{
	// Nothing to do, no dynamic model
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandParent*/, model::LocaleNode const* const /*parent*/, model::StringsNode const& /*node*/) noexcept
{
	// Nothing to do, no dynamic model
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const entity, model::ConfigurationNode const* const grandParent, model::AudioUnitNode const* const /*parent*/, model::StreamPortInputNode const& node) noexcept
{
	auto* const n = _controlledEntity->getModelAccessStrategy().getStreamPortInputNode(grandParent->descriptorIndex, node.descriptorIndex, TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	if (!n)
	{
		_isError = true;
		_errorMessage = "Failed to get StreamPortInputNode";
		return;
	}
	// Call the builder
	utils::invokeProtectedMethod<void (model::VirtualEntityBuilder::*)(ControlledEntity const*, entity::model::StreamPortIndex, entity::model::DescriptorType, entity::model::StreamPortNodeStaticModel const&, entity::model::StreamPortNodeDynamicModel&)>(&model::VirtualEntityBuilder::build, _builder, entity, node.descriptorIndex, node.descriptorType, n->staticModel, n->dynamicModel);
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const entity, model::ConfigurationNode const* const grandParent, model::AudioUnitNode const* const /*parent*/, model::StreamPortOutputNode const& node) noexcept
{
	auto* const n = _controlledEntity->getModelAccessStrategy().getStreamPortOutputNode(grandParent->descriptorIndex, node.descriptorIndex, TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	if (!n)
	{
		_isError = true;
		_errorMessage = "Failed to get StreamPortOutputNode";
		return;
	}
	// Call the builder
	utils::invokeProtectedMethod<void (model::VirtualEntityBuilder::*)(ControlledEntity const*, entity::model::StreamPortIndex, entity::model::DescriptorType, entity::model::StreamPortNodeStaticModel const&, entity::model::StreamPortNodeDynamicModel&)>(&model::VirtualEntityBuilder::build, _builder, entity, node.descriptorIndex, node.descriptorType, n->staticModel, n->dynamicModel);
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const entity, model::ConfigurationNode const* const grandGrandParent, model::AudioUnitNode const* const /*grandParent*/, model::StreamPortNode const* const /*parent*/, model::AudioClusterNode const& node) noexcept
{
	auto* const n = _controlledEntity->getModelAccessStrategy().getAudioClusterNode(grandGrandParent->descriptorIndex, node.descriptorIndex, TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	if (!n)
	{
		_isError = true;
		_errorMessage = "Failed to get AudioClusterNode";
		return;
	}
	// Call the builder
	utils::invokeProtectedMethod<void (model::VirtualEntityBuilder::*)(ControlledEntity const*, entity::model::ClusterIndex, entity::model::AudioClusterNodeStaticModel const&, entity::model::AudioClusterNodeDynamicModel&)>(&model::VirtualEntityBuilder::build, _builder, entity, node.descriptorIndex, n->staticModel, n->dynamicModel);
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandGrandParent*/, model::AudioUnitNode const* const /*grandParent*/, model::StreamPortNode const* const /*parent*/, model::AudioMapNode const& /*node*/) noexcept
{
	// Nothing to do, no dynamic model
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const entity, model::ConfigurationNode const* const grandGrandParent, model::AudioUnitNode const* const /*grandParent*/, model::StreamPortNode const* const parent, model::ControlNode const& node) noexcept
{
	auto* const n = _controlledEntity->getModelAccessStrategy().getControlNode(grandGrandParent->descriptorIndex, node.descriptorIndex, TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	if (!n)
	{
		_isError = true;
		_errorMessage = "Failed to get ControlNode";
		return;
	}
	// Call the builder
	utils::invokeProtectedMethod<void (model::VirtualEntityBuilder::*)(ControlledEntity const*, entity::model::ControlIndex, entity::model::DescriptorType, entity::model::ControlNodeStaticModel const&, entity::model::ControlNodeDynamicModel&)>(&model::VirtualEntityBuilder::build, _builder, entity, node.descriptorIndex, parent->descriptorType, n->staticModel, n->dynamicModel);
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const entity, model::ConfigurationNode const* const grandParent, model::AudioUnitNode const* const parent, model::ControlNode const& node) noexcept
{
	auto* const n = _controlledEntity->getModelAccessStrategy().getControlNode(grandParent->descriptorIndex, node.descriptorIndex, TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	if (!n)
	{
		_isError = true;
		_errorMessage = "Failed to get ControlNode";
		return;
	}
	// Call the builder
	utils::invokeProtectedMethod<void (model::VirtualEntityBuilder::*)(ControlledEntity const*, entity::model::ControlIndex, entity::model::DescriptorType, entity::model::ControlNodeStaticModel const&, entity::model::ControlNodeDynamicModel&)>(&model::VirtualEntityBuilder::build, _builder, entity, node.descriptorIndex, parent->descriptorType, n->staticModel, n->dynamicModel);
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const entity, model::ConfigurationNode const* const parent, model::ControlNode const& node) noexcept
{
	auto* const n = _controlledEntity->getModelAccessStrategy().getControlNode(parent->descriptorIndex, node.descriptorIndex, TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	if (!n)
	{
		_isError = true;
		_errorMessage = "Failed to get ControlNode";
		return;
	}
	// Call the builder
	utils::invokeProtectedMethod<void (model::VirtualEntityBuilder::*)(ControlledEntity const*, entity::model::ControlIndex, entity::model::DescriptorType, entity::model::ControlNodeStaticModel const&, entity::model::ControlNodeDynamicModel&)>(&model::VirtualEntityBuilder::build, _builder, entity, node.descriptorIndex, parent->descriptorType, n->staticModel, n->dynamicModel);
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const entity, model::ConfigurationNode const* const parent, model::ClockDomainNode const& node) noexcept
{
	auto* const n = _controlledEntity->getModelAccessStrategy().getClockDomainNode(parent->descriptorIndex, node.descriptorIndex, TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	if (!n)
	{
		_isError = true;
		_errorMessage = "Failed to get ClockDomainNode";
		return;
	}
	// Call the builder
	utils::invokeProtectedMethod<void (model::VirtualEntityBuilder::*)(ControlledEntity const*, entity::model::ClockDomainIndex, entity::model::ClockDomainNodeStaticModel const&, entity::model::ClockDomainNodeDynamicModel&)>(&model::VirtualEntityBuilder::build, _builder, entity, node.descriptorIndex, n->staticModel, n->dynamicModel);
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandParent*/, model::ClockDomainNode const* const /*parent*/, model::ClockSourceNode const& /*node*/) noexcept
{
	// Ignore virtual parenting
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const entity, model::ConfigurationNode const* const parent, model::TimingNode const& node) noexcept
{
	auto* const n = _controlledEntity->getModelAccessStrategy().getTimingNode(parent->descriptorIndex, node.descriptorIndex, TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	if (!n)
	{
		_isError = true;
		_errorMessage = "Failed to get TimingNode";
		return;
	}
	// Call the builder
	utils::invokeProtectedMethod<void (model::VirtualEntityBuilder::*)(ControlledEntity const*, entity::model::TimingIndex, entity::model::TimingNodeStaticModel const&, entity::model::TimingNodeDynamicModel&)>(&model::VirtualEntityBuilder::build, _builder, entity, node.descriptorIndex, n->staticModel, n->dynamicModel);
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const entity, model::ConfigurationNode const* const parent, model::PtpInstanceNode const& node) noexcept
{
	auto* const n = _controlledEntity->getModelAccessStrategy().getPtpInstanceNode(parent->descriptorIndex, node.descriptorIndex, TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	if (!n)
	{
		_isError = true;
		_errorMessage = "Failed to get PtpInstanceNode";
		return;
	}
	// Call the builder
	utils::invokeProtectedMethod<void (model::VirtualEntityBuilder::*)(ControlledEntity const*, entity::model::PtpInstanceIndex, entity::model::PtpInstanceNodeStaticModel const&, entity::model::PtpInstanceNodeDynamicModel&)>(&model::VirtualEntityBuilder::build, _builder, entity, node.descriptorIndex, n->staticModel, n->dynamicModel);
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandParent*/, model::TimingNode const* const /*parent*/, model::PtpInstanceNode const& /*node*/) noexcept
{
	// Ignore virtual parenting
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandParent*/, model::PtpInstanceNode const* const /*parent*/, model::ControlNode const& /*node*/) noexcept
{
	// Ignore virtual parenting
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const entity, model::ConfigurationNode const* const grandParent, model::PtpInstanceNode const* const /*parent*/, model::PtpPortNode const& node) noexcept
{
	auto* const n = _controlledEntity->getModelAccessStrategy().getPtpPortNode(grandParent->descriptorIndex, node.descriptorIndex, TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	if (!n)
	{
		_isError = true;
		_errorMessage = "Failed to get PtpPortNode";
		return;
	}
	// Call the builder
	utils::invokeProtectedMethod<void (model::VirtualEntityBuilder::*)(ControlledEntity const*, entity::model::PtpPortIndex, entity::model::PtpPortNodeStaticModel const&, entity::model::PtpPortNodeDynamicModel&)>(&model::VirtualEntityBuilder::build, _builder, entity, node.descriptorIndex, n->staticModel, n->dynamicModel);
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandGrandParent*/, model::TimingNode const* const /*grandParent*/, model::PtpInstanceNode const* const /*parent*/, model::ControlNode const& /*node*/) noexcept
{
	// Ignore virtual parenting
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandGrandParent*/, model::TimingNode const* const /*grandParent*/, model::PtpInstanceNode const* const /*parent*/, model::PtpPortNode const& /*node*/) noexcept
{
	// Ignore virtual parenting
}

#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*parent*/, model::RedundantStreamInputNode const& /*node*/) noexcept
{
	// Nothing to do, no dynamic model
}
void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*parent*/, model::RedundantStreamOutputNode const& /*node*/) noexcept
{
	// Nothing to do, no dynamic model
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const entity, model::ConfigurationNode const* const grandParent, model::RedundantStreamNode const* const parent, model::StreamInputNode const& node) noexcept
{
	auto* const n = _controlledEntity->getModelAccessStrategy().getStreamInputNode(grandParent->descriptorIndex, node.descriptorIndex, TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	if (!n)
	{
		_isError = true;
		_errorMessage = "Failed to get StreamInputNode";
		return;
	}
	// Call the builder
	utils::invokeProtectedMethod<void (model::VirtualEntityBuilder::*)(ControlledEntity const*, entity::model::StreamIndex, model::VirtualIndex, entity::model::StreamNodeStaticModel const&, entity::model::StreamInputNodeDynamicModel&)>(&model::VirtualEntityBuilder::build, _builder, entity, node.descriptorIndex, parent->virtualIndex, n->staticModel, n->dynamicModel);
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const entity, model::ConfigurationNode const* const grandParent, model::RedundantStreamNode const* const parent, model::StreamOutputNode const& node) noexcept
{
	auto* const n = _controlledEntity->getModelAccessStrategy().getStreamOutputNode(grandParent->descriptorIndex, node.descriptorIndex, TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	if (!n)
	{
		_isError = true;
		_errorMessage = "Failed to get StreamOutputNode";
		return;
	}
	// Call the builder
	utils::invokeProtectedMethod<void (model::VirtualEntityBuilder::*)(ControlledEntity const*, entity::model::StreamIndex, model::VirtualIndex, entity::model::StreamNodeStaticModel const&, entity::model::StreamOutputNodeDynamicModel&)>(&model::VirtualEntityBuilder::build, _builder, entity, node.descriptorIndex, parent->virtualIndex, n->staticModel, n->dynamicModel);
}
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

} // namespace controller
} // namespace avdecc
} // namespace la
