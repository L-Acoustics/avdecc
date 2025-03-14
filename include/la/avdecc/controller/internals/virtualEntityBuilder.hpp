/*
* Copyright (C) 2016-2025, L-Acoustics and its contributors

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
* @file virtualEntityBuilder.hpp
* @author Christophe Calmejane
* @brief Virtual Entity Builder for a #la::avdecc::controller::ControlledEntity.
*/

#pragma once

#include "exports.hpp"
#include "avdeccControlledEntity.hpp"

#include <any>
#include <string>
#include <vector>
#include <optional>
#include <map>
#include <functional>
#include <set>
#include <deque>

namespace la::avdecc::controller::model
{
/** Visitor base class for the builder of a Virtual ControlledEntity. */
class VirtualEntityBuilder
{
public:
	// ADP related information, queried first
	virtual void build(la::avdecc::entity::model::EntityTree const& entityTree, la::avdecc::entity::Entity::CommonInformation& commonInformation, la::avdecc::entity::Entity::InterfacesInformation& intfcInformation) noexcept = 0;

	// Global state building
	virtual void build(la::avdecc::controller::model::AcquireState& acquireState, la::avdecc::UniqueIdentifier& owningController) noexcept = 0;
	virtual void build(la::avdecc::controller::model::LockState& lockState, la::avdecc::UniqueIdentifier& lockingController) noexcept = 0;
	virtual void build(bool& unsolicitedNotificationsSupported, bool& subscribedToUnsolicitedNotifications) noexcept = 0;
	virtual void build(std::uint64_t& aecpRetryCounter, std::uint64_t& aecpTimeoutCounter, std::uint64_t& aecpUnexpectedResponseCounter, std::chrono::milliseconds& aecpResponseAverageTime, std::uint64_t& aemAecpUnsolicitedCounter, std::uint64_t& aemAecpUnsolicitedLossCounter, std::chrono::milliseconds& enumerationTime) noexcept = 0;
	virtual void build(la::avdecc::entity::model::MilanInfo& milanInfo, la::avdecc::entity::model::MilanDynamicState& milanDynamicState) noexcept = 0;
	virtual void build(la::avdecc::controller::ControlledEntity::CompatibilityFlags& compatibilityFlags) noexcept = 0;

