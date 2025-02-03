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
		auto state = model::AcquireState::NotSupported;
		auto owningController = UniqueIdentifier{};

		// Call the builder
		utils::invokeProtectedMethod<void (model::VirtualEntityBuilder::*)(model::AcquireState&, UniqueIdentifier&)>(&model::VirtualEntityBuilder::build, _builder, state, owningController);

		// Check if the state is valid
		switch (state)
		{
			case model::AcquireState::Undefined:
				_isError = true;
				_errorMessage = "AcquireState cannot be 'Undefined'";
				return;
			case model::AcquireState::NotSupported:
			case model::AcquireState::NotAcquired:
				owningController = UniqueIdentifier{};
				break;
			case model::AcquireState::AcquireInProgress:
			case model::AcquireState::Acquired:
			case model::AcquireState::AcquiredByOther:
			case model::AcquireState::ReleaseInProgress:
			{
				if (!owningController)
				{
					_isError = true;
					_errorMessage = "Invalid owningController";
					return;
				}
				break;
			}
			default:
				_isError = true;
				_errorMessage = "Invalid AcquireState";
				return;
		}

		// Set the state
		_controlledEntity->setAcquireState(state);
		_controlledEntity->setOwningController(owningController);
	}
	// LockState
	{
		auto lockState = model::LockState::NotSupported;
		auto lockingController = UniqueIdentifier{};

		// Call the builder
		utils::invokeProtectedMethod<void (model::VirtualEntityBuilder::*)(model::LockState&, UniqueIdentifier&)>(&model::VirtualEntityBuilder::build, _builder, lockState, lockingController);

		// Check if the state is valid
		switch (lockState)
		{
			case model::LockState::Undefined:
				_isError = true;
				_errorMessage = "LockState cannot be 'Undefined'";
				return;
			case model::LockState::NotSupported:
			case model::LockState::NotLocked:
				lockingController = UniqueIdentifier{};
				break;
			case model::LockState::LockInProgress:
			case model::LockState::Locked:
			case model::LockState::LockedByOther:
			case model::LockState::UnlockInProgress:
			{
				if (!lockingController)
				{
					_isError = true;
					_errorMessage = "Invalid lockingController";
					return;
				}
				break;
			}
			default:
				_isError = true;
				_errorMessage = "Invalid LockState";
				return;
		}

		// Set the state
		_controlledEntity->setLockState(lockState);
		_controlledEntity->setLockingController(lockingController);
	}
	// UnsolicitedNotifications
	{
		auto isSupported = false;
		auto isSubscribed = false;

		// Call the builder
		utils::invokeProtectedMethod<void (model::VirtualEntityBuilder::*)(bool&, bool&)>(&model::VirtualEntityBuilder::build, _builder, isSupported, isSubscribed);

		// Set the state
		_controlledEntity->setUnsolicitedNotificationsSupported(isSupported);
		_controlledEntity->setSubscribedToUnsolicitedNotifications(isSubscribed);
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

		// Set the statistics
		_controlledEntity->setAecpRetryCounter(aecpRetryCounter);
		_controlledEntity->setAecpTimeoutCounter(aecpTimeoutCounter);
		_controlledEntity->setAecpUnexpectedResponseCounter(aecpUnexpectedResponseCounter);
		_controlledEntity->setAecpResponseAverageTime(aecpResponseAverageTime);
		_controlledEntity->setAemAecpUnsolicitedCounter(aemAecpUnsolicitedCounter);
		_controlledEntity->setAemAecpUnsolicitedLossCounter(aemAecpUnsolicitedLossCounter);
		_controlledEntity->setEnumerationTime(enumerationTime);
	}
	// Diagnostics should be computed automatically
	// CompatibilityFlags
	{
		auto flags = ControlledEntity::CompatibilityFlags{};

		// Call the builder
		utils::invokeProtectedMethod<void (model::VirtualEntityBuilder::*)(ControlledEntity::CompatibilityFlags&)>(&model::VirtualEntityBuilder::build, _builder, flags);

		// Set the flags
		_controlledEntity->setCompatibilityFlags(flags);
	}
	// MilanInfo
	{
		auto info = entity::model::MilanInfo{};

		// Call the builder
		utils::invokeProtectedMethod<void (model::VirtualEntityBuilder::*)(entity::model::MilanInfo&)>(&model::VirtualEntityBuilder::build, _builder, info);

		// Set the info
		_controlledEntity->setMilanInfo(info);
	}
}

