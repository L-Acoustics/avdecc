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
#if 0
	// Setters of the global state
	void setEntity(entity::Entity const& entity) noexcept;
	InterfaceLinkStatus setAvbInterfaceLinkStatus(entity::model::AvbInterfaceIndex const avbInterfaceIndex, InterfaceLinkStatus const linkStatus) noexcept; // Returns previous link status
	void setAcquireState(model::AcquireState const state) noexcept;
	void setOwningController(UniqueIdentifier const controllerID) noexcept;
	void setLockState(model::LockState const state) noexcept;
	void setLockingController(UniqueIdentifier const controllerID) noexcept;
	void setMilanInfo(entity::model::MilanInfo const& info) noexcept;

	// Setters of the Statistics
	void setAecpRetryCounter(std::uint64_t const value) noexcept;
	void setAecpTimeoutCounter(std::uint64_t const value) noexcept;
	void setAecpUnexpectedResponseCounter(std::uint64_t const value) noexcept;
	void setAecpResponseAverageTime(std::chrono::milliseconds const& value) noexcept;
	void setAemAecpUnsolicitedCounter(std::uint64_t const value) noexcept;
	void setAemAecpUnsolicitedLossCounter(std::uint64_t const value) noexcept;
	void setEnumerationTime(std::chrono::milliseconds const& value) noexcept;

	// Setters of the Diagnostics
	void setDiagnostics(Diagnostics const& diags) noexcept;
#endif
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
	auto* const model = _controlledEntity->getModelAccessStrategy().getEntityNodeDynamicModel(TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	if (!model)
	{
		_isError = true;
		_errorMessage = "Failed to get EntityNodeDynamicModel";
		return;
	}
	// Call the visitor
	utils::invokeProtectedMethod<void (model::VirtualEntityBuilder::*)(ControlledEntity const*, entity::model::EntityNodeDynamicModel&)>(&model::VirtualEntityBuilder::build, _builder, entity, *model);
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::EntityNode const* const /*parent*/, model::ConfigurationNode const& node) noexcept
{
	auto* const model = _controlledEntity->getModelAccessStrategy().getConfigurationNodeDynamicModel(node.descriptorIndex, TreeModelAccessStrategy::NotFoundBehavior::IgnoreAndReturnNull);
	if (!model)
	{
		_isError = true;
		_errorMessage = "Failed to get ConfigurationNodeDynamicModel";
		return;
	}
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::AudioUnitNode const& node) noexcept
{
	//
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::StreamInputNode const& node) noexcept
{
	//
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::StreamOutputNode const& node) noexcept
{
	//
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::JackInputNode const& node) noexcept
{
	//
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::JackOutputNode const& node) noexcept
{
	//
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandParent, model::JackNode const* const parent, model::ControlNode const& node) noexcept
{
	//
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::AvbInterfaceNode const& node) noexcept
{
	//
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::ClockSourceNode const& node) noexcept
{
	//
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::MemoryObjectNode const& node) noexcept
{
	//
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::LocaleNode const& node) noexcept
{
	//
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandParent, model::LocaleNode const* const parent, model::StringsNode const& node) noexcept
{
	//
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandParent, model::AudioUnitNode const* const parent, model::StreamPortInputNode const& node) noexcept
{
	//
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandParent, model::AudioUnitNode const* const parent, model::StreamPortOutputNode const& node) noexcept
{
	//
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandGrandParent, model::AudioUnitNode const* const grandParent, model::StreamPortNode const* const parent, model::AudioClusterNode const& node) noexcept
{
	//
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandGrandParent, model::AudioUnitNode const* const grandParent, model::StreamPortNode const* const parent, model::AudioMapNode const& node) noexcept
{
	//
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandGrandParent, model::AudioUnitNode const* const grandParent, model::StreamPortNode const* const parent, model::ControlNode const& node) noexcept
{
	//
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandParent, model::AudioUnitNode const* const parent, model::ControlNode const& node) noexcept
{
	//
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::ControlNode const& node) noexcept
{
	//
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::ClockDomainNode const& node) noexcept
{
	//
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandParent*/, model::ClockDomainNode const* const /*parent*/, model::ClockSourceNode const& /*node*/) noexcept
{
	// Ignore virtual parenting
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::TimingNode const& node) noexcept
{
	//
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::PtpInstanceNode const& node) noexcept
{
	//
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandParent*/, model::TimingNode const* const /*parent*/, model::PtpInstanceNode const& /*node*/) noexcept
{
	// Ignore virtual parenting
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandParent*/, model::PtpInstanceNode const* const /*parent*/, model::ControlNode const& /*node*/) noexcept
{
	// Ignore virtual parenting
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandParent, model::PtpInstanceNode const* const parent, model::PtpPortNode const& node) noexcept
{
	//
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
void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::RedundantStreamInputNode const& node) noexcept
{
	//
}
void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::RedundantStreamOutputNode const& node) noexcept
{
	//
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandParent, model::RedundantStreamNode const* const parent, model::StreamInputNode const& node) noexcept
{
	//
}

void VirtualEntityModelVisitor::visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandParent, model::RedundantStreamNode const* const parent, model::StreamOutputNode const& node) noexcept
{
	//
}
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

} // namespace controller
} // namespace avdecc
} // namespace la