	// EntityModel building
	virtual void build(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::EntityNodeStaticModel const& staticModel, la::avdecc::entity::model::EntityNodeDynamicModel& dynamicModel) noexcept = 0;
	virtual void build(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::ConfigurationIndex const descriptorIndex, la::avdecc::entity::model::ConfigurationNodeStaticModel const& staticModel, la::avdecc::entity::model::ConfigurationNodeDynamicModel& dynamicModel) noexcept = 0;
	virtual void build(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::ControlIndex const descriptorIndex, la::avdecc::entity::model::DescriptorType const attachedTo, la::avdecc::entity::model::ControlNodeStaticModel const& staticModel, la::avdecc::entity::model::ControlNodeDynamicModel& dynamicModel) noexcept = 0;
	virtual void build(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::AudioUnitIndex const descriptorIndex, la::avdecc::entity::model::AudioUnitNodeStaticModel const& staticModel, la::avdecc::entity::model::AudioUnitNodeDynamicModel& dynamicModel) noexcept = 0;
	virtual void build(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::StreamIndex const descriptorIndex, la::avdecc::entity::model::StreamNodeStaticModel const& staticModel, la::avdecc::entity::model::StreamInputNodeDynamicModel& dynamicModel) noexcept = 0;
	virtual void build(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::StreamIndex const descriptorIndex, la::avdecc::entity::model::StreamNodeStaticModel const& staticModel, la::avdecc::entity::model::StreamOutputNodeDynamicModel& dynamicModel) noexcept = 0;
	virtual void build(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::JackIndex const descriptorIndex, la::avdecc::entity::model::JackNodeStaticModel const& staticModel, la::avdecc::entity::model::JackNodeDynamicModel& dynamicModel) noexcept = 0;
	virtual void build(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::AvbInterfaceIndex const descriptorIndex, la::avdecc::entity::model::AvbInterfaceNodeStaticModel const& staticModel, la::avdecc::entity::model::AvbInterfaceNodeDynamicModel& dynamicModel) noexcept = 0;
	virtual void build(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::ClockSourceIndex const descriptorIndex, la::avdecc::entity::model::ClockSourceNodeStaticModel const& staticModel, la::avdecc::entity::model::ClockSourceNodeDynamicModel& dynamicModel) noexcept = 0;
	virtual void build(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::MemoryObjectIndex const descriptorIndex, la::avdecc::entity::model::MemoryObjectNodeStaticModel const& staticModel, la::avdecc::entity::model::MemoryObjectNodeDynamicModel& dynamicModel) noexcept = 0;
	virtual void build(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::StreamPortIndex const descriptorIndex, la::avdecc::entity::model::DescriptorType const portType, la::avdecc::entity::model::StreamPortNodeStaticModel const& staticModel, la::avdecc::entity::model::StreamPortNodeDynamicModel& dynamicModel) noexcept = 0;
	virtual void build(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::ClusterIndex const descriptorIndex, la::avdecc::entity::model::AudioClusterNodeStaticModel const& staticModel, la::avdecc::entity::model::AudioClusterNodeDynamicModel& dynamicModel) noexcept = 0;
	virtual void build(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::ClockDomainIndex const descriptorIndex, la::avdecc::entity::model::ClockDomainNodeStaticModel const& staticModel, la::avdecc::entity::model::ClockDomainNodeDynamicModel& dynamicModel) noexcept = 0;
	virtual void build(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::TimingIndex const descriptorIndex, la::avdecc::entity::model::TimingNodeStaticModel const& staticModel, la::avdecc::entity::model::TimingNodeDynamicModel& dynamicModel) noexcept = 0;
	virtual void build(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::PtpInstanceIndex const descriptorIndex, la::avdecc::entity::model::PtpInstanceNodeStaticModel const& staticModel, la::avdecc::entity::model::PtpInstanceNodeDynamicModel& dynamicModel) noexcept = 0;
	virtual void build(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::PtpPortIndex const descriptorIndex, la::avdecc::entity::model::PtpPortNodeStaticModel const& staticModel, la::avdecc::entity::model::PtpPortNodeDynamicModel& dynamicModel) noexcept = 0;
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
	virtual void build(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::StreamIndex const descriptorIndex, la::avdecc::controller::model::VirtualIndex const redundantIndex, la::avdecc::entity::model::StreamNodeStaticModel const& staticModel, la::avdecc::entity::model::StreamInputNodeDynamicModel& dynamicModel) noexcept = 0;
	virtual void build(la::avdecc::controller::ControlledEntity const* const entity, la::avdecc::entity::model::StreamIndex const descriptorIndex, la::avdecc::controller::model::VirtualIndex const redundantIndex, la::avdecc::entity::model::StreamNodeStaticModel const& staticModel, la::avdecc::entity::model::StreamOutputNodeDynamicModel& dynamicModel) noexcept = 0;
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

	// Defaulted compiler auto-generated methods
	VirtualEntityBuilder() noexcept = default;
	virtual ~VirtualEntityBuilder() noexcept = default;
	VirtualEntityBuilder(VirtualEntityBuilder const&) = default;
	VirtualEntityBuilder(VirtualEntityBuilder&&) = default;
	VirtualEntityBuilder& operator=(VirtualEntityBuilder const&) = default;
	VirtualEntityBuilder& operator=(VirtualEntityBuilder&&) = default;
};

/** Defaulted version of the builder base class. */
class DefaultedVirtualEntityBuilder : public VirtualEntityBuilder
{
public:
	// ADP related information, queried first
	virtual void build(la::avdecc::entity::model::EntityTree const& /*entityTree*/, la::avdecc::entity::Entity::CommonInformation& /*commonInformation*/, la::avdecc::entity::Entity::InterfacesInformation& /*intfcInformation*/) noexcept override {}