void VirtualEntityModelVisitor::validate() noexcept
{
	try
	{
		auto const activeConfigurationIndex = _controlledEntity->getCurrentConfigurationIndex();

		// Check active configuration
		{
			// Process all configurations and check if 'isActiveConfiguration' is true for exactly one configuration (and check the index is the same than _controlledEntity->getCurrentConfigurationIndex())
			auto activeConfigurationFound = false;

			auto const* const entityNode = _controlledEntity->getEntityNode(TreeModelAccessStrategy::NotFoundBehavior::Throw);
			for (auto const& [confIndex, confNode] : entityNode->configurations)
			{
				// No active configuration found yet
				if (!activeConfigurationFound)
				{
					// Found an active configuration
					if (confNode.dynamicModel.isActiveConfiguration)
					{
						activeConfigurationFound = true;
						if (confIndex != activeConfigurationIndex)
						{
							_isError = true;
							_errorMessage = std::string{ "configuration[" } + std::to_string(confIndex) + "].dynamicModel.isActiveConfiguration set to true but entity.dynamicModel.currentConfiguration is " + std::to_string(activeConfigurationIndex);
							return;
						}
					}
				}
				else
				{
					// Another active configuration found
					if (confNode.dynamicModel.isActiveConfiguration)
					{
						_isError = true;
						_errorMessage = "Multiple configuration.dynamicModel.isActiveConfiguration set to true";
						return;
					}
				}
			}
			// No active configuration found
			if (!activeConfigurationFound)
			{
				_isError = true;
				_errorMessage = "No configuration.dynamicModel.isActiveConfiguration set to true";
				return;
			}
		}

		// Check AudioUnitNodeDynamicModel.currentSamplingRate for the active configuration
		{
			auto const* const configurationNode = _controlledEntity->getConfigurationNode(activeConfigurationIndex, TreeModelAccessStrategy::NotFoundBehavior::Throw);
			for (auto const& [audioUnitIndex, audioUnitNode] : configurationNode->audioUnits)
			{
				// Check currentSamplingRate is set to one of the supported sampling rates
				auto const& supportedSamplingRates = audioUnitNode.staticModel.samplingRates;
				auto const& currentSamplingRate = audioUnitNode.dynamicModel.currentSamplingRate;
				if (std::find(supportedSamplingRates.begin(), supportedSamplingRates.end(), currentSamplingRate) == supportedSamplingRates.end())
				{
					_isError = true;
					_errorMessage = "AudioUnitNode[" + std::to_string(audioUnitIndex) + "].dynamicModel.currentSamplingRate is not in the supported sampling rates: " + std::to_string(currentSamplingRate);
					return;
				}
			}
		}

		// Check StreamNodeDynamicModel.streamFormat for the active configuration
		{
			auto const* const configurationNode = _controlledEntity->getConfigurationNode(activeConfigurationIndex, TreeModelAccessStrategy::NotFoundBehavior::Throw);
			for (auto const& [streamIndex, streamNode] : configurationNode->streamInputs)
			{
				// Check streamFormat is set to one of the supported stream formats
				auto const& supportedStreamFormats = streamNode.staticModel.formats;
				auto const& streamFormat = streamNode.dynamicModel.streamFormat;
				if (std::find(supportedStreamFormats.begin(), supportedStreamFormats.end(), streamFormat) == supportedStreamFormats.end())
				{
					_isError = true;
					_errorMessage = "StreamInputNode[" + std::to_string(streamIndex) + "].dynamicModel.streamFormat is not in the supported stream formats: " + std::to_string(streamFormat);
					return;
				}
			}
			for (auto const& [streamIndex, streamNode] : configurationNode->streamOutputs)
			{
				// Check streamFormat is set to one of the supported stream formats
				auto const& supportedStreamFormats = streamNode.staticModel.formats;
				auto const& streamFormat = streamNode.dynamicModel.streamFormat;
				if (std::find(supportedStreamFormats.begin(), supportedStreamFormats.end(), streamFormat) == supportedStreamFormats.end())
				{
					_isError = true;
					_errorMessage = "StreamOutputNode[" + std::to_string(streamIndex) + "].dynamicModel.streamFormat is not in the supported stream formats: " + std::to_string(streamFormat);
					return;
				}
			}
		}

		// Check ClockDomainNodeDynamicModel.clockSourceIndex for the active configuration
		{
			auto const* const configurationNode = _controlledEntity->getConfigurationNode(activeConfigurationIndex, TreeModelAccessStrategy::NotFoundBehavior::Throw);
			for (auto const& [clockDomainIndex, clockDomainNode] : configurationNode->clockDomains)
			{
				// Check clockSourceIndex is set to one of the supported clock sources
				auto const& supportedClockSources = clockDomainNode.staticModel.clockSources;
				auto const& clockSourceIndex = clockDomainNode.dynamicModel.clockSourceIndex;
				if (std::find(supportedClockSources.begin(), supportedClockSources.end(), clockSourceIndex) == supportedClockSources.end())
				{
					_isError = true;
					_errorMessage = "ClockDomainNode[" + std::to_string(clockDomainIndex) + "].dynamicModel.clockSourceIndex is not in the supported clock sources: " + std::to_string(clockSourceIndex);
					return;
				}
			}
		}
	}
	catch (ControlledEntity::Exception const& e)
	{
		_isError = true;
		_errorMessage = std::string{ "Exception: " } + std::string{ e.what() };
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
