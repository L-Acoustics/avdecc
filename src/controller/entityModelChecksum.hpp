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
* @file avdeccControllerImpl.hpp
* @author Christophe Calmejane
*/

#pragma once

#include "la/avdecc/controller/internals/avdeccControlledEntityModel.hpp"

#include <memory>
#include <string>

namespace la
{
namespace avdecc
{
namespace controller
{
class HashSerializer
{
public:
	HashSerializer() noexcept = default;
	virtual ~HashSerializer() = default;
	virtual std::string getHash() noexcept = 0;
};

class ChecksumEntityModelVisitor : public model::EntityModelVisitor
{
public:
	explicit ChecksumEntityModelVisitor(std::uint32_t const checksumVersion) noexcept;

	std::string getHash() const noexcept;

	// Deleted compiler auto-generated methods
	ChecksumEntityModelVisitor(ChecksumEntityModelVisitor const&) = delete;
	ChecksumEntityModelVisitor(ChecksumEntityModelVisitor&&) = delete;
	ChecksumEntityModelVisitor& operator=(ChecksumEntityModelVisitor const&) = delete;
	ChecksumEntityModelVisitor& operator=(ChecksumEntityModelVisitor&&) = delete;

private:
	// Private methods
	void serializeNode(model::Node const* const node) noexcept;
	void serializeNode(model::EntityModelNode const& node) noexcept;
	void serializeNode(model::VirtualNode const& node) noexcept;
	void serializeModel(model::ControlNode const& node) noexcept;
	void serializeModel(model::JackNode const& node) noexcept;
	void serializeModel(model::StreamPortNode const& node) noexcept;
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
	void serializeModel(model::RedundantStreamNode const& node) noexcept;
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

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
	std::uint32_t const _checksumVersion{ 0u };
	std::unique_ptr<HashSerializer> _serializer{ nullptr };
};

} // namespace controller
} // namespace avdecc
} // namespace la