	// Global state building
	virtual void build(la::avdecc::controller::model::AcquireState& /*acquireState*/, la::avdecc::UniqueIdentifier& /*owningController*/) noexcept override {}
	virtual void build(la::avdecc::controller::model::LockState& /*lockState*/, la::avdecc::UniqueIdentifier& /*lockingController*/) noexcept override {}
	virtual void build(bool& /*unsolicitedNotificationsSupported*/, bool& /*subscribedToUnsolicitedNotifications*/) noexcept override {}
	virtual void build(std::uint64_t& /*aecpRetryCounter*/, std::uint64_t& /*aecpTimeoutCounter*/, std::uint64_t& /*aecpUnexpectedResponseCounter*/, std::chrono::milliseconds& /*aecpResponseAverageTime*/, std::uint64_t& /*aemAecpUnsolicitedCounter*/, std::uint64_t& /*aemAecpUnsolicitedLossCounter*/, std::chrono::milliseconds& /*enumerationTime*/) noexcept override {}
	virtual void build(la::avdecc::entity::model::MilanInfo& /*milanInfo*/, la::avdecc::entity::model::MilanDynamicState& /*milanDynamicState*/) noexcept override {}
	virtual void build(la::avdecc::controller::ControlledEntity::CompatibilityFlags& /*compatibilityFlags*/) noexcept override {}

