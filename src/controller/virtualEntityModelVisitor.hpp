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
* @file virtualEntityModelVisitor.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "avdeccControlledEntityImpl.hpp"
#include "la/avdecc/controller/internals/virtualEntityBuilder.hpp"

#include <memory>
#include <string>

namespace la
{
namespace avdecc
{
namespace controller
{
class VirtualEntityModelVisitor : public model::EntityModelVisitor
{
public:
	explicit VirtualEntityModelVisitor(ControlledEntityImpl* const controlledEntity, model::VirtualEntityBuilder* const visitor) noexcept;
	bool isError() const noexcept;
	std::string getErrorMessage() const noexcept;

	// Deleted compiler auto-generated methods
	VirtualEntityModelVisitor(VirtualEntityModelVisitor const&) = delete;
	VirtualEntityModelVisitor(VirtualEntityModelVisitor&&) = delete;
	VirtualEntityModelVisitor& operator=(VirtualEntityModelVisitor const&) = delete;
	VirtualEntityModelVisitor& operator=(VirtualEntityModelVisitor&&) = delete;

private:
	// Private methods
	// la::avdecc::controller::model::EntityModelVisitor overrides
	virtual void visit(ControlledEntity const* const /*entity*/, model::EntityNode const& node) noexcept override;
	virtual void visit(ControlledEntity const* const /*entity*/, model::EntityNode const* const parent, model::ConfigurationNode const& node) noexcept override;
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::AudioUnitNode const& node) noexcept override;
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::StreamInputNode const& node) noexcept override;
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::StreamOutputNode const& node) noexcept override;
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::JackInputNode const& node) noexcept override;
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::JackOutputNode const& node) noexcept override;
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandParent, model::JackNode const* const parent, model::ControlNode const& node) noexcept override;
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::AvbInterfaceNode const& node) noexcept override;
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::ClockSourceNode const& node) noexcept override;
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::MemoryObjectNode const& node) noexcept override;
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::LocaleNode const& node) noexcept override;
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandParent, model::LocaleNode const* const parent, model::StringsNode const& node) noexcept override;
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandParent, model::AudioUnitNode const* const parent, model::StreamPortInputNode const& node) noexcept override;
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandParent, model::AudioUnitNode const* const parent, model::StreamPortOutputNode const& node) noexcept override;
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandGrandParent, model::AudioUnitNode const* const grandParent, model::StreamPortNode const* const parent, model::AudioClusterNode const& node) noexcept override;
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandGrandParent, model::AudioUnitNode const* const grandParent, model::StreamPortNode const* const parent, model::AudioMapNode const& node) noexcept override;
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandGrandParent, model::AudioUnitNode const* const grandParent, model::StreamPortNode const* const parent, model::ControlNode const& node) noexcept override;
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandParent, model::AudioUnitNode const* const parent, model::ControlNode const& node) noexcept override;
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::ControlNode const& node) noexcept override;
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::ClockDomainNode const& node) noexcept override;
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandParent, model::ClockDomainNode const* const parent, model::ClockSourceNode const& node) noexcept override;
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*parent*/, model::TimingNode const& /*node*/) noexcept override;
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*parent*/, model::PtpInstanceNode const& /*node*/) noexcept override;
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandParent*/, model::TimingNode const* const /*parent*/, model::PtpInstanceNode const& /*node*/) noexcept override;
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandParent*/, model::PtpInstanceNode const* const /*parent*/, model::ControlNode const& /*node*/) noexcept override;
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandParent*/, model::PtpInstanceNode const* const /*parent*/, model::PtpPortNode const& /*node*/) noexcept override;
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandGrandParent*/, model::TimingNode const* const /*grandParent*/, model::PtpInstanceNode const* const /*parent*/, model::ControlNode const& /*node*/) noexcept override;
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const /*grandGrandParent*/, model::TimingNode const* const /*grandParent*/, model::PtpInstanceNode const* const /*parent*/, model::PtpPortNode const& /*node*/) noexcept override;
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::RedundantStreamInputNode const& node) noexcept override;
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const parent, model::RedundantStreamOutputNode const& node) noexcept override;
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandParent, model::RedundantStreamNode const* const parent, model::StreamInputNode const& node) noexcept override;
	virtual void visit(ControlledEntity const* const /*entity*/, model::ConfigurationNode const* const grandParent, model::RedundantStreamNode const* const parent, model::StreamOutputNode const& node) noexcept override;
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

	// Private members
	ControlledEntityImpl* _controlledEntity{ nullptr };
	model::VirtualEntityBuilder* _builder{ nullptr };
	bool _isError{ false };
	std::string _errorMessage{};
};

} // namespace controller
} // namespace avdecc
} // namespace la