	// EntityModel building
	virtual void build(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::EntityNodeStaticModel const& /*staticModel*/, la::avdecc::entity::model::EntityNodeDynamicModel& /*dynamicModel*/) noexcept override {}
	virtual void build(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::ConfigurationIndex const /*descriptorIndex*/, la::avdecc::entity::model::ConfigurationNodeStaticModel const& /*staticModel*/, la::avdecc::entity::model::ConfigurationNodeDynamicModel& /*dynamicModel*/) noexcept override {}
	virtual void build(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::ControlIndex const /*descriptorIndex*/, la::avdecc::entity::model::DescriptorType const /*attachedTo*/, la::avdecc::entity::model::ControlNodeStaticModel const& /*staticModel*/, la::avdecc::entity::model::ControlNodeDynamicModel& /*dynamicModel*/) noexcept override {}
	virtual void build(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::AudioUnitIndex const /*descriptorIndex*/, la::avdecc::entity::model::AudioUnitNodeStaticModel const& /*staticModel*/, la::avdecc::entity::model::AudioUnitNodeDynamicModel& /*dynamicModel*/) noexcept override {}
	virtual void build(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamIndex const /*descriptorIndex*/, la::avdecc::entity::model::StreamNodeStaticModel const& /*staticModel*/, la::avdecc::entity::model::StreamInputNodeDynamicModel& /*dynamicModel*/) noexcept override {}
	virtual void build(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamIndex const /*descriptorIndex*/, la::avdecc::entity::model::StreamNodeStaticModel const& /*staticModel*/, la::avdecc::entity::model::StreamOutputNodeDynamicModel& /*dynamicModel*/) noexcept override {}
	virtual void build(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::JackIndex const /*descriptorIndex*/, la::avdecc::entity::model::JackNodeStaticModel const& /*staticModel*/, la::avdecc::entity::model::JackNodeDynamicModel& /*dynamicModel*/) noexcept override {}
	virtual void build(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::AvbInterfaceIndex const /*descriptorIndex*/, la::avdecc::entity::model::AvbInterfaceNodeStaticModel const& /*staticModel*/, la::avdecc::entity::model::AvbInterfaceNodeDynamicModel& /*dynamicModel*/) noexcept override {}
	virtual void build(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::ClockSourceIndex const /*descriptorIndex*/, la::avdecc::entity::model::ClockSourceNodeStaticModel const& /*staticModel*/, la::avdecc::entity::model::ClockSourceNodeDynamicModel& /*dynamicModel*/) noexcept override {}
	virtual void build(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::MemoryObjectIndex const /*descriptorIndex*/, la::avdecc::entity::model::MemoryObjectNodeStaticModel const& /*staticModel*/, la::avdecc::entity::model::MemoryObjectNodeDynamicModel& /*dynamicModel*/) noexcept override {}
	virtual void build(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamPortIndex const /*descriptorIndex*/, la::avdecc::entity::model::DescriptorType const /*portType*/, la::avdecc::entity::model::StreamPortNodeStaticModel const& /*staticModel*/, la::avdecc::entity::model::StreamPortNodeDynamicModel& /*dynamicModel*/) noexcept override {}
	virtual void build(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::ClusterIndex const /*descriptorIndex*/, la::avdecc::entity::model::AudioClusterNodeStaticModel const& /*staticModel*/, la::avdecc::entity::model::AudioClusterNodeDynamicModel& /*dynamicModel*/) noexcept override {}
	virtual void build(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::ClockDomainIndex const /*descriptorIndex*/, la::avdecc::entity::model::ClockDomainNodeStaticModel const& /*staticModel*/, la::avdecc::entity::model::ClockDomainNodeDynamicModel& /*dynamicModel*/) noexcept override {}
	virtual void build(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::TimingIndex const /*descriptorIndex*/, la::avdecc::entity::model::TimingNodeStaticModel const& /*staticModel*/, la::avdecc::entity::model::TimingNodeDynamicModel& /*dynamicModel*/) noexcept override {}
	virtual void build(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::PtpInstanceIndex const /*descriptorIndex*/, la::avdecc::entity::model::PtpInstanceNodeStaticModel const& /*staticModel*/, la::avdecc::entity::model::PtpInstanceNodeDynamicModel& /*dynamicModel*/) noexcept override {}
	virtual void build(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::PtpPortIndex const /*descriptorIndex*/, la::avdecc::entity::model::PtpPortNodeStaticModel const& /*staticModel*/, la::avdecc::entity::model::PtpPortNodeDynamicModel& /*dynamicModel*/) noexcept override {}
#ifdef ENABLE_AVDECC_FEATURE_REDUNDANCY
	virtual void build(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamIndex const /*descriptorIndex*/, la::avdecc::controller::model::VirtualIndex const /*redundantIndex*/, la::avdecc::entity::model::StreamNodeStaticModel const& /*staticModel*/, la::avdecc::entity::model::StreamInputNodeDynamicModel& /*dynamicModel*/) noexcept override {}
	virtual void build(la::avdecc::controller::ControlledEntity const* const /*entity*/, la::avdecc::entity::model::StreamIndex const /*descriptorIndex*/, la::avdecc::controller::model::VirtualIndex const /*redundantIndex*/, la::avdecc::entity::model::StreamNodeStaticModel const& /*staticModel*/, la::avdecc::entity::model::StreamOutputNodeDynamicModel& /*dynamicModel*/) noexcept override {}
#endif // ENABLE_AVDECC_FEATURE_REDUNDANCY

	// Defaulted compiler auto-generated methods
	DefaultedVirtualEntityBuilder() noexcept = default;
	virtual ~DefaultedVirtualEntityBuilder() noexcept override = default;
	DefaultedVirtualEntityBuilder(DefaultedVirtualEntityBuilder const&) = default;
	DefaultedVirtualEntityBuilder(DefaultedVirtualEntityBuilder&&) = default;
	DefaultedVirtualEntityBuilder& operator=(DefaultedVirtualEntityBuilder const&) = default;
	DefaultedVirtualEntityBuilder& operator=(DefaultedVirtualEntityBuilder&&) = default;
};


} // namespace la::avdecc::controller::model
